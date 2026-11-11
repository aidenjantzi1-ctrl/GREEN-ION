import {analyzeOriginals} from "./v15_analysis_core.mjs";
self.onmessage=e=>{
 try{
  const {id,binBytes,senBytes,name,checksum}=e.data;
  const report=analyzeOriginals(binBytes,senBytes,{name,checksum});
  self.postMessage({id,report});
 }catch(error){self.postMessage({id,error:error.message});}
};
