/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QGesture>
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
#include <QSharedPointer>
#include <QDebug>
#include <QTextStream>

bool optIgnoreTouch = false;
QVector<Qt::GestureType> optGestures;

static inline void drawCircle(const QPointF &center, qreal radius, const QColor &color, QPainter &painter)
{
    const QPen oldPen = painter.pen();
    QPen pen = oldPen;
    pen.setColor(color);
    painter.setPen(pen);
    painter.drawEllipse(center, radius, radius);
    painter.setPen(oldPen);
}

static inline void fillCircle(const QPointF &center, qreal radius, const QColor &color, QPainter &painter)
{
    QPainterPath painterPath;
    painterPath.addEllipse(center, radius, radius);
    painter.fillPath(painterPath, color);
}

// Draws an arrow assuming a mathematical coordinate system, Y axis pointing
// upwards, angle counterclockwise (that is, 45' is pointing up/right).
static void drawArrow(const QPointF &center, qreal length, qreal angleDegrees,
                      const QColor &color, int arrowSize, QPainter &painter)
{
    painter.save();
    painter.translate(center); // Transform center to (0,0) rotate and draw arrow pointing right.
    painter.rotate(-angleDegrees);
    QPen pen = painter.pen();
    pen.setColor(color);
    pen.setWidth(2);
    painter.setPen(pen);
    const QPointF endPoint(length, 0);
    painter.drawLine(QPointF(0, 0), endPoint);
    painter.drawLine(endPoint, endPoint + QPoint(-arrowSize, -arrowSize));
    painter.drawLine(endPoint, endPoint + QPoint(-arrowSize, arrowSize));
    painter.restore();
}

// Hierarchy of classes containing gesture parameters and drawing functionality.
class Gesture {
    Q_DISABLE_COPY(Gesture)
public:
    static Gesture *fromQGesture(const QWidget *w, const  QGesture *source);
    virtual ~Gesture() {}

    virtual void draw(const QRectF &rect, QPainter &painter) const = 0;

protected:
    explicit Gesture(const QWidget *w, const QGesture *source) : m_type(source->gestureType())
        , m_hotSpot(w->mapFromGlobal(source->hotSpot().toPoint()))
        , m_hasHotSpot(source->hasHotSpot()) {}

    QPointF drawHotSpot(const QRectF &rect, QPainter &painter) const
    {
        const QPointF h = m_hasHotSpot ? m_hotSpot : rect.center();
        painter.drawEllipse(h, 15, 15);
        return h;
    }

private:
    Qt::GestureType m_type;
    QPointF m_hotSpot;
    bool m_hasHotSpot;
};

class PanGesture : public Gesture {
public:
    explicit PanGesture(const QWidget *w, const QPanGesture *source) : Gesture(w, source)
        , m_offset(source->offset()) {}

    void draw(const QRectF &rect, QPainter &painter) const Q_DECL_OVERRIDE
    {
        const QPointF hotSpot = drawHotSpot(rect, painter);
        painter.drawLine(hotSpot, hotSpot + m_offset);
    }

private:
    QPointF m_offset;
};

class SwipeGesture : public Gesture {
public:
    explicit SwipeGesture(const QWidget *w, const QSwipeGesture *source) : Gesture(w, source)
        , m_horizontal(source->horizontalDirection()), m_vertical(source->verticalDirection())
        , m_angle(source->swipeAngle()) {}

    void draw(const QRectF &rect, QPainter &painter) const Q_DECL_OVERRIDE;

private:
    QSwipeGesture::SwipeDirection m_horizontal;
    QSwipeGesture::SwipeDirection m_vertical;
    qreal m_angle;
};

static qreal swipeDirectionAngle(QSwipeGesture::SwipeDirection d)
{
    switch (d) {
    case QSwipeGesture::NoDirection:
    case QSwipeGesture::Right:
        break;
    case QSwipeGesture::Left:
        return 180;
    case QSwipeGesture::Up:
        return 90;
    case QSwipeGesture::Down:
        return 270;
    }
    return 0;
}

void SwipeGesture::draw(const QRectF &rect, QPainter &painter) const
{
    enum { arrowLength = 50, arrowHeadSize = 10 };
    const QPointF hotSpot = drawHotSpot(rect, painter);
    drawArrow(hotSpot, arrowLength, swipeDirectionAngle(m_horizontal), Qt::red, arrowHeadSize, painter);
    drawArrow(hotSpot, arrowLength, swipeDirectionAngle(m_vertical), Qt::green, arrowHeadSize, painter);
    drawArrow(hotSpot, arrowLength, m_angle, Qt::blue, arrowHeadSize, painter);
}

Gesture *Gesture::fromQGesture(const QWidget *w, const QGesture *source)
{
    Gesture *result = Q_NULLPTR;
    switch (source->gestureType()) {
    case Qt::TapGesture:
    case Qt::TapAndHoldGesture:
    case Qt::PanGesture:
        result = new PanGesture(w, static_cast<const QPanGesture *>(source));
        break;
    case Qt::PinchGesture:
    case Qt::CustomGesture:
    case Qt::LastGestureType:
        break;
    case Qt::SwipeGesture:
        result = new SwipeGesture(w, static_cast<const QSwipeGesture *>(source));
        break;
    }
    return result;
}

