# GREEN ION Past Versions

This folder contains every Green Ion version that is not the current release.

- `V1` through `V7` contain historical records where original firmware was not preserved.
- `Green_Ion_V8` through `Green_Ion_V9` contain preserved firmware snapshots.
- The current release remains at the repository root so it is easy to find and compile.

## Future release rule

Before adding a new root-level firmware folder, move the formerly current firmware folder here without modifying its contents. Update `README.md`, `CHANGELOG.md`, `VERSIONS.md`, tests, and the GitHub Actions workflow to point to the new root-level release.

Archived releases are retained for reference. Use the root-level firmware for new installations.
