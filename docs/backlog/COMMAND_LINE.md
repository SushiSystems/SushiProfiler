# Command line

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-36 | P2 | open | Never remove a build tree in a build; refuse one CMake cannot reuse | sp build keeps every tree, refuses another generator or source folder, and names sp clean |  |  | 2026-11-21 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
| KI-1 | own | In PowerShell `sp build` runs Set-ItemProperty instead of building | run `sushiprofiler build` | none, `sushiprofiler` is the same command under its full name |  | 2026-10-26 |
