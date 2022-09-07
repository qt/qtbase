// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import { RunnerStatus, TestStatus } from './batchedtestrunner.js';

// Sends messages to the running emrun instance via POST requests.
export class EmrunCommunication {
    #indexOfMessage = 0;
    #postOutputPromises = [];

    #post(body) {
        return fetch('stdio.html', {
            method: 'POST',
            body
        });
    }

    // Returns a promise whose resolution signals that all outstanding traffic to the emrun instance
    // has been completed.
    waitUntilAllSent() {
        return Promise.all(this.#postOutputPromises);
    }

    // Posts the exit status to the running emrun instance. Emrun will drop connection unless it is
    // run with --serve_after_exit, therefore this method will throw most of the times.
    postExit(status) {
        return this.#post(`^exit^${status}`);
    }

    // Posts an indexed output chunk to the running emrun instance. Each consecutive call to this
    // method increments the output index by 1.
    postOutput(output) {
        const newPromise = this.#post(`^out^${this.#indexOfMessage++}^${output}`);
        this.#postOutputPromises.push(newPromise);
        newPromise.finally(() => {
            this.#postOutputPromises.splice(this.#postOutputPromises.indexOf(newPromise), 1);
        });
        return newPromise;
    }
}

// Wraps a test module runner; forwards its output and resolution state to the running emrun
// instance.
export class EmrunAdapter {
    #communication;
    #batchedTestRunner;
    #sentLines = 0;
    #onExitSent;

    constructor(communication, batchedTestRunner, onExitSent) {
        this.#communication = communication;
        this.#batchedTestRunner = batchedTestRunner;
        this.#onExitSent = onExitSent;
    }

    // Starts listening to test module runner's state changes. When the test module runner finishes
    // or reports output, sends suitable messages to the emrun instance.
    run() {
        this.#batchedTestRunner.onStatusChanged.addEventListener(
            status => this.#onRunnerStatusChanged(status));
        this.#batchedTestRunner.onTestStatusChanged.addEventListener(
            (test, status) => this.#onTestStatusChanged(test, status));
        this.#batchedTestRunner.onTestOutputChanged.addEventListener(
            (test, output) => this.#onTestOutputChanged(test, output));

        const currentTest = [...this.#batchedTestRunner.results.entries()].find(
            entry => entry[1].status === TestStatus.Running)?.[0];

        const output = this.#batchedTestRunner.results.get(currentTest)?.output;
        if (output)
            this.#onTestOutputChanged(testName, output);
        this.#onRunnerStatusChanged(this.#batchedTestRunner.status);
    }

    #toExitCode(status) {
        switch (status) {
            case RunnerStatus.Error:
                return -1;
            case RunnerStatus.Passed:
                return 0;
            case RunnerStatus.Running:
                throw new Error('No exit code when still running');
            case RunnerStatus.TestCrashed:
                return -2;
            case RunnerStatus.TestsFailed:
                return this.#batchedTestRunner.numberOfFailed;
        }
    }

    async #onRunnerStatusChanged(status) {
        if (RunnerStatus.Running === status)
            return;

        const exit = this.#toExitCode(status);
        if (RunnerStatus.Error === status)
            this.#communication.postOutput(this.#batchedTestRunner.errorDetails);

        await this.#communication.waitUntilAllSent();
        try {
            await this.#communication.postExit(exit);
        } catch {
            // no-op: The remote end will drop connection on exit.
        } finally {
            this.#onExitSent?.();
        }
    }

    async #onTestOutputChanged(_, output) {
        const notSent = output.slice(this.#sentLines);
        for (const out of notSent)
            this.#communication.postOutput(out);
        this.#sentLines = output.length;
    }

    async #onTestStatusChanged(_, status) {
        if (status === TestStatus.Running)
            this.#sentLines = 0;
    }
}
