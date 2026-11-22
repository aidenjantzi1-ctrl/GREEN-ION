// Explicit CI gate for a reviewed checkout containing real V18 app source.
// Run: node --test tools/v18_contract_gate.test.mjs
// Or:  V18_SOURCE_ROOT=<integration-checkout> node --test tools/v18_contract_gate.test.mjs
import test from "node:test";
import assert from "node:assert/strict";
import {existsSync} from "node:fs";
import {spawnSync} from "node:child_process";
import {dirname,join,resolve} from "node:path";
import {fileURLToPath} from "node:url";

const workerRoot=resolve(dirname(fileURLToPath(import.meta.url)),"..");
const sourceRoot=resolve(process.env.V18_SOURCE_ROOT||workerRoot);
const required=["save_valid_completion","save_second_flight","save_wrong_flight",
 "save_blank_metadata","save_missing_marker","save_incomplete","save_partial_records",
 "save_byte_mismatch","save_record_mismatch","save_zero_records",
 "save_contradictory_fields","save_metadata_error","save_delayed_new_flight",
 "save_delayed_disconnection"];

test("real V18 source passes the independent completion acceptance gate",()=>{
 assert.ok(existsSync(join(sourceRoot,"web","v18_offline_core.mjs")),
  "V18 core missing: supply V18_SOURCE_ROOT pointing at a reviewed Integration checkout; this gate must not skip.");
 assert.ok(existsSync(join(sourceRoot,"web","v18_offline_app.mjs")),
  "V18 app missing: this gate must not skip.");
 const run=spawnSync(process.execPath,[join(workerRoot,"tools","v18_contract_probe.mjs"),
  "--source-root",sourceRoot],{encoding:"utf8"});
 assert.equal(run.error,undefined);
 const report=JSON.parse(run.stdout);
 assert.equal(report.schema,"green-ion-v18-contract-probe-1");
 const byId=new Map(report.results.map(x=>[x.id,x.state]));
 for(const id of required)assert.equal(byId.get(id),"PASS",`${id}: ${byId.get(id)??"MISSING"}`);
 assert.equal(run.status,0,
  `Probe exit ${run.status}; failures: ${JSON.stringify(report.results.filter(x=>x.state!=="PASS"))}`);
});
