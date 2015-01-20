#include <qdebug.h>
#include <qglobal.h>
#include <qcoreapplication.h>
#include <qcommandlineparser.h>
#include <qlibraryinfo.h>
#include <qdir.h>
#include <qhash.h>
#include <qset.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

// QtNaclDeployer Usage: set the public options and call deploy();
class QtNaclDeployer
{
public:
    QString tmplate;
    bool quickImports;
    bool isApp;
    QString outDir;
    bool print;
    bool run;
    bool debug;
    bool testlibMode;
    QString binary;
    QString qtBaseDir;

    int deploy();

private:
    QByteArray mainHtmlFileName;
    QHash<QByteArray, QByteArray> templateReplacements;
    bool isPNaCl;
    QString appName;
    QString nmf;
    QSet<QByteArray> permissions;
    QString stdoutCaptureFile;
    QString stderrCaptureFile;

    QList<QByteArray> quote(const QList<QByteArray> &list);
    void runCommand(const QString &command);
    bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath);
    QByteArray instantiateTemplate(const QByteArray &tmplate);
    void instantiateWriteTemplate(const QByteArray &tmplate, const QString &filePath);
};

int QtNaclDeployer::deploy()
{
    mainHtmlFileName = "index.html";
    // Create the default Chrome App permission set.
    permissions = { "clipboardRead", "clipboardWrite" };
    stdoutCaptureFile = "qt_stdout";
    stderrCaptureFile = "qt_stderr";

    if (testlibMode && !run) // "--testlib" implies "run"
        run = true;

    if (run && debug) // "--debug" takes priority over "run"
        run = false;

    // outDir should end with "/" and exist on disk.
    if (!outDir.endsWith("/"))
        outDir.append("/");
    QDir().mkpath(outDir);

    isPNaCl = binary.endsWith(".bc");

    QString appName = binary;
    appName.replace(".nexe", "");
    appName.replace(".bc", "");
    nmf = appName + ".nmf";

    if (!QFile(binary).exists()) {
        qDebug() << "File not found" << binary;
        return 0;
    }
    
    if (!quickImports && QFile::exists(binary + "_qml_plugin_import.cpp")) {
        qDebug() << "Note: qml_plugin_import.cpp file found. Auto-enabling --quick option.";
        quickImports = true;
    }

    // Set up template replacments. There is one global map
    // for all templates.
    templateReplacements["%MAINHTML%"] = mainHtmlFileName;
    templateReplacements["%APPNAME%"] = appName.toUtf8();
    templateReplacements["%APPTYPE%"] = isPNaCl ? "application/x-pnacl" : "application/x-nacl";
    templateReplacements["%CHROMEAPP%"] = isApp ? "true;" : "false;";
    templateReplacements["%PERMISSIONS%"] = quote(permissions.toList()).join(", ");

    // Get the NaCl SDK root from environment
    QString naclSdkRoot = qgetenv("NACL_SDK_ROOT");
    if (naclSdkRoot.isEmpty()) {
        qDebug() << "Plese set NACL_SDK_ROOT";
        qDebug() << "For example: /Users/foo/nacl_sdk/pepper_35";
        return 0;
    }

    // Get the location of the deployment scripts in the nacl sdk:
    QString createNmf = naclSdkRoot + "/tools/create_nmf.py";
    if (!QFile(createNmf).exists()) {
        qDebug() << "create_nmf.py not found at" << createNmf;
        return 0;
    }
    QString createHtml = naclSdkRoot + "/tools/create_html.py";
    if (!QFile(createHtml).exists()) {
        qDebug() << "create_html.py not found at" << createHtml;
        return 0;
    }
    QString pnaclFinalize = naclSdkRoot + "/toolchain/mac_pnacl/bin/pnacl-finalize";
    if (!QFile(pnaclFinalize).exists())
        pnaclFinalize = naclSdkRoot + "/toolchain/linux_pnacl/bin/pnacl-finalize";
    if (!QFile(pnaclFinalize).exists()) {
        qDebug() << "pnacl-finalize not found at" << pnaclFinalize;
        return 0;
    }

    QString qtLibDir = qtBaseDir + "/lib";
    QString qtBinDir = qtBaseDir + "/bin";
    QString qtImportsDir = qtBaseDir + "/qml";
    QString nmfFileName = appName + ".nmf";
    QString nmfFilePath = outDir + nmfFileName;

    // Print info about the environment; exept in testlib mdoe, where we only
    // want testlib output.
    if (!testlibMode) {
        qDebug() << " ";
        qDebug() << "Deploying" << binary;
        qDebug() << "Using SDK" << naclSdkRoot;
        qDebug() << "Qt libs in" << qtLibDir;
        qDebug() << "Output directory:" << QDir(outDir).canonicalPath();
        qDebug() << " ";
    }

    // Deply Qt Quick imports
    if (quickImports) {
        // TODO: At some pont use qmlimportscanner to select imports in use.
        // For now simply deploy all imports.
        QStringList imports = QDir(qtImportsDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        QString targetBase = outDir + "qml";
        QDir().mkpath(targetBase);

        foreach (const QString &import, imports) {
            QString source = qtImportsDir + "/" + import;
            QString target = targetBase + "/" + import;

            // TODO: Skip the binaries for static builds; they will be built into the main nexe
            copyRecursively(source, target);
        }
    }

    QString finalBinary = binary;

    // On PNaCl, the output from "make" is a bitcode .bc file. Create
    // a .pexe suitable for distribution using "pnacl-finalize".
    if (isPNaCl) {
        finalBinary.replace(".bc", ".pexe");
        QString finalizeCommand = pnaclFinalize + " -o " + outDir + finalBinary + " " + binary;
        runCommand(finalizeCommand);
    } else {
        QFile::copy(binary, outDir + finalBinary);
    }

    const QString finalBinaryPath = outDir + finalBinary;

    // create the .nmf manifest file
    QString nmfCommand = QStringLiteral("python ") + createNmf
                + " -o " + nmfFilePath
                + " -L " + qtLibDir           // Add Qt libs search payh
                + " -s  . "                   // copy dependencies 
//                + " --debug-libs"    
                + " " + finalBinaryPath
                + " " + (debug ? binary : QString("")); // deploy .bc when debugging (adds a "pnacl-debug" section to the nmf)
    
    runCommand(nmfCommand.toLatin1().constData());

    // select template. See the template_*.cpp files.
    if (tmplate.isEmpty())
        tmplate = "windowed";
    QByteArray html;
    if (tmplate== "fullscreen") {
        extern const char * templateFullscreen;
        html = QByteArray(templateFullscreen);
    } else if (tmplate== "debug") {
        extern const char * templateDebug;
        html = QByteArray(templateDebug);
    } else if (tmplate== "windowed") {
        extern const char * templateWindowed;
        html = QByteArray(templateWindowed);
    }

    // Write the main html file.
    instantiateWriteTemplate(html, outDir + mainHtmlFileName);

    // Write qtloader.js
    extern const char *loaderScript;
    instantiateWriteTemplate(QByteArray(loaderScript),  outDir + "qtloader.js");

    // Create Chrome App support files.
    if (isApp) {
        // Write manifest.json
        extern const char *templateAppManifest;
        instantiateWriteTemplate(QByteArray(templateAppManifest), outDir + "manifest.json");

        // Write background.js
        extern const char *templateBackgroundJs;
        instantiateWriteTemplate(QByteArray(templateBackgroundJs), outDir + "background.js");
    }

    // NOTE: At this point deployment is done. The following are
    // development aides for running or debugging the app.

    if (testlibMode) {
        QFile::remove(stdoutCaptureFile);
        QFile::remove(stderrCaptureFile);
    }

    // Find the debugger, print startup instructions
    QString gdb = naclSdkRoot + "/toolchain/mac_x86_glibc/bin/i686-nacl-gdb";
    if (!QFile(gdb).exists())
        gdb = naclSdkRoot + "/toolchain/linux_x86_glibc/bin/i686-nacl-gdb";
    QString nmfPath = QDir(nmfFilePath).canonicalPath();
    QString gdbOptions = " -eval-command='nacl-manifest " + nmfPath + "'";

    if (print) {
        qDebug() << "debugger";
        qDebug() << qPrintable("  " + gdb);
        qDebug() << qPrintable("    target remote localhost:4014");
        qDebug() << qPrintable("    nacl-manifest " + nmfFilePath);
        qDebug() << "";
    }

    // Find chrome, print startup instructions
    QString chromeExecutable = "/Applications/Google\\ Chrome.app/Contents/MacOS/Google\\ Chrome";
    QString chromeNormalOptions = " --disable-cache --no-default-browser-check";
    QString chromeDebugOptions = chromeNormalOptions + " --enable-nacl-debug --no-sandbox";
    QString chromeOpenAppOptions = " --load-and-launch-app=" + outDir;
    QString chromeOpenWindowOptions = " --incognito --new-window \"http://localhost:8000\"";
    QString chromeOpenOptions = isApp ? chromeOpenAppOptions : chromeOpenWindowOptions;
    QString chromeRedirectOptions = testlibMode ? " 1>" + stdoutCaptureFile + " 2>" + stderrCaptureFile: " ";

    if (print) {
        qDebug() << "chrome:";
        qDebug() << qPrintable("  " + chromeExecutable + chromeNormalOptions + chromeOpenOptions);
        qDebug() << qPrintable("chrome (wait for debugger)");
        qDebug() << qPrintable("  " + chromeExecutable + chromeDebugOptions + chromeOpenOptions);
        qDebug() << "";
    }

    // Start Chrome with proper options
    if (run)
        runCommand(chromeExecutable + chromeNormalOptions + chromeOpenOptions + chromeRedirectOptions + " &");
    else if (debug)
        runCommand(chromeExecutable + chromeDebugOptions + chromeOpenOptions + chromeRedirectOptions + " &");

    // Start the debugger
    if (debug) {
        // print help string, then lanch gdb.
        QString gdbCommand = "echo Connect to Chrome: target remote localhost:4014;\n\n"
                           + gdb + gdbOptions;
        // tell osascript to tell Terminal to tell gdb to debug the App.
        QString terminal = R"STR(osascript -e 'tell application "Terminal" to do script "'"SCRIPT"'" ')STR";
        terminal.replace("SCRIPT", gdbCommand);
        runCommand(terminal);
    }

    // Start a HTTP server and serve the app.
    QString serverRedirectOptions = testlibMode ? "> /dev/null 2>&1 &" : " ";
    if (run || debug) {
        runCommand("python -m SimpleHTTPServer " + serverRedirectOptions);
    }

    if (testlibMode) {
        // testlibMode: At this point the test is starting up / running with output
        // redirected to stdoutCaptureFile. Poll the file at regular intervals to
        // check for new lines. Echo them to standard out. Keep doing this
        // until test completion, which is tested for by looking for a line like
        // ********* Finished testing of tst_Foo *********
        int skipLines = 0;
        while (true) {
            usleep(500 * 1000);
            QFile out(stdoutCaptureFile);
            out.open(QIODevice::ReadOnly);

            // read and discard already printed lines;
            for (int i = 0; i < skipLines; ++i)
                out.readLine();

            // read and print new lines
            QByteArray line;
            do {
                line = out.readLine().trimmed();
                if (!line.isEmpty()) {
                    printf("L %s\n", line.constData());
                    ++skipLines;
                }
                if (line.contains("********* Finished testing of")) {
                    // DONE: Kill process group: nacldeployqt, Chrome, and the Python server.
                    QFile::remove(stdoutCaptureFile);
                    QFile::remove(stderrCaptureFile);
                    kill(0, 9);
                }
            } while (!line.isEmpty());

            // check Chrome stderr for Qt process crash/exit.
            QFile err(stderrCaptureFile);
            err.open(QIODevice::ReadOnly);
            QByteArray contents = err.readAll();
            if (contents.contains("NaCl process exited with status")
                || contents.contains("NaCl untrusted code called _exit")) {
                QFile::remove(stdoutCaptureFile);
                QFile::remove(stderrCaptureFile);
                kill(0, 9);
            }
        } // while (true)
    } // testlibMode

    return 0;
}

QList<QByteArray> QtNaclDeployer::quote(const QList<QByteArray> &list)
{
    QList<QByteArray> result;
    for (const QByteArray &item : list)
        result.append('"' + item + '"');
    return result;
}

void QtNaclDeployer::runCommand(const QString &command)
{
    QByteArray c = command.toLatin1();
    // qDebug() << "RUN" << c;

    // This is a host tool, which means no QProcess. Use 'system':
    int r = system(c.constData());
    Q_UNUSED(r);
}

bool QtNaclDeployer::copyRecursively(const QString &srcFilePath,
                                            const QString &tgtFilePath)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        targetDir.mkdir(QFileInfo(tgtFilePath).fileName());
        if (!targetDir.exists())
            return false;
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                    = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                    = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath))
            return false;
    }
    return true;
}


