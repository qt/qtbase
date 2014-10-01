#include <qdebug.h>
#include <qglobal.h>
#include <qlibraryinfo.h>
#include <qdir.h>
#include <qprocess.h>

int main(int argc, char **argv)
{

    // Get target nexe file name from command line, or find one
    // in the cuerrent directory
    QStringList nexes = QDir().entryList(QStringList() << "*.nexe", QDir::Files);
    QString nexe;
    if (argc < 2) {
        if (nexes.count() == 0) {
            qDebug() << "Usage: nacldeployqt app.nexe";
            return 0;
        }
        nexe = nexes.at(0);
    } else {
        nexe = argv[1];
    }

    QString appName = nexe;
    appName.chop(QStringLiteral(".nexe").length());
    QString nmf = appName + ".nmf";
    
    if (!QFile(nexe).exists()) {
        qDebug() << "File not found" << nexe;
        return 0;
    }
    
    // Get the NaCl SDK root from environment
    QString naclSdkRoot = qgetenv("NACL_SDK_ROOT");
    if (naclSdkRoot.isEmpty()) {
        qDebug() << "Plese set NACL_SDK_ROOT";
        qDebug() << "For example: /Users/foo/nacl_sdk/pepper_35";
        return 0;
    }

    // Get deployment scripts:
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
    
    // Get the lib dir for this Qt install
    // Use argv[0]: path/to/qtbase/bin/nacldeployqt -> path/to/qtbase/lib
    QString nacldeployqtPath = argv[0];
    nacldeployqtPath.chop(QStringLiteral("nacldeployqt").length());
    QString qtBaseDir = QDir(nacldeployqtPath + QStringLiteral("../")).canonicalPath(); 
    QString qtLibDir = qtBaseDir + "/lib";

    qDebug() << " ";
    qDebug() << "Deploying" << nexe;
    qDebug() << "Using SDK" << naclSdkRoot;
    qDebug() << "Qt libs in" << qtLibDir;
    qDebug() << " ";
    
    // create the .nmf manifest file
    QString nmfCommand = QStringLiteral("python ") + createNmf
                + " -o " + appName + ".nmf"
                + " -L " + qtLibDir           // Add Qt libs search payh
                + " -s  . "                   // copy dependencies 
//                + " --debug-libs"    
                + " " + nexe;
    
    system(nmfCommand.toLatin1().constData());

    // create html file
    QString hmtlCommand = QStringLiteral("python ") + createHtml + " " + nmf 
                + " -o index.html";
    system(hmtlCommand.toLatin1().constData());
    
    qDebug() << "Serving on localhost:8000";
    system("python -m SimpleHTTPServer");
}