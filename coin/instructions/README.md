# Information about Coin instruction templates

## Build templates

* ``coin_qtbase_build_template_v2.yaml`` did not exist. The build instructions were directly embedded into
  ``module_config.yaml`` and did not support repos outside of qtbase, also no cross-compilation.
* ``coin_qtbase_build_template_v2`` introduced support for building other repos, and also enabled
  build cross-compiling targets like ``Android`` and ``iOS``.
  A bit later the template gained the ability to build ``qemu`` cross-compiling configurations.
  The counterpart to qtbase to build other repositories is ``coin_module_build_template_v2``

## Test templates

* ``coin_module_test_template_v1`` did not exist. The test instructions were directly embedded into
  ``module_config.yaml`` and did not support repos outside of qtbase, also no cross-compilation.
* ``coin_module_test_template_v2`` introduced support for building tests for other repos, and made
  sure not to build and run tests on cross-compiling configuration. A bit later the template gained
  the ability to build and run tests for ``qemu`` cross-compiling configurations.
* ``coin_module_test_template_v3`` changed the run test instructions to not ignore the exit code
  and thus enforce that tests pass in the CI.

# Environment variable description and usage

The following environment variables are used in Coin instructions when building Qt, tests, etc:

`CONFIGURE_ARGS`               - contains platform-specific ``configure-style`` arguments
                                 (e.g. `-shared`), that will be passed to a qtbase configure call
`CMAKE_ARGS`                   - contains platform-specific ``CMake-style`` arguments
                                 (e.g. `-DOPENSSL_ROOT_DIR=Foo`) that will be passed to a qtbase
                                 configure call
`NON_QTBASE_CONFIGURE_ARGS`    - contains platform-specific ``configure-style`` arguments
                                 that will be passed to a non-qtbase qt-configure-module call
`NON_QTBASE_CMAKE_ARGS`        - contains platform-specific ``CMake-style`` arguments
                                 that will be passed to a non-qtbase qt-configure-module call
`COMMON_CMAKE_ARGS`            - platform-independent ``CMake-style`` args set in
                                 `prepare_building_env.yaml` that apply to qtbase configurations
                                  only.
`COMMON_NON_QTBASE_CMAKE_ARGS` - platform-independent ``CMake-style`` args set in
                                 `prepare_building_env.yaml` that apply to
                                 configuration of repos other than qtbase
`COMMON_TEST_CMAKE_ARGS`       - platform-independent ``CMake-style`` args set in
                                 `prepare_building_env.yaml` that apply to configuration of
                                 all standalone tests

All of the above apply to host builds only.

There is a a set of environment variables that apply to target builds when cross-compiling which
mirror the ones above. They are:

`TARGET_CONFIGURE_ARGS`
`TARGET_CMAKE_ARGS`
`NON_QTBASE_TARGET_CONFIGURE_ARGS`
`NON_QTBASE_TARGET_CMAKE_ARGS`

`COMMON_TARGET_CMAKE_ARGS`
`COMMON_NON_QTBASE_TARGET_CMAKE_ARGS`
`COMMON_TARGET_TEST_CMAKE_ARGS`

Currently, there are no common ``configure-style`` variables for configuring
repos or tests, only ``CMake-style` ones.


`COIN_CMAKE_ARGS` contains the final set of cmake args that is passed to
`configure` / `qt-configure-module`, it is built up from the variables above + any additional values added
by custom instructions, like specification of `CMAKE_INSTALL_PREFIX` etc.

`INSTALL_DIR_SUFFIX` is used to append either `/host` or `/target` suffixes to install paths in
instructions when cross-building.

`CONFIGURE_EXECUTABLE` contains a platform-specific path to `configure` / `qt-configure-module`
or `cmake`/ `qt-cmake` depending on whether `UseConfigure` feature is enabled.

`CONFIGURE_ENV_PREFIX` contains the value of either `ENV_PREFIX` or `TARGET_ENV_PREFIX` depending on
whether it's a cross-build configure call. The values are used when configuring and building, to ensure
that things like compilers are found correctly.

We use `unixPathSeparators` to pass an install prefix with forward slashes even on Windows,
to avoid escaping issues when using configure.
