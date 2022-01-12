/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
let g_maxTime = 2000;
var g_timeoutId = undefined;
var g_testResolve = undefined;
var g_testResult = undefined;

function completeTestFunction(result)
{
    // Reset timeout
    if (g_timeoutId !== undefined) {
        clearTimeout(g_timeoutId);
        g_timeoutId = undefined;
    }

    // Set test result directy, or resolve the pending promise
    if (g_testResolve === undefined) {
        g_testResult = result
    } else {
        g_testResolve(result);
        g_testResolve = undefined;
    }
}

function runTestFunction(instance, name)
{
    if (g_timeoutId !== undefined)
        console.log("existing timer found");

    // Set timer which will catch test functions
    // which fail to call completeTestFunction()
    g_timeoutId = setTimeout( () => {
        if (g_timeoutId === undefined)
            return;
        g_timeoutId = undefined;
        completeTestFunction("FAIL")
    }, g_maxTime);

    instance.runTestFunction(name);

    // If the test function completed with a result immediately then return
    // the result directly, otherwise return a Promise to the result.
    if (g_testResult !== undefined) {
        let result = g_testResult;
        g_testResult = undefined;
        return result;
    } else {
        return new Promise((resolve) => {
            g_testResolve = resolve;
        });
    }
}

async function runTestCaseImpl(testFunctionStarted, testFunctionCompleted, qtContainers)
{
    // Create test case instance
    let config = {
        qtContainerElements : qtContainers || []
    }
    let instance = await createQtAppInstance(config);

    // Run all test functions
    let functionsString = instance.getTestFunctions();
    let functions = functionsString.split(" ").filter(Boolean);
    for (name of functions) {
        testFunctionStarted(name);
        let result = await runTestFunction(instance, name);
        testFunctionCompleted(name, result);
    }

    // Cleanup
    instance.cleanupTestCase();
}

var g_htmlLogElement = undefined;

function testFunctionStarted(name) {
    let line = name + ": ";
    g_htmlLogElement.innerHTML += line;
}

function testFunctionCompleted(name, status) {
    var color = "black";
    switch (status) {
        case "PASS":
            color = "green";
        break;
        case "FAIL":
            color = "red";
        break;
    }
    let line = "<text style='color:" + color + ";'>" + status + "</text><br>";
    g_htmlLogElement.innerHTML += line;
}

async function runTestCase(htmlLogElement, qtContainers)
{
    g_htmlLogElement = htmlLogElement;
    try {
        await runTestCaseImpl(testFunctionStarted, testFunctionCompleted, qtContainers);
        g_htmlLogElement.innerHTML += "<br> DONE"
    } catch (err) {
        g_htmlLogElement.innerHTML += err
    }
}
