This package contains sources for a webpage whose scripts run batched WASM tests - a single
executable with a number of linked test classes.
The webpage operates on an assumption that the test program, when run without arguments,
prints out a list of test classes inside its module. Then, when run with the first argument
equal to the name of one of the test classes, the test program will execute all tests within
that single class.

The following query parameters are recognized by the webpage:

qtestname=testname - the test case to run. When batched test module is used, the test is assumed to
    be a part of the batch. If a standalone test module is used, this is assumed to be the name of
    the wasm module.

quseemrun - if specified, the test communicates with the emrun instance via the protocol expected
    by emrun.

qtestoutputformat=txt|xml|lightxml|junitxml|tap - specifies the output format for the test case.

qbatchedtest - if specified, the script will load the test_batch.wasm module and either run all
    testcases in it or a specific test case, depending on the existence of the qtestname parameter.
    Otherwise, the test is assumed to be a standalone binary whose name is determined by the
    qtestname parameter.

The scripts in the page will load the wasm file as specified by a combination of qbatchedtest and
qtestname.

Public interface for querying the test execution status is accessible via the global object
'qtTestRunner':

qtTestRunner.status - this contains the status of the test runner itself, of the enumeration type
RunnerStatus.

qtTestRunner.results - a map of test class name to test result. The result contains a test status
(status, of the enumeration TestStatus), text output chunks (output), and in case of a terminal
status, also the test's exit code (exitCode)

qtTestRunner.onStatusChanged - an event for changes in state of the runner itself. The possible
values are those of the enumeration RunnerStatus.

qtTestRunner.onTestStatusChanged - an event for changes in state of a single tests class. The
possible values are those of the enumeration TestStatus. When a terminal state is reached
(Completed, Error, Crashed), the text results and exit code are filled in, if available, and
will not change.

Typical usage:
Run all tests in a batch:
    - load the webpage batchedtestrunner.html

Run a single test in a batch:
    - load the webpage batchedtestrunner.html?qtestname=tst_mytest

Query for test execution state:
    - qtTestRunner.onStatusChanged.addEventListener((runnerStatus) => (...)))
    - qtTestRunner.onTestStatusChanged.addEventListener((testName, status) => (...))
    - qtTestRunner.status === (...)
    - qtTestRunner.results['tst_mytest'].status === (...)
    - qtTestRunner.results['tst_mytest'].textOutput

When queseemrun is specified, the built-in emrun support module will POST the test output to the
emrun instance and will report ^exit^ with a suitable exit code to it when testing is finished.
