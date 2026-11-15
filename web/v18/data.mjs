import {parseRkt4,analyzeFlight,analysisCsv,flightReport} from "../analysis/rkt4.mjs";

export function analyzeImportedBin(input){
  const bytes=input instanceof Uint8Array?input:new Uint8Array(input);
  const before=bytes.slice();
  const parsed=parseRkt4(bytes);
  const analysis=analyzeFlight(parsed);
  for(let i=0;i<bytes.length;i++)if(bytes[i]!==before[i])throw new Error("Analysis modified the original BIN bytes");
  return analysis;
}
export function analysisSummary(analysis){
  const last=analysis.samples.at(-1);
  return {
    analysisVersion:analysis.analysisVersion,
    sampleRateHz:analysis.rateHz,
    samples:analysis.samples.length,
    durationS:Number.isFinite(last?.timeS)?last.timeS:0,
    peakAltitudeM:analysis.peakAltitudeM,
    maxAscentMps:analysis.maxAscentMps,
    maxDescentMps:analysis.maxDescentMps,
    phases:analysis.phases.map(p=>({name:p.name,timeS:p.timeS,altitudeM:p.altitudeM,confidence:p.confidence,confidenceScore:p.confidenceScore})),
    warnings:[...new Set(analysis.warnings)]
  };
}
export function artifactWarnings(analysis){
  const out=[...analysis.warnings];
  let invalid=0,nonMonotonic=0;
  let previous=null;
  for(const s of analysis.samples){
    if(!s.valid)invalid++;
    if(previous!==null&&Number.isFinite(s.timestampUs)&&s.timestampUs<=previous)nonMonotonic++;
    if(Number.isFinite(s.timestampUs))previous=s.timestampUs;
  }
  if(invalid)out.push(`${invalid} sample(s) were marked invalid by the recorder.`);
  if(nonMonotonic)out.push(`${nonMonotonic} non-monotonic timestamp(s) detected.`);
  return [...new Set(out)];
}
export function comparisonRows(entries){
  return entries.map(({meta,analysis})=>({
    label:meta.label||"",
    originalName:meta.originalName,
    sampleRateHz:analysis.rateHz,
    durationS:analysis.samples.at(-1)?.timeS??0,
    peakAltitudeM:analysis.peakAltitudeM,
    maxAscentMps:analysis.maxAscentMps,
    maxDescentMps:analysis.maxDescentMps
  }));
}
export function reportJson(analysis,name,label=""){
  return {...flightReport(analysis,name),label};
}
export {analysisCsv};

export function drawAltitudeGraph(canvas,analyses){
  if(!canvas?.getContext)return;
  const list=Array.isArray(analyses)?analyses:[analyses];
  const series=list.filter(Boolean).map(a=>a.samples.filter(s=>Number.isFinite(s.timeS)&&Number.isFinite(s.altitudeM)));
  const ctx=canvas.getContext("2d"),w=canvas.width=canvas.clientWidth||800,h=canvas.height=canvas.clientHeight||280;
  ctx.clearRect(0,0,w,h);
  if(!series.length||!series.some(s=>s.length)){ctx.fillText("No valid altitude samples",16,28);return;}
  const maxT=Math.max(...series.flat().map(s=>s.timeS),1);
  const vals=series.flat().map(s=>s.altitudeM);
  const minA=Math.min(...vals,0),maxA=Math.max(...vals,1),range=Math.max(1,maxA-minA);
  ctx.strokeStyle="rgba(150,175,195,.45)";ctx.lineWidth=1;
  ctx.beginPath();ctx.moveTo(44,10);ctx.lineTo(44,h-28);ctx.lineTo(w-10,h-28);ctx.stroke();
  const palette=["#72e5ab","#7bdbff","#ffdd88","#ff91a0"];
  series.forEach((samples,index)=>{
    ctx.strokeStyle=palette[index%palette.length];ctx.lineWidth=2;ctx.beginPath();
    samples.forEach((s,i)=>{
      const x=44+(w-58)*(s.timeS/maxT),y=10+(h-38)*(1-(s.altitudeM-minA)/range);
      if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
    });
    ctx.stroke();
  });
  ctx.fillStyle="#a5bbcd";ctx.font="12px system-ui";
  ctx.fillText(`0 s`,44,h-8);ctx.fillText(`${maxT.toFixed(1)} s`,Math.max(50,w-65),h-8);
  ctx.fillText(`${maxA.toFixed(1)} m`,4,18);ctx.fillText(`${minA.toFixed(1)} m`,4,h-30);
}
