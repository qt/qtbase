// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/**
 * Loads the instance of a WASM module.
 *
 * @param config May contain any key normally accepted by emscripten and the 'qt' extra key, with
 *               the following sub-keys:
 * - environment: { [name:string] : string }
 *      environment variables set on the instance
 * - onExit: (exitStatus: { text: string, code?: number, crashed: bool }) => void
 *      called when the application has exited for any reason. exitStatus.code is defined in
 *      case of a normal application exit. This is not called on exit with return code 0, as
 *      the program does not shutdown its runtime and technically keeps running async.
 * - containerElements: HTMLDivElement[]
 *      Array of host elements for Qt screens. Each of these elements is mapped to a QScreen on
 *      launch.
 * - fontDpi: number
 *      Specifies font DPI for the instance
 * - onLoaded: () => void
 *      Called when the module has loaded.
 * - entryFunction: (emscriptenConfig: object) => Promise<EmscriptenModule>
 *      Qt always uses emscripten's MODULARIZE option. This is the MODULARIZE entry function.
 * - module: Promise<WebAssembly.Module>
 *      The module to create the instance from (optional). Specifying the module allows optimizing
 *      use cases where several instances are created from a single WebAssembly source.
 *
 * @return Promise<instance: EmscriptenModule>
 *      The promise is resolved when the module has been instantiated and its main function has been
 *      called.
 *
 * @see https://github.com/DefinitelyTyped/DefinitelyTyped/blob/master/types/emscripten for
 *      EmscriptenModule
 */
async function qtLoad(config)
{
    const throwIfEnvUsedButNotExported = (instance, config) =>
    {
        const environment = config.environment;
        if (!environment || Object.keys(environment).length === 0)
            return;
        const isEnvExported = typeof instance.ENV === 'object';
        if (!isEnvExported)
            throw new Error('ENV must be exported if environment variables are passed');
    };

    if (typeof config !== 'object')
        throw new Error('config is required, expected an object');
    if (typeof config.qt !== 'object')
        throw new Error('config.qt is required, expected an object');
    if (typeof config.qt.entryFunction !== 'function')
        throw new Error('config.qt.entryFunction is required, expected a function');

    config.qtContainerElements = config.qt.containerElements;
    delete config.qt.containerElements;
    config.qtFontDpi = config.qt.fontDpi;
    delete config.qt.fontDpi;

    // Used for rejecting a failed load's promise where emscripten itself does not allow it,
    // like in instantiateWasm below. This allows us to throw in case of a load error instead of
    // hanging on a promise to entry function, which emscripten unfortunately does.
    let circuitBreakerReject;
    const circuitBreaker = new Promise((_, reject) => { circuitBreakerReject = reject; });

    // If module async getter is present, use it so that module reuse is possible.
    if (config.qt.module) {
        config.instantiateWasm = async (imports, successCallback) =>
        {
            try {
                const module = await config.qt.module;
                successCallback(
                    await WebAssembly.instantiate(module, imports), module);
            } catch (e) {
                circuitBreakerReject(e);
            }
        }
    }

    const originalPreRun = config.preRun;
    config.preRun = instance =>
    {
        originalPreRun?.();

        throwIfEnvUsedButNotExported(instance, config);
        for (const [name, value] of Object.entries(config.qt.environment ?? {}))
            instance.ENV[name] = value;
    };

    config.onRuntimeInitialized = () => config.qt.onLoaded?.();

    // This is needed for errors which occur right after resolving the instance promise but
    // before exiting the function (i.e. on call to main before stack unwinding).
    let loadTimeException = undefined;
    // We don't want to issue onExit when aborted
    let aborted = false;
    const originalQuit = config.quit;
    config.quit = (code, exception) =>
    {
        originalQuit?.(code, exception);

        if (exception)
            loadTimeException = exception;
        if (!aborted && code !== 0) {
            config.qt.onExit?.({
                text: exception.message,
                code,
                crashed: false
            });
        }
    };

    const originalOnAbort = config.onAbort;
    config.onAbort = text =>
    {
        originalOnAbort?.();

        aborted = true;
        config.qt.onExit?.({
            text,
            crashed: true
        });
    };

    // Call app/emscripten module entry function. It may either come from the emscripten
    // runtime script or be customized as needed.
    const instance = await Promise.race(
        [circuitBreaker, config.qt.entryFunction(config)]);
    if (loadTimeException && loadTimeException.name !== 'ExitStatus')
        throw loadTimeException;

    return instance;
}
