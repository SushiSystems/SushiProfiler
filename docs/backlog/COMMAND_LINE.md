# Command line

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-35 | P1 | open | Add the shared clean command to sp and drop --clean from build and test | sp clean says what it would remove and removes trees only with --yes, and sp build and test have no --clean |  | `cli/sushiprofiler/cli.py` | 2026-11-21 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
| KI-1 | own | In PowerShell `sp build` runs Set-ItemProperty instead of building | run `sushiprofiler build` | none, `sushiprofiler` is the same command under its full name |  | 2026-10-26 |
