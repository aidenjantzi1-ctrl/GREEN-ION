from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
HEADER = struct.Struct("<4sHHHHfH14x")
RECORD = struct.Struct("<IffHH")


class V12Rkt4Tests(unittest.TestCase):
    def test_golden_header_and_record_bytes(self):
        header = HEADER.pack(b"RKT4", 2, 32, 16, 25, 101325.0, 4000)
        record = RECORD.pack(0xFFFFFFF0, 100000.0, 21.5, 3890, 1)
        self.assertEqual(
            header.hex(),
            "524b5434020020001000190080e6c547a00f0000000000000000000000000000",
        )
        self.assertEqual(record.hex(), "f0ffffff0050c3470000ac41320f0100")

    def test_interrupted_log_counts_only_complete_records(self):
        data = HEADER.pack(b"RKT4", 2, 32, 16, 50, 101325.0, 4100)
        data += RECORD.pack(1, 101325.0, 20.0, 4100, 1) * 3 + b"torn"
        count, trailing = divmod(len(data) - 32, 16)
        self.assertEqual((count, trailing), (3, 4))

    def test_v12_firmware_writes_immutable_rkt4_contract(self):
        firmware = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        header = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonRkt4.h").read_text(encoding="utf-8")
        core = (ROOT / "libraries" / "GreenIonRecorder" / "src" / "GreenIonRecorderCore.h").read_text(encoding="utf-8")
        self.assertIn("#include <GreenIonRkt4.h>", firmware)
        self.assertIn("RecorderCore<config::PREBUFFER_RECORDS>", firmware)
        self.assertIn("rkt4::encodeHeader", core)
        self.assertIn("rkt4::encodeRecord", core)
        self.assertIn('memcpy(out, "RKT4", 4)', header)
        self.assertIn("kHeaderBytes = 32", header)
        self.assertIn("kRecordBytes = 16", header)

    def test_v11_source_remains_the_format_authority(self):
        v11 = (ROOT / "Past_Versions" / "Green_Ion_V11" / "Green_Ion_V11.ino").read_text(encoding="utf-8")
        self.assertIn('memcpy(hdr, "RKT4", 4)', v11)
        self.assertIn("w16(hdr + 4, 2)", v11)
        self.assertIn("w16(hdr + 6, 32)", v11)
        self.assertIn("w16(hdr + 8, 16)", v11)


if __name__ == "__main__":
    unittest.main()

