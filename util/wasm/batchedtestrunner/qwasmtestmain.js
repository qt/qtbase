// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import { BatchedTestRunner } from './batchedtestrunner.js'
import { EmrunAdapter, EmrunCommunication } from './emrunadapter.js'
import {
    ModuleLoader,
    ResourceFetcher,
    ResourceLocator,
} from './qwasmjsruntime.js';
import { parseQuery } from './util.js';
import { VisualOutputProducer, UI, ScannerFactory } from './qtestoutputreporter.js'

const StandardArg = {
    qVisualOutput: 'qvisualoutput',
    qTestName: 'qtestname',
    qBatchedTest: 'qbatchedtest',
    qUseEmrun: 'quseemrun',
    qTestOutputFormat: 'qtestoutputformat',
}

const allArgs = new Set(Object.getOwnPropertyNames(StandardArg).map(arg => StandardArg[arg]));
Object.defineProperty(StandardArg, 'isKnown', {
    get()
    {
        return name => allArgs.has(name);
    },
});

(() => {
    const setPageTitle = (useEmrun, testName, isBatch) => {
        document.title = 'Qt WASM test runner';
        if (useEmrun || testName || isBatch) {
            document.title += `(${[
                    ...[useEmrun ? ['emrun'] : []],
                    ...[testName ? ['test=' + testName] : []],
                    ...[isBatch ? ['batch'] : []]
                ].flat().join(", ")})`;
        }
    }

    const parsed = parseQuery(location.search);
    const outputInPage = parsed.has(StandardArg.qVisualOutput);
    const testName = parsed.get(StandardArg.qTestName);
    const isBatch = parsed.has(StandardArg.qBatchedTest);
    const useEmrun = parsed.has(StandardArg.qUseEmrun);
    const functions = [...parsed.keys()].filter(arg => !StandardArg.isKnown(arg));

    if (testName === undefined) {
        if (!isBatch)
            throw new Error('The qtestname parameter is required if not running a batch');
    } else if (testName === '') {
        throw new Error(`The qtestname=${testName} parameter is incorrect`);
    }

    const testOutputFormat = (() => {
        const format = parsed.get(StandardArg.qTestOutputFormat) ?? 'txt';
        if (-1 === ['txt', 'xml', 'lightxml', 'junitxml', 'tap'].indexOf(format))
            throw new Error(`Bad file format: ${format}`);
        return format;
    })();

    const resourceLocator = new ResourceLocator('');
    const testRunner = new BatchedTestRunner(
        new ModuleLoader(new ResourceFetcher(resourceLocator), resourceLocator),
    );
    window.qtTestRunner = testRunner;

    if (useEmrun) {
        const adapter = new EmrunAdapter(new EmrunCommunication(), testRunner, () => {
            if (!outputInPage)
                window.close();
        });
        adapter.run();
    }
    if (outputInPage) {
        const scanner = ScannerFactory.createScannerForFormat(testOutputFormat);
        const ui = new UI(document.querySelector('body'), !!scanner);
        const adapter =
            new VisualOutputProducer(ui.outputArea, ui.counters, scanner, testRunner);
        adapter.run();
    }
    setPageTitle(useEmrun, testName, isBatch);

    testRunner.run(isBatch, testName, functions, testOutputFormat);
})();
