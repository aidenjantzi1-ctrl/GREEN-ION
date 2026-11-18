#!/usr/bin/env python3
"""Read-only, offline audit of original RKT4 v2 flight files.

The format has no per-record checksum. A structurally valid file therefore
cannot be called corruption-free without independent completion evidence.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

HEADER = 32
RECORD = 16
RATES = {10, 25, 50}
UINT32 = 1 << 32


def audit_bytes(data: bytes, *, metadata: str | None = None,
                recovery: str | None = None, journal_present: bool = False,
                expected_name: str | None = None) -> dict:
    result = {
        "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data),
        "complete_records": 0, "trailing_bytes": 0,
        "status": "incomplete", "issues": [], "warnings": [],
    }
    issues, warnings = result["issues"], result["warnings"]
    if len(data) < HEADER:
        issues.append("short_header")
        return result
    magic, version, header_size, record_size, rate, baseline, battery = struct.unpack_from(
        "<4sHHHHfH", data)
    if (magic, version, header_size, record_size) != (b"RKT4", 2, HEADER, RECORD):
        issues.append("invalid_rkt4_v2_header")
        result["status"] = "invalid"
        return result
    result.update(rate_hz=rate, baseline_pressure_pa=baseline,
                  start_battery_mv=battery)
    if rate not in RATES:
        issues.append("invalid_sample_rate")
    if not math.isfinite(baseline) or baseline <= 0:
        issues.append("invalid_baseline_pressure")
    count, tail = divmod(len(data) - HEADER, RECORD)
    result.update(complete_records=count, trailing_bytes=tail)
    if tail:
        issues.append("truncated_record_tail")
    if count == 0:
        issues.append("no_complete_records")
    previous = None
    invalid_reads = 0
    gaps = 0
    nonmonotonic = 0
    bad_values = 0
    for index in range(count):
        stamp, pressure, temperature, mv, valid = struct.unpack_from(
            "<IffHH", data, HEADER + index * RECORD)
        if valid not in (0, 1):
            bad_values += 1
        elif valid == 0:
            invalid_reads += 1
        elif not (math.isfinite(pressure) and pressure > 0 and
                  math.isfinite(temperature)):
            bad_values += 1
        if previous is not None and rate in RATES:
            delta = (stamp - previous) % UINT32
            if delta == 0 or delta >= UINT32 // 2:
                nonmonotonic += 1
            elif delta > 4_000_000 // rate:
                gaps += 1
        previous = stamp
    result.update(invalid_sensor_reads=invalid_reads,
                  timestamp_gaps=gaps, nonmonotonic_timestamps=nonmonotonic)
    if bad_values:
        issues.append("invalid_record_fields")
    if invalid_reads:
        warnings.append("invalid_sensor_reads")
    if gaps:
        warnings.append("timestamp_gaps")
    if nonmonotonic:
        warnings.append("nonmonotonic_timestamps")
    if journal_present:
        issues.append("pending_journal")
    if recovery is not None:
        issues.append("interrupted_recovery_report")
    if metadata is None:
        warnings.append("missing_completion_metadata")
    else:
        lines = metadata.splitlines()
        fields = {}
        for line in lines[1:]:
            if "=" in line:
                key, value = line.split("=", 1)
                fields[key] = value
        try:
            metadata_matches = (lines[0] == "GREEN-ION-COMPLETION-1" and
                                fields.get("status") == "complete" and
                                int(fields["file_bytes"]) == len(data) and
                                int(fields["complete_records"]) == count and
                                (expected_name is None or
                                 fields.get("file", "").casefold() == expected_name.casefold()))
        except (IndexError, KeyError, ValueError):
            metadata_matches = False
        if not metadata_matches:
            issues.append("invalid_completion_metadata")
    # Even matching completion metadata cannot prove unchecksummed record bytes.
    if issues:
        result["status"] = "invalid" if any(issue in issues for issue in (
            "invalid_sample_rate", "invalid_baseline_pressure", "invalid_record_fields")) else "incomplete"
    elif metadata is None:
        result["status"] = "unverified"
    else:
        result["status"] = "structurally_complete"
        warnings.append("no_per_record_checksum")
    return result


def audit_file(path: Path) -> dict:
    # Resolve sidecars by name, without changing names or source bytes. This
    # also works for lowercase exports on case-sensitive test machines.
    siblings = {p.name.casefold(): p for p in path.parent.iterdir() if p.is_file()}
    def sidecar(extension: str) -> Path | None:
        return siblings.get((path.stem + extension).casefold())
    txt, rcv = sidecar(".TXT"), sidecar(".RCV")
    journals = [sidecar(s) for s in (".JRA", ".JRB")]
    return {"file": path.name, **audit_bytes(
        path.read_bytes(),
        metadata=txt.read_text(errors="replace") if txt else None,
        recovery=rcv.read_text(errors="replace") if rcv else None,
        journal_present=any(p is not None for p in journals),
        expected_name=path.name)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--summary", action="store_true",
                        help="emit a versioned JSON object with status counts and files")
    parser.add_argument("paths", nargs="+", type=Path,
                        help="RKT4 BIN files or directories containing them")
    args = parser.parse_args()
    files = set()
    unmatched = []
    for item in args.paths:
        if item.is_dir():
            found = {p for p in item.iterdir() if p.is_file() and p.suffix.casefold() == ".bin"}
        elif item.is_file() and item.suffix.casefold() == ".bin":
            found = {item}
        else:
            found = set()
        if not found:
            unmatched.append(str(item))
        files.update(found)
    files = sorted(files)
    reports = []
    for path in files:
        try:
            reports.append(audit_file(path))
        except OSError:
            reports.append({"file": path.name, "status": "unverified",
                            "issues": ["read_error"], "warnings": []})
    if args.summary:
        counts = {status: sum(r["status"] == status for r in reports)
                  for status in ("structurally_complete", "incomplete", "invalid", "unverified")}
        print(json.dumps({"schema": "green-ion-rkt4-audit-1",
                          "summary": {"total": len(reports), **counts},
                          "files": reports,
                          "errors": ["no_bin_files"] if unmatched else []}, indent=2))
    else:
        if reports:
            print(json.dumps(reports, indent=2))
        if unmatched:
            print("No RKT4 .BIN files found in one or more input paths", file=sys.stderr)
    if unmatched:
        return 2
    return 1 if any(r["status"] != "structurally_complete" for r in reports) else 0


if __name__ == "__main__":
    raise SystemExit(main())
