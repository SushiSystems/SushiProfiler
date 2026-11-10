# cli/tests

The pytest suite of `sp`: `cd cli && python -m pytest tests -q`. Every test carries one size mark
(`small`, `medium`, `large`) on its class or file; `conftest.py` fails collection otherwise and sets
the timeout from it.

- `conftest.py`: the size rule, and the `project` fixture, a scratch checkout whose workspace runs on
  sushicore's `RecordingRunner` and `RecordingConsole`, so a test reads the argv a command produced.
- `test_cli.py`: medium. Each command parses its options and hands them to the project service; the
  help lists every command, `test --all` selects every size, and a failing service becomes the exit code.
- `test_config.py`: small. The compiler and generator choice, on a `MemoryHost`.
- `test_console.py`: small, a `LazyConsole` not yet used has built nothing and asked for no folder;
  medium, `--help` works outside a checkout (it changes the working folder).
- `test_env.py`: small, without a compiler the environment is the host's, as a copy; medium, a found
  compiler's folder comes first on PATH (it creates the build folder under a temporary folder).
- `test_host.py`: `HostContract`, the promises of the `Host` port, run on `MemoryHost` (small) and on
  `SystemHost` (medium, it reads the real environment).
- `test_main.py`: medium. `python -m sushiprofiler --help` runs in a child process.
- `test_package_contract.py`: medium. The `--lean` build, the `package` command and `SUSHISTACK_CXX`.
- `services/test_project.py`: medium. The command lines the project service hands cmake, ctest and the
  checkers, one test per option; a clean build removes the tree before it configures.
