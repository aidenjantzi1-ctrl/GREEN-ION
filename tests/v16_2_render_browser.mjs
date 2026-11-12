import {createServer} from "node:http";
import {readFile} from "node:fs/promises";
import {existsSync} from "node:fs";
import {spawnSync} from "node:child_process";
import {extname,join,normalize} from "node:path";

const candidates=[
 process.env.CHROME_BIN,
 "/usr/bin/google-chrome","/usr/bin/google-chrome-stable","/usr/bin/chromium","/usr/bin/chromium-browser"
].filter(Boolean);
const chrome=candidates.find(existsSync);
if(!chrome){
 console.log("SKIP: no Chrome/Chromium executable available for actual WebGL browser validation.");
 process.exit(0);
}
const root=new URL("../web/",import.meta.url);
const harness=`<!doctype html><html><body><section id="history"></section>
<script type="module">
window.addEventListener("error",e=>document.body.dataset.error=e.message||"error");
try{
 const m=await import("/v16_2_webgl.mjs");
 m.initV162Webgl();
 const canvas=document.getElementById("v1623dCanvas");
 const result=m.runWebglSelfTest(canvas);
 canvas.dispatchEvent(new PointerEvent("pointerdown",{pointerId:1,clientX:20,clientY:20,bubbles:true}));
 canvas.dispatchEvent(new PointerEvent("pointermove",{pointerId:1,clientX:45,clientY:32,buttons:1,bubbles:true}));
 canvas.dispatchEvent(new PointerEvent("pointerup",{pointerId:1,clientX:45,clientY:32,bubbles:true}));
 canvas.dispatchEvent(new WheelEvent("wheel",{deltaY:20,bubbles:true,cancelable:true}));
 document.body.dataset.result=result.passed?"pass":"fail";
 document.body.dataset.pixel=(result.pixel||[]).join(",");
}catch(error){document.body.dataset.result="error";document.body.dataset.error=error.message;}
</script></body></html>`;
const mime={".mjs":"text/javascript",".js":"text/javascript",".css":"text/css",".html":"text/html",".json":"application/json"};
const server=createServer(async(req,res)=>{
 try{
  if(req.url==="/harness"){res.writeHead(200,{"Content-Type":"text/html"});res.end(harness);return;}
  const path=normalize(decodeURIComponent(req.url||"/")).replace(/^[/\\]+/,"");
  const base=new URL(path,root);
  if(!base.href.startsWith(root.href))throw Error("blocked");
  const body=await readFile(base);res.writeHead(200,{"Content-Type":mime[extname(path)]||"application/octet-stream","Cache-Control":"no-store"});res.end(body);
 }catch{res.writeHead(404);res.end("not found");}
});
await new Promise(resolve=>server.listen(0,"127.0.0.1",resolve));
const port=server.address().port,url=`http://127.0.0.1:${port}/harness`;
const result=spawnSync(chrome,[
 "--headless=new","--no-sandbox","--disable-dev-shm-usage","--enable-webgl","--ignore-gpu-blocklist",
 "--use-gl=angle","--use-angle=swiftshader","--virtual-time-budget=3500","--dump-dom",url
],{encoding:"utf8",timeout:30000,maxBuffer:10*1024*1024});
server.close();
if(result.error)throw result.error;
const dom=result.stdout||"";
if(!/data-result="pass"/.test(dom)){
 console.error(result.stderr||"");console.error(dom.slice(-5000));
 throw Error("Headless browser did not complete the real WebGL render/interaction smoke test");
}
console.log("PASS: headless Chrome created a WebGL context, compiled/drew the V16.2 shader path, read a rendered pixel, and accepted pointer/wheel interaction.");
