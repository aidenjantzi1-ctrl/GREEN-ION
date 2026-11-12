import {reconstructFlight} from "./v16_visualization.mjs";

self.onmessage=event=>{
 const {id,binBytes,senBytes}=event.data||{};
 try{
  if(!(binBytes instanceof ArrayBuffer))throw Error("RKT4 bytes missing");
  const data=reconstructFlight(binBytes,senBytes instanceof ArrayBuffer?senBytes:null);
  self.postMessage({id,data});
 }catch(error){
  self.postMessage({id,error:String(error?.message||error)});
 }
};
