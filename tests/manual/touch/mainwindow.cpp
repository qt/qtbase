// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QGestureEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointingDevice>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QWindow>

#ifdef Q_OS_WIN
#  include <QCheckBox>
#  include <QDialogButtonBox>
#  include <QVBoxLayout>
#  include <QtGui/private/qguiapplication_p.h>
#  include <QtGui/qpa/qplatformintegration.h>
#endif

#include <optional>

bool optIgnoreTouch = false;
QList<Qt::GestureType> optGestures;
QWidgetList mainWindows;
EventTypeVector eventTypes;
EventFilter *globalEventFilter = nullptr;

namespace {

std::optional<MainWindow::LogBucket> eventBucket(QEvent::Type t)
{
    using LogBucket = MainWindow::LogBucket;
    switch (t) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        return LogBucket::Touch;
    case QEvent::NativeGesture:
        return LogBucket::NativeGesture;
    case QEvent::Gesture:
    case QEvent::GestureOverride:
        return LogBucket::Gesture;
    case QEvent::Wheel:
        return LogBucket::Wheel;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        return LogBucket::Mouse;
    default:
        return std::nullopt;
    }
}

} // namespace

Point::Point(const QPointF &p, PointType t, Qt::MouseEventSource s, QSizeF diameters)
    : pos(p), horizontalDiameter(qMax(2., diameters.width())),
      verticalDiameter(qMax(2., diameters.height())), type(t), source(s)
{
}

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

EventFilter::EventFilter(const EventTypeVector &types, QObject *p)
    : QObject(p), m_types(types)
{
}

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
        // Throttle UI updates to 30 Hz. Messages are joined and flushed in a
        // single appendPlainText to avoid one viewport-scroll repaint per
        // event, which can become a bottleneck under an event storm.
        m_pendingMessages.append(message);
        const std::optional<MainWindow::LogBucket> bucket = eventBucket(e->type());
        Q_ASSERT(bucket.has_value()); // guaranteed by content of m_types
        incrementCount(*bucket);
        if (!m_flushScheduled) {
            m_flushScheduled = true;
            QTimer::singleShot(33, this, &EventFilter::flushPending);
        }
    }
    return false;
}

void EventFilter::flushPending()
{
    m_flushScheduled = false;
    if (!m_pendingMessages.isEmpty()) {
        emit eventReceived(m_pendingMessages.join(QLatin1Char('\n')));
        m_pendingMessages.clear();
    }
    for (auto it = m_counts.cbegin(); it != m_counts.cend(); ++it)
        emit countChanged(it.key(), it.value());
}

void EventFilter::resetCounts()
{
    m_counts.clear();
}

TouchTestWidget::TouchTestWidget(QWidget *parent)
    : QWidget(parent), m_gestureTypes(optGestures), m_drawPoints(true),
      m_acceptTouch(!optIgnoreTouch), m_grabGestures(true)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    for (Qt::GestureType t : std::as_const(m_gestureTypes))
        grabGesture(t);
}

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

void TouchTestWidget::setGrabGestures(bool on)
{
    if (m_grabGestures == on)
        return;
    m_grabGestures = on;
    for (Qt::GestureType t : std::as_const(m_gestureTypes)) {
        if (on)
            grabGesture(t);
        else
            ungrabGesture(t);
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
            m_points.append(Point(me->position(),
                                  type == QEvent::MouseButtonPress ? MousePress : MouseRelease,
                                  me->source()));
            update();
        }
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
        if (m_drawPoints) {
            for (const QEventPoint &p : static_cast<const QPointerEvent *>(event)->points())
                m_points.append(Point(p.position(), TouchPoint, Qt::MouseEventNotSynthesized, p.ellipseDiameters()));
            update();
        }
        Q_FALLTHROUGH();
    case QEvent::TouchEnd:
        if (!m_acceptTouch)
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
    const auto gestures = gestureEvent->gestures();
    for (QGesture *gesture : gestures) {
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
    for (const Point &point : std::as_const(m_points)) {
        if (geom.contains(point.pos)) {
            if (point.type == MouseRelease)
                drawEllipse(point.pos, point.horizontalDiameter, point.verticalDiameter, point.color(), painter);
            else
                fillEllipse(point.pos, point.horizontalDiameter, point.verticalDiameter, point.color(), painter);
        }
    }
    for (const GesturePtr &gp : std::as_const(m_gestures))
        gp->draw(geom, painter);
}

#ifdef Q_OS_WIN
using QWindowsApplication = QNativeInterface::Private::QWindowsApplication;
using TouchWindowTouchType = QWindowsApplication::TouchWindowTouchType;
using TouchWindowTouchTypes = QWindowsApplication::QWindowsApplication::TouchWindowTouchTypes;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Settings");
    auto layout = new QVBoxLayout(this);

    TouchWindowTouchTypes touchTypes;
    if (auto nativeWindowsApp = qGuiApp->nativeInterface<QWindowsApplication>())
        touchTypes = nativeWindowsApp->touchWindowTouchType();

    m_fineCheckBox = new QCheckBox("Fine Touch", this);
    m_fineCheckBox->setChecked(touchTypes.testFlag(TouchWindowTouchType::FineTouch));
    layout->addWidget(m_fineCheckBox);
    connect(m_fineCheckBox, &QAbstractButton::toggled, this, &SettingsDialog::touchTypeToggled);
    m_palmCheckBox = new QCheckBox("Palm Touch", this);
    connect(m_palmCheckBox, &QAbstractButton::toggled, this, &SettingsDialog::touchTypeToggled);
    m_palmCheckBox->setChecked(touchTypes.testFlag(TouchWindowTouchType::WantPalmTouch));
    layout->addWidget(m_palmCheckBox);

    auto box = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(box);
}

