// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import { RunnerStatus, TestStatus } from './batchedtestrunner.js'

class AttentionType
{
    static None = 1;
    static Bad = 2;
    static Good = 3;
    static Warning = 4;
    static Info = 5;
    static Ignore = 6;
};

export class IncidentType
{
    // See QAbstractTestLogger::IncidentTypes (and keep in sync with it):
    static Pass = 'pass';
    static Fail = 'fail';
    static Skip = 'skip';
    static XFail = 'xfail';
    static XPass = 'xpass';
    static BlacklistedPass = 'bpass';
    static BlacklistedFail = 'bfail';
    static BlacklistedXPass = 'bxpass';
    static BlacklistedXFail = 'bxfail';

    // The following is not mapped from QAbstractTestLogger::IncidentTypes and is used internally:
    static None = 'none';

    static values()
    {
        return Object.getOwnPropertyNames(IncidentType)
            .filter(
                propertyName =>
                    ['length', 'prototype', 'values', 'name'].indexOf(propertyName) === -1)
            .map(propertyName => IncidentType[propertyName]);
    }
}

class OutputArea
{
    #outputDiv;

    constructor()
    {
        this.#outputDiv = document.createElement('div');
        this.#outputDiv.classList.add('output-area');
        this.#outputDiv.classList.add('light-background');
        document.querySelector('body').appendChild(this.#outputDiv);
    }

    addOutput(text, attentionType)
    {
        const newContentWrapper = document.createElement('span');
        newContentWrapper.className = 'output-line';

        newContentWrapper.innerText = text;

        switch (attentionType) {
            case AttentionType.Bad:
                newContentWrapper.classList.add('bad');
                break;
            case AttentionType.Good:
                newContentWrapper.classList.add('good');
                break;
            case AttentionType.Warning:
                newContentWrapper.classList.add('warning');
                break
            case AttentionType.Info:
                newContentWrapper.classList.add('info');
                break;
            case AttentionType.Ignore:
                newContentWrapper.classList.add('ignore');
                break;
            default:
                break;
        }
        this.#outputDiv.appendChild(newContentWrapper);
    }
}

class Counter
{
    #count = 0;
    #decriptionElement;
    #counterElement;

    constructor(parentElement, incidentType)
    {
        this.#decriptionElement = document.createElement('span');
        this.#decriptionElement.classList.add(incidentType);
        this.#decriptionElement.classList.add('zero');
        this.#decriptionElement.innerText = Counter.#humanReadableIncidentName(incidentType);
        parentElement.appendChild(this.#decriptionElement);

        this.#counterElement = document.createElement('span');
        this.#counterElement.classList.add(incidentType);
        this.#counterElement.classList.add('zero');
        parentElement.appendChild(this.#counterElement);
    }

    increment()
    {
        if (!this.#count++) {
            this.#decriptionElement.classList.remove('zero');
            this.#counterElement.classList.remove('zero');
        }
        this.#counterElement.innerText = this.#count;
    }

    static #humanReadableIncidentName(incidentName)
    {
        switch (incidentName) {
            case IncidentType.Pass:
                return 'Passed';
            case IncidentType.Fail:
                return 'Failed';
            case IncidentType.Skip:
                return 'Skipped';
            case IncidentType.XFail:
                return 'Known failure';
            case IncidentType.XPass:
                return 'Unexpectedly passed';
            case IncidentType.BlacklistedPass:
                return 'Blacklisted passed';
            case IncidentType.BlacklistedFail:
                return 'Blacklisted failed';
            case IncidentType.BlacklistedXPass:
                return 'Blacklisted unexpectedly passed';
            case IncidentType.BlacklistedXFail:
                return 'Blacklisted unexpectedly failed';
            case IncidentType.None:
                throw new Error('Incident of the None type cannot be displayed');
        }
    }
}

class Counters
{
    #contentsDiv;
    #counters;

    constructor(parentElement)
    {
        this.#contentsDiv = document.createElement('div');
        this.#contentsDiv.className = 'counter-box';
        parentElement.appendChild(this.#contentsDiv);

        const centerDiv = document.createElement('div');
        this.#contentsDiv.appendChild(centerDiv);

        this.#counters = new Map(IncidentType.values()
            .filter(incidentType => incidentType !== IncidentType.None)
            .map(incidentType => [incidentType, new Counter(centerDiv, incidentType)]));
    }

    incrementIncidentCounter(incidentType)
    {
        this.#counters.get(incidentType).increment();
    }
}

export class UI
{
    #contentsDiv;

    #counters;
    #outputArea;

    constructor(parentElement, hasCounters)
    {
        this.#contentsDiv = document.createElement('div');
        parentElement.appendChild(this.#contentsDiv);

        if (hasCounters)
            this.#counters = new Counters(this.#contentsDiv);
        this.#outputArea = new OutputArea(this.#contentsDiv);
    }

    get counters()
    {
        return this.#counters;
    }

    get outputArea()
    {
        return this.#outputArea;
    }

    htmlElement()
    {
        return this.#contentsDiv;
    }
}

class OutputScanner
{
    static #supportedIncidentTypes = IncidentType.values().filter(
        incidentType => incidentType !== IncidentType.None);

    static get supportedIncidentTypes()
    {
        return this.#supportedIncidentTypes;
    }

    #regex;

    constructor(regex)
    {
        this.#regex = regex;
    }

    classifyOutputLine(line)
    {
        const match = this.#regex.exec(line);
        if (!match)
            return IncidentType.None;
        match.splice(0, 1);
        // Find the index of the first non-empty matching group and recover an incident type for it.
        return OutputScanner.supportedIncidentTypes[match.findIndex(element => !!element)];
    }
}

class XmlOutputScanner extends OutputScanner
{
    constructor()
    {
        // Scan for any line with an incident of type from supportedIncidentTypes. The matching
        //  group at offset n will contain the type. The match type can be preceded by any number of
        // whitespace characters to factor in the indentation.
        super(new RegExp(`^\\s*<Incident type="${OutputScanner.supportedIncidentTypes
            .map(incidentType => `(${incidentType})`).join('|')}"`));
    }
}

class TextOutputScanner extends OutputScanner
{
    static #incidentNameMap = new Map([
        [IncidentType.Pass, 'PASS'],
        [IncidentType.Fail, 'FAIL!'],
        [IncidentType.Skip, 'SKIP'],
        [IncidentType.XFail, 'XFAIL'],
        [IncidentType.XPass, 'XPASS'],
        [IncidentType.BlacklistedPass, 'BPASS'],
        [IncidentType.BlacklistedFail, 'BFAIL'],
        [IncidentType.BlacklistedXPass, 'BXPASS'],
        [IncidentType.BlacklistedXFail, 'BXFAIL']
    ]);