typedef QSharedPointer<Gesture> GesturePtr;
typedef QVector<GesturePtr> GesturePtrs;

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
        QDebug debug(&message);
        debug << '#' << n++ << ' ' << o->objectName() << ' ';
        switch (e->type()) {
        case QEvent::Gesture:
        case QEvent::GestureOverride:
            debug << static_cast<const QGestureEvent *>(e); // Special operator
            break;
        default:
            debug << e;
            break;
        }
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
        case Qt::MouseEventSynthesizedByApplication:
            globalColor = Qt::green;
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
        foreach (Qt::GestureType t, optGestures)
            grabGesture(t);
    }

    bool drawPoints() const { return m_drawPoints; }

public slots:
    void clearPoints();
    void setDrawPoints(bool drawPoints);

signals:
    void logMessage(const QString &);

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    void handleGestureEvent(QGestureEvent *gestureEvent);

    QVector<Point> m_points;
    GesturePtrs m_gestures;
    bool m_drawPoints;
};

void TouchTestWidget::clearPoints()
{
    if (!m_points.isEmpty() || !m_gestures.isEmpty()) {
        m_points.clear();
        m_gestures.clear();
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
    case QEvent::Gesture:
        handleGestureEvent(static_cast<QGestureEvent *>(event));
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void TouchTestWidget::handleGestureEvent(QGestureEvent *gestureEvent)
{
    foreach (QGesture *gesture, gestureEvent->gestures()) {
        if (optGestures.contains(gesture->gestureType())) {
            switch (gesture->state()) {
            case Qt::NoGesture:
                break;
            case Qt::GestureStarted:
            case Qt::GestureUpdated:
                gestureEvent->accept(gesture);
                break;
            case Qt::GestureFinished:
                gestureEvent->accept(gesture);
                if (Gesture *g = Gesture::fromQGesture(this, gesture)) {
                    m_gestures.append(GesturePtr(g));
                    update();
                }
                break;
            case Qt::GestureCanceled:
                emit logMessage(QLatin1String("=== Qt::GestureCanceled ==="));
                break;
            }
        }
    }
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
            const qreal radius = point.type == TouchPoint ? 1 : 4;
            if (point.type == MouseRelease) {
                drawCircle(point.pos, radius, point.color(), painter);
            } else
                fillCircle(point.pos, radius, point.color(), painter);
        }
    }
    foreach (const GesturePtr &gp, m_gestures)
        gp->draw(geom, painter);
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
    connect(m_touchWidget, &TouchTestWidget::logMessage, this, &MainWindow::appendToLog);

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
    const QCommandLineOption noTouchLogOption(QStringLiteral("notouchlog"),
                                              QStringLiteral("Do not log touch events (for testing gestures)."));
    parser.addOption(noTouchLogOption);
    const QCommandLineOption noMouseLogOption(QStringLiteral("nomouselog"),
                                              QStringLiteral("Do not log mouse events (for testing gestures)."));
    parser.addOption(noMouseLogOption);

    const QCommandLineOption tapGestureOption(QStringLiteral("tap"), QStringLiteral("Grab tap gesture."));
    parser.addOption(tapGestureOption);
    const QCommandLineOption tapAndHoldGestureOption(QStringLiteral("tap-and-hold"),
                                                     QStringLiteral("Grab tap-and-hold gesture."));
    parser.addOption(tapAndHoldGestureOption);
    const QCommandLineOption panGestureOption(QStringLiteral("pan"), QStringLiteral("Grab pan gesture."));
    parser.addOption(panGestureOption);
    const QCommandLineOption pinchGestureOption(QStringLiteral("pinch"), QStringLiteral("Grab pinch gesture."));
    parser.addOption(pinchGestureOption);
    const QCommandLineOption swipeGestureOption(QStringLiteral("swipe"), QStringLiteral("Grab swipe gesture."));
    parser.addOption(swipeGestureOption);
    parser.process(QApplication::arguments());
    optIgnoreTouch = parser.isSet(ignoreTouchOption);
    if (parser.isSet(tapGestureOption))
        optGestures.append(Qt::TapGesture);
    if (parser.isSet(tapAndHoldGestureOption))
        optGestures.append(Qt::TapAndHoldGesture);
    if (parser.isSet(panGestureOption))
        optGestures.append(Qt::PanGesture);
    if (parser.isSet(pinchGestureOption))
        optGestures.append(Qt::PinchGesture);
    if (parser.isSet(swipeGestureOption))
        optGestures.append(Qt::SwipeGesture);

    MainWindow w;
    const QSize screenSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    w.resize(screenSize / 2);
    const QSize sizeDiff = screenSize - w.size();
    w.move(sizeDiff.width() / 2, sizeDiff.height() / 2);
    w.show();

    EventTypeVector eventTypes;
    if (!parser.isSet(noMouseLogOption))
        eventTypes << QEvent::MouseButtonPress << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick;
    if (parser.isSet(mouseMoveOption))
        eventTypes << QEvent::MouseMove;
    if (!parser.isSet(noTouchLogOption))
        eventTypes << QEvent::TouchBegin << QEvent::TouchUpdate << QEvent::TouchEnd;
    if (!optGestures.isEmpty())
        eventTypes << QEvent::Gesture << QEvent::GestureOverride;
    QObject *filterTarget = parser.isSet(globalFilterOption)
            ? static_cast<QObject *>(&a)
            : static_cast<QObject *>(w.touchWidget());
    EventFilter *filter = new EventFilter(eventTypes, filterTarget);
    filterTarget->installEventFilter(filter);
    QObject::connect(filter, SIGNAL(eventReceived(QString)), &w, SLOT(appendToLog(QString)));

    return a.exec();
}

#include "main.moc"
