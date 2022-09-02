// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import { BatchedTestRunner } from './batchedtestrunner.js'
import { EmrunAdapter, EmrunCommunication } from './emrunadapter.js'
import {
    ModuleLoader,
    ResourceFetcher,
    ResourceLocator,
} from './qwasmjsruntime.js';
import { parseQuery } from './util.js';
import { VisualOutputProducer, UI, ScannerFactory } from './qtestoutputreporter.js'

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
    const outputInPage = parsed.get('qvisualoutput') !== undefined;
    const testName = parsed.get('qtestname');
    const isBatch = parsed.get('qbatchedtest') !== undefined;
    const useEmrun = parsed.get('quseemrun') !== undefined;

    if (testName === undefined) {
        if (!isBatch)
            throw new Error('The qtestname parameter is required if not running a batch');
    } else if (testName === '') {
        throw new Error(`The qtestname=${testName} parameter is incorrect`);
    }

    const testOutputFormat = (() => {
        const format = parsed.get('qtestoutputformat') ?? 'txt';
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

    testRunner.run(isBatch, testName, testOutputFormat);
})();
