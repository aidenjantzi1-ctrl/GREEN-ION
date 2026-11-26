"""GREEN ION V18 release-only Windows update manager."""
from __future__ import annotations
from dataclasses import dataclass
import hashlib,json,os,re,shutil,subprocess,sys,tempfile,threading,time
from pathlib import Path
from typing import Callable
from urllib.error import HTTPError,URLError
from urllib.request import Request,urlopen

REPOSITORY="aidenjantzi1-ctrl/GREEN-ION"
RELEASES_API=f"https://api.github.com/repos/{REPOSITORY}/releases?per_page=20"
SETUP_ASSET="GREEN_ION_Setup.exe"
SETUP_SHA_ASSET="GREEN_ION_Setup.exe.sha256"
MANIFEST_ASSET="GREEN_ION_Windows_manifest.json"
RELEASE_TAG=re.compile(r"^v(18\.\d+\.\d+(?:\.[0-9]+)?)$")
SHA256_RE=re.compile(r"^[0-9a-f]{64}$")
MAX_INSTALLER_BYTES=300*1024*1024
MAX_TEXT_ASSET_BYTES=1024*1024

def _version_tuple(value:str)->tuple[int,...]:
    m=re.match(r"^(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?",str(value or ""))
    return tuple(int(x or 0) for x in m.groups()) if m else (0,0,0,0)

def _is_newer(candidate:str,installed:str)->bool:
    c,i=_version_tuple(candidate),_version_tuple(installed)
    if c!=i:return c>i
    return "-" in str(installed) and "-" not in str(candidate)

def _sha256(path:Path)->str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""):h.update(chunk)
    return h.hexdigest()

def _safe_release_url(url:str)->bool:
    return isinstance(url,str) and url.startswith(f"https://github.com/{REPOSITORY}/releases/download/")

@dataclass(frozen=True)
class ReleaseInfo:
    tag:str;version:str;name:str;notes:str;published_at:str;assets:dict[str,str]

class UpdateError(RuntimeError):pass