void SettingsDialog::touchTypeToggled()
{
    TouchWindowTouchTypes types;
    if (m_fineCheckBox->isChecked())
        types.setFlag(TouchWindowTouchType::FineTouch);
    if (m_palmCheckBox->isChecked())
        types.setFlag(TouchWindowTouchType::WantPalmTouch);
    if (auto nativeWindowsApp = qGuiApp->nativeInterface<QWindowsApplication>())
        nativeWindowsApp->setTouchWindowTouchType(types);
    else
        qWarning("Missing Interface QWindowsApplication");
}
#endif // Q_OS_WIN

MainWindow *MainWindow::createMainWindow()
{
    MainWindow *result = new MainWindow;
    const QSize screenSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    result->resize(screenSize / 2);
    const QSize sizeDiff = screenSize - result->size();
    const QPoint pos = QPoint(sizeDiff.width() / 2, sizeDiff.height() / 2);
    result->move(pos);
    result->show();

    EventFilter *eventFilter = globalEventFilter;
    if (!eventFilter) {
        eventFilter = new EventFilter(eventTypes, result->touchWidget());
        result->touchWidget()->installEventFilter(eventFilter);
    }
    // Note: when --global is used, eventFilter is shared across all MainWindow
    // instances. The per-bucket counters and the Reset Counters action are
    // therefore shared state, so every window's status-bar labels reflect the
    // same global counts and any window can reset them.
    QObject::connect(eventFilter, &EventFilter::eventReceived, result, &MainWindow::appendToLog);
    QObject::connect(eventFilter, &EventFilter::countChanged, result, &MainWindow::updateBucketLabel);
    QObject::connect(result->resetCountersAction(), &QAction::triggered,
                     eventFilter, &EventFilter::resetCounts);
    QObject::connect(result->resetCountersAction(), &QAction::triggered,
                     result, &MainWindow::resetBucketLabels);

    mainWindows.append(result);
    return result;
}

