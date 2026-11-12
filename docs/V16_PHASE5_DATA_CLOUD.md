# GREEN ION V16 Phase 5 — Local-first data and optional cloud

## Goal

A flight must remain usable with no internet and after a browser/app restart. The data path is deliberately ordered:

1. Feather SD recording is the primary onboard evidence.
2. Finished files are downloaded only after recording is inactive.
3. The browser stores immutable originals by SHA-256.
4. Windows localhost mirrors the same originals to the user data folder when available.
5. Supabase is an optional additional private copy after internet returns and the user is signed in.

Cloud failure never blocks launch, recording, local recovery, file download or local analysis.

## One flight library entry

V16 groups files by flight base name so one flight is shown as one library entry rather than three unrelated files.

- BIN is the primary RKT4 recording.
- SEN is the optional GIS1 high-resolution sensor companion.
- TXT is the optional metadata companion.

A BIN without SEN/TXT is still a usable preserved recording and is labeled RECORDED, not corrupt. BIN+SEN+TXT is labeled FULL. Companion-only entries are visible. If two different checksums claim the same flight/kind, V16 reports COLLISION and does not silently choose/overwrite the conflicting original.

## Local integrity

Every browser original is content-addressed by SHA-256. Phase 5 adds a Verify Local Checksums action that rereads stored bytes and compares the digest without modifying originals.

The manifest export records packages, checksums, sizes, sources and mirror/cloud status. Checksums prove stored-byte integrity; they do not prove sensor accuracy, flight validity or RF/GPS quality.

## Feather transfer recovery

Finished BIN imports now use a persistent transfer journal.

- DOWNLOADING increments a bounded attempt count.
- Partial byte counts are rejected.
- Failed transfers become DEFERRED with exponential retry delay.
- Browser restart preserves retry state.
- A verified matching local file is skipped instead of downloaded again.
- Successful transfer records the final SHA-256.
- Companion downloads remain optional and cannot invalidate a safely stored BIN.
- Imports run in bounded batches after recording/maintenance/home-test/update activity is inactive.

This is resumable at file granularity: completed files are never re-uploaded/re-downloaded just because a later file failed. It intentionally does not claim HTTP byte-range resume support that the Feather does not provide.

## Windows disk mirror

The localhost ground station remains an additional content-addressed copy under the Windows user data directory. It verifies the posted checksum and never overwrites an existing original. Browser startup/reconciliation can restore a missing browser copy from that verified local disk archive.

## Supabase backup queue

The browser cloud state uses schema `GREEN-ION-V16-CLOUD-1` and exposes:

- signed-out / unavailable / offline;
- pending / syncing / retrying / synced;
- restoring / error;
- retry time and failure count;
- last successful backup time;
- pending count;
- per-file checksum status and attempt count.

Backup is file-granular and idempotent:

- the remote metadata list is compared by SHA-256;
- already stored checksums are skipped;
- storage uses a user-owned path `<user-id>/<sha256>.<kind>`;
- object upload uses no overwrite/upsert;
- a conflict from an already-present object is treated as retry-safe success;
- metadata is upserted by `(user_id, checksum)`;
- after interruption, the next run queries remote checksums and resumes with only missing files;
- failures back off up to the existing bounded retry window;
- original local files remain available throughout.

Cloud restore validates metadata, byte count and SHA-256 before saving a restored original. Existing originals are not overwritten.

## Live Supabase backend audit

A read-only audit of the connected GREEN ION Supabase project on November 11, 2026 confirmed:

- `public.flight_backups` exists with RLS enabled and primary key `(user_id, checksum)`;
- authenticated SELECT/INSERT/UPDATE/DELETE privileges are present and RLS policies restrict rows to `auth.uid()`;
- INSERT and UPDATE include owner checks;
- `green-ion-flights` is a private bucket with a 52,428,800-byte file limit;
- storage object policies restrict access to the authenticated user's first path segment;
- the browser uses the publishable key only; no service-role secret is placed in repository/browser code.

Supabase's security advisor currently reports leaked-password protection disabled. That setting is not changed by this code phase because it changes account-password policy; it should be reviewed separately in the Supabase Auth settings.

No live user sign-in/upload/restore transaction was performed by CI, so account email flow and an actual cross-device restore still require end-to-end validation.

## 2026 Supabase setup note

For a newly recreated project, granting a role table privileges and enabling RLS are separate requirements. The connected project already has authenticated table privileges plus owner-scoped RLS, so Phase 5 does not apply a new schema migration.

## Safety and compatibility

- Feather/Uno firmware is unchanged.
- RKT4 v2, GIS1 and the radio protocol are unchanged.
- No service-role or secret key is stored in browser source.
- No cloud OTA or remote flight commands are added.
- Deleting/signing out of cloud never deletes local originals.
- Cloud is not required for flight.
