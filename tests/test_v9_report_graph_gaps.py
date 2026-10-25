#!/usr/bin/env python3
"""Check that printed flight graphs do not connect across invalid measurements."""
import json
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")
START = "function reportGraph(rows,column,title){"
END = "\nfunction exportFlightPDF()"
assert DASH.count(START) == 1 and DASH.count(END) == 1
GRAPH_JS = START + DASH.split(START, 1)[1].split(END, 1)[0]


@unittest.skipUnless(shutil.which("node"), "Node.js is required for browser graph tests")
class ReportGraphTests(unittest.TestCase):
    def graph(self, rows):
        harness = """
const operations = [];
const ctx = {
 fillRect(){}, strokeRect(){}, fillText(){}, beginPath(){},
 moveTo(){operations.push("M")}, lineTo(){operations.push("L")},
 stroke(){}, setLineDash(){}
};
const document = {createElement: () => ({
 getContext: () => ctx,
 toDataURL: () => "data:image/png;base64,dGVzdA=="
})};
const flightPhases = [];
const safeHTML = v => String(v);
"""
        harness += GRAPH_JS + "\n"
        harness += "const rows=" + json.dumps(rows) + ";\n"
        harness += 'const html=reportGraph(rows,5,"Altitude");\n'
        harness += "console.log(JSON.stringify({operations,html}));\n"
        result = subprocess.run(
            ["node", "-e", harness], capture_output=True, text=True,
            check=True, timeout=15,
        )
        return json.loads(result.stdout)

    @staticmethod
    def sample(t, valid=True):
        return [t, 101325, 20, 3.9, 1 if valid else 0, 100 + t]

    def test_continuous_valid_samples_draw_one_line(self):
        result = self.graph([self.sample(i) for i in range(4)])
        self.assertEqual(result["operations"], ["M", "L", "L", "L"])

    def test_invalid_sample_splits_line(self):
        result = self.graph([
            self.sample(0), self.sample(1), self.sample(2, False),
            self.sample(3), self.sample(4),
        ])
        self.assertEqual(result["operations"], ["M", "L", "M", "L"])

    def test_downsampled_graph_keeps_gap(self):
        rows = [self.sample(i) for i in range(5001)]
        rows[2500] = self.sample(2500, False)
        result = self.graph(rows)
        self.assertGreaterEqual(result["operations"].count("M"), 2)

    def test_insufficient_data_is_explained(self):
        result = self.graph([self.sample(0, False), self.sample(1)])
        self.assertIn("insufficient data", result["html"])
        self.assertFalse(result["operations"])


if __name__ == "__main__":
    unittest.main()
