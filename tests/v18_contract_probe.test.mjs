import test from "node:test";
import assert from "node:assert/strict";
import {existsSync} from "node:fs";
import {spawnSync} from "node:child_process";
import {fileURLToPath} from "node:url";
import {dirname,resolve,join} from "node:path";

const root=resolve(dirname(fileURLToPath(import.meta.url)),"..");
const probe=join(root,"tools","v18_contract_probe.mjs");

test("independent V18 contract probe reports evidence or explicit NOT TESTED",()=>{
 const run=spawnSync(process.execPath,[probe,"--source-root",root],{encoding:"utf8"});
 assert.equal(run.error,undefined);
 const report=JSON.parse(run.stdout);
 assert.equal(report.schema,"green-ion-v18-contract-probe-1");
 assert.ok(Array.isArray(report.results));
 if(!existsSync(join(root,"web","v18_offline_core.mjs"))){
  assert.equal(run.status,2);
  assert.deepEqual(report.results.map(x=>x.state),["NOT TESTED"]);
 }else{
  assert.equal(run.status,0,JSON.stringify(report.results.filter(x=>x.state!=="PASS")));
  assert.equal(report.results.length,25,"integration checkout must execute the full reviewed 25-case matrix");
  assert.equal(new Set(report.results.map(x=>x.id)).size,25,"probe case IDs must remain unique");
  assert.ok(report.results.every(x=>x.state==="PASS"),"all 25 independent cases must pass");
 }
});
