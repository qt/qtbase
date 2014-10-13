#include <QtCore>
#include <QtGui>

class First
{
public:
    First() {
        // Enable spesific logging here
//        QLoggingCategory::setFilterRules(QStringLiteral("qt.platform.pepper.*=true"));
    }
};
First first;

void testfile(const QString &filename)
{
    qDebug() << " ";
    QFile f(filename);
    qDebug() << "exists" << f.fileName() << f.exists();
    bool ok = f.open(QIODevice::ReadOnly);
    qDebug() << "open ok" << ok;
    QByteArray contents = f.readAll();        
    qDebug() << "resource file contents: " << contents;
}

QWindow *window = 0;

// App-provided init and exit functions:
void app_init(const QStringList &arguments)
{

    window = new QWindow();
    window->show();

    qDebug() << "Running app_init";
    
    testfile(":/resource.txt");
    testfile(":/fonts/Vera.ttf");
}

void app_exit()
{
    delete window;
    qDebug() << "Running app_exit";
}

// Register functions with Qt. The type of register function
// selects the QApplicaiton type.
Q_GUI_MAIN(app_init, app_exit);
