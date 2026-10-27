# RKT4 version 2 binary format

This is the immutable raw flight-log format emitted by the V10 recorder baseline and consumed by V11 browser tools. All multi-byte values are little-endian.

## Header: 32 bytes

| Offset | Size | Type | Meaning |
|---:|---:|---|---|
| 0 | 4 | ASCII | `RKT4` magic |
| 4 | 2 | uint16 | Format version: `2` |
| 6 | 2 | uint16 | Header size: `32` |
| 8 | 2 | uint16 | Record size: `16` |
| 10 | 2 | uint16 | Selected sample rate: 10, 25, or 50 Hz |
| 12 | 4 | float32 | Calibrated baseline pressure in Pa |
| 16 | 2 | uint16 | Battery millivolts at recording start |
| 18 | 14 | bytes | Reserved, zero-filled by V10 |

## Records: 16 bytes each

| Offset | Size | Type | Meaning |
|---:|---:|---|---|
| 0 | 4 | uint32 | `micros()` timestamp in microseconds; wraps at uint32 |
| 4 | 4 | float32 | Raw BMP388 pressure in Pa |
| 8 | 4 | float32 | BMP388 temperature in degrees C |
| 12 | 2 | uint16 | Battery millivolts |
| 14 | 2 | uint16 | `1` valid BMP388 read, `0` invalid/missing read |

An interrupted file is still usable when its header is valid: parse only `floor((file_size - 32) / 16)` complete records and report the remaining trailing bytes. Original BIN files must never be changed during analysis or recovery.

## Transfer contract

`GET /api/download?name=FLT0001.BIN` is unavailable while recording and responds with `409`. Successful responses are streamed with `application/octet-stream`, `Content-Disposition`, `X-Green-Ion-Bytes`, and `X-Green-Ion-RKT4-Version: 2`. A browser must compare received bytes with `X-Green-Ion-Bytes`; HTTP success alone is not integrity verification.
