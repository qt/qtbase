// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

function output(message)
{
    const outputLine = document.createElement('div');
    outputLine.style.fontFamily = 'monospace';
    outputLine.innerText = message;

    document.body.appendChild(outputLine);

    console.log(message);
}

export class TestRunner
{
    #testClassInstance

    constructor(testClassInstance)
    {
        this.#testClassInstance = testClassInstance;
    }

    async run(testCase)
    {
        const prototype = Object.getPrototypeOf(this.#testClassInstance);
        try {
            output(`Running ${testCase}`);
            if (!prototype.hasOwnProperty(testCase))
                throw new Error(`No such testcase ${testCase}`);

            if (prototype.beforeEach) {
                await prototype.beforeEach.apply(this.#testClassInstance);
            }

            await new Promise((resolve, reject) =>
            {
                let rejected = false;
                const timeout = window.setTimeout(() =>
                {
                    rejected = true;
                    reject(new Error('Timeout after 2 seconds'));
                }, 2000);
                prototype[testCase].apply(this.#testClassInstance).then(() =>
                {
                    if (!rejected) {
                        window.clearTimeout(timeout);
                        output(`✅ Test passed ${testCase}`);
                        resolve();
                    }
                }).catch(reject);
            });
        } catch (e) {
            output(`❌ Failed ${testCase}: exception ${e} ${e.stack}`);
        } finally {
            if (prototype.afterEach) {
                await prototype.afterEach.apply(this.#testClassInstance);
            }
        }
    }

    async runAll()
    {
        const SPECIAL_FUNCTIONS =
            ['beforeEach', 'afterEach', 'beforeAll', 'afterAll', 'constructor'];
        const prototype = Object.getPrototypeOf(this.#testClassInstance);
        const testFunctions =
            Object.getOwnPropertyNames(prototype).filter(
                entry => SPECIAL_FUNCTIONS.indexOf(entry) === -1);

        if (prototype.beforeAll)
            await prototype.beforeAll.apply(this.#testClassInstance);
        for (const fn of testFunctions)
            await this.run(fn);
        if (prototype.afterAll)
            await prototype.afterAll.apply(this.#testClassInstance);
    }
}
