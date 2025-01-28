# RunCMake tests

These test suites use upstream [CMake's `RunCMake`][RunCMake-cmake] test module. See the upstream
documentation on how to add tests.

A few minor adjustments we make:
- `add_RunCMake_test()` function is defined in `QtRunCMakeTestHelpers` module
instead of the `CMakeLists.txt`
- `CMAKE_CMAKE_COMMAND` is replaced with `CMAKE_COMMAND`
- `CMAKE_MODULE_PATH` points to `CMAKE_CURRENT_LIST_DIR` (indirectly via variable `RunCMakeDir`)
  instead of `CMAKE_CURRENT_SOURCE_DIR`

Update `/cmake/3rdparty/cmake` as needed. Last fetched from `v3.31.5`.

[RunCMake-cmake]: https://gitlab.kitware.com/cmake/cmake/-/blob/master/Tests/RunCMake/README.rst
