"""Deterministic synthetic RKT4 v2 cases. These are not real flight records."""
from __future__ import annotations

import struct


def make_log(timestamps=(0, 20_000, 40_000), *, rate=50,
             pressure=101325.0, validity=1) -> bytes:
    header = struct.pack("<4sHHHHfH14x", b"RKT4", 2, 32, 16, rate,
                         101325.0, 3900)
    records = b"".join(struct.pack("<IffHH", stamp, pressure, 21.5,
                                   3890, validity) for stamp in timestamps)
    return header + records


def completion_text(data: bytes, name="V12D001.BIN") -> str:
    return ("GREEN-ION-COMPLETION-1\nstatus=complete\n"
            f"file={name}\nfile_bytes={len(data)}\n"
            f"complete_records={max(0, (len(data) - 32) // 16)}\n"
            "duration_ms=1200\n")


def cases():
    """Return fresh byte buffers and expected structural states for each case."""
    valid = make_log()
    bad_magic = b"NOPE" + valid[4:]
    bad_version = bytearray(valid)
    struct.pack_into("<H", bad_version, 4, 3)
    bad_rate = bytearray(valid)
    struct.pack_into("<H", bad_rate, 10, 99)
    invalid_pressure = bytearray(valid)
    struct.pack_into("<f", invalid_pressure, 32 + 4, float("nan"))
    completion = completion_text(valid)
    return {
        "empty": (b"", None, "incomplete"),
        "tiny": (b"RKT4", None, "incomplete"),
        "valid": (valid, completion, "structurally_complete"),
        "missing_txt": (valid, None, "unverified"),
        "blank_txt": (valid, "", "incomplete"),
        "bad_magic": (bad_magic, completion, "invalid"),
        "bad_version": (bytes(bad_version), completion, "invalid"),
        "bad_rate": (bytes(bad_rate), completion, "invalid"),
        "partial_tail": (valid + b"\x01\x02\x03", completion, "incomplete"),
        "whole_record_cut": (valid[:-16], completion, "incomplete"),
        "wrong_name": (valid, completion.replace("V12D001.BIN", "V12D002.BIN"), "incomplete"),
        "wrong_bytes": (valid, completion.replace("file_bytes=80", "file_bytes=96"), "incomplete"),
        "wrong_count": (valid, completion.replace("complete_records=3", "complete_records=2"), "incomplete"),
        "missing_marker": (valid, completion.replace("GREEN-ION-COMPLETION-1\n", ""), "incomplete"),
        "partial_metadata": (valid, "GREEN-ION-COMPLETION-1\nstatus=complete\n", "incomplete"),
        "contradictory_name": (valid, completion + "file=OTHER.BIN\n", "incomplete"),
        "duplicate_time": (make_log((0, 20_000, 20_000)), None, "unverified"),
        "backward_time": (make_log((0, 40_000, 20_000)), None, "unverified"),
        "clock_wrap": (make_log((0xFFFFD000, 0x1E20)), None, "unverified"),
        "timestamp_gap": (make_log((0, 20_000, 500_000)), None, "unverified"),
        "invalid_pressure": (bytes(invalid_pressure), completion, "invalid"),
        "invalid_validity": (make_log(validity=7), completion, "invalid"),
        "missing_sensor": (make_log(validity=0), completion, "structurally_complete"),
    }
