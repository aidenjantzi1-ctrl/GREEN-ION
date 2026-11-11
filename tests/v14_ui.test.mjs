import test from "node:test";
import assert from "node:assert/strict";
import {requiresFeather} from "../web/v14_ui.mjs";
test("hardware controls block while offline; local analysis and settings stay usable",()=>{
 for(const id of ["homeStart","homeRun","sdtest","installFirmware","v14DownloadUpdate","rollbackFirmware","fieldStop","refresh","saveFlightProfile"])
  assert.equal(requiresFeather(id),true,id);
 for(const id of ["exportFlightPDF","v14Analyze","v14Compare","v14ExportCsv",
    "v14LocalExport","v14LocalRestore","v14ManualDownload","pause","clear","sunlightMode",
    "openFlightStats","v14CloudBackup","v14CheckUpdates"])
  assert.equal(requiresFeather(id),false,id);
});
