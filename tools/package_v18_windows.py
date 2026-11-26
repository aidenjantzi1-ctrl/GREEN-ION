import argparse,hashlib,json,shutil
from datetime import datetime,timezone
from pathlib import Path
def sha(p):
 h=hashlib.sha256()
 with Path(p).open("rb") as f:
  for c in iter(lambda:f.read(1048576),b""):h.update(c)
 return h.hexdigest()
p=argparse.ArgumentParser();p.add_argument("--exe",required=True);p.add_argument("--setup",required=True);p.add_argument("--output",required=True);p.add_argument("--version",required=True);p.add_argument("--commit",required=True);p.add_argument("--signature-required",action="store_true");a=p.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=True);assets={}
for src,name in ((Path(a.exe),"GREEN_ION.exe"),(Path(a.setup),"GREEN_ION_Setup.exe")):
 if not src.is_file():raise SystemExit("missing "+str(src))
 dst=out/name;shutil.copy2(src,dst);h=sha(dst);assets[name]={"sha256":h,"bytes":dst.stat().st_size};(out/(name+".sha256")).write_text(h+"  "+name+"\n",encoding="ascii")
m={"schema":"GREEN-ION-V18-WINDOWS-BUILD-1","version":a.version,"commit":a.commit,"builtUtc":datetime.now(timezone.utc).isoformat(),"platform":"windows-x64","interface":["Flight","Recovery","Data","Settings"],"offlineRuntime":True,"releaseSource":"GitHub Releases only","signatureRequired":bool(a.signature_required),"assets":assets,"dataProtection":{"flightDataOutsideInstall":True,"settingsOutsideInstall":True,"telemetryUpload":False}}
(out/"GREEN_ION_Windows_manifest.json").write_text(json.dumps(m,indent=2),encoding="utf-8")
