import test from "node:test";
import assert from "node:assert/strict";
import {perspectiveMatrix,lookAtMatrix,multiplyMatrix} from "../web/v16_2_webgl.mjs";
import {reconstructFlight} from "../web/v16_visualization.mjs";
import {readFileSync} from "node:fs";

function rkt4(count=200,rate=50){
 const size=32+count*16,b=new ArrayBuffer(size),v=new DataView(b),u=new Uint8Array(b);
 "RKT4".split("").forEach((c,i)=>u[i]=c.charCodeAt(0));v.setUint16(4,2,true);v.setUint16(6,32,true);v.setUint16(8,16,true);v.setUint16(10,rate,true);v.setFloat32(12,101325,true);v.setUint16(16,4100,true);
 for(let i=0;i<count;i++){const o=32+i*16,t=i/rate,p=101325-Math.sin(Math.PI*i/(count-1))*2200;v.setUint32(o,Math.round(t*1e6),true);v.setFloat32(o+4,p,true);v.setFloat32(o+8,22,true);v.setUint16(o+12,4000,true);v.setUint16(o+14,1,true);}
 return b;
}
test("matrix path produces finite perspective depth transforms",()=>{
 const p=perspectiveMatrix(Math.PI/4,16/9,.1,1000),v=lookAtMatrix([10,10,10],[0,0,0],[0,0,1]),m=multiplyMatrix(p,v);
 assert.equal(m.length,16);assert.ok([...m].every(Number.isFinite));assert.notEqual(m[11],0);
});
test("reconstruction remains worker-friendly and pressure-only never invents horizontal movement",()=>{
 const r=reconstructFlight(rkt4(),null);assert.equal(r.schema,"GREEN-ION-V16.2-VISUALIZATION-1");
 assert.equal(r.mode,"pressure-only");assert.ok(r.points.length>5);assert.ok(r.points.every(p=>p.x===0&&p.y===0));
 assert.ok(r.timeline.length>0);assert.ok(r.timeline.length<=6000);assert.equal(r.sensorTimeline.length,0);
});
test("viewer source uses genuine WebGL depth shaders, camera modes, timestamp speeds and cleanup",()=>{
 const source=readFileSync(new URL("../web/v16_2_webgl.mjs",import.meta.url),"utf8");
 assert.match(source,/getContext\("webgl"/);assert.match(source,/gl\.DEPTH_TEST/);assert.match(source,/VERTEX_SHADER/);
 assert.match(source,/perspectiveMatrix/);assert.match(source,/lookAtMatrix/);
 for(const mode of ["follow","ground","top","side","free"])assert.ok(source.includes(mode),mode);
 for(const speed of [".25",".5","1","2","5"])assert.ok(source.includes(">"+speed+"</option>"),speed);
 for(const jump of ["launch","apogee","descent","landing"])assert.ok(source.includes('data-jump="'+jump+'"'),jump);
 assert.match(source,/deleteBuffer/);assert.match(source,/deleteProgram/);assert.match(source,/pagehide/);
 assert.match(source,/green-ion-v16-replay-time/);assert.match(source,/green-ion-v16-graph-progress/);
});
