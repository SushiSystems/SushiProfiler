# Command line

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-34 | P2 | open | Add sp run, which picks and runs a built program like the sibling CLIs | sp run NAME runs a built program and sp run --pick lists the built examples with --all for every program |  | `cli/sushiprofiler/services/project.py` | 2026-11-21 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
| KI-1 | own | In PowerShell `sp build` runs Set-ItemProperty instead of building | run `sushiprofiler build` | none, `sushiprofiler` is the same command under its full name |  | 2026-10-26 |
