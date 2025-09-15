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
 *      called when the application has exited for any reason. There are two cases:
 *      aborted: crashed is true, text contains an error message.
 *      exited: crashed is false, code contians the exit code.
 *
 *      Note that by default Emscripten does not exit when main() returns. This behavior
 *      is controlled by the EXIT_RUNTIME linker flag; set "-s EXIT_RUNTIME=1" to make
 *      Emscripten tear down the runtime and exit when main() returns.
 *
 * - containerElements: HTMLDivElement[]
 *      Array of host elements for Qt screens. Each of these elements is mapped to a QScreen on
 *      launch.
 * - fontDpi: number
 *      Specifies font DPI for the instance
 * - onLoaded: () => void
 *      Called when the module has loaded, at the point in time where any loading placeholder
 *      should be hidden and the application window should be shown.
 * - entryFunction: (emscriptenConfig: object) => Promise<EmscriptenModule>
 *      Qt always uses emscripten's MODULARIZE option. This is the MODULARIZE entry function.
 * - wasmFile: string
 *      The wasm file to download and create the instance from. Specifing the wasm file
 *      makes qtloader take control over the download, which allows it to provide progress
 *      information.
 * - module: Promise<WebAssembly.Module>
 *      The module to create the instance from (optional). Specifying the module allows optimizing
 *      use cases where several instances are created from a single WebAssembly source.
 * - qtdir: string
 *      Path to Qt installation. This path will be used for loading Qt shared libraries and plugins.
 *      The path is set to 'qt' by default, and is relative to the path of the web page's html file.
 *      This property is not in use when static linking is used, since this build mode includes all
 *      libraries and plugins in the wasm file.
 * - onProgress: (receivedBytes: number, totalBytes: number, percent: number) => void
 *      Called as the wasm file and the preload files download. totalBytes grows as
 *      each download reports its content-length, so percent can step backwards early
 *      on; it is a progress indication, not a monotonic one. A server that does not
 *      send content-length contributes received bytes but no total.
 * - preload: [string]: Array of file paths to json-encoded files which specifying which files to preload.
 *      The preloaded files will be downloaded at application startup and copied to the in-memory file
 *      system provided by Emscripten.
 *
 *      Each json file must contain an array of source, destination objects:
 *      [
 *           {
 *               "source": "path/to/source",
 *               "destination": "/path/to/destination"
 *           },
 *           ...
 *      ]
 *      The source path is relative to the html file path. The destination path must be
 *      an absolute path.
 *
 *      $QTDIR may be used as a placeholder for the "qtdir" configuration property (see @qtdir), for instance:
 *          "source": "$QTDIR/plugins/imageformats/libqjpeg.so"
 *  - localFonts.requestPermission: bool
 *       Whether Qt should request for local fonts access permission on startup (default false).
 *  - localFonts.familiesCollection string
 *       Specifies a collection of local fonts to load. Possible values are:
 *          "NoFontFamilies"      : Don't load any font families
 *          "DefaultFontFamilies" : A subset of available font families; currently the "web-safe" fonts (default).
 *          "AllFontFamilies"     : All local font families (not reccomended)
 *  - localFonts.extraFamilies: [string]
 *       Adds additional font families to be loaded at startup.
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
    // DownloadTracker provides download progress by reading
    // the content-length header and intercepting the the download
    // stream. Use the fetch() and response() API to start and
    // complete a download.
    class DownloadTracker {

        constructor() {
            this.resources = new Map();
            this.totalBytes = 0;
            this.receivedBytes = 0;
            this.callback = null;
        }

        clear() {
            this.resources = null;
            this.callback = null;
        }

        setProgressCallback(cb) {
            this.callback = cb;
        }

        _notify() {
            const completed = Math.floor((this.receivedBytes / this.totalBytes) * 100);
            if (this.callback) {
                this.callback(this.receivedBytes, this.totalBytes, completed);
            }
        }

        // Initiates a resouce/file download from url
        fetch(resource) {
            if (this.resources.has(resource))
                return;
            const responsePromise = fetch(resource)
                .then(res => {
                    const len = res.headers.get("content-length");
                    if (len) {
                        this.totalBytes += parseInt(len, 10);
                        this._notify();
                    }
                    return res;
                });
            this.resources.set(resource, responsePromise);
        }

        // Returrns the response object for the given tracked download url.
        // Call fetch() first to initiate the download.
        async response(resource) {
            const res = await this.resources.get(resource);

            const ts = new TransformStream({
                transform: (chunk, controller) => {
                    this.receivedBytes += chunk.length;
                    this._notify();
                    controller.enqueue(chunk);
                }
            });

            res.body.pipeTo(ts.writable);
            return new Response(ts.readable, {
                headers: res.headers,
                status: res.status,
                statusText: res.statusText,
            });
        }
    }

    const downloadTracker = new DownloadTracker();
    if (typeof config.qt.onProgress === 'function')
        downloadTracker.setProgressCallback(config.qt.onProgress);

    class Preloader {

        constructor(downloadTracker) {
            this.downloadTracker = downloadTracker
            this.qtdir = "/";
            this.preloadFileList = [];   // list of files to preload in {source, destinaton} pairs
            this.preloadSourceList = []; // list of resolved sources
            this.preloadDataList = []; // list of downloaded preload file data
        }

        // Initiates download of preload files
        async fetchPreloadFiles() {
            // First, fetch the preload .json file(s) which specifies which
            // files to preload
            this.preloadFileList = (await Promise.all(config.qt.preload.map(async path =>  {
                const response = await fetch(path);
                if (!response.ok)
                    throw new Error("Could not fetch preload file: " + path +
                        ". For a shared (dynamic) build, run wasmdeployqt on the app to " +
                        "generate the Qt preload manifests before serving it.");
                return response.json();
            }))).flat();

            // Then, start downloading each file
            this.preloadSourceList = this.preloadFileList.map(file => {
                const source = file.source.replace('$QTDIR', config.qt.qtdir); // ### take config as ctor arg?
                downloadTracker.fetch(source); // kick off donwload
                return source;
            });
        }

        // Waits for preload file download completion, stores file data in preloadSourceList
        async awaitPreloadFiles() {
            this.preloadDataList = await Promise.all(
              this.preloadSourceList.map(x =>
                downloadTracker.response(x).then(res => res.arrayBuffer())
              )
            );
            this.preloadSourceList = null;
        }

        // Creates Emscripten file system files for all preloaded files.
        // Synchronous: preRun cannot await, and the data is already in memory
        // by this point because awaitPreloadFiles() ran before instantiation.
        createPreloadFiles(instance) {

            // Recursively create directories in filePath
            const makeDirs = (FS, filePath) => {
                const parts = filePath.split("/");
                let path = "/";
                for (let i = 0; i < parts.length - 1; ++i) {
                    const part = parts[i];
                    if (part == "")
                        continue;
                    path += part + "/";
                    try {
                        FS.mkdir(path);
                    } catch (error) {
                        const EEXIST = 20;
                        if (error.errno != EEXIST)
                            throw error;
                    }
                }
            }

            for (let i = 0; i < this.preloadFileList.length; i++) {
                const destination = this.preloadFileList[i].destination;
                const data  = this.preloadDataList[i];
                makeDirs(instance.FS, destination);
                instance.FS.writeFile(destination, new Uint8Array(data));
            }
            this.preloadFileList = null;
            this.preloadDataList = null;
        }
    }

    const preloader = new Preloader(downloadTracker);

    const throwIfEnvUsedButNotExported = (instance, config) =>
    {
        const environment = config.qt.environment;
        if (!environment || Object.keys(environment).length === 0)
            return;
        const descriptor = Object.getOwnPropertyDescriptor(instance, 'ENV');
        const isEnvExported = typeof descriptor.value === 'object';
        if (!isEnvExported) {
            throw new Error('ENV must be exported if environment variables are passed, ' +
                            'add it to the QT_WASM_EXTRA_EXPORTED_METHODS CMake target property');
        }
    };

    if (typeof config !== 'object')
        throw new Error('config is required, expected an object');
    if (typeof config.qt !== 'object')
        throw new Error('config.qt is required, expected an object');
    if (typeof config.qt.entryFunction !== 'function')
        throw new Error('config.qt.entryFunction is required, expected a function');

    config.qt.qtdir ??= 'qt';
    config.qt.preload ??= [];

    config.qtContainerElements = config.qt.containerElements;
    delete config.qt.containerElements;
    config.qtFontDpi = config.qt.fontDpi;
    delete config.qt.fontDpi;

    // Make Emscripten not call main(); this gives us more control over
    // the startup sequence.
    const originalNoInitialRun = config.noInitialRun;
    const originalArguments = config.arguments;
    config.noInitialRun = true;

    // Used for rejecting a failed load's promise where emscripten itself does not allow it,
    // like in instantiateWasm below. This allows us to throw in case of a load error instead of
    // hanging on a promise to entry function, which emscripten unfortunately does.
    let circuitBreakerReject;
    const circuitBreaker = new Promise((_, reject) => { circuitBreakerReject = reject; });

    // Use a custom instantiateWasm function if either a module or wasm file as been specified.
    if (config.qt.module || config.qt.wasmFile) {
        config.instantiateWasm = async (imports, successCallback) =>
        {
            try {
                // Get the wasm module, either as a config property or by streaming
                // download / compile.
                let module;
                if (config.qt.module) {
                    module = await config.qt.module;
                } else {
                    let wasmFileResponse = downloadTracker.response(config.qt.wasmFile);
                    module = await WebAssembly.compileStreaming(wasmFileResponse);
                }

                // Await preloaded files to make sure they are ready before main() is called.
                await preloader.awaitPreloadFiles();

                // Create the wasm instance
                successCallback(await WebAssembly.instantiate(module, imports), module);
            } catch (e) {
                circuitBreakerReject(e);
            }
        }
    }
    const qtPreRun = (instance) => {

        // Copy qt.environment to instance.ENV
        throwIfEnvUsedButNotExported(instance, config);
        for (const [name, value] of Object.entries(config.qt.environment ?? {}))
            instance.ENV[name] = value;

        // Copy preload files to the file syste
        const isFsExported = typeof instance.FS === 'object';
        if (!isFsExported)
            throw new Error('FS must be exported if preload is used');
        preloader.createPreloadFiles(instance);
    }

    if (!config.preRun)
        config.preRun = [];
    config.preRun.push(qtPreRun);

    const originalOnRuntimeInitialized = config.onRuntimeInitialized;
    config.onRuntimeInitialized = () => {
        originalOnRuntimeInitialized?.();
        config.qt.onLoaded?.();
    }

    config.locateFile ??= (filename, prefix) => {
        // Preloaded shared objects are named by their in memory destination, map
        // them back to the url they are served from.
        const preloadSource = preloadedSharedObjects.get(filename);
        if (preloadSource)
            return (prefix ?? '') + preloadSource;

        // dynamic linking: locate Qt libraries at qtdir location
        // wasmdeployqt relies on this behavior, update both in case of change
        if (filename.startsWith('libQt6'))
            return `${config.qt.qtdir}/lib/${filename}`;

        // Mirror Emscripten's default locateFile and prepend the prefix
        return (prefix ?? '') + filename;
    }

    let onExitCalled = false;
    const originalOnExit = config.onExit;
    config.onExit = code => {
        originalOnExit?.();

        if (!onExitCalled) {
            onExitCalled = true;
            config.qt.onExit?.({
                code,
                crashed: false
            });
        }
    }

    const originalOnAbort = config.onAbort;
    config.onAbort = text =>
    {
        originalOnAbort?.();
        
        if (!onExitCalled) {
            onExitCalled = true;
            config.qt.onExit?.({
                text,
                crashed: true
            });
        }
    };

    // Start all downloads using the download tracker which updates the download progress.
    if (config.qt.wasmFile)
        downloadTracker.fetch(config.qt.wasmFile)
    await preloader.fetchPreloadFiles(); // Note: awaits fetch start only, not the complete download

    // Side modules must be instantiated identically on every thread. Emscripten's
    // preload plugin instantiates each preloaded .so on the calling thread only,
    // which leaves pthread workers with a shorter function table, so any cross
    // thread indirect call traps with "table index is out of bounds". Route the
    // shared objects through dynamicLibraries instead, which every thread loads
    // at startup and which workers take from the main thread's compiled modules,
    // and keep the plain files in the filesystem so plugin metadata scans still
    // find them. Runs here rather than before the Preloader, which is what now
    // knows the file list, and still ahead of the entry function below, which
    // is what reads config.dynamicLibraries.
    const preloadedSharedObjects = new Map();
    for (const file of preloader.preloadFileList) {
        if (file.destination.endsWith(".so"))
            preloadedSharedObjects.set(file.destination,
                file.source.replace("$QTDIR", config.qt.qtdir));
    }
    if (preloadedSharedObjects.size > 0) {
        config.noWasmDecoding = true;
        config.dynamicLibraries =
            [...preloadedSharedObjects.keys()].concat(config.dynamicLibraries ?? []);
    }

    // The instantiateWasm path awaits the preload downloads before creating the
    // instance, but it only exists when a module or a wasm file was given. With
    // neither, Emscripten does the wasm download itself and nothing waits for
    // the preload data, so preRun would write empty files. Await here instead:
    // preRun runs inside the entry function below.
    if (!config.qt.module && !config.qt.wasmFile)
        await preloader.awaitPreloadFiles();

    // Call the emscripten instance entry (constructor)function which creates the instance.
    // Pass the config ovject, which contains user config as well as config options set above.
    // This will wait until all dowloads have completed and the WebAssembly module has been
    // compiled, and then return the instance.
    let instance;
    try {
        instance = await Promise.race(
            [circuitBreaker, config.qt.entryFunction(config)]);

        // Call main after creating the instance. We've opted into manually
        // calling main() by setting noInitialRun in the config. Thie Works around
        // issue where Emscripten suppresses all exceptions thrown during main.
        if (!originalNoInitialRun)
            instance.callMain(originalArguments);

    } catch (e) {
        // If this is the exception thrown by app.exec() then that is a normal
        // case and we suppress it.
        if (e == "unwind") // not much to go on
            return;

        if (!onExitCalled) {
            onExitCalled = true;
            config.qt.onExit?.({
                text: e.message,
                crashed: true
            });
        }
        throw e;

    } finally {
        downloadTracker.clear();
    }

    return instance;
}

