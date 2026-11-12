/* GREEN ION V16.2 data-quality scorecard. This is NOT a launch safety score. */
const $=id=>document.getElementById(id);let last=null;
function row(label,value,detail=""){
 const d=document.createElement("div");d.className="row";const a=document.createElement("span"),b=document.createElement("strong");a.textContent=label;b.textContent=value+(detail?" · "+detail:"");d.append(a,b);return d;
}
function render(report){
 last=report;const card=$("v162QualityCard"),body=$("v162QualityRows");if(!card||!body)return;body.replaceChildren();
 const o=report.observed||{},s=report.sensors||{},score=Number(report.dataCompletenessScore);
 $("v162QualityScore").textContent=Number.isFinite(score)?Math.round(score)+"/100 data quality":"Not scored";
 body.append(
  row("Timestamp continuity",o.timestampGaps===0&&o.nonMonotonicTimestamps===0?"PASS":"REVIEW",(o.timestampGaps||0)+" gap(s), "+(o.nonMonotonicTimestamps||0)+" non-monotonic"),
  row("Sample validity",o.sampleCount?Math.round((o.validPressureSamples||0)/o.sampleCount*100)+"% valid":"NO DATA",(o.validPressureSamples||0)+" / "+(o.sampleCount||0)),
  row("Recording tail",o.trailingBytes===0?"COMPLETE RECORD BOUNDARY":"REVIEW",(o.trailingBytes||0)+" trailing byte(s)"),
  row("GPS availability",s.freshGpsSamples>0?"AVAILABLE":"MISSING",s.freshGpsSamples?String(s.freshGpsSamples)+" current fix sample(s)":"No current GPS samples in SEN"),
  row("Sensor CRC",s.crc32ValidatedPrefix===true?"VERIFIED PREFIX":s.crc32ValidatedPrefix===false?"REVIEW":"NOT AVAILABLE"),
  row("Original checksum",report.checksum?"IDENTIFIED":"NOT SUPPLIED",report.checksum?String(report.checksum).slice(0,12)+"…":""),
  row("Radio evidence","SEPARATE","Use mission/receiver evidence; LoRa is not part of SD data-quality scoring")
 );
 $("v162QualityNote").textContent=(report.warnings||[]).slice(0,5).join(" · ")||
  "No file-integrity warning reported. This does not establish sensor accuracy or flight safety.";
 card.hidden=false;
 const quick=$("v162ViewReport");if(quick){quick.hidden=false;quick.textContent="VIEW FLIGHT REPORT · "+report.name;}
}
function inject(){
 const reportCard=$("v15ReportCard"),history=$("history");if(!history||$("v162QualityCard"))return;
 const card=document.createElement("div");card.className="card";card.id="v162QualityCard";card.hidden=true;
 card.innerHTML='<h2>Flight data-quality scorecard</h2><p class="muted">Integrity/completeness summary only — NOT a launch-safety score, certification, or sensor-accuracy rating.</p><div class="value" id="v162QualityScore">Not scored</div><div id="v162QualityRows"></div><p id="v162QualityNote" class="muted"></p>';
 if(reportCard)reportCard.after(card);else history.append(card);
 const dash=$("v16DashboardSummary");if(dash&&!$("v162ViewReport")){
  const b=document.createElement("button");b.id="v162ViewReport";b.className="secondary wide";b.hidden=true;b.textContent="VIEW FLIGHT REPORT";
  b.onclick=()=>{document.querySelector('[data-tab="history"]')?.click();$ ("v15ReportCard")?.scrollIntoView({behavior:"smooth",block:"start"});};
  dash.querySelector(".actions")?.append(b);
 }
}
export function initV162Scorecard(){
 inject();window.addEventListener("green-ion-postflight-report",e=>{if(e.detail?.report)render(e.detail.report);});
}
