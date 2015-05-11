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
#include <QToolBar>
#include <QVector>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QPlainTextEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QScreen>
#include <QDebug>
#include <QTextStream>

bool optIgnoreTouch = false;

QDebug operator<<(QDebug debug, const QTouchDevice *d)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QTouchDevice(" << d->name() << ',';
    switch (d->type()) {
    case QTouchDevice::TouchScreen:
        debug << "TouchScreen";
        break;
    case QTouchDevice::TouchPad:
        debug << "TouchPad";
        break;
    }
    debug << ", capabilities=" << d->capabilities()
        << ", maximumTouchPoints=" << d->maximumTouchPoints() << ')';
    return debug;
}

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

enum PointType {
    TouchPoint,
    MousePress,
    MouseRelease
};

struct Point
{
    Point(const QPointF &p = QPoint(), PointType t = TouchPoint,
          Qt::MouseEventSource s = Qt::MouseEventNotSynthesized) : pos(p), type(t), source(s) {}

    QColor color() const;

    QPointF pos;
    PointType type;
    Qt::MouseEventSource source;
};

QColor Point::color() const
{
    Qt::GlobalColor globalColor = Qt::black;
    if (type != TouchPoint) {
        switch (source) {
        case Qt::MouseEventSynthesizedBySystem:
            globalColor = Qt::red;
            break;
        case Qt::MouseEventSynthesizedByQt:
            globalColor = Qt::blue;
            break;
        case Qt::MouseEventNotSynthesized:
            break;
        }
    }
    const QColor result(globalColor);
    return type == MousePress ? result.lighter() : result;
}

class TouchTestWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool drawPoints READ drawPoints WRITE setDrawPoints)
public:
    explicit TouchTestWidget(QWidget *parent = 0) : QWidget(parent), m_drawPoints(true)
    {
        setAttribute(Qt::WA_AcceptTouchEvents);
    }

    bool drawPoints() const { return m_drawPoints; }

public slots:
    void clearPoints();
    void setDrawPoints(bool drawPoints);

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
     QVector<Point> m_points;
     bool m_drawPoints;
};

void TouchTestWidget::clearPoints()
{
    if (!m_points.isEmpty()) {
        m_points.clear();
        update();
    }
}

void TouchTestWidget::setDrawPoints(bool drawPoints)
{
    if (m_drawPoints != drawPoints) {
        clearPoints();
        m_drawPoints = drawPoints;
    }
}

bool TouchTestWidget::event(QEvent *event)
{
    const QEvent::Type type = event->type();
    switch (type) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        if (m_drawPoints) {
            const QMouseEvent *me = static_cast<const QMouseEvent *>(event);
            m_points.append(Point(me->localPos(),
                                  type == QEvent::MouseButtonPress ? MousePress : MouseRelease,
                                  me->source()));
            update();
        }
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
        if (m_drawPoints) {
            foreach (const QTouchEvent::TouchPoint &p, static_cast<const QTouchEvent *>(event)->touchPoints())
                m_points.append(Point(p.pos(), TouchPoint));
            update();
        }
    case QEvent::TouchEnd:
        if (optIgnoreTouch)
            event->ignore();
        else
            event->accept();
        return true;
    default:
        break;
    }
    return QWidget::event(event);
}

void TouchTestWidget::paintEvent(QPaintEvent *)
{
    // Draw touch points as dots, mouse press as light filled circles, mouse release as circles.
    QPainter painter(this);
    const QRectF geom = QRectF(QPointF(0, 0), QSizeF(size()));
    painter.fillRect(geom, Qt::white);
    painter.drawRect(QRectF(geom.topLeft(), geom.bottomRight() - QPointF(1, 1)));
    foreach (const Point &point, m_points) {
        if (geom.contains(point.pos)) {
            QPainterPath painterPath;
            const qreal radius = point.type == TouchPoint ? 1 : 4;
            painterPath.addEllipse(point.pos, radius, radius);
            if (point.type == MouseRelease)
                painter.strokePath(painterPath, QPen(point.color()));
            else
                painter.fillPath(painterPath, point.color());
        }
    }
}

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
    TouchTestWidget *m_touchWidget;
    QPlainTextEdit *m_logTextEdit;
};

MainWindow::MainWindow()
    : m_touchWidget(new TouchTestWidget)
    , m_logTextEdit(new QPlainTextEdit)
{
    setWindowTitle(QStringLiteral("Touch Event Tester ") + QT_VERSION_STR);

    setObjectName("MainWin");
    QToolBar *toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, toolBar);
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *dumpDeviceAction = fileMenu->addAction(QStringLiteral("Dump devices"));
    dumpDeviceAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(dumpDeviceAction, SIGNAL(triggered()), this, SLOT(dumpTouchDevices()));
    toolBar->addAction(dumpDeviceAction);
    QAction *clearLogAction = fileMenu->addAction(QStringLiteral("Clear Log"));
    clearLogAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    connect(clearLogAction, SIGNAL(triggered()), m_logTextEdit, SLOT(clear()));
    toolBar->addAction(clearLogAction);
    QAction *toggleDrawPointAction = fileMenu->addAction(QStringLiteral("Draw Points"));
    toggleDrawPointAction->setCheckable(true);
    toggleDrawPointAction->setChecked(m_touchWidget->drawPoints());
    connect(toggleDrawPointAction, SIGNAL(toggled(bool)), m_touchWidget, SLOT(setDrawPoints(bool)));
    toolBar->addAction(toggleDrawPointAction);
    QAction *clearPointAction = fileMenu->addAction(QStringLiteral("Clear Points"));
    clearPointAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    connect(clearPointAction, SIGNAL(triggered()), m_touchWidget, SLOT(clearPoints()));
    toolBar->addAction(clearPointAction);
    QAction *quitAction = fileMenu->addAction(QStringLiteral("Quit"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    toolBar->addAction(quitAction);

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);

    m_touchWidget->setObjectName(QStringLiteral("TouchWidget"));
    mainSplitter->addWidget(m_touchWidget);

    m_logTextEdit->setObjectName(QStringLiteral("LogTextEdit"));
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

    const QCommandLineOption ignoreTouchOption(QStringLiteral("ignore"),
                                               QStringLiteral("Ignore touch events (for testing mouse emulation)."));
    parser.addOption(ignoreTouchOption);
    parser.process(QApplication::arguments());
    optIgnoreTouch = parser.isSet(ignoreTouchOption);

    MainWindow w;
    const QSize screenSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    w.resize(screenSize / 2);
    const QSize sizeDiff = screenSize - w.size();
    w.move(sizeDiff.width() / 2, sizeDiff.height() / 2);
    w.show();

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
