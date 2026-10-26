# Known issues

Next id: KI-2

## Command line

### KI-1 In PowerShell `sp build` runs Set-ItemProperty instead of building
- Origin: own
- Where: `cli/pyproject.toml`
- Cause: `sp` is PowerShell's built-in alias of `Set-ItemProperty`
- Recognise: PowerShell asks for a `Path` or `Name` parameter, or reports a missing property
- Workaround: run `sushiprofiler build`
- Fix: none, `sushiprofiler` is the same command under its full name