class UpdateManager:
    def __init__(self,installed_version:str,*,state_root:Path|None=None,opener:Callable=urlopen,
                 safety_probe:Callable[[],tuple[bool,str]]|None=None,
                 authenticode_checker:Callable[[Path],str]|None=None)->None:
        local=os.environ.get("LOCALAPPDATA") or str(Path.home()/"AppData"/"Local")
        self.state_root=Path(state_root or (Path(local)/"GREEN_ION"))
        self.update_root=self.state_root/"updates";self.rollback_root=self.state_root/"rollback"
        self.preferences_file=self.state_root/"desktop-settings.json";self.rollback_meta=self.rollback_root/"rollback.json"
        self.installed_version=str(installed_version);self.opener=opener
        self.safety_probe=safety_probe or (lambda:(False,"Hardware state is unknown."))
        self.authenticode_checker=authenticode_checker or self._authenticode_status
        self._lock=threading.RLock();self._release=None;self._downloaded_installer=None;self._downloaded_sha256=""
        self._signature="not-checked";self._state="idle";self._error="";self._progress=0.0;self._internet_available=None
        self._preferences=self._load_preferences()

    def _load_preferences(self)->dict:
        try:
            v=json.loads(self.preferences_file.read_text(encoding="utf-8"))
            if isinstance(v,dict):return {"auto_check":bool(v.get("auto_check",False))}
        except (OSError,ValueError,TypeError):pass
        return {"auto_check":False}

    @property
    def auto_check(self)->bool:return bool(self._preferences.get("auto_check",False))

    def set_auto_check(self,enabled:bool)->None:
        self.state_root.mkdir(parents=True,exist_ok=True);self._preferences={"auto_check":bool(enabled)}
        t=self.preferences_file.with_suffix(".tmp");t.write_text(json.dumps(self._preferences,indent=2),encoding="utf-8");os.replace(t,self.preferences_file)

    def status(self)->dict:
        safe,reason=self.safety_probe()
        with self._lock:
            r=self._release
            return {"schema":"GREEN-ION-V18-WINDOWS-UPDATER-1","installed_version":self.installed_version,
              "latest_version":r.version if r else None,"latest_tag":r.tag if r else None,"release_name":r.name if r else "",
              "release_notes":r.notes if r else "","published_at":r.published_at if r else "",
              "update_available":bool(r and _is_newer(r.version,self.installed_version)),
              "downloaded_verified":bool(self._downloaded_installer and self._downloaded_installer.is_file()),
              "download_progress":round(self._progress,4),"signature":self._signature,"state":self._state,"error":self._error,
              "internet_available":self._internet_available,"auto_check":self.auto_check,"safe_to_install":bool(safe),
              "safety_reason":reason,"release_source":f"https://github.com/{REPOSITORY}/releases","release_only":True}

    def _request(self,url:str):
        return self.opener(Request(url,headers={"Accept":"application/vnd.github+json","User-Agent":"GREEN-ION-V18-Windows-Updater","X-GitHub-Api-Version":"2022-11-28"}),timeout=20)

    def _read_limited(self,url:str,limit:int)->bytes:
        with self._request(url) as r:data=r.read(limit+1)
        if len(data)>limit:raise UpdateError("Release metadata asset is unexpectedly large.")
        return data

    def _select_release(self,releases:object)->ReleaseInfo|None:
        if not isinstance(releases,list):raise UpdateError("GitHub returned an invalid releases response.")
        candidates=[]
        for item in releases:
            if not isinstance(item,dict) or item.get("draft") or item.get("prerelease"):continue
            tag=str(item.get("tag_name") or "");m=RELEASE_TAG.fullmatch(tag)
            if not m:continue
            assets={}
            for a in item.get("assets") or []:
                if not isinstance(a,dict):continue
                n=str(a.get("name") or "");u=str(a.get("browser_download_url") or "")
                if n in (SETUP_ASSET,SETUP_SHA_ASSET,MANIFEST_ASSET) and _safe_release_url(u):assets[n]=u
            if SETUP_ASSET not in assets or SETUP_SHA_ASSET not in assets:continue
            candidates.append(ReleaseInfo(tag,m.group(1),str(item.get("name") or tag)[:160],str(item.get("body") or "")[:12000],str(item.get("published_at") or ""),assets))
        return max(candidates,key=lambda r:_version_tuple(r.version),default=None)

    def check(self)->dict:
        with self._lock:self._state="checking";self._error=""
        try:
            r=self._select_release(json.loads(self._read_limited(RELEASES_API,MAX_TEXT_ASSET_BYTES).decode("utf-8")))
            with self._lock:
                self._release=r;self._internet_available=True
                self._state="available" if r and _is_newer(r.version,self.installed_version) else "up-to-date"
        except (OSError,HTTPError,URLError,ValueError,UnicodeError,json.JSONDecodeError,UpdateError) as e:
            with self._lock:self._internet_available=False;self._state="error";self._error="GitHub update check failed: "+str(e)[:300]
        return self.status()

    def _download_file(self,url:str,target:Path,limit:int)->None:
        partial=target.with_suffix(target.suffix+".partial");partial.unlink(missing_ok=True);total=0
        try:
            with self._request(url) as r,partial.open("wb") as f:
                h=r.headers.get("Content-Length") if getattr(r,"headers",None) else None
                expected=int(h) if h and str(h).isdigit() else None
                if expected is not None and expected>limit:raise UpdateError("Release asset exceeds the allowed size.")
                while True:
                    c=r.read(512*1024)
                    if not c:break
                    total+=len(c)
                    if total>limit:raise UpdateError("Release asset exceeds the allowed size.")
                    f.write(c)
                    with self._lock:self._progress=min(.98,total/expected) if expected else min(.98,total/(20*1024*1024))
                f.flush();os.fsync(f.fileno())
            os.replace(partial,target)
        except Exception:
            partial.unlink(missing_ok=True);target.unlink(missing_ok=True);raise

    def download(self)->dict:
        with self._lock:r=self._release
        if r is None:self.check();r=self._release
        if r is None or not _is_newer(r.version,self.installed_version):raise UpdateError("No newer approved V18 Windows release is available.")
        root=self.update_root/r.tag;root.mkdir(parents=True,exist_ok=True);setup=root/SETUP_ASSET;sha_file=root/SETUP_SHA_ASSET;manifest_file=root/MANIFEST_ASSET
        with self._lock:self._state="downloading";self._error="";self._progress=0.;self._downloaded_installer=None;self._downloaded_sha256="";self._signature="not-checked"
        try:
            self._download_file(r.assets[SETUP_SHA_ASSET],sha_file,4096);expected=sha_file.read_text(encoding="ascii",errors="strict").split()[0].lower()
            if not SHA256_RE.fullmatch(expected):raise UpdateError("Published installer checksum is invalid.")
            self._download_file(r.assets[SETUP_ASSET],setup,MAX_INSTALLER_BYTES);actual=_sha256(setup)
            if actual!=expected:raise UpdateError("Downloaded installer SHA-256 does not match the approved release.")
            manifest=None
            if r.assets.get(MANIFEST_ASSET):
                self._download_file(r.assets[MANIFEST_ASSET],manifest_file,MAX_TEXT_ASSET_BYTES);manifest=json.loads(manifest_file.read_text(encoding="utf-8"))
                if str(manifest.get("version") or "")!=r.version:raise UpdateError("Release manifest version does not match its GitHub tag.")
                bound=str((((manifest.get("assets") or {}).get(SETUP_ASSET) or {}).get("sha256") or "")).lower()
                if bound and bound!=actual:raise UpdateError("Release manifest does not bind to the downloaded installer.")
            sig=self.authenticode_checker(setup)
            if sig=="invalid":raise UpdateError("Installer has an invalid Authenticode signature.")
            if manifest and bool(manifest.get("signatureRequired")) and sig!="valid":raise UpdateError("This release requires a valid Authenticode signature.")
            with self._lock:self._downloaded_installer=setup;self._downloaded_sha256=actual;self._signature=sig;self._progress=1.;self._state="ready"
        except Exception as e:
            with self._lock:self._state="error";self._error="Update download/verification failed: "+str(e)[:300];self._progress=0.
            if isinstance(e,UpdateError):raise
            raise UpdateError(str(e)) from e
        return self.status()

    @staticmethod
    def _authenticode_status(path:Path)->str:
        if os.name!="nt":return "unavailable"
        q=str(path).replace("'","''");cmd="$s=Get-AuthenticodeSignature -LiteralPath '"+q+"';if($s.Status -eq 'Valid'){'valid'}elseif($s.Status -eq 'NotSigned'){'unsigned'}else{'invalid'}"
        try:
            r=subprocess.run(["powershell.exe","-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass","-Command",cmd],capture_output=True,text=True,timeout=15,check=False)
            v=(r.stdout or "").strip().lower();return v if v in ("valid","unsigned","invalid") else "invalid"
        except (OSError,subprocess.SubprocessError):return "unavailable"

    def _current_executable(self)->Path|None:return Path(sys.executable).resolve() if getattr(sys,"frozen",False) else None

    def _prepare_rollback(self,current:Path)->None:
        self.rollback_root.mkdir(parents=True,exist_ok=True);backup=self.rollback_root/"GREEN_ION_previous.exe";stage=backup.with_suffix(".new")
        shutil.copy2(current,stage);actual=_sha256(stage);os.replace(stage,backup)
        t=self.rollback_meta.with_suffix(".tmp");t.write_text(json.dumps({"schema":"GREEN-ION-V18-ROLLBACK-1","version":self.installed_version,"sha256":actual,"original":str(current),"createdUtc":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())},indent=2),encoding="utf-8");os.replace(t,self.rollback_meta)

    def _safe_or_raise(self)->None:
        safe,reason=self.safety_probe()
        if not safe:raise UpdateError("Installation deferred: "+reason)

    def install(self)->dict:
        self._safe_or_raise()
        with self._lock:setup=self._downloaded_installer
        if not setup or not setup.is_file():raise UpdateError("Download and verify the approved installer before installation.")
        if _sha256(setup)!=self._downloaded_sha256:raise UpdateError("Verified installer changed after download; installation cancelled.")
        current=self._current_executable()
        if current and current.is_file():self._prepare_rollback(current)
        local=Path(os.environ.get("LOCALAPPDATA") or str(Path.home()/"AppData"/"Local"));target=local/"Programs"/"GREEN ION"/"GREEN_ION.exe"
        helper=Path(tempfile.gettempdir())/("green-ion-update-"+str(os.getpid())+".ps1")
        cur=str(current or target).replace("'","''");bak=str(self.rollback_root/"GREEN_ION_previous.exe").replace("'","''");setupq=str(setup).replace("'","''");tgt=str(target).replace("'","''");log=str(self.state_root/"last-update.log").replace("'","''")
        helper.write_text(f"""$ErrorActionPreference='Continue'
$pidToWait={os.getpid()}
while(Get-Process -Id $pidToWait -ErrorAction SilentlyContinue){{Start-Sleep -Milliseconds 250}}
$p=Start-Process -FilePath '{setupq}' -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CLOSEAPPLICATIONS','/RESTARTAPPLICATIONS','/CURRENTUSER','/LOG={log}') -Wait -PassThru
if($p.ExitCode -ne 0){{if(Test-Path '{bak}'){{Copy-Item -LiteralPath '{bak}' -Destination '{cur}' -Force}};if(Test-Path '{cur}'){{Start-Process -FilePath '{cur}'}};exit $p.ExitCode}}
if(Test-Path '{tgt}'){{Start-Process -FilePath '{tgt}'}}elseif(Test-Path '{cur}'){{Start-Process -FilePath '{cur}'}}
Remove-Item -LiteralPath $MyInvocation.MyCommand.Path -Force -ErrorAction SilentlyContinue
""",encoding="utf-8")
        subprocess.Popen(["powershell.exe","-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass","-File",str(helper)],close_fds=True)
        with self._lock:self._state="installing"
        out=self.status();out["exit_requested"]=True;out["message"]="Verified installer launched. GREEN ION will close and restart after installation.";return out

    def rollback(self)->dict:
        self._safe_or_raise();current=self._current_executable()
        if current is None:raise UpdateError("Rollback is available from the packaged GREEN_ION.exe application.")
        try:meta=json.loads(self.rollback_meta.read_text(encoding="utf-8"))
        except (OSError,ValueError,TypeError) as e:raise UpdateError("No verified previous Windows executable is available.") from e
        backup=self.rollback_root/"GREEN_ION_previous.exe";expected=str(meta.get("sha256") or "").lower()
        if not backup.is_file() or not SHA256_RE.fullmatch(expected) or _sha256(backup)!=expected:raise UpdateError("Rollback executable is missing or failed SHA-256 verification.")
        helper=Path(tempfile.gettempdir())/("green-ion-rollback-"+str(os.getpid())+".ps1");bak=str(backup).replace("'","''");cur=str(current).replace("'","''")
        helper.write_text(f"""$ErrorActionPreference='Stop'
$pidToWait={os.getpid()}
while(Get-Process -Id $pidToWait -ErrorAction SilentlyContinue){{Start-Sleep -Milliseconds 250}}
Copy-Item -LiteralPath '{bak}' -Destination '{cur}' -Force
Start-Process -FilePath '{cur}'
Remove-Item -LiteralPath $MyInvocation.MyCommand.Path -Force -ErrorAction SilentlyContinue
""",encoding="utf-8")
        subprocess.Popen(["powershell.exe","-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass","-File",str(helper)],close_fds=True)
        with self._lock:self._state="rolling-back"
        out=self.status();out["exit_requested"]=True;out["message"]="Verified previous executable selected. GREEN ION will close and restart.";return out
