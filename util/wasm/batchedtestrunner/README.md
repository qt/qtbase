This package contains sources for a webpage whose scripts run batched WASM tests - a single
executable with a number of linked test classes.
The webpage operates on an assumption that the test program, when run without arguments,
prints out a list of test classes inside its module. Then, when run with the first argument
equal to the name of one of the test classes, the test program will execute all tests within
that single class.

The scripts in the page will load the wasm file called 'test_batch.wasm' with its corresponding
js script 'test_batch.js'.

Public interface for querying the test execution status is accessible via the global object
'qtTestRunner':

qtTestRunner.status - this contains the status of the test runner itself, of the enumeration type
RunnerStatus.

qtTestRunner.results - a map of test class name to test result. The result contains a test status
(status, of the enumeration TestStatus), and in case of a terminal status, also the test's exit code
(exitCode) and xml text output (textOutput), if available.

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
