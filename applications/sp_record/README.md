# sp_record

The record tool as a program: it passes its arguments to `run_record_tool` from
`modules/tools/record_tool/` and returns its exit code. `sp build` builds it into
`build/applications/<config>/`, and `sp record` runs it.

```bash
sp record validate build/profiles/run.sprec.json
sp record show run.sprec.json --analysis breakdown --analysis top_spans --top 10
sp record compare before.sprec.json after.sprec.json
sp record check run.sprec.json budgets.json
```

The program holds no logic of its own, so it has no test; the tool's tests are in
`tests/tools/record_tool/`.
