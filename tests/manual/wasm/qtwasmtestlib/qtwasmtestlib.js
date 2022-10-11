// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// A minimal async test runner for Qt async auto tests.
//
// Usage: Call runTest(name, testFunctionCompleted), where "name" is the name of the app
// (the .wasm file name), and testFunctionCompleted is a test-function-complete
// callback. The test runner will then instantiate the app and run tests.
//
// The test runner expects that the app instance defines the following
// functions:
//
//  void cleanupTestCase()
//  string getTestFunctions()
//  runTestFunction(string)
//
// Further, the test runner expects that the app instance calls
// completeTestFunction() (below - note that both the instance and this
// file have a function with that name) when a test function finishes. This
// can be done during runTestFunction(), or after it has returned (this
// is the part which enables async testing). Test functions which fail
// to call completeTestFunction() will time out after 2000ms.
//
const g_maxTime = 2000;

class TestFunction {
    constructor(instance, name) {
        this.instance = instance;
        this.name = name;
        this.resolve = undefined;
        this.reject = undefined;
        this.timeoutId = undefined;
    }

    complete(result, details) {
        // Reset timeout
        clearTimeout(this.timeoutId);
        this.timeoutId = undefined;

        const callback = result.startsWith('FAIL') ? this.reject : this.resolve;
        callback(`${result}${details ? ': ' + details : ''}`);
    }

    run() {
        // Set timer which will catch test functions
        // which fail to call completeTestFunction()
        this.timeoutId = setTimeout(() => {
            completeTestFunction(this.name, 'FAIL', `Timeout after ${g_maxTime} ms`)
        }, g_maxTime);

        return new Promise((resolve, reject) => {
            this.resolve = resolve;
            this.reject = reject;

            this.instance.runTestFunction(this.name);
        });
    }
};

function completeTestFunction(testFunctionName, result, details) {
    if (!window.currentTestFunction || testFunctionName !== window.currentTestFunction.name)
        return;

    window.currentTestFunction.complete(result, details);
}

async function runTestFunction(instance, name) {
    if (window.currentTestFunction) {
        throw new Error(`While trying to run ${name}: Last function hasn't yet finished`);
    }
    window.currentTestFunction = new TestFunction(instance, name);
    try {
        const result = await window.currentTestFunction.run();
        return result;
    } finally {
        delete window.currentTestFunction;
    }
}

async function runTestCaseImpl(testFunctionStarted, testFunctionCompleted, qtContainers) {
    // Create test case instance
    const config = {
        qtContainerElements: qtContainers || []
    }
    const instance = await createQtAppInstance(config);

    // Run all test functions
    const functionsString = instance.getTestFunctions();
    const functions = functionsString.split(" ").filter(Boolean);
    for (const name of functions) {
        testFunctionStarted(name);
        try {
            const result = await runTestFunction(instance, name);
            testFunctionCompleted(result);
        } catch (err) {
            testFunctionCompleted(err.message ?? err);
        }
    }

    // Cleanup
    instance.cleanupTestCase();
}

var g_htmlLogElement = undefined;

function testFunctionStarted(name) {
    let line = name + ": ";
    g_htmlLogElement.innerHTML += line;
}

function testFunctionCompleted(status) {

    const color = (status) => {
        if (status.startsWith("PASS"))
            return "green";
        if (status.startsWith("FAIL"))
            return "red";
        if (status.startsWith("SKIP"))
            return "tan";
        return "black";
    };

    const line = `<span style='color: ${color(status)};'>${status}</text><br>`;
    g_htmlLogElement.innerHTML += line;
}

async function runTestCase(htmlLogElement, qtContainers) {
    g_htmlLogElement = htmlLogElement;
    try {
        await runTestCaseImpl(testFunctionStarted, testFunctionCompleted, qtContainers);
        g_htmlLogElement.innerHTML += "<br> DONE"
    } catch (err) {
        g_htmlLogElement.innerHTML += err
    }
}
