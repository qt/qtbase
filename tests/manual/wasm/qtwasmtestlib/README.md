QtWasmTestLib - async auto tests for WebAssembly
================================================

QtWasmTestLib supports auto-test cases in the web browser. Like QTestLib, each
test case is defined by a QObject subclass with one or more test functions. The
test functions may be asynchronous, where they return early and then complete
at some later point.

The test lib is implemented as a C++ and JavaScript library, where the test is written
using C++ and a hosting html page calls JavaScript API to run the test.

Implementing a basic test case
------------------------------

In the test cpp file, define the test functions as private slots. All test
functions must call completeTestFunction() exactly once, or will time out
otherwise. Subsequent calls to completeTestFunction will be disregarded.
It is advised to use QWASMSUCCESS/QWASMFAIL for reporting the test execution
status and QWASMCOMPARE/QWASMVERIFY to assert on test conditions. The call can
be made after the test function itself has returned.

    class TestTest: public QObject
    {
        Q_OBJECT
    private slots:
        void timerTest() {
            QTimer::singleShot(timeout, [](){
                completeTestFunction();
             });
        }
    };

Then define a main() function which calls initTestCase(). The main()
function is async too, as per Emscripten default. Build the .cpp file
as a normal Qt for WebAssembly app.

    int main(int argc, char **argv)
    {
        auto testObject = std::make_shared<TestTest>();
        initTestCase<QCoreApplication>(argc, argv, testObject);
        return 0;
    }

Finally provide an html file which hosts the test runner and calls runTestCase()

    <!doctype html>
    <script type="text/javascript" src="qtwasmtestlib.js"></script>
    <script type="text/javascript" src="test_case.js"></script>
    <script>
        window.onload = async () => {
            runTestCase(document.getElementById("log"));
        };
    </script>
    <p>Running Foo auto test.</p>
    <div id="log"></div>

Implementing a GUI test case
----------------------------

This is similar to implementing a basic test case, with the difference that the hosting
html file provides container elements which becomes QScreens for the test code.

    <!doctype html>
    <script type="text/javascript" src="qtwasmtestlib.js"></script>
    <script type="text/javascript" src="test_case.js"></script>
    <script>
        window.onload = async () => {
            let log = document.getElementById("log")
            let containers = [document.getElementById("container")];
            runTestCase(log, containers);
        };
    </script>
    <p>Running Foo auto test.</p>
    <div id="container"></div>
    <div id="log"></div>
