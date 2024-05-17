// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import { Mock, assert, TestRunner } from './testrunner.js';

export class QtLoaderIntegrationTests
{
    #testScreenContainers = []

    async beforeEach()
    {
        this.#addScreenContainer('screen-container-0', { width: '200px', height: '300px' });
    }

    async afterEach()
    {
        this.#testScreenContainers.forEach(screenContainer =>
        {
            document.body.removeChild(screenContainer);
        });
        this.#testScreenContainers = [];
    }

    async missingConfig()
    {
        let caughtException;
        try {
            await qtLoad();
        } catch (e) {
            caughtException = e;
        }

        assert.isNotUndefined(caughtException);
        assert.equal('config is required, expected an object', caughtException.message);
    }

    async missingQtSection()
    {
        let caughtException;
        try {
            await qtLoad({});
        } catch (e) {
            caughtException = e;
        }

        assert.isNotUndefined(caughtException);
        assert.equal(
            'config.qt is required, expected an object', caughtException.message);
    }

    async missingEntryFunction()
    {
        let caughtException;
        try {
            await qtLoad({ qt: {}});
        } catch (e) {
            caughtException = e;
        }

        assert.isNotUndefined(caughtException);
        assert.equal(
            'config.qt.entryFunction is required, expected a function', caughtException.message);
    }

    async badEntryFunction()
    {
        let caughtException;
        try {
            await qtLoad({ qt: { entryFunction: 'invalid' }});
        } catch (e) {
            caughtException = e;
        }

        assert.isNotUndefined(caughtException);
        assert.equal(
            'config.qt.entryFunction is required, expected a function', caughtException.message);
    }

    async environmentVariables()
    {
        const instance = await qtLoad({
            qt: {
                environment: {
                    variable1: 'value1',
                    variable2: 'value2'
                },
                entryFunction: tst_qtloader_integration_entry,
                containerElements: [this.#testScreenContainers[0]]
            }
        });
        assert.isTrue(instance.getEnvironmentVariable('variable1') === 'value1');
        assert.isTrue(instance.getEnvironmentVariable('variable2') === 'value2');
    }

    async screenContainerManipulations()
    {
        // ... (do other things), then call addContainerElement() to add a new container/screen.
        // This can happen either before or after load() is called - loader will route the
        // call to instance when it's ready.
        this.#addScreenContainer('appcontainer1', { width: '100px', height: '100px' })

        const instance = await qtLoad({
            qt: {
                entryFunction: tst_qtloader_integration_entry,
                containerElements: this.#testScreenContainers
            }
        });
        {
            const screenInformation = this.#getScreenInformation(instance);

            assert.equal(2, screenInformation.length);
            assert.equal(200, screenInformation[0].width);
            assert.equal(300, screenInformation[0].height);
            assert.equal(100, screenInformation[1].width);
            assert.equal(100, screenInformation[1].height);
        }

        this.#addScreenContainer('appcontainer2', { width: '234px', height: '99px' })
        instance.qtSetContainerElements(this.#testScreenContainers);

        {
            const screenInformation = this.#getScreenInformation(instance);

            assert.equal(3, screenInformation.length);
            assert.equal(200, screenInformation[0].width);
            assert.equal(300, screenInformation[0].height);
            assert.equal(100, screenInformation[1].width);
            assert.equal(100, screenInformation[1].height);
            assert.equal(234, screenInformation[2].width);
            assert.equal(99, screenInformation[2].height);
        }

        document.body.removeChild(this.#testScreenContainers.splice(2, 1)[0]);
        instance.qtSetContainerElements(this.#testScreenContainers);
        {
            const screenInformation = this.#getScreenInformation(instance);

            assert.equal(2, screenInformation.length);
            assert.equal(200, screenInformation[0].width);
            assert.equal(300, screenInformation[0].height);
            assert.equal(100, screenInformation[1].width);
            assert.equal(100, screenInformation[1].height);
        }
    }

    async primaryScreenIsAlwaysFirst()
    {
        const instance = await qtLoad({
            qt: {
                entryFunction: tst_qtloader_integration_entry,
                containerElements: this.#testScreenContainers,
            }
        });
        this.#addScreenContainer(
            'appcontainer3', { width: '12px', height: '24px' },
            container => this.#testScreenContainers.splice(0, 0, container));
        this.#addScreenContainer(
            'appcontainer4', { width: '34px', height: '68px' },
            container => this.#testScreenContainers.splice(1, 0, container));

        instance.qtSetContainerElements(this.#testScreenContainers);
        {
            const screenInformation = this.#getScreenInformation(instance);

            assert.equal(3, screenInformation.length);
            // The primary screen (at position 0) is always at 0
            assert.equal(12, screenInformation[0].width);
            assert.equal(24, screenInformation[0].height);
            // Other screens are pushed at the back
            assert.equal(200, screenInformation[1].width);
            assert.equal(300, screenInformation[1].height);
            assert.equal(34, screenInformation[2].width);
            assert.equal(68, screenInformation[2].height);
        }

        this.#testScreenContainers.forEach(screenContainer =>
        {
            document.body.removeChild(screenContainer);
        });
        this.#testScreenContainers = [
            this.#addScreenContainer('appcontainer5', { width: '11px', height: '12px' }),
            this.#addScreenContainer('appcontainer6', { width: '13px', height: '14px' }),
        ];

        instance.qtSetContainerElements(this.#testScreenContainers);
        {
            const screenInformation = this.#getScreenInformation(instance);

            assert.equal(2, screenInformation.length);
            assert.equal(11, screenInformation[0].width);
            assert.equal(12, screenInformation[0].height);
            assert.equal(13, screenInformation[1].width);
            assert.equal(14, screenInformation[1].height);
        }
    }

    async multipleInstances()
    {
        // Fetch/Compile the module once; reuse for each instance. This is also if the page wants to
        // initiate the .wasm file download fetch as early as possible, before the browser has
        // finished fetching and parsing testapp.js and qtloader.js
        const module = WebAssembly.compileStreaming(fetch('tst_qtloader_integration.wasm'));

        const instances = await Promise.all([1, 2, 3].map(i => qtLoad({
            qt: {
                entryFunction: tst_qtloader_integration_entry,
                containerElements: [this.#addScreenContainer(`screen-container-${i}`, {
                    width: `${i * 10}px`,
                    height: `${i * 10}px`,
                })],
                module,
            }
        })));
        // Confirm the identity of instances by querying their screen widths and heights
        {
            const screenInformation = this.#getScreenInformation(instances[0]);
            console.log();
            assert.equal(1, screenInformation.length);
            assert.equal(10, screenInformation[0].width);
            assert.equal(10, screenInformation[0].height);
        }
        {
            const screenInformation = this.#getScreenInformation(instances[1]);
            assert.equal(1, screenInformation.length);
            assert.equal(20, screenInformation[0].width);
            assert.equal(20, screenInformation[0].height);
        }
        {
            const screenInformation = this.#getScreenInformation(instances[2]);
            assert.equal(1, screenInformation.length);
            assert.equal(30, screenInformation[0].width);
            assert.equal(30, screenInformation[0].height);
        }
    }

    async consoleMode()
    {
        // 'Console mode' for autotesting type scenarios
        let accumulatedStdout = '';
        const instance = await qtLoad({
            arguments: ['--no-gui'],
            print: output =>
            {
                accumulatedStdout += output;
            },
            qt: {
                entryFunction: tst_qtloader_integration_entry,
            }
        });

        this.#callTestInstanceApi(instance, 'produceOutput');
        assert.equal('Sample output!', accumulatedStdout);
    }

    async modulePromiseProvided()
    {
        await qtLoad({
            qt: {
                entryFunction: createQtAppInstance,
                containerElements: [this.#testScreenContainers[0]],
                module: WebAssembly.compileStreaming(
                    fetch('tst_qtloader_integration.wasm'))
            }
        });
    }

    async moduleProvided()
    {
        await qtLoad({
            qt: {
                entryFunction: tst_qtloader_integration_entry,
                containerElements: [this.#testScreenContainers[0]],
                module: await WebAssembly.compileStreaming(
                    fetch('tst_qtloader_integration.wasm'))
            }
        });
    }

    async arguments()
    {
        const instance = await qtLoad({
            arguments: ['--no-gui', 'arg1', 'other', 'yetanotherarg'],
            qt: {
                entryFunction: tst_qtloader_integration_entry,
            }
        });
        const args = this.#callTestInstanceApi(instance, 'retrieveArguments');
        assert.equal(5, args.length);
        assert.isTrue('arg1' === args[2]);
        assert.equal('other', args[3]);
        assert.equal('yetanotherarg', args[4]);
    }

    async moduleProvided_exceptionThrownInFactory()
    {
        let caughtException;
        try {
            await qtLoad({
                qt: {
                    entryFunction: tst_qtloader_integration_entry,
                    containerElements: [this.#testScreenContainers[0]],
                    module: Promise.reject(new Error('Failed to load')),
                }
            });
        } catch (e) {
            caughtException = e;
        }
        assert.isTrue(caughtException !== undefined);
        assert.equal('Failed to load', caughtException.message);
    }

    async abort()
    {
        const onExitMock = new Mock();
        const instance = await qtLoad({
            arguments: ['--no-gui'],
            qt: {
                onExit: onExitMock,
                entryFunction: tst_qtloader_integration_entry,
            }
        });
        try {
            instance.crash();
        } catch { }
        assert.equal(1, onExitMock.calls.length);
        const exitStatus = onExitMock.calls[0][0];
        assert.isTrue(exitStatus.crashed);
        assert.isUndefined(exitStatus.code);
        assert.isNotUndefined(exitStatus.text);
    }

    async abortImmediately()
    {
        const onExitMock = new Mock();
        let caughtException;
        try {
            await qtLoad({
                arguments: ['--no-gui', '--crash-immediately'],
                qt: {
                    onExit: onExitMock,
                    entryFunction: tst_qtloader_integration_entry,
                }
            });
        } catch (e) {
            caughtException = e;
        }

        assert.isTrue(caughtException !== undefined);
        assert.equal(1, onExitMock.calls.length);
        const exitStatus = onExitMock.calls[0][0];
        assert.isTrue(exitStatus.crashed);
        assert.isUndefined(exitStatus.code);
        assert.isNotUndefined(exitStatus.text);
    }

    async stackOwerflowImmediately()
    {
        const onExitMock = new Mock();
        let caughtException;
        try {
            await qtLoad({
                arguments: ['--no-gui', '--stack-owerflow-immediately'],
                qt: {
                    onExit: onExitMock,
                    entryFunction: tst_qtloader_integration_entry,
                }
            });
        } catch (e) {
            caughtException = e;
        }

        assert.isTrue(caughtException !== undefined);
        assert.equal(1, onExitMock.calls.length);
        const exitStatus = onExitMock.calls[0][0];
        assert.isTrue(exitStatus.crashed);
        assert.isUndefined(exitStatus.code);
        // text should be "RangeError: Maximum call stack
        // size exceeded", or similar.
        assert.isNotUndefined(exitStatus.text);
    }

    async userAbortCallbackCalled()
    {
        const onAbortMock = new Mock();
        let instance = await qtLoad({
            arguments: ['--no-gui'],
            onAbort: onAbortMock,
            qt: {
                entryFunction: tst_qtloader_integration_entry,
            }
        });
        try {
            instance.crash();
        } catch (e) {
            // emscripten throws an 'Aborted' error here, which we ignore for the sake of the test
        }
        assert.equal(1, onAbortMock.calls.length);
    }

    async exit()
    {
        const onExitMock = new Mock();
        let instance = await qtLoad({
            arguments: ['--no-gui'],
            qt: {
                onExit: onExitMock,
                entryFunction: tst_qtloader_integration_entry,
            }
        });
        // The module is running. onExit should not have been called.
        assert.equal(0, onExitMock.calls.length);
        try {
            instance.exitApp();
        } catch (e) {
            // emscripten throws a 'Runtime error: unreachable' error here. We ignore it for the
            // sake of the test.
        }
        assert.equal(1, onExitMock.calls.length);
        const exitStatus = onExitMock.calls[0][0];
        assert.isFalse(exitStatus.crashed);
        assert.equal(instance.EXIT_VALUE_FROM_EXIT_APP, exitStatus.code);
        assert.isUndefined(exitStatus.text);
    }

    async exitImmediately()
    {
        const onExitMock = new Mock();
        const instance = await qtLoad({
            arguments: ['--no-gui', '--exit-immediately'],
            qt: {
                onExit: onExitMock,
                entryFunction: tst_qtloader_integration_entry,
            }
        });
        assert.equal(1, onExitMock.calls.length);

        const exitStatusFromOnExit = onExitMock.calls[0][0];

        assert.isFalse(exitStatusFromOnExit.crashed);
        assert.equal(instance.EXIT_VALUE_IMMEDIATE_RETURN, exitStatusFromOnExit.code);
        assert.isUndefined(exitStatusFromOnExit.text);
    }

    async userQuitCallbackCalled()
    {
        const quitMock = new Mock();
        let instance = await qtLoad({
            arguments: ['--no-gui'],
            quit: quitMock,
            qt: {
                entryFunction: tst_qtloader_integration_entry,
            }
        });
        try {
            instance.exitApp();
        } catch (e) {
            // emscripten throws a 'Runtime error: unreachable' error here. We ignore it for the
            // sake of the test.
        }
        assert.equal(1, quitMock.calls.length);
        const [exitCode, exception] = quitMock.calls[0];
        assert.equal(instance.EXIT_VALUE_FROM_EXIT_APP, exitCode);
        assert.equal('ExitStatus', exception.name);
    }

    async preloadFiles()
    {
        const instance = await qtLoad({
            arguments: ["--no-gui"],
            qt: {
                preload: ['preload.json'],
                qtdir: '.',
            }
        });
        const preloadedFiles = instance.preloadedFiles();
        // Verify that preloaded file list matches files specified in preload.json
        assert.equal("[qtloader.js,qtlogo.svg]", preloadedFiles);
    }

    #callTestInstanceApi(instance, apiName)
    {
        return eval(instance[apiName]());
    }

    #getScreenInformation(instance)
    {
        return this.#callTestInstanceApi(instance, 'screenInformation').map(elem => ({
            x: elem[0],
            y: elem[1],
            width: elem[2],
            height: elem[3],
        }));
    }

    #addScreenContainer(id, style, inserter)
    {
        const container = (() =>
        {
            const container = document.createElement('div');
            container.id = id;
            container.style.width = style.width;
            container.style.height = style.height;
            document.body.appendChild(container);
            return container;
        })();
        inserter ? inserter(container) : this.#testScreenContainers.push(container);
        return container;
    }
}

(async () =>
{
    const runner = new TestRunner(new QtLoaderIntegrationTests(), {
        timeoutSeconds: 10
    });
    await runner.runAll();
})();
