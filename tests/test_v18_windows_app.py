from pathlib import Path
import hashlib,json,sys,tempfile,unittest
from unittest import mock
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/"Ground_Station"))
import desktop_app
from update_manager import UpdateError,UpdateManager,SETUP_ASSET,SETUP_SHA_ASSET,MANIFEST_ASSET
class Resp:
 def __init__(self,data,fail=False):self.data=data;self.pos=0;self.fail=fail;self.headers={"Content-Length":str(len(data))}
 def __enter__(self):return self
 def __exit__(self,*a):return False
 def read(self,n=-1):
  if self.fail and self.pos>0:raise OSError("interrupted")
  if n<0:n=len(self.data)-self.pos
  c=self.data[self.pos:self.pos+n];self.pos+=len(c);return c
def release(v):
 t="v"+v;b=f"https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/download/{t}/"
 return json.dumps([{"draft":False,"prerelease":False,"tag_name":t,"name":t,"body":"notes","published_at":"2026-11-26","assets":[{"name":SETUP_ASSET,"browser_download_url":b+"setup"},{"name":SETUP_SHA_ASSET,"browser_download_url":b+"sha"},{"name":MANIFEST_ASSET,"browser_download_url":b+"manifest"}]}]).encode()
class T(unittest.TestCase):
 def m(self,op,root,v="18.0.0"):return UpdateManager(v,state_root=Path(root),opener=op,safety_probe=lambda:(True,"idle"),authenticode_checker=lambda p:"unsigned")
 def test_check_and_offline(self):
  with tempfile.TemporaryDirectory() as td:self.assertTrue(self.m(lambda r,timeout=0:Resp(release("18.0.1")),td).check()["update_available"])
  with tempfile.TemporaryDirectory() as td:self.assertEqual(self.m(lambda r,timeout=0:Resp(release("18.0.0")),td).check()["state"],"up-to-date")
  with tempfile.TemporaryDirectory() as td:self.assertTrue(self.m(lambda r,timeout=0:Resp(release("18.0.0")),td,"18.0.0-dev.1").check()["update_available"])
  def off(r,timeout=0):raise OSError("offline")
  with tempfile.TemporaryDirectory() as td:self.assertEqual(self.m(off,td).check()["state"],"error")
 def test_checksum_and_interruption(self):
  setup=b"x"*700000;good=hashlib.sha256(setup).hexdigest();man=json.dumps({"version":"18.0.1","assets":{SETUP_ASSET:{"sha256":good}}}).encode()
  def bad(req,timeout=0):
   u=req.full_url
   if "releases?per_page" in u:return Resp(release("18.0.1"))
   if u.endswith("/sha"):return Resp(("0"*64+"  x").encode())
   if u.endswith("/setup"):return Resp(setup)
   if u.endswith("/manifest"):return Resp(man)
  with tempfile.TemporaryDirectory() as td:
   m=self.m(bad,td);m.check()
   with self.assertRaises(UpdateError):m.download()
  def interrupted(req,timeout=0):
   u=req.full_url
   if "releases?per_page" in u:return Resp(release("18.0.1"))
   if u.endswith("/sha"):return Resp((good+"  x").encode())
   if u.endswith("/setup"):return Resp(setup,True)
   if u.endswith("/manifest"):return Resp(man)
  with tempfile.TemporaryDirectory() as td:
   root=Path(td);m=self.m(interrupted,root);m.check()
   with self.assertRaises(UpdateError):m.download()
   self.assertFalse(any(root.rglob("*.partial")))
 def test_verified_download_and_rollback_helpers(self):
  setup=b"approved installer";digest=hashlib.sha256(setup).hexdigest()
  manifest=json.dumps({"version":"18.0.1","assets":{SETUP_ASSET:{"sha256":digest}},"signatureRequired":False}).encode()
  def good(req,timeout=0):
   u=req.full_url
   if "releases?per_page" in u:return Resp(release("18.0.1"))
   if u.endswith("/sha"):return Resp((digest+"  "+SETUP_ASSET).encode())
   if u.endswith("/setup"):return Resp(setup)
   if u.endswith("/manifest"):return Resp(manifest)
   raise AssertionError(u)
  with tempfile.TemporaryDirectory() as td:
   root=Path(td);m=self.m(good,root);m.check();state=m.download()
   self.assertEqual(state["state"],"ready");self.assertTrue(state["downloaded_verified"]);self.assertEqual(state["signature"],"unsigned")
   current=root/"current.exe";current.write_bytes(b"current executable")
   with mock.patch.object(sys,"frozen",True,create=True),mock.patch.object(sys,"executable",str(current)),mock.patch("update_manager.subprocess.Popen") as popen:
    installed=m.install()
   self.assertEqual(installed["state"],"installing")
   script=Path(popen.call_args.args[0][-1]).read_text(encoding="utf-8")
   self.assertIn("/SILENT",script);self.assertIn("$p.ExitCode -ne 0",script);self.assertIn("Copy-Item",script)
   backup=m.rollback_root/"GREEN_ION_previous.exe";self.assertTrue(backup.is_file())
   with mock.patch.object(sys,"frozen",True,create=True),mock.patch.object(sys,"executable",str(current)),mock.patch("update_manager.subprocess.Popen") as popen2:
    rolled=m.rollback()
   self.assertEqual(rolled["state"],"rolling-back")
   rollback_script=Path(popen2.call_args.args[0][-1]).read_text(encoding="utf-8")
   self.assertIn("Copy-Item",rollback_script);self.assertIn("GREEN_ION_previous.exe",rollback_script)
   backup.write_bytes(b"tampered")
   with mock.patch.object(sys,"frozen",True,create=True),mock.patch.object(sys,"executable",str(current)),mock.patch("update_manager.subprocess.Popen"):
    with self.assertRaises(UpdateError):m.rollback()
 def test_manifest_mismatch_is_corrupt_update(self):
  setup=b"installer";digest=hashlib.sha256(setup).hexdigest()
  manifest=json.dumps({"version":"18.0.1","assets":{SETUP_ASSET:{"sha256":"f"*64}}}).encode()
  def opener(req,timeout=0):
   u=req.full_url
   if "releases?per_page" in u:return Resp(release("18.0.1"))
   if u.endswith("/sha"):return Resp((digest+"  "+SETUP_ASSET).encode())
   if u.endswith("/setup"):return Resp(setup)
   if u.endswith("/manifest"):return Resp(manifest)
   raise AssertionError(u)
  with tempfile.TemporaryDirectory() as td:
   m=self.m(opener,td);m.check()
   with self.assertRaises(UpdateError):m.download()
 def test_safety(self):
  self.assertFalse(desktop_app.evaluate_safety(None,None)[0]);link={"controls_verified":True};s={"recording":False,"armed":False,"flight_prep_active":False}
  self.assertTrue(desktop_app.evaluate_safety(s,link)[0]);self.assertFalse(desktop_app.evaluate_safety(s,link,"data-import")[0]);self.assertFalse(desktop_app.evaluate_safety({**s,"recording":True},link)[0])
 def test_contracts(self):
  h=(ROOT/"web/v18-offline.html").read_text();tabs="\n".join(x for x in h.splitlines() if 'data-tab="' in x);self.assertEqual(tabs.count('data-tab="'),4);self.assertIn("Software updates",h)
  j=(ROOT/"web/v18_offline_app.mjs").read_text();self.assertIn('markDesktopOperation("data-import",true)',j);self.assertIn("/api/app/update/install",j)
  i=(ROOT/"packaging/GREEN_ION.iss").read_text();self.assertIn("PrivilegesRequired=lowest",i);self.assertIn("{autodesktop}",i)
if __name__=="__main__":unittest.main()
