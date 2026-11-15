# GREEN ION ChatGPT role prompts

Copy the contents of any role file into a separate ChatGPT/Codex conversation or custom GPT instructions. **These prompts define responsibilities; they do not automatically create agents, schedule tasks, or grant repository access.** Connect GitHub and explicitly authorize edits as needed.

| File | Role | Ownership |
|---|---|---|
| [01_Project_Coordinator.md](01_Project_Coordinator.md) | Overall GREEN ION coordinator | Scope, delegation, status, priorities and final user guidance |
| [02_Primary_Coder.md](02_Primary_Coder.md) | Core firmware engineer | Feather, SD-first recording, sensors, flight state, firmware API |
| [03_Secondary_Coder.md](03_Secondary_Coder.md) | UI and analysis engineer | Quick Fly, dashboard, mobile/offline analyzer and tests |
| [04_Integration_Manager.md](04_Integration_Manager.md) | Release and integration engineer | Review PRs, resolve conflicts, CI, merge safely, version alignment |
| [05_Estes_So_Long_Build_Helper.md](05_Estes_So_Long_Build_Helper.md) | Physical rocket and payload helper | Estes build, GREEN ION mounting, stability, recovery and preflight |

## Suggested workflow
1. Open **Project Coordinator** first and establish the active milestone and current GitHub state.
2. Give **Primary Coder** and **Secondary Coder** separate branches and bounded changes. Have them open separate PRs, not edit main directly.
3. Have **Integration Manager** review both PRs and coordinate conflict resolution. Only merge after required tests pass; avoid modifying other people's branches without coordination.
4. Use **So Long Build Helper** for hardware build guidance and physical measurements, *not* for pushing firmware changes.
5. Report PR URLs, commit SHAs, workflow results and hardware-test unknowns back to the coordinator.

## Current priorities (verify against latest main)
The V17.0.1 SD-first baseline came from PR #55. Quick Fly PR #56 and integration PR #53 were draft/conflicted when these prompts were created: **recheck their live status**, don't assume they remain unfinished. SD-first 50 Hz flight logging, one-press PREPARE & ARM, repeated flights/FLY AGAIN, and LoRa/GPS-optional autonomy take precedence over cosmetic features.

## Important distinctions
- A **successful compile/CI** means the software checks ran, not that a rocket is flight qualified.
- RKT4 v2 records pressure, temperature, battery, time and validity; **do not claim it contains IMU acceleration** unless a newer verified format says so.
- Two-stage rocket integration can change CG, recovery and staging. Confirm manufacturer guidance and obtain a mentor/range safety review before flight.

## Source references
- [GREEN ION repository](https://github.com/aidenjantzi1-ctrl/GREEN-ION)
- [RKT4 v2 format](https://github.com/aidenjantzi1-ctrl/GREEN-ION/blob/main/docs/RKT4_V2_FORMAT.md)
- [Estes](https://estesrockets.com/)
- [NAR](https://www.nar.org/)
