"""GREEN ION V18 Windows desktop launcher."""
from __future__ import annotations
import argparse,ctypes,json,os,subprocess,sys,threading,time,webbrowser
from datetime import datetime,timezone
from pathlib import Path
HERE=Path(__file__).resolve().parent
if str(HERE) not in sys.path:sys.path.insert(0,str(HERE))
import ground_station as gs
from update_manager import UpdateError,UpdateManager
APP_MUTEX="Local\\GREEN_ION_V18_DESKTOP";ERROR_ALREADY_EXISTS=183;HEARTBEAT_TIMEOUT_S=180;DESKTOP_SCHEMA="GREEN-ION-V18-DESKTOP-1"

def resource_root()->Path:return Path(getattr(sys,"_MEIPASS",HERE.parent))
def load_info():
    for p in (resource_root()/"Ground_Station"/"desktop_version.json",HERE/"desktop_version.json"):
        try:
            v=json.loads(p.read_text(encoding="utf-8"))
            if isinstance(v,dict):return v
        except (OSError,ValueError,TypeError):pass
    return {"version":"18.0.0-dev.1","channel":"development","repository":"aidenjantzi1-ctrl/GREEN-ION"}
DESKTOP_INFO=load_info()
def state_root():
    return Path(os.environ.get("LOCALAPPDATA") or str(Path.home()/"AppData"/"Local"))/"GREEN_ION"
RUNTIME_FILE=state_root()/"desktop-runtime.json";HEARTBEAT={"seen":False,"at":time.monotonic()};OPERATION={"name":"","at":0.};OP_LOCK=threading.Lock()

class SingleInstance:
    def __init__(self):
        self.handle=None;self.duplicate=False
        if os.name=="nt":
            k=ctypes.windll.kernel32;self.handle=k.CreateMutexW(None,False,APP_MUTEX);self.duplicate=k.GetLastError()==ERROR_ALREADY_EXISTS
    def close(self):
        if self.handle and os.name=="nt":ctypes.windll.kernel32.CloseHandle(self.handle);self.handle=None

def show_error(msg):
    if os.name=="nt":
        try:ctypes.windll.user32.MessageBoxW(None,str(msg),"GREEN ION",0x10);return
        except Exception:pass
    try:sys.stderr.write(str(msg)+"\n")
    except Exception:pass

def browser_candidates():
    e=os.environ
    vals=[Path(e.get("PROGRAMFILES(X86)",""))/"Microsoft/Edge/Application/msedge.exe",Path(e.get("PROGRAMFILES",""))/"Microsoft/Edge/Application/msedge.exe",Path(e.get("LOCALAPPDATA",""))/"Microsoft/Edge/Application/msedge.exe",Path(e.get("PROGRAMFILES",""))/"Google/Chrome/Application/chrome.exe",Path(e.get("PROGRAMFILES(X86)",""))/"Google/Chrome/Application/chrome.exe",Path(e.get("LOCALAPPDATA",""))/"Google/Chrome/Application/chrome.exe"]
    return [p for p in vals if str(p) not in (".","") and p.is_file()]
def open_ui(url):
    for b in browser_candidates():
        try:subprocess.Popen([str(b),"--app="+url,"--new-window","--no-first-run"],close_fds=True);return b.name
        except OSError:pass
    webbrowser.open(url);return "system-default"
def write_runtime(port,browser=""):
    RUNTIME_FILE.parent.mkdir(parents=True,exist_ok=True);t=RUNTIME_FILE.with_suffix(".tmp")
    t.write_text(json.dumps({"schema":DESKTOP_SCHEMA,"pid":os.getpid(),"port":int(port),"browser":browser,"startedUtc":datetime.now(timezone.utc).isoformat()},indent=2),encoding="utf-8");os.replace(t,RUNTIME_FILE)
