"use strict";
// Same-origin Vercel relay for allowlisted public GREEN ION Feather release assets.
// It never accepts arbitrary URLs and never contacts the Feather itself.
const REPO="aidenjantzi1-ctrl/GREEN-ION";
const MANIFEST={max:200000,type:"application/json; charset=utf-8"};
const IMAGE={max:8388608,type:"application/octet-stream"};
function imageForTag(tag){
 if(/^v17\.\d+\.\d+(?:-[a-zA-Z0-9.-]+)?$/.test(tag))return "Green_Ion_V17.ino.bin";
 const m=/^v(16)\.(\d+)\.\d+(?:-[a-zA-Z0-9.-]+)?$/.exec(tag);
 if(m)return "Green_Ion_V16_"+Number(m[2])+".ino.bin";
 if(/^v14\.\d+\.\d+(?:-[a-zA-Z0-9.-]+)?$/.test(tag))return "Green_Ion_V14.ino.bin";
 return null;
}
module.exports=async function handler(req,res){
 res.setHeader("Cache-Control","no-store");
 res.setHeader("X-Content-Type-Options","nosniff");
 if(req.method!=="GET"){res.setHeader("Allow","GET");return res.status(405).end("Method not allowed");}
 const tag=typeof req.query?.tag==="string"?req.query.tag:"";
 const name=typeof req.query?.name==="string"?req.query.name:"";
 const imageName=imageForTag(tag);
 const policy=name==="SHA256_MANIFEST.json"?MANIFEST:name===imageName?IMAGE:null;
 if(!imageName||!policy)return res.status(400).end("Unsupported release or firmware asset");
 try{
  const meta=await fetch("https://api.github.com/repos/"+REPO+"/releases/tags/"+encodeURIComponent(tag),{
   headers:{"Accept":"application/vnd.github+json","User-Agent":"GREEN-ION-firmware-relay"},redirect:"error",
   signal:AbortSignal.timeout(12000)
  });
  if(!meta.ok)return res.status(502).end("GitHub release lookup failed");
  const release=await meta.json();
  if(release.draft||release.tag_name!==tag)return res.status(404).end("Published release not found");
  const asset=release.assets?.find(a=>a.name===name && a.state==="uploaded");
  const safeURL="https://github.com/"+REPO+"/releases/download/"+tag+"/"+name;
  if(!asset||asset.browser_download_url!==safeURL||!Number.isSafeInteger(asset.size)||asset.size<1||asset.size>policy.max)
   return res.status(404).end("Official release asset unavailable");
  const upstream=await fetch(safeURL,{
   headers:{"User-Agent":"GREEN-ION-firmware-relay"},redirect:"follow",
   signal:AbortSignal.timeout(30000)
  });
  if(!upstream.ok)return res.status(502).end("Firmware asset fetch failed");
  const len=Number(upstream.headers.get("content-length")||0);
  if(len>policy.max)return res.status(413).end("Release asset too large");
  const buffer=Buffer.from(await upstream.arrayBuffer());
  if(buffer.length!==asset.size||buffer.length>policy.max)
   return res.status(502).end("Release asset length mismatch");
  res.setHeader("Content-Type",policy.type);
  res.setHeader("Content-Length",String(buffer.length));
  return res.status(200).end(buffer);
 }catch(_error){
  return res.status(502).end("Unable to retrieve official GitHub release asset");
 }
};
