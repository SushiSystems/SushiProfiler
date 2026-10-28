# KI-1 In PowerShell `sp build` runs Set-ItemProperty instead of building

The row of this entry is in `docs/backlog/COMMAND_LINE.md`. This file holds what did not fit the row.

- Origin: own
- Where: `cli/pyproject.toml`
- Cause: `sp` is PowerShell's built-in alias of `Set-ItemProperty`
- Recognise: PowerShell asks for a `Path` or `Name` parameter, or reports a missing property
- Workaround: run `sushiprofiler build`
- Fix: none, `sushiprofiler` is the same command under its full name
