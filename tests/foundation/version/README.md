# tests/foundation/version

- `test_version.cpp`: the version string reads as the three numbers the constants hold.
- `compile_export.cpp`: at compile time, `SUSHIPROFILER_API` can mark a declaration. That the symbols
  are exported and imported is checked by the link of every test program.