MainWindow::MainWindow()
    : m_touchWidget(new TouchTestWidget)
    , m_logTextEdit(new QPlainTextEdit)
    , m_screenLabel(new QLabel)
{
    QString title;
    QTextStream(&title) << "Touch Event Tester " << QT_VERSION_STR << ' '
        << qApp->platformName() << " #" << (mainWindows.size() + 1);
    setWindowTitle(title);

    setObjectName("MainWin");
    QToolBar *toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, toolBar);
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *newWindowAction = fileMenu->addAction(QStringLiteral("New Window"), this, &MainWindow::newWindow);
    newWindowAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    toolBar->addAction(newWindowAction);
    fileMenu->addSeparator();
    QAction *dumpDeviceAction = fileMenu->addAction(QStringLiteral("Dump devices"));
    dumpDeviceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(dumpDeviceAction, &QAction::triggered, this, &MainWindow::dumpTouchDevices);
    toolBar->addAction(dumpDeviceAction);
    toolBar->addSeparator();
    QAction *clearLogAction = fileMenu->addAction(QStringLiteral("Clear Log"));
    clearLogAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(clearLogAction, &QAction::triggered, m_logTextEdit, &QPlainTextEdit::clear);
    toolBar->addAction(clearLogAction);
    m_resetCountersAction = fileMenu->addAction(QStringLiteral("Reset Counters"));
    m_resetCountersAction->setToolTip(
        QStringLiteral("Reset per-event-type counters in the status bar to zero."));
    toolBar->addAction(m_resetCountersAction);
    QAction *clearPointAction = fileMenu->addAction(QStringLiteral("Clear Points"));
    clearPointAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(clearPointAction, &QAction::triggered, m_touchWidget, &TouchTestWidget::clearPoints);
    toolBar->addAction(clearPointAction);
    toolBar->addSeparator();
    QAction *toggleDrawPointAction = fileMenu->addAction(QStringLiteral("Draw Points"));
    toggleDrawPointAction->setCheckable(true);
    toggleDrawPointAction->setChecked(m_touchWidget->drawPoints());
    connect(toggleDrawPointAction, &QAction::toggled, m_touchWidget, &TouchTestWidget::setDrawPoints);
    toolBar->addAction(toggleDrawPointAction);
    QAction *synthMouseAction = fileMenu->addAction(QStringLiteral("Synthesize Mouse"));
    synthMouseAction->setCheckable(true);
    synthMouseAction->setChecked(
        QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents));
    synthMouseAction->setToolTip(
        QStringLiteral("Toggle Qt::AA_SynthesizeMouseForUnhandledTouchEvents at runtime."));
    connect(synthMouseAction, &QAction::toggled, qApp, [](bool on) {
        QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, on);
    });
    toolBar->addAction(synthMouseAction);
    QAction *acceptTouchAction = fileMenu->addAction(QStringLiteral("Accept Touch"));
    acceptTouchAction->setCheckable(true);
    acceptTouchAction->setChecked(m_touchWidget->acceptTouch());
    acceptTouchAction->setToolTip(
        QStringLiteral("Call QEvent::accept() (on) or QEvent::ignore() (off) on incoming touch events."));
    connect(acceptTouchAction, &QAction::toggled, m_touchWidget, &TouchTestWidget::setAcceptTouch);
    toolBar->addAction(acceptTouchAction);
    QAction *grabGesturesAction = fileMenu->addAction(QStringLiteral("Grab Gestures"));
    grabGesturesAction->setCheckable(true);
    grabGesturesAction->setChecked(m_touchWidget->grabGestures());
    grabGesturesAction->setToolTip(
        QStringLiteral("Call grabGesture() / ungrabGesture() for the active gesture set at runtime."));
    connect(grabGesturesAction, &QAction::toggled, m_touchWidget, &TouchTestWidget::setGrabGestures);
    toolBar->addAction(grabGesturesAction);
    toolBar->addSeparator();
    QAction *quitAction = fileMenu->addAction(QStringLiteral("Quit"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    toolBar->addAction(quitAction);

    auto settingsMenu = menuBar()->addMenu("Settings");
    auto settingsAction = settingsMenu->addAction("Settings",
                                                  this, &MainWindow::settingsDialog);
#ifdef Q_OS_WIN
    Q_UNUSED(settingsAction);
#else
    settingsAction->setEnabled(false);
#endif

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);

    m_touchWidget->setObjectName(QStringLiteral("TouchWidget"));
    mainSplitter->addWidget(m_touchWidget);
    connect(m_touchWidget, &TouchTestWidget::logMessage, this, &MainWindow::appendToLog);

    m_logTextEdit->setObjectName(QStringLiteral("LogTextEdit"));
    m_logTextEdit->setMaximumBlockCount(5000); // cap memory; oldest lines drop off
    mainSplitter->addWidget(m_logTextEdit);
    setCentralWidget(mainSplitter);

    const auto metaLogBucket = QMetaEnum::fromType<LogBucket>();
    m_bucketLabels.reserve(metaLogBucket.keyCount());
    m_bucketPrefixes.reserve(metaLogBucket.keyCount());
    for (int i = 0; i < metaLogBucket.keyCount(); ++i) {
        QString prefix = QLatin1StringView(metaLogBucket.key(i)) + QStringLiteral(": ");
        m_bucketPrefixes.append(prefix);
        auto *label = new QLabel(prefix + QLatin1Char('0'), this);
        m_bucketLabels.append(label);
        statusBar()->addPermanentWidget(label);
    }
    statusBar()->addPermanentWidget(m_screenLabel);

    dumpTouchDevices();
}

QWidget *MainWindow::touchWidget() const
{
    return m_touchWidget;
}

void MainWindow::appendToLog(const QString &text)
{
    m_logTextEdit->appendPlainText(text);
}

void MainWindow::updateBucketLabel(MainWindow::LogBucket bucket, int count)
{
    const int idx = static_cast<int>(bucket);
    if (idx < 0 || idx >= m_bucketLabels.size())
        return;
    m_bucketLabels[idx]->setText(m_bucketPrefixes[idx] + QString::number(count));
}

void MainWindow::resetBucketLabels()
{
    for (qsizetype i = 0; i < m_bucketLabels.size(); ++i)
        m_bucketLabels[i]->setText(m_bucketPrefixes[i] + QStringLiteral("0"));
}

void MainWindow::settingsDialog()
{
#ifdef Q_OS_WIN
    SettingsDialog dialog(this);
    dialog.exec();
#endif
}

void MainWindow::setVisible(bool visible)
{
    QMainWindow::setVisible(visible);
    connect(windowHandle(), &QWindow::screenChanged, this, &MainWindow::updateScreenLabel);
    updateScreenLabel();
}

void MainWindow::updateScreenLabel()
{
    QString text;
    QTextStream str(&text);
    const QScreen *screen = windowHandle()->screen();
    const QRect geometry = screen->geometry();
    const qreal dpr = screen->devicePixelRatio();
    str << '"' << screen->name() << "\" " << geometry.width() << 'x' << geometry.height()
        << Qt::forcesign << geometry.x() << geometry.y() << Qt::noforcesign;
    if (!qFuzzyCompare(dpr, qreal(1)))
        str << ", dpr=" << dpr;
    m_screenLabel->setText(text);
}

void MainWindow::dumpTouchDevices()
{
    QString message;
    QDebug debug(&message);
    const auto devices = QPointingDevice::devices();
    debug << devices.size() << "Device(s):\n";
    for (int i = 0; i < devices.size(); ++i)
        debug << "Device #" << i << devices.at(i) << '\n';
    appendToLog(message);
}