    constructor()
    {
        // Scan for any line with an incident of type from incidentNameMap. The matching group
        // at offset n will contain the type. The type can be preceded by any number of whitespace
        // characters to factor in the indentation.
        super(new RegExp(`^\\s*${OutputScanner.supportedIncidentTypes
            .map(incidentType =>
                `(${TextOutputScanner.#incidentNameMap.get(incidentType)})`).join('|')}\\s`));
    }
}

export class ScannerFactory
{
    static createScannerForFormat(format)
    {
        switch (format) {
            case 'txt':
                return new TextOutputScanner();
            case 'xml':
                return new XmlOutputScanner();
            default:
                return null;
        }
    }
}

export class VisualOutputProducer
{
    #batchedTestRunner;

    #outputArea;
    #counters;
    #outputScanner;
    #processedLines;

    constructor(outputArea, counters, outputScanner, batchedTestRunner)
    {
        this.#outputArea = outputArea;
        this.#counters = counters;
        this.#outputScanner = outputScanner;
        this.#batchedTestRunner = batchedTestRunner;
        this.#processedLines = 0;
    }

    run()
    {
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

    async #onRunnerStatusChanged(status)
    {
        if (RunnerStatus.Running === status)
            return;

        this.#outputArea.addOutput(
            `Runner exited with status: ${status}`,
            status === RunnerStatus.Passed ? AttentionType.Good : AttentionType.Bad);
        if (RunnerStatus.Error === status)
            this.#outputArea.addOutput(`The error was: ${this.#batchedTestRunner.errorDetails}`);
    }

    async #onTestOutputChanged(_, output)
    {
        const notSent = output.slice(this.#processedLines);
        for (const out of notSent) {
            const incidentType = this.#outputScanner?.classifyOutputLine(out);
            if (incidentType !== IncidentType.None)
                this.#counters.incrementIncidentCounter(incidentType);
            this.#outputArea.addOutput(
                out,
                (() =>
                {
                    switch (incidentType) {
                        case IncidentType.Fail:
                        case IncidentType.XPass:
                            return AttentionType.Bad;
                        case IncidentType.Pass:
                            return AttentionType.Good;
                        case IncidentType.XFail:
                            return AttentionType.Warning;
                        case IncidentType.Skip:
                            return AttentionType.Info;
                        case IncidentType.BlacklistedFail:
                        case IncidentType.BlacklistedPass:
                        case IncidentType.BlacklistedXFail:
                        case IncidentType.BlacklistedXPass:
                            return AttentionType.Ignore;
                        case IncidentType.None:
                            return AttentionType.None;
                    }
                })());
        }
        this.#processedLines = output.length;
    }

    async #onTestStatusChanged(_, status)
    {
        if (status === TestStatus.Running)
            this.#processedLines = 0;
        await new Promise(resolve => window.setTimeout(resolve, 500));
    }
}