QByteArray QtNaclDeployer::instantiateTemplate(const QByteArray &tmplate)
{
    QByteArray instantiated = tmplate;
    int replacementCount;

    do {
        replacementCount = 0;
        for (auto it : templateReplacements.keys()) {
            if (!instantiated.contains(it))
                continue;
            ++replacementCount;
            instantiated.replace(it, templateReplacements[it]);
        }
    } while (replacementCount > 0);

    return instantiated;
}

void QtNaclDeployer::instantiateWriteTemplate(const QByteArray &tmplate, const QString &filePath)
{
    QByteArray fileContents = instantiateTemplate(tmplate);
    QFile outFile(filePath);
    outFile.open(QIODevice::WriteOnly|QIODevice::Truncate);
    outFile.write(fileContents);
}

int main(int argc, char **argv)
{
    // Command line handling
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("nacldeployqt");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("binary", "Application binary file (.nexe or .bc)");

    parser.addOption(QCommandLineOption(QStringList() << "t" << "template",
                        "Selects a html template. Can be either 'fullscreen' or 'debug'. "
                        "Omit this option to use the default nacl_sdk template, as created "
                        "by create_html.py", "template", ""));

    parser.addOption(QCommandLineOption(QStringList() << "q" << "quick", "Deploy Qt Quick imports"));
    parser.addOption(QCommandLineOption(QStringList() << "o" << "out", "Specify an output directory", "outDir", "."));

    parser.addOption(QCommandLineOption(QStringList() << "a" << "app", "Create Chrome App."));

    parser.addOption(QCommandLineOption(QStringList() << "p" << "print", "Print Chrome and debugging help."));
    parser.addOption(QCommandLineOption(QStringList() << "r" << "run", "Run the application in Chrome."));
    parser.addOption(QCommandLineOption(QStringList() << "d" << "debug", "Debug the application in Chrome."));
    parser.addOption(QCommandLineOption(QStringList() << "e" << "testlib", "Run in QTestLib mode"));

    parser.process(app);
    QStringList args = parser.positionalArguments();

    QtNaclDeployer deployer;
    deployer.tmplate = parser.value("template");
    deployer.quickImports = parser.isSet("quick");
    deployer.isApp = parser.isSet("app");
    deployer.outDir = parser.value("out");
    deployer.print = parser.isSet("print");
    deployer.run = parser.isSet("run");
    deployer.debug = parser.isSet("debug");
    deployer.testlibMode = parser.isSet("testlib");

    // Get target binary file name from command line, or find one
    // in the currrent directory
    QStringList binaries = QDir().entryList(QStringList() << "*.nexe" << "*.bc", QDir::Files);
    if (args.count() == 0) {
        if (binaries.count() == 0) {
            parser.showHelp(0);
            return 0;
        }
        deployer.binary = binaries.at(0);
    } else {
        deployer.binary = args.at(0);
    }

    // Find the path to the Qt install. Go via argv[0]:
    // path/to/qtbase/bin/nacldeployqt -> path/to/qtbase/
    QString nacldeployqtPath = argv[0];
    nacldeployqtPath.chop(QStringLiteral("nacldeployqt").length());
    deployer.qtBaseDir = QDir(nacldeployqtPath + QStringLiteral("../")).canonicalPath();

    return deployer.deploy();
}
