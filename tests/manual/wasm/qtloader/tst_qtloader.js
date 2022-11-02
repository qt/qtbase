// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import { TestRunner } from '../shared/testrunner.js';

class QtLoaderTests
{
    async beforeEach() { sinon.stub(window, 'alert'); }

    async afterEach() { sinon.restore(); }

    async sampleTestCase()
    {
        await new Promise(resolve =>
        {
            window.alert();
            sinon.assert.calledOnce(window.alert);
            window.setTimeout(resolve, 4000);
        });
    }

    async sampleTestCase2()
    {
        await new Promise(resolve =>
        {
            window.alert();
            sinon.assert.calledOnce(window.alert);
            window.setTimeout(resolve, 1000);
        });
    }

    async constructQtLoader()
    {
        new QtLoader({});
    }
}

(async () =>
{
    const runner = new TestRunner(new QtLoaderTests());
    await runner.runAll();
})();
