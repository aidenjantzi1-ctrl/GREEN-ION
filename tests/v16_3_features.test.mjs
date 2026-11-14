import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const spi=readFileSync(new URL("../web/v16_3_shared_spi.mjs",import.meta.url),"utf8");
const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
const sw=readFileSync(new URL("../web/service-worker.js",import.meta.url),"utf8");
const diag=readFileSync(new URL("../web/v16_1_diagnostics.mjs",import.meta.url),"utf8");
const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");

test("V16.3 keeps the exact five-tab dark-blue shell",()=>{
 const visible=[...html.matchAll(/<button[^>]+data-tab="(status|flight|history|testing|system)"[^>]*>([^<]+)/g)];
 assert.equal(visible.length,5);
 assert.match(html,/--bg:#081420/);
});

test("V16.3 shared-SPI evidence adds status, not another control panel",()=>{
 assert.match(spi,/Shared SD \+ LoRa SPI/);
 assert.match(spi,/Uno packets during SPI stress/);
 assert.match(spi,/NOT TESTED — LoRa receiver not connected/);
 assert.match(spi,/home_shared_spi_result/);
 assert.match(spi,/receiver_bench_packets/);
 assert.match(spi,/rssi_dbm/);
 assert.match(spi,/snr_db/);
 assert.doesNotMatch(spi,/createElement\("button"\)/);
});

test("receiver PASS requires actual received bench packets",()=>{
 assert.match(spi,/benchDelta>=activeStress\.target/);
 assert.match(spi,/receiver="PASS"/);
 assert.match(spi,/receiver="NOT TESTED"/);
 assert.match(spi,/receiver="INCONCLUSIVE"/);
});

test("technical shared-SPI counters stay under expandable System details",()=>{
 assert.match(spi,/createElement\("details"\)/);
 for(const field of [
  "shared_spi_sd_sections","shared_spi_radio_sections",
  "shared_spi_radio_deferred_busy","shared_spi_radio_deferred_deadline",
  "shared_spi_ownership_errors","radio_tx_started","radio_tx_completed",
  "radio_tx_timeouts","missed_sample_periods"
 ])assert.match(spi,new RegExp(field));
});

test("diagnostic export contains shared-SPI evidence and human guidance",()=>{
 assert.match(diag,/shared_spi:/);
 assert.match(diag,/ownership_errors/);
 assert.match(diag,/radio_tx_timeouts/);
 assert.match(diag,/software scheduling counters only/);
 assert.match(diag,/LoRa transmissions timed out/);
});

test("V16.3 module is initialized and cached offline",()=>{
 assert.match(app,/initV163SharedSpiEvidence/);
 assert.match(app,/V16\.3 · shared-SPI reliability/);
 assert.match(sw,/green-ion-v(?:16-3-shared-spi|17-0-integrated|17-0-1-flight-readiness)/);
 assert.match(sw,/\.\/v16_3_shared_spi\.mjs/);
});
