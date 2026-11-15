import {parseReceiverLine,recoveryView} from "../v18_offline_core.mjs";

export class LineDecoder{
  constructor(limit=131072){this.pending="";this.limit=limit;this.decoder=new TextDecoder();}
  push(chunk){
    this.pending+=this.decoder.decode(chunk,{stream:true});
    if(this.pending.length>this.limit){this.pending="";return {lines:[],overflow:true};}
    const lines=[];
    let i;
    while((i=this.pending.indexOf("\n"))>=0){
      lines.push(this.pending.slice(0,i).trim());
      this.pending=this.pending.slice(i+1);
    }
    return {lines,overflow:false};
  }
}
export class ReceiverSerial{
  constructor({onState=()=>{},onMessage=()=>{},onInvalid=()=>{}}={}){
    this.onState=onState;this.onMessage=onMessage;this.onInvalid=onInvalid;
    this.port=null;this.reader=null;this.manual=false;this.running=false;this.decoder=new LineDecoder();
    this.invalidLines=0;this.lastMessage=null;this.lastKnown=null;
    this.boundConnect=()=>this.reconnectGranted().catch(()=>{});
    this.boundDisconnect=event=>{
      if(!this.port||event.target===this.port||event.port===this.port){
        this.onState({state:"disconnected",message:"Arduino Uno disconnected."});
        this.port=null;this.reader=null;
        if(!this.manual)setTimeout(()=>this.reconnectGranted().catch(()=>{}),600);
      }
    };
  }
  supported(){
    const nav=globalThis.navigator;
    return !!nav?.serial && (globalThis.isSecureContext===true || globalThis.location?.hostname==="localhost" || globalThis.location?.hostname==="127.0.0.1");
  }
  startAutoReconnect(){
    if(!globalThis.navigator?.serial)return;
    navigator.serial.addEventListener?.("connect",this.boundConnect);
    navigator.serial.addEventListener?.("disconnect",this.boundDisconnect);
    this.reconnectGranted().catch(()=>{});
  }
  stopAutoReconnect(){
    globalThis.navigator?.serial?.removeEventListener?.("connect",this.boundConnect);
    globalThis.navigator?.serial?.removeEventListener?.("disconnect",this.boundDisconnect);
  }
  async connectReceiver(){
    if(!this.supported())throw new Error("Web Serial requires Chrome or Edge on Windows from HTTPS or localhost.");
    this.manual=false;
    const port=await navigator.serial.requestPort();
    await this.open(port);
  }
  async reconnectGranted(){
    if(this.manual||this.port||!this.supported()||!navigator.serial.getPorts)return false;
    const ports=await navigator.serial.getPorts();
    if(!ports.length)return false;
    try{await this.open(ports[0]);return true;}catch{return false;}
  }
  async open(port){
    if(this.port)return;
    this.port=port;
    this.onState({state:"connecting",message:"Opening Arduino Uno at 115200 baud..."});
    await port.open({baudRate:115200,bufferSize:4096});
    this.running=true;this.manual=false;
    this.onState({state:"connected",message:"Arduino Uno connected. Waiting for validated LoRa packets."});
    this.readLoop(port).catch(error=>{
      this.onState({state:"error",message:"Receiver read error: "+error.message});
    }).finally(async()=>{
      if(this.reader){try{this.reader.releaseLock();}catch{}}
      this.reader=null;
      if(this.port===port)this.port=null;
      try{await port.close();}catch{}
      if(!this.manual)this.onState({state:"disconnected",message:"Receiver disconnected; reconnect will be attempted for previously granted ports."});
    });
  }
  async readLoop(port){
    while(this.running&&port.readable){
      const reader=port.readable.getReader();this.reader=reader;
      try{
        while(this.running){
          const {value,done}=await reader.read();
          if(done)break;
          const decoded=this.decoder.push(value);
          if(decoded.overflow)this.onInvalid({reason:"Receiver line buffer overflow"});
          for(const line of decoded.lines){
            const message=parseReceiverLine(line,Date.now());
            if(!message){this.invalidLines++;this.onInvalid({reason:"Invalid or incompatible receiver JSON",line});continue;}
            this.lastMessage=message;
            if(message.kind==="packet"&&message.latitude!=null&&message.longitude!=null){
              this.lastKnown={latitude:message.latitude,longitude:message.longitude,at:message.receivedAt};
            }
            this.onMessage(message,this.view());
          }
        }
      }finally{
        try{reader.releaseLock();}catch{}
        if(this.reader===reader)this.reader=null;
      }
    }
  }
  view(now=Date.now()){return recoveryView(this.lastMessage,this.lastKnown,now);}
  async disconnect(){
    this.manual=true;this.running=false;
    try{await this.reader?.cancel();}catch{}
    try{await this.port?.close();}catch{}
    this.reader=null;this.port=null;
    this.onState({state:"disconnected",message:"Arduino Uno disconnected by user."});
  }
}
