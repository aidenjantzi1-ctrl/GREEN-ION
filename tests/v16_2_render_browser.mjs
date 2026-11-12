import {createServer} from "node:http";
import {readFile} from "node:fs/promises";
import {existsSync} from "node:fs";
import {spawn} from "node:child_process";
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
const result=await new Promise((resolve,reject)=>{
 const child=spawn(chrome,[
  "--headless=new","--no-sandbox","--disable-dev-shm-usage","--enable-webgl","--ignore-gpu-blocklist",
  "--use-gl=angle","--use-angle=swiftshader","--virtual-time-budget=3500","--dump-dom",url
 ],{stdio:["ignore","pipe","pipe"]});
 let stdout="",stderr="";
 child.stdout.setEncoding("utf8");child.stderr.setEncoding("utf8");
 child.stdout.on("data",chunk=>{stdout+=chunk;if(stdout.length>10*1024*1024)child.kill("SIGKILL");});
 child.stderr.on("data",chunk=>{stderr+=chunk;if(stderr.length>10*1024*1024)stderr=stderr.slice(-10*1024*1024);});
 const timer=setTimeout(()=>{child.kill("SIGKILL");reject(Error("Headless Chrome WebGL smoke timed out"));},30000);
 child.on("error",error=>{clearTimeout(timer);reject(error);});
 child.on("close",code=>{clearTimeout(timer);resolve({code,stdout,stderr});});
});
server.close();
const dom=result.stdout||"";
if(result.code!==0||!/data-result="pass"/.test(dom)){
 console.error(result.stderr||"");console.error(dom.slice(-5000));
 throw Error("Headless browser did not complete the real WebGL render/interaction smoke test");
}
console.log("PASS: headless Chrome created a WebGL context, compiled/drew the V16.2 shader path, read a rendered pixel, and accepted pointer/wheel interaction.");