def existing_url():
    try:
        p=int(json.loads(RUNTIME_FILE.read_text(encoding="utf-8")).get("port"))
        if 1<=p<=65535:return f"http://127.0.0.1:{p}/v18-offline.html#flight"
    except (OSError,ValueError,TypeError):pass
    return None
def set_operation(name,active):
    with OP_LOCK:OPERATION["name"]=str(name)[:80] if active else "";OPERATION["at"]=time.monotonic() if active else 0.
def active_operation():
    with OP_LOCK:
        if not OPERATION["name"]:return ""
        if time.monotonic()-OPERATION["at"]>120:OPERATION["name"]="";OPERATION["at"]=0.;return ""
        return OPERATION["name"]
def evaluate_safety(status,link,operation=""):
    if operation:return False,"Critical local operation is active: "+operation+"."
    if not isinstance(link,dict) or not link.get("controls_verified"):return False,"Feather state is unknown or stale; connect the Feather and wait for a fresh idle status."
    if not isinstance(status,dict):return False,"Feather state is unknown."
    for f in ("recording","armed","flight_prep_active"):
        if status.get(f) is not False:return False,f"Feather {f} is active or unknown."
    for f in ("home_test_active","maintenance_active","firmware_maintenance_active","ota_active"):
        if status.get(f) is True:return False,f"Feather critical operation is active: {f}."
    if str(status.get("imu_test_result") or "").upper()=="RUNNING" or str(status.get("gps_test_result") or "").upper()=="RUNNING":return False,"Firmware diagnostic is still running."
    return True,"Fresh Feather status is idle; installation can proceed."
def safety_probe():
    link=gs.BRIDGE.telemetry_source();return evaluate_safety(link.get("status") if isinstance(link,dict) else None,link,active_operation())
UPDATER=UpdateManager(str(DESKTOP_INFO.get("version") or "unknown"),safety_probe=safety_probe)
def read_json(h,limit=4096):
    try:n=int(h.headers.get("Content-Length","0"))
    except ValueError:n=0
    if n<0 or n>limit:raise ValueError("Request body is too large.")
    if not n:return {}
    v=json.loads(h.rfile.read(n).decode("utf-8"))
    if not isinstance(v,dict):raise ValueError("JSON object required.")
    return v

class DesktopHandler(gs.Handler):
    def do_GET(self):
        if self.path.split("?",1)[0]=="/api/app/update/status":
            if not self._safe_request():return
            self._json(UPDATER.status());return
        super().do_GET()
    def do_POST(self):
        p=self.path.split("?",1)[0]
        if p.startswith("/api/app/"):
            if not self._safe_request():return
            try:
                b=read_json(self)
                if p=="/api/app/heartbeat":HEARTBEAT["seen"]=True;HEARTBEAT["at"]=time.monotonic();self._json({"success":True});return
                if p=="/api/app/operation":
                    name=str(b.get("name") or "")
                    if name not in ("data-import","diagnostics","data-export",""):raise ValueError("Unsupported operation marker.")
                    set_operation(name,bool(b.get("active")));self._json({"success":True,"operation":active_operation()});return
                if p=="/api/app/update/preferences":UPDATER.set_auto_check(bool(b.get("auto_check")));self._json(UPDATER.status());return
                if p=="/api/app/update/check":self._json(UPDATER.check());return
                if p=="/api/app/update/download":self._json(UPDATER.download());return
                if p=="/api/app/update/install":
                    r=UPDATER.install();self._json(r)
                    if r.get("exit_requested"):threading.Thread(target=self.server.shutdown,daemon=True).start()
                    return
                if p=="/api/app/update/rollback":
                    r=UPDATER.rollback();self._json(r)
                    if r.get("exit_requested"):threading.Thread(target=self.server.shutdown,daemon=True).start()
                    return
            except (UpdateError,ValueError,OSError) as e:self._json({"success":False,"error":str(e),**UPDATER.status()},409);return
        super().do_POST()

