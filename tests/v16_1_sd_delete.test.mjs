import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const layout=readFileSync(new URL("../web/v16_layout.mjs",import.meta.url),"utf8");
const css=readFileSync(new URL("../web/v14_layout.css",import.meta.url),"utf8");
const data=readFileSync(new URL("../web/v16_data.mjs",import.meta.url),"utf8");

test("Feather SD delete UI is explicit and backup-safe",()=>{
 assert.match(html,/Delete from SD/);
 assert.match(html,/&confirm="\+encodeURIComponent\(f\.name\)/);
 assert.match(html,/id="sdDeleteFeedback"/);
 assert.match(html,/Browser, Windows, and cloud backups are not deleted/);
 assert.match(html,/If launch watching is active, GREEN ION will restart stationary calibration/);
 assert.doesNotMatch(html,/protect\?name=/);
 assert.doesNotMatch(html,/note\?name=/);
 assert.doesNotMatch(html,/integrity\?name=/);
 assert.doesNotMatch(html,/\/api\/manifest/);
 assert.match(html,/\/api\/sensors\?name=/);
});

test("SD delete UI blocks destructive action during critical states",()=>{
 assert.match(html,/state\?\.recording/);
 assert.match(html,/state\?\.home_test_active/);
 assert.match(html,/state\?\.maintenance_mode/);
 assert.match(html,/state\?\.update_in_progress/);
 assert.match(html,/result\.verified!==true/);
});

test("V16 layout keeps the same five blue tabs while improving hierarchy",()=>{
 const block=html.match(/<nav class="tabs v16-tabs"[^>]*>([\s\S]*?)<\/nav>/)?.[1]||"";
 const labels=[...block.matchAll(/<button[^>]*>([\s\S]*?)<\/button>/g)]
  .map(x=>x[1].replace(/&amp;/g,"&").trim());
 assert.deepEqual(labels,["Dashboard","Flight","Flights & Analysis","Testing","Settings & Advanced"]);
 assert.match(html,/--bg:#081420/);
 assert.match(layout,/v16SdManagerCard/);
 assert.match(layout,/Profiles, sensors & recording","v16SettingsGeneral",false/);
 assert.match(css,/grid-template-columns:repeat\(5,minmax\(0,1fr\)\)/);
 assert.match(css,/#v16SdManagerCard/);
 assert.match(css,/#v16VisCard/);
});

test("local flight library stays next to Feather SD manager",()=>{
 assert.match(data,/const sdManager=\$\("v16SdManagerCard"\)/);
 assert.match(data,/if\(sdManager\)sdManager\.after\(card\)/);
});
