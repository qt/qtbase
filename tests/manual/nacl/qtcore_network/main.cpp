#include <QtCore>
#include <QtNetwork>
#include <QtGui>

class First
{
public:
    First() {
        // Enable spesific logging here
        QLoggingCategory::setFilterRules(QStringLiteral("qt.platform.pepper.coreeventdispatcher.debug=false\n"
                                                        "qt.platform.pepper.eventdispatcher.debug=false\n"
                                                        "qt.platform.pepper.network.debug=true\n"
                                                        "qt.platform.pepper.instance.debug=false\n"));

    }
};
First first;

class NetworkAccessTester : public QObject
{
Q_OBJECT
    QNetworkAccessManager *networkAccessManager;
public:
    NetworkAccessTester() {
        connect(this, SIGNAL(signal()), this, SLOT(callback()), Qt::QueuedConnection);
    }
signals:
    void signal();
private slots:
    void callback() {
        qDebug() << "NetworkAccessTester callback" << QThread::currentThread();
        QThread::msleep(500);
        networkAccessManager = new QNetworkAccessManager();
        // send GET request which should show up in the server access log if successful
        networkAccessManager->get(QNetworkRequest(QUrl("localfile.txt")));
    }
};

QThread *thread = 0;
NetworkAccessTester *mainTester;
NetworkAccessTester *workerThreadTester;

void app_init(int argc, char **argv)
{
    thread = new QThread();
    thread->start();

//  crashes
//    mainTester = new NetworkAccessTester;
//    emit mainTester->signal();

    workerThreadTester = new NetworkAccessTester;
    workerThreadTester->moveToThread(thread);
    emit workerThreadTester->signal();
}

void app_exit()
{
    qDebug() << "Running app_exit";
}

Q_GUI_MAIN(app_init, app_exit);

#include "main.moc"
