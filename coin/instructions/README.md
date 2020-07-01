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
