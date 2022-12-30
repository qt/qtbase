// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

function parseQuery()
{
    const trimmed = window.location.search.substring(1);
    return new Map(
        trimmed.length === 0 ?
            [] :
            trimmed.split('&').map(paramNameAndValue =>
            {
                const [name, value] = paramNameAndValue.split('=');
                return [decodeURIComponent(name), value ? decodeURIComponent(value) : ''];
            }));
}

export class assert
{
    static isFalse(value)
    {
        if (value !== false)
            throw new Error(`Assertion failed, expected to be false, was ${value}`);
    }

    static isTrue(value)
    {
        if (value !== true)
            throw new Error(`Assertion failed, expected to be true, was ${value}`);
    }

    static isUndefined(value)
    {
        if (typeof value !== 'undefined')
            throw new Error(`Assertion failed, expected to be undefined, was ${value}`);
    }

    static isNotUndefined(value)
    {
        if (typeof value === 'undefined')
            throw new Error(`Assertion failed, expected not to be undefined, was ${value}`);
    }

    static equal(expected, actual)
    {
        if (expected !== actual)
            throw new Error(`Assertion failed, expected to be ${expected}, was ${actual}`);
    }

    static notEqual(expected, actual)
    {
        if (expected === actual)
            throw new Error(`Assertion failed, expected not to be ${expected}`);
    }
}

export class Mock extends Function
{
    #calls = [];

    constructor()
    {
        super()
        const proxy = new Proxy(this, {
            apply: (target, _, args) => target.onCall(...args)
        });
        proxy.thisMock = this;

        return proxy;
    }

    get calls()
    {
        return this.thisMock.#calls;
    }

    onCall(...args)
    {
        this.#calls.push(args);
    }
}

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
    #timeoutSeconds

    constructor(testClassInstance, config)
    {
        this.#testClassInstance = testClassInstance;
        this.#timeoutSeconds = config?.timeoutSeconds ?? 2;
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
                    reject(new Error(`Timeout after ${this.#timeoutSeconds} seconds`));
                }, this.#timeoutSeconds * 1000);
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
        const query = parseQuery();
        const testFilter = query.has('testfilter') ? new RegExp(query.get('testfilter')) : undefined;

        const SPECIAL_FUNCTIONS =
            ['beforeEach', 'afterEach', 'beforeAll', 'afterAll', 'constructor'];
        const prototype = Object.getPrototypeOf(this.#testClassInstance);
        const testFunctions =
            Object.getOwnPropertyNames(prototype).filter(
                entry => SPECIAL_FUNCTIONS.indexOf(entry) === -1 && (!testFilter || entry.match(testFilter)));

        if (prototype.beforeAll)
            await prototype.beforeAll.apply(this.#testClassInstance);
        for (const fn of testFunctions)
            await this.run(fn);
        if (prototype.afterAll)
            await prototype.afterAll.apply(this.#testClassInstance);
    }
}
