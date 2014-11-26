#include <qdebug.h>
#include <qglobal.h>
#include <qcoreapplication.h>
#include <qcommandlineparser.h>
#include <qlibraryinfo.h>
#include <qdir.h>
#include <qprocess.h>


void runCommand(const QString &command)
{
    QByteArray c = command.toLatin1();
    // qDebug() << "RUN" << c;

    // This is a host tool, which means no QProcess. Use 'system':
    int r = system(c.constData());
    Q_UNUSED(r);
}

static bool copyRecursively(const QString &srcFilePath,
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

    parser.addOption(QCommandLineOption(QStringList() << "p" << "print", "Print Chrome and debugging help."));
    parser.addOption(QCommandLineOption(QStringList() << "r" << "run", "Run the application in Chrome."));
    parser.addOption(QCommandLineOption(QStringList() << "d" << "debug", "Debug the application in Chrome."));

    parser.process(app);
    const QStringList args = parser.positionalArguments();
    QString tmplate = parser.value("template");
    const bool quickImports = parser.isSet("quick");
    QString outDir = parser.value("out");
    const bool print = parser.isSet("print");
    bool run = parser.isSet("run");
    const bool debug = parser.isSet("debug");
    if (run && debug) // "--debug" takes priority over "run"
        run = false;

    // outDir should end with "/" and exist on disk.
    if (!outDir.endsWith("/"))
        outDir.append("/");
    QDir().mkpath(outDir);

    // Get target binary file name from command line, or find one
    // in the currrent directory
    QStringList binaries = QDir().entryList(QStringList() << "*.nexe" << "*.bc", QDir::Files);
    QString binary;
    if (args.count() == 0) {
        if (binaries.count() == 0) {
            parser.showHelp(0);
            return 0;
        }
        binary = binaries.at(0);
    } else {
        binary = args.at(0);
    }

    bool isPNaCl = binary.endsWith(".bc");

    QString appName = binary;
    appName.replace(".nexe", "");
    appName.replace(".bc", "");
    QString nmf = appName + ".nmf";
    
    if (!QFile(binary).exists()) {
        qDebug() << "File not found" << binary;
        return 0;
    }
    
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

    // Get the lib dir for this Qt install
    // Use argv[0]: path/to/qtbase/bin/nacldeployqt -> path/to/qtbase/lib
    QString nacldeployqtPath = argv[0];
    nacldeployqtPath.chop(QStringLiteral("nacldeployqt").length());
    QString qtBaseDir = QDir(nacldeployqtPath + QStringLiteral("../")).canonicalPath(); 
    QString qtLibDir = qtBaseDir + "/lib";
    QString qtBinDir = qtBaseDir + "/bin";
    QString qtImportsDir = qtBaseDir + "/qml";
    QString nmfFileName = appName + ".nmf";
    QString nmfFilePath = outDir + nmfFileName;

    qDebug() << " ";
    qDebug() << "Deploying" << binary;
    qDebug() << "Using SDK" << naclSdkRoot;
    qDebug() << "Qt libs in" << qtLibDir;
    qDebug() << "Output directory:" << QDir(outDir).canonicalPath();
    qDebug() << " ";

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

    // create the index.html file. Use a built-in template if specified,
    // else use create_html.py. For PNaCl always use a templace since
    // create_html generates nacl-only html.
    if (isPNaCl && tmplate.isEmpty())
        tmplate = "debug";

    if (!isPNaCl && tmplate.isEmpty()) {
        QString outFile = outDir + "index.html";
        QString hmtlCommand = QStringLiteral("python ") + createHtml + " " + nmf
                    + " -o " + outFile;
        runCommand(hmtlCommand.toLatin1().constData());
    } else {
        // select template. See the template_*.cpp files.
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

        // instantiate template. Order matters, LOADERSCRIPT should be
        // replaced first.
        extern const char * loaderScript;
        html.replace("%LOADERSCRIPT%", QByteArray(loaderScript));
        html.replace("%APPNAME%", appName.toUtf8());
        html.replace("%APPTYPE%", isPNaCl ? "application/x-pnacl" : "application/x-nacl");

        // write html contents to index.html
        QFile indexHtml(outDir + "index.html");
        indexHtml.open(QIODevice::WriteOnly|QIODevice::Truncate);
        indexHtml.write(html);
    }

    // NOTE: At this point deployment is done. The following are
    // development aides for running or debugging the app.

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
    QString chromeApp = "/Applications/Google\\ Chrome.app/Contents/MacOS/Google\\ Chrome";
    QString chromeNormalOptions = " --incognito --disable-cache --no-default-browser-check --new-window \"http://localhost:8000\"";
    QString chromeDebugOptions = chromeNormalOptions + " --enable-nacl-debug --no-sandbox";
    if (print) {
        qDebug() << "chrome:";
        qDebug() << qPrintable("  " + chromeApp + chromeNormalOptions);
        qDebug() << qPrintable("chrome (wait for debugger)");
        qDebug() << qPrintable("  " + chromeApp + chromeDebugOptions);
        qDebug() << "";
    }

    // Start Chrome with proper options
    if (run)
        runCommand(chromeApp + chromeNormalOptions + " &");
    else if (debug)
        runCommand(chromeApp + chromeDebugOptions + " &");

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
    if (run || debug) {
        runCommand("python -m SimpleHTTPServer");
    }
}
