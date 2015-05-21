/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMainWindow>
#include <QSplitter>
#include <QVector>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QPlainTextEdit>
#include <QPaintEvent>
#include <QScreen>
#include <QDebug>
#include <QTextStream>

typedef QVector<QEvent::Type> EventTypeVector;

class EventFilter : public QObject {
    Q_OBJECT
public:
    explicit EventFilter(const EventTypeVector &types, QObject *p) : QObject(p), m_types(types) {}

    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;

signals:
    void eventReceived(const QString &);

private:
    const EventTypeVector m_types;
};

bool EventFilter::eventFilter(QObject *o, QEvent *e)
{
    static int n = 0;
    if (m_types.contains(e->type())) {
        QString message;
        QDebug(&message) << '#' << n++ << ' ' << o->objectName() << ' ' << e;
        emit eventReceived(message);
    }
    return false;
}

class TouchTestWidget : public QWidget {
public:
    explicit TouchTestWidget(QWidget *parent = 0) : QWidget(parent)
    {
        setAttribute(Qt::WA_AcceptTouchEvents);
    }

    bool event(QEvent *event) Q_DECL_OVERRIDE
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            event->accept();
            return true;
        default:
            break;
        }
        return QWidget::event(event);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    QWidget *touchWidget() const { return m_touchWidget; }

public slots:
    void appendToLog(const QString &text) { m_logTextEdit->appendPlainText(text); }
    void dumpTouchDevices();

private:
    QWidget *m_touchWidget;
    QPlainTextEdit *m_logTextEdit;
};

MainWindow::MainWindow()
    : m_touchWidget(new TouchTestWidget)
    , m_logTextEdit(new QPlainTextEdit)
{
    setWindowTitle(QStringLiteral("Touch Event Tester ") + QT_VERSION_STR);

    setObjectName("MainWin");
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *da = fileMenu->addAction(QStringLiteral("Dump devices"));
    da->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(da, SIGNAL(triggered()), this, SLOT(dumpTouchDevices()));
    QAction *qa = fileMenu->addAction(QStringLiteral("Quit"));
    qa->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(qa, SIGNAL(triggered()), this, SLOT(close()));

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical);

    m_touchWidget->setObjectName(QStringLiteral("TouchWidget"));
    const QSize screenSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    m_touchWidget->setMinimumSize(screenSize / 2);
    mainSplitter->addWidget(m_touchWidget);

    m_logTextEdit->setObjectName(QStringLiteral("LogTextEdit"));
    m_logTextEdit->setMinimumHeight(screenSize.height() / 4);
    mainSplitter->addWidget(m_logTextEdit);
    setCentralWidget(mainSplitter);

    dumpTouchDevices();
}

void MainWindow::dumpTouchDevices()
{
    QString message;
    QDebug debug(&message);
    const QList<const QTouchDevice *> devices = QTouchDevice::devices();
    debug << devices.size() << "Device(s):\n";
    for (int i = 0; i < devices.size(); ++i)
        debug << "Device #" << i << devices.at(i) << '\n';
    appendToLog(message);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Touch/Mouse tester"));
    parser.addHelpOption();
    const QCommandLineOption mouseMoveOption(QStringLiteral("mousemove"),
                                             QStringLiteral("Log mouse move events"));
    parser.addOption(mouseMoveOption);
    const QCommandLineOption globalFilterOption(QStringLiteral("global"),
                                             QStringLiteral("Global event filter"));
    parser.addOption(globalFilterOption);
    parser.process(QApplication::arguments());

    MainWindow w;
    w.show();
    const QSize pos = QGuiApplication::primaryScreen()->availableGeometry().size() - w.size();
    w.move(pos.width() / 2, pos.height() / 2);

    EventTypeVector eventTypes;
    eventTypes << QEvent::MouseButtonPress << QEvent::MouseButtonRelease
        << QEvent::MouseButtonDblClick
        << QEvent::TouchBegin << QEvent::TouchUpdate << QEvent::TouchEnd;
    if (parser.isSet(mouseMoveOption))
        eventTypes << QEvent::MouseMove;
    QObject *filterTarget = parser.isSet(globalFilterOption)
            ? static_cast<QObject *>(&a)
            : static_cast<QObject *>(w.touchWidget());
    EventFilter *filter = new EventFilter(eventTypes, filterTarget);
    filterTarget->installEventFilter(filter);
    QObject::connect(filter, SIGNAL(eventReceived(QString)), &w, SLOT(appendToLog(QString)));

    return a.exec();
}

#include "main.moc"