// Compatibility API. This API is deprecated,
// and will be removed in a future version of Qt.
function QtLoader(qtConfig) {

    const warning = 'Warning: The QtLoader API is deprecated and will be removed in ' +
                    'a future version of Qt. Please port to the new qtLoad() API.';
    console.warn(warning);

    let emscriptenConfig = qtConfig.moduleConfig || {}
    qtConfig.moduleConfig = undefined;
    const showLoader = qtConfig.showLoader;
    qtConfig.showLoader = undefined;
    const showError = qtConfig.showError;
    qtConfig.showError = undefined;
    const showExit = qtConfig.showExit;
    qtConfig.showExit = undefined;
    const showCanvas = qtConfig.showCanvas;
    qtConfig.showCanvas = undefined;
    if (qtConfig.canvasElements) {
        qtConfig.containerElements = qtConfig.canvasElements
        qtConfig.canvasElements = undefined;
    } else {
        qtConfig.containerElements = qtConfig.containerElements;
        qtConfig.containerElements = undefined;
    }
    emscriptenConfig.qt = qtConfig;

    let qtloader = {
        exitCode: undefined,
        exitText: "",
        loadEmscriptenModule: _name => {
            try {
                qtLoad(emscriptenConfig);
            } catch (e) {
                showError?.(e.message);
            }
        }
    }

    qtConfig.onLoaded = () => {
        showCanvas?.();
    }

    qtConfig.onExit = exit => {
        qtloader.exitCode = exit.code
        qtloader.exitText = exit.text;
        showExit?.();
    }

    showLoader?.("Loading");

    return qtloader;
};
