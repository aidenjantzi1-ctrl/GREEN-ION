# GREEN ION V16.2 — SD file management

Primary flight files remain `V12D###.BIN` (RKT4 v2) with optional/associated `.SEN` (GIS1), `.TXT` metadata and recovery checkpoint/evidence files.

## Download

Flights & Analysis can:

- refresh Feather SD files;
- open flight details;
- download an individual BIN/TXT/CSV;
- download a BIN + SEN + TXT package;
- SHA-256 check transferred bytes against a verified browser original when one already exists;
- show browser, Windows archive and cloud backup evidence.

## Delete

V16.2 only enables the safe cleanup workflow for a flight with clean completion evidence.

Before deletion the app shows:

- exact filename and size;
- completion/recovery state;
- browser copy status;
- Windows archive status;
- cloud status;
- deletion verification behavior.

If no checksum-verified external backup is known, the app warns you to download/verify one first.

Deletion requires typing the exact `V12D###.BIN` filename. Firmware removes companion/recovery files first and the BIN last, then verifies the package is absent. Interrupted/recovery-evidence flights are retained for review.

Deletion is blocked during active recording, home testing, OTA/maintenance, open recorder files and other critical SD operations.

Deleting from Feather SD never deletes browser IndexedDB, `GREEN_ION_Data` or cloud copies.
