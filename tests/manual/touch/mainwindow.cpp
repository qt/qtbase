// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGestureEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointingDevice>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QWindow>

#ifdef Q_OS_WIN
#  include <QCheckBox>
#  include <QDialogButtonBox>
#  include <QVBoxLayout>
#  include <QtGui/private/qguiapplication_p.h>
#  include <QtGui/qpa/qplatformintegration.h>
#endif

bool optIgnoreTouch = false;
QList<Qt::GestureType> optGestures;
QWidgetList mainWindows;
EventTypeVector eventTypes;
EventFilter *globalEventFilter = nullptr;

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

TouchTestWidget::TouchTestWidget(QWidget *parent) : QWidget(parent), m_drawPoints(true)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    for (Qt::GestureType t : optGestures)
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
    QObject::connect(eventFilter, &EventFilter::eventReceived, result, &MainWindow::appendToLog);

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
    QAction *clearLogAction = fileMenu->addAction(QStringLiteral("Clear Log"));
    clearLogAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(clearLogAction, &QAction::triggered, m_logTextEdit, &QPlainTextEdit::clear);
    toolBar->addAction(clearLogAction);
    QAction *toggleDrawPointAction = fileMenu->addAction(QStringLiteral("Draw Points"));
    toggleDrawPointAction->setCheckable(true);
    toggleDrawPointAction->setChecked(m_touchWidget->drawPoints());
    connect(toggleDrawPointAction, &QAction::toggled, m_touchWidget, &TouchTestWidget::setDrawPoints);
    toolBar->addAction(toggleDrawPointAction);
    QAction *clearPointAction = fileMenu->addAction(QStringLiteral("Clear Points"));
    clearPointAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(clearPointAction, &QAction::triggered, m_touchWidget, &TouchTestWidget::clearPoints);
    toolBar->addAction(clearPointAction);
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
    mainSplitter->addWidget(m_logTextEdit);
    setCentralWidget(mainSplitter);

    statusBar()->addPermanentWidget(m_screenLabel);

    dumpTouchDevices();
}

void MainWindow::appendToLog(const QString &text)
{
    m_logTextEdit->appendPlainText(text);
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
