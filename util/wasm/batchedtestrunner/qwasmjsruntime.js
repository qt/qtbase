// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Exposes platform capabilities as static properties

export class AbortedError extends Error {
    constructor(stdout) {
        super(`The program has been aborted`)

        this.stdout = stdout;
    }
}
export class Platform {
    static #webAssemblySupported = typeof WebAssembly !== 'undefined';

    static #canCompileStreaming = WebAssembly.compileStreaming !== 'undefined';

    static #webGLSupported = (() => {
        // We expect that WebGL is supported if WebAssembly is; however
        // the GPU may be blacklisted.
        try {
            const canvas = document.createElement('canvas');
            return !!(
                window.WebGLRenderingContext &&
                (canvas.getContext('webgl') || canvas.getContext('experimental-webgl'))
            );
        } catch (e) {
            return false;
        }
    })();

    static #canLoadQt = Platform.#webAssemblySupported && Platform.#webGLSupported;

    static get webAssemblySupported() {
        return this.#webAssemblySupported;
    }
    static get canCompileStreaming() {
        return this.#canCompileStreaming;
    }
    static get webGLSupported() {
        return this.#webGLSupported;
    }
    static get canLoadQt() {
        return this.#canLoadQt;
    }
}

// Locates a resource, based on its relative path
export class ResourceLocator {
    #rootPath;

    constructor(rootPath) {
        this.#rootPath = rootPath;
        if (rootPath.length > 0 && !rootPath.endsWith('/')) rootPath += '/';
    }

    locate(relativePath) {
        return this.#rootPath + relativePath;
    }
}

// Allows fetching of resources, such as text resources or wasm modules.
export class ResourceFetcher {
    #locator;

    constructor(locator) {
        this.#locator = locator;
    }

    async fetchText(filePath) {
        return (await this.#fetchRawResource(filePath)).text();
    }

    async fetchCompileWasm(filePath, onFetched) {
        const fetchResponse = await this.#fetchRawResource(filePath);
        onFetched?.();

        if (Platform.canCompileStreaming) {
            try {
                return await WebAssembly.compileStreaming(fetchResponse);
            } catch {
                // NOOP - fallback to sequential fetching below
            }
        }
        return WebAssembly.compile(await fetchResponse.arrayBuffer());
    }

    async #fetchRawResource(filePath) {
        const response = await fetch(this.#locator.locate(filePath));
        if (!response.ok)
            throw new Error(
                `${response.status} ${response.statusText} ${response.url}`
            );
        return response;
    }
}

// Represents a WASM module, wrapping the instantiation and execution thereof.
export class CompiledModule {
    #createQtAppInstanceFn;
    #js;
    #wasm;
    #resourceLocator;

    constructor(createQtAppInstanceFn, js, wasm, resourceLocator) {
        this.#createQtAppInstanceFn = createQtAppInstanceFn;
        this.#js = js;
        this.#wasm = wasm;
        this.#resourceLocator = resourceLocator;
    }

    static make(js, wasm, entryFunctionName, resourceLocator)
    {
        const exports = {};
        const module = {};
        eval(js);
        if (!module.exports) {
            throw new Error(
                '${entryFunctionName} has not been exported by the main script'
            );
        }

        return new CompiledModule(
            module.exports, js, wasm, resourceLocator
        );
    }

    async exec(parameters) {
        return await new Promise(async (resolve, reject) => {
            let instance = undefined;
            let result = undefined;

            let testFinished = false;
            const testFinishedEvent = new CustomEvent('testFinished');
            instance = await this.#createQtAppInstanceFn((() => {
                const params = this.#makeDefaultExecParams({
                    onInstantiationError: (error) => { reject(error); },
                });
                params.arguments = parameters?.args;
                let data = '';
                params.print = (out) => {
                    parameters?.onStdout?.(out);
                    data += `${out}\n`;
                };
                params.printErr = () => { };
                params.onAbort = () => reject(new AbortedError(data));
                params.quit = (code, exception) => {
                    if (exception && exception.name !== 'ExitStatus')
                        reject(exception);
                };
                params.notifyTestFinished = (code) => {
                    result = { stdout: data, exitCode: code };
                    testFinished = true;
                    window.dispatchEvent(testFinishedEvent);
                };
                return params;
            })());
            if (!testFinished) {
                await new Promise((resolve) => {
                    window.addEventListener('testFinished', () => {
                        resolve();
                    });
                });
            }
            resolve({
                stdout: result.stdout,
                exitCode: result.exitCode,
                instance,
            });
        });
    }

    #makeDefaultExecParams(params) {
        const instanceParams = {};
        instanceParams.instantiateWasm = async (imports, onDone) => {
            try {
                onDone(await WebAssembly.instantiate(this.#wasm, imports), this.#wasm);
            } catch (e) {
                params?.onInstantiationError?.(e);
            }
        };
        instanceParams.locateFile = (filename) =>
            this.#resourceLocator.locate(filename);
        instanceParams.monitorRunDependencies = (name) => { };
        instanceParams.print = (text) => true && console.log(text);
        instanceParams.printErr = (text) => true && console.warn(text);

        instanceParams.mainScriptUrlOrBlob = new Blob([this.#js], {
            type: 'text/javascript',
        });
        return instanceParams;
    }
}

// Streamlines loading of WASM modules.
export class ModuleLoader {
    #fetcher;
    #resourceLocator;

    constructor(
        fetcher,
        resourceLocator
    ) {
        this.#fetcher = fetcher;
        this.#resourceLocator = resourceLocator;
    }

    // Loads an emscripten module named |moduleName| from the main resource path. Provides
    // progress of 'downloading' and 'compiling' to the caller using the |onProgress| callback.
    async loadEmscriptenModule(
        moduleName, onProgress
    ) {
        if (!Platform.webAssemblySupported)
            throw new Error('Web assembly not supported');
        if (!Platform.webGLSupported)
            throw new Error('WebGL is not supported');

        onProgress('downloading');

        const jsLoadPromise = this.#fetcher.fetchText(`${moduleName}.js`);
        const wasmLoadPromise = this.#fetcher.fetchCompileWasm(
            `${moduleName}.wasm`,
            () => {
                onProgress('compiling');
            }
        );

        const [js, wasm] = await Promise.all([jsLoadPromise, wasmLoadPromise]);
        return CompiledModule.make(js, wasm, `${moduleName}_entry`, this.#resourceLocator);
    }
}
