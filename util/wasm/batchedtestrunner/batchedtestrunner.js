// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import {
    AbortedError,
} from './qwasmjsruntime.js';

import { EventSource } from './util.js';

class ProgramError extends Error {
    constructor(exitCode) {
        super(`The program reported an exit code of ${exitCode}`)
    }
}

export class RunnerStatus {
    static Running = 'Running';
    static Passed = 'Passed';
    static Error = 'Error';
    static TestCrashed = 'TestCrashed';
    static TestsFailed = 'TestsFailed';
}

export class TestStatus {
    static Pending = 'Pending';
    static Running = 'Running';
    static Completed = 'Completed';
    static Error = 'Error';
    static Failed = 'Failed';
    static Crashed = 'Crashed';
}

export class BatchedTestRunner {
    static #TestBatchModuleName = 'test_batch';

    #loader;

    #results = new Map();
    #status = RunnerStatus.Running;
    #numberOfFailed = 0;
    #statusChangedEventPrivate;
    #testStatusChangedEventPrivate;
    #testOutputChangedEventPrivate;
    #errorDetails;

    onStatusChanged =
        new EventSource((privateInterface) => this.#statusChangedEventPrivate = privateInterface);
    onTestStatusChanged =
        new EventSource((privateInterface) =>
            this.#testStatusChangedEventPrivate = privateInterface);
    onTestOutputChanged =
        new EventSource(
            (privateInterface) => this.#testOutputChangedEventPrivate = privateInterface);

    constructor(loader) {
        this.#loader = loader;
    }

    get results() { return this.#results; }

    get status() { return this.#status; }

    get numberOfFailed() {
        if (this.#status !== RunnerStatus.TestsFailed)
            throw new Error(`numberOfFailed called with status=${this.#status}`);
        return this.#numberOfFailed;
    }

    get errorDetails() { return this.#errorDetails; }

    async run(targetIsBatch, testName, functions, testOutputFormat) {
        try {
            await this.#doRun(targetIsBatch, testName, functions, testOutputFormat);
        } catch (e) {
            this.#setTestRunnerError(e.message);
            return;
        }

        const status = (() => {
            const hasAnyCrashedTest =
                !![...window.qtTestRunner.results.values()].find(
                    result => result.status === TestStatus.Crashed);
            if (hasAnyCrashedTest)
                return { code: RunnerStatus.TestCrashed };
            const numberOfFailed = [...window.qtTestRunner.results.values()].reduce(
                (previous, current) => previous + current.exitCode, 0);
            return {
                code: (numberOfFailed ? RunnerStatus.TestsFailed : RunnerStatus.Passed),
                numberOfFailed
            };
        })();

        this.#setTestRunnerStatus(status.code, status.numberOfFailed);
    }

    async #doRun(targetIsBatch, testName, functions, testOutputFormat) {
        const module = await this.#loader.loadEmscriptenModule(
            targetIsBatch ? BatchedTestRunner.#TestBatchModuleName : testName,
            () => { }
        );

        const testsToExecute = (testName || !targetIsBatch)
            ? [testName] : await this.#getTestClassNames(module);
        testsToExecute.forEach(testClassName => this.#registerTest(testClassName));

        for (const testClassName of testsToExecute) {
            let result = {};
            this.#setTestStatus(testClassName, TestStatus.Running);

            try {
                const LogToStdoutSpecialFilename = '-';
                result = await module.exec({
                    args: [...(targetIsBatch ? [testClassName] : []),
                           ...(functions ?? []),
                           '-o', `${LogToStdoutSpecialFilename},${testOutputFormat}`],
                    onStdout: (output) => {
                        this.#addTestOutput(testClassName, output);
                    }
                });

                if (result.exitCode < 0)
                    throw new ProgramError(result.exitCode);
                result.status = result.exitCode > 0 ? TestStatus.Failed : TestStatus.Completed;
                // Yield to other tasks on the main thread.
                await new Promise(resolve => window.setTimeout(resolve, 0));
            } catch (e) {
                result.status = e instanceof ProgramError ? TestStatus.Error : TestStatus.Crashed;
                result.stdout = e instanceof AbortedError ? e.stdout : result.stdout;
            }
            this.#setTestResultData(testClassName, result.status, result.exitCode);
        }
    }

    async #getTestClassNames(module) {
        return (await module.exec()).stdout.trim().split(' ');
    }

    #registerTest(testName) {
        this.#results.set(testName, { status: TestStatus.Pending, output: [] });
    }

    #setTestStatus(testName, status) {
        const testData = this.#results.get(testName);
        if (testData.status === status)
            return;
        this.#results.get(testName).status = status;
        this.#testStatusChangedEventPrivate.fireEvent(testName, status);
    }

    #setTestResultData(testName, testStatus, exitCode) {
        const testData = this.#results.get(testName);
        const statusChanged = testStatus !== testData.status;
        testData.status = testStatus;
        testData.exitCode = exitCode;
        if (statusChanged)
            this.#testStatusChangedEventPrivate.fireEvent(testName, testStatus);
    }

    #setTestRunnerStatus(status, numberOfFailed) {
        if (status === this.#status)
            return;
        this.#status = status;
        this.#numberOfFailed = numberOfFailed;
        this.#statusChangedEventPrivate.fireEvent(status);
    }

    #setTestRunnerError(details) {
        this.#status = RunnerStatus.Error;
        this.#errorDetails = details;
        this.#statusChangedEventPrivate.fireEvent(this.#status);
    }

    #addTestOutput(testName, output) {
        const testData = this.#results.get(testName);
        testData.output.push(output);
        this.#testOutputChangedEventPrivate.fireEvent(testName, testData.output);
    }
}
