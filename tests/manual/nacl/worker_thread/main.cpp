#include <QtCore>
#include <QtGui>

class First
{
public:
    First() {
        // Enable spesific logging here
        QLoggingCategory::setFilterRules(QStringLiteral("qt.platform.pepper.coreeventdispatcher.debug=true\n"
                                                        "qt.platform.pepper.eventdispatcher.debug=true\n"
                                                        "qt.platform.pepper.instance.debug=false\n"));

    }
};
First first;

int emitCounter = 0;

class MainThreadObject : public QObject
{
Q_OBJECT
    public:
public slots:
    void callback() {
        qDebug() << "main thread callback";
    }
};

MainThreadObject *mainThreadObject = 0;

class WorkerThreadObject : public QObject
{
Q_OBJECT
public:
    WorkerThreadObject() {
        qDebug() << "hello test object";
        connect(this, SIGNAL(signal()), this, SLOT(callback()), Qt::QueuedConnection);
        connect(this, SIGNAL(signal()), mainThreadObject, SLOT(callback()), Qt::QueuedConnection);
    }
signals:
    void signal();
private slots:
    void callback() {
        qDebug() << "worker thread callback";
        QThread::msleep(500);
        ++emitCounter;
        if (emitCounter < 50)
            emit signal();
    }
};

QThread *thread = 0;
WorkerThreadObject *workerThreadObject = 0;

void app_init(int argc, char **argv)
{
    qDebug() << "Running app_init";
    
    // Start a worker thread, verify that queued signal-slot connections work.
    thread = new QThread();
    thread->start();
    mainThreadObject = new MainThreadObject;
    workerThreadObject = new WorkerThreadObject;
    workerThreadObject->moveToThread(thread);
    emit workerThreadObject->signal();
}

void app_exit()
{
    qDebug() << "Running app_exit";
}

Q_GUI_MAIN(app_init, app_exit);

#include "main.moc"
