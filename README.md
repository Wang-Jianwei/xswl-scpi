# xswl-scpi

## Tests ✅

Build and run tests (CTest):

- Configure and build with tests enabled (default):

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Release
```

- Run tests with CTest:

```bash
ctest --test-dir build --output-on-failure
# or, from the build dir:
# cmake --build build --target check
```

Tests are organized in `tests/` and integrated with CTest. Use `ctest -L <label>` to run tests with a specific label (e.g., `ctest -L phase2`).