def configure_resources():
    web=resource_root()/"web"
    if not (web/"v18-offline.html").is_file():raise RuntimeError("Bundled V18 web application is missing.")
    gs.STATIC_ROOT=web;gs.APP_INFO=dict(DESKTOP_INFO);data=Path(os.environ.get("GREEN_ION_DATA_DIR",str(Path.home()/"GREEN_ION_Data")))
    os.environ["GREEN_ION_DATA_DIR"]=str(data);gs.DATA_ROOT=data;gs.FLIGHT_DIR=data/"flights";gs.LOG_DIR=data/"logs";gs.RUNTIME["mode"]="v18-desktop-wifi+browser-serial";gs.RUNTIME["receiver_auto_discovery"]=False;gs.RUNTIME["receiver_port"]=None
def create_server(port):
    from http.server import ThreadingHTTPServer
    try:return ThreadingHTTPServer(("127.0.0.1",port),DesktopHandler)
    except OSError:
        if port==0:raise
        return ThreadingHTTPServer(("127.0.0.1",0),DesktopHandler)
def heartbeat_watch(server,stop):
    while not stop.wait(10):
        if HEARTBEAT["seen"] and time.monotonic()-HEARTBEAT["at"]>HEARTBEAT_TIMEOUT_S:server.shutdown();return
def parse_args(argv=None):
    p=argparse.ArgumentParser(description="GREEN ION V18 Windows desktop application")
    for a in ("version","check-update","update","rollback"):p.add_argument("--"+a,action="store_true")
    p.add_argument("--no-browser",action="store_true",help=argparse.SUPPRESS);p.add_argument("--http-port",type=int,default=8765,help=argparse.SUPPRESS);p.add_argument("--test-exit-after",type=float,default=0,help=argparse.SUPPRESS);return p.parse_args(argv)
def cli_action(a):
    if a.version:print(DESKTOP_INFO.get("version","unknown"));return 0
    if a.check_update:
        r=UPDATER.check();print(json.dumps(r,indent=2));return 0 if r.get("state")!="error" else 2
    if a.update:
        r=UPDATER.check()
        if not r.get("update_available"):print(json.dumps(r,indent=2));return 0 if r.get("state")!="error" else 2
        try:print(json.dumps(UPDATER.download(),indent=2));print("Installer verified. Start installation from Settings while fresh Feather idle state is known.");return 0
        except UpdateError as e:print(str(e));return 2
    if a.rollback:print("For safety, rollback must be started from Settings while fresh Feather idle state is known.");return 3
    return None
def main(argv=None):
    a=parse_args(argv);action=cli_action(a)
    if action is not None:return action
    instance=SingleInstance()
    if instance.duplicate:
        if not a.no_browser:
            u=existing_url()
            if u:open_ui(u)
        instance.close();return 0
    stop=threading.Event();server=None
    try:
        configure_resources();server=create_server(a.http_port);port=int(server.server_address[1]);url=f"http://127.0.0.1:{port}/v18-offline.html#flight";write_runtime(port)
        threading.Thread(target=gs.BRIDGE.run,args=(stop,),daemon=True).start();threading.Thread(target=heartbeat_watch,args=(server,stop),daemon=True).start()
        if UPDATER.auto_check:threading.Thread(target=UPDATER.check,daemon=True).start()
        if a.test_exit_after>0:threading.Timer(a.test_exit_after,server.shutdown).start()
        if not a.no_browser:write_runtime(port,open_ui(url))
        server.serve_forever(poll_interval=.3);return 0
    except Exception as e:show_error("GREEN ION could not start.\n\n"+str(e));return 1
    finally:
        stop.set()
        if server:
            try:server.server_close()
            except OSError:pass
        try:
            if RUNTIME_FILE.exists() and int(json.loads(RUNTIME_FILE.read_text(encoding="utf-8")).get("pid",-1))==os.getpid():RUNTIME_FILE.unlink(missing_ok=True)
        except (OSError,ValueError,TypeError):pass
        instance.close()
if __name__=="__main__":raise SystemExit(main())
