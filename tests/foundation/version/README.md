# tests/foundation/version

- `test_version.cpp`: the version string equals the dotted numbers the three constants hold.
- `test_export.cpp`: a test program, which links the shared library, sees `SUSHIPROFILER_API` as an
  import.
