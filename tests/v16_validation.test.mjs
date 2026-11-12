import test from "node:test";
import assert from "node:assert/strict";
import {
 VALIDATION_SCHEMA,REQUIREMENTS,newLedger,normalizeLedger,updateEvidence,
 validationSummary,addDiagnosticSnapshot
} from "../web/v16_validation.mjs";

test("every physical requirement defaults to NOT_TESTED",()=>{
 const ledger=newLedger(0);
 assert.equal(ledger.schema,VALIDATION_SCHEMA);
 assert.ok(REQUIREMENTS.length>=10);
 for(const req of REQUIREMENTS)assert.equal(ledger.records[req.id].status,"NOT_TESTED");
 const summary=validationSummary(ledger);
 assert.equal(summary.state,"PHYSICAL_EVIDENCE_PENDING");
 assert.equal(summary.flight_authorized,false);
});

test("capturing software diagnostics can never pass physical evidence",()=>{
 let ledger=newLedger(0);
 ledger=addDiagnosticSnapshot(ledger,{"/api/app/health":{version:"16.0.0"}},1000);
 assert.equal(ledger.diagnostic_snapshots.length,1);
 assert.equal(validationSummary(ledger).required_passed,0);
 for(const req of REQUIREMENTS)assert.equal(ledger.records[req.id].status,"NOT_TESTED");
});

test("failed evidence is explicit and not overridable by unrelated records",()=>{
 let ledger=newLedger(0);
 ledger=updateEvidence(ledger,"sd_recording_loss",{status:"FAIL",note:"file truncated"},1000);
 ledger=updateEvidence(ledger,"mobile_offline",{status:"PASS",note:"phone offline analysis works"},2000);
 const summary=validationSummary(ledger);
 assert.equal(summary.state,"REVIEW_FAILED_EVIDENCE");
 assert.deepEqual(summary.failed_required,["sd_recording_loss"]);
 assert.equal(summary.flight_authorized,false);
});

test("even all required PASS only reaches human review, never launch authorization",()=>{
 let ledger=newLedger(0);
 for(const req of REQUIREMENTS.filter(x=>x.required))
  ledger=updateEvidence(ledger,req.id,{status:"PASS",note:"measured evidence recorded"},1000);
 const summary=validationSummary(ledger);
 assert.equal(summary.state,"READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION");
 assert.equal(summary.required_passed,summary.required_total);
 assert.equal(summary.flight_authorized,false);
});

test("unknown or corrupted statuses normalize back to NOT_TESTED",()=>{
 const raw=newLedger(0);
 raw.records.power_lipo.status="CERTIFIED";
 raw.records.power_lipo.note="ok\u0000bad";
 const normalized=normalizeLedger(raw,1000);
 assert.equal(normalized.records.power_lipo.status,"NOT_TESTED");
 assert.equal(normalized.records.power_lipo.note,"ok bad");
});
