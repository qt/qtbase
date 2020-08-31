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

#include "../../../shared/highdpi.h"

#include <qboxlayout.h>
#include <qapplication.h>
#include <qbitmap.h>
#include <qdebug.h>
#include <qeventloop.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qstyle.h>
#include <qwidget.h>
#include <qstylefactory.h>
#include <qdesktopwidget.h>
#include <private/qwidget_p.h>
#include <private/qwidgetrepaintmanager_p.h>
#include <private/qapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <qcalendarwidget.h>
#include <qmainwindow.h>
#include <qdockwidget.h>
#include <qrandom.h>
#include <qstylehints.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qbackingstore.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qscreen.h>
#include <qmenubar.h>
#include <qcompleter.h>
#include <qtableview.h>
#include <qtreewidget.h>
#include <qabstractnativeeventfilter.h>
#include <qproxystyle.h>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsProxyWidget>
#include <QtGui/qwindow.h>
#include <qtimer.h>
#include <QtWidgets/QDoubleSpinBox>

#if defined(Q_OS_MACOS)
#include "tst_qwidget_mac_helpers.h"  // Abstract the ObjC stuff out so not everyone must run an ObjC++ compile.
#endif

#include <QtTest/QTest>
#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#  include <QtCore/qt_windows.h>
#  include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>

#include <algorithm>

static HWND winHandleOf(const QWidget *w)
{
    static QPlatformNativeInterface *nativeInterface
            = QGuiApplicationPrivate::platformIntegration()->nativeInterface();
    if (void *handle = nativeInterface->nativeResourceForWindow("handle", w->window()->windowHandle()))
        return reinterpret_cast<HWND>(handle);
    qWarning() << "Cannot obtain native handle for " << w;
    return nullptr;
}

#  define Q_CHECK_PAINTEVENTS \
    if (::SwitchDesktop(::GetThreadDesktop(::GetCurrentThreadId())) == 0) \
        QSKIP("desktop is not visible, this test would fail");

#else // Q_OS_WIN && !Q_OS_WINRT
#  define Q_CHECK_PAINTEVENTS
#endif

#ifdef Q_OS_MACOS
#include <Security/AuthSession.h>
bool macHasAccessToWindowsServer()
{
    SecuritySessionId mySession;
    SessionAttributeBits sessionInfo;
    SessionGetInfo(callerSecuritySession, &mySession, &sessionInfo);
    return (sessionInfo & sessionHasGraphicAccess);
}
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
static inline void setWindowsAnimationsEnabled(bool enabled)
{
    ANIMATIONINFO animation = { sizeof(ANIMATIONINFO), enabled };
    SystemParametersInfo(SPI_SETANIMATION, 0, &animation, 0);
}

static inline bool windowsAnimationsEnabled()
{
    ANIMATIONINFO animation = { sizeof(ANIMATIONINFO), 0 };
    SystemParametersInfo(SPI_GETANIMATION, 0, &animation, 0);
    return animation.iMinAnimate;
}
#else // Q_OS_WIN  && !Q_OS_WINRT
inline void setWindowsAnimationsEnabled(bool) {}
static inline bool windowsAnimationsEnabled() { return false; }
#endif // !Q_OS_WIN || Q_OS_WINRT

template <class T>
static QByteArray msgComparisonFailed(T v1, const char *op, T v2)
{
    QString s;
    QDebug(&s) << v1 << op << v2;
    return s.toLocal8Bit();
}

class tst_QWidget : public QObject
{
    Q_OBJECT

public:
    tst_QWidget();
    virtual ~tst_QWidget();

public slots:
    void initTestCase();
    void cleanup();
private slots:
    void getSetCheck();
    void fontPropagation();
    void fontPropagation2();
    void fontPropagation3();
    void fontPropagationDynamic();
    void palettePropagation();
    void palettePropagation2();
    void palettePropagationDynamic();
    void enabledPropagation();
    void ignoreKeyEventsWhenDisabled_QTBUG27417();
    void properTabHandlingWhenDisabled_QTBUG27417();
#if QT_CONFIG(draganddrop)
    void acceptDropsPropagation();
#endif
    void isEnabledTo();
    void visible();
    void visible_setWindowOpacity();
    void isVisibleTo();
    void isHidden();
    void fonts();
    void mapFromAndTo_data();
    void mapFromAndTo();
    void focusChainOnHide();
    void focusChainOnReparent();
    void defaultTabOrder();
    void reverseTabOrder();
    void tabOrderWithProxy();
    void tabOrderWithCompoundWidgets();
    void tabOrderWithCompoundWidgetsNoFocusPolicy();
    void tabOrderNoChange();
    void tabOrderNoChange2();
    void appFocusWidgetWithFocusProxyLater();
    void appFocusWidgetWhenLosingFocusProxy();
    void explicitTabOrderWithComplexWidget();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void activation();
#endif
    void reparent();
    void windowState();
    void showMaximized();
    void showFullScreen();
    void showMinimized();
    void showMinimizedKeepsFocus();
    void icon();
    void hideWhenFocusWidgetIsChild();
    void normalGeometry();
    void setGeometry();
    void setGeometryHidden();
    void windowOpacity();
    void raise();
    void lower();
    void stackUnder();
    void testContentsPropagation();
    void saveRestoreGeometry();
    void restoreVersion1Geometry_data();
    void restoreVersion1Geometry();

    void widgetAt();
#ifdef Q_OS_MACOS
    void setMask();
#endif
    void optimizedResizeMove();
    void optimizedResize_topLevel();
    void resizeEvent();
    void task110173();

    void testDeletionInEventHandlers();

    void childDeletesItsSibling();

    void setMinimumSize();
    void setMaximumSize();
    void setFixedSize();

    void ensureCreated();
    void createAndDestroy();
    void winIdChangeEvent();
    void persistentWinId();
    void showNativeChild();
    void transientParent();
    void qobject_castInDestroyedSlot();

    void showHideEvent_data();
    void showHideEvent();
    void showHideEventWhileMinimize();
    void showHideChildrenWhileMinimize_QTBUG50589();

    void lostUpdatesOnHide();

    void update();
    void isOpaque();

#ifndef Q_OS_MACOS
    void scroll();
    void scrollNativeChildren();
#endif

    // tests QWidget::setGeometry()
    void setWindowGeometry_data();
    void setWindowGeometry();

    // tests QWidget::move() and resize()
    void windowMoveResize_data();
    void windowMoveResize();

    void moveChild_data();
    void moveChild();
    void showAndMoveChild();

    void subtractOpaqueSiblings();

#if defined (Q_OS_WIN) && !defined(Q_OS_WINRT)
    void setGeometry_win();
#endif

    void setLocale();
    void propagateLocale();
    void deleteStyle();
    void multipleToplevelFocusCheck();
    void setFocus();
#ifndef QT_NO_CURSOR
    void setCursor();
#endif
    void setToolTip();
    void testWindowIconChangeEventPropagation();

    void minAndMaxSizeWithX11BypassWindowManagerHint();
    void showHideShowX11();
    void clean_qt_x11_enforce_cursor();

    void childEvents();
    void render();
    void renderInvisible();
    void renderWithPainter();
    void render_task188133();
    void render_task211796();
    void render_task217815();
    void render_windowOpacity();
    void render_systemClip();
    void render_systemClip2_data();
    void render_systemClip2();
    void render_systemClip3_data();
    void render_systemClip3();
    void render_task252837();
    void render_worldTransform();

    void setContentsMargins();

    void moveWindowInShowEvent_data();
    void moveWindowInShowEvent();

    void repaintWhenChildDeleted();
    void hideOpaqueChildWhileHidden();
    void updateWhileMinimized();
    void alienWidgets();
    void nativeWindowPosition_data();
    void nativeWindowPosition();
    void adjustSize();
    void adjustSize_data();
    void updateGeometry();
    void updateGeometry_data();
    void sendUpdateRequestImmediately();
    void doubleRepaint();
    void resizeInPaintEvent();
    void opaqueChildren();

    void setMaskInResizeEvent();
    void moveInResizeEvent();

#ifdef QT_BUILD_INTERNAL
    void immediateRepaintAfterInvalidateBackingStore();
#endif

    void effectiveWinId();
    void effectiveWinId2();
    void customDpi();
    void customDpiProperty();

    void quitOnCloseAttribute();
    void moveRect();

#if defined (Q_OS_WIN) && !defined(Q_OS_WINRT)
    void gdiPainting();
    void paintOnScreenPossible();
#endif
    void reparentStaticWidget();
    void QTBUG6883_reparentStaticWidget2();

    void translucentWidget();

    void setClearAndResizeMask();
    void maskedUpdate();
#ifndef QT_NO_CURSOR
    void syntheticEnterLeave();
    void taskQTBUG_4055_sendSyntheticEnterLeave();
    void underMouse();
    void taskQTBUG_27643_enterEvents();
#endif
    void windowFlags();
    void initialPosForDontShowOnScreenWidgets();
    void updateOnDestroyedSignal();
    void toplevelLineEditFocus();

    void focusWidget_task254563();
    void rectOutsideCoordinatesLimit_task144779();
    void setGraphicsEffect();

#ifdef QT_BUILD_INTERNAL
    void destroyBackingStore();
#endif

    void activateWindow();

    void openModal_taskQTBUG_5804();

    void focusProxy();
    void focusProxyAndInputMethods();
#ifdef QT_BUILD_INTERNAL
    void scrollWithoutBackingStore();
#endif

    void taskQTBUG_7532_tabOrderWithFocusProxy();
    void movedAndResizedAttributes();
    void childAt();
#ifdef Q_OS_MACOS
    void taskQTBUG_11373();
#endif
    void taskQTBUG_17333_ResizeInfiniteRecursion();

    void nativeChildFocus();
    void grab();
    void grabMouse();
    void grabKeyboard();

    void touchEventSynthesizedMouseEvent();
    void touchUpdateOnNewTouch();
    void touchEventsForGesturePendingWidgets();

    void styleSheetPropagation();

    void destroyedSignal();

    void keyboardModifiers();
    void mouseDoubleClickBubbling_QTBUG29680();
    void largerThanScreen_QTBUG30142();

    void resizeStaticContentsChildWidget_QTBUG35282();

    void qmlSetParentHelper();

    void testForOutsideWSRangeFlag();

    void tabletTracking();

    void closeEvent();
    void closeWithChildWindow();

    void winIdAfterClose();
    void receivesLanguageChangeEvent();
    void deleteWindowInCloseEvent();

private:
    bool ensureScreenSize(int width, int height);

    const QString m_platform;
    QSize m_testWidgetSize;
    QPoint m_availableTopLeft;
    QPoint m_safeCursorPos;
    const bool m_windowsAnimationsEnabled;
    QTouchDevice *m_touchScreen;
    const int m_fuzz;
};

bool tst_QWidget::ensureScreenSize(int width, int height)
{
    const QSize available = QGuiApplication::primaryScreen()->availableGeometry().size();
    return (available.width() >= width && available.height() >= height);
}

// Testing get/set functions
void tst_QWidget::getSetCheck()
{
    QWidget obj1;
    QWidget child1(&obj1);
    // QStyle * QWidget::style()
    // void QWidget::setStyle(QStyle *)
    QScopedPointer<QStyle> var1(QStyleFactory::create(QLatin1String("Windows")));
    obj1.setStyle(var1.data());
    QCOMPARE(static_cast<QStyle *>(var1.data()), obj1.style());
    obj1.setStyle(nullptr);
    QVERIFY(var1.data() != obj1.style());
    QVERIFY(obj1.style() != nullptr); // style can never be 0 for a widget

    // int QWidget::minimumWidth()
    // void QWidget::setMinimumWidth(int)
    obj1.setMinimumWidth(0);
    QCOMPARE(obj1.minimumWidth(), 0);
    obj1.setMinimumWidth(INT_MIN);
    QCOMPARE(obj1.minimumWidth(), 0); // A widgets width can never be less than 0
    obj1.setMinimumWidth(INT_MAX);

    child1.setMinimumWidth(0);
    QCOMPARE(child1.minimumWidth(), 0);
    child1.setMinimumWidth(INT_MIN);
    QCOMPARE(child1.minimumWidth(), 0); // A widgets width can never be less than 0
    child1.setMinimumWidth(INT_MAX);
    QCOMPARE(child1.minimumWidth(), QWIDGETSIZE_MAX); // The largest minimum size should only be as big as the maximium

    // int QWidget::minimumHeight()
    // void QWidget::setMinimumHeight(int)
    obj1.setMinimumHeight(0);
    QCOMPARE(obj1.minimumHeight(), 0);
    obj1.setMinimumHeight(INT_MIN);
    QCOMPARE(obj1.minimumHeight(), 0); // A widgets height can never be less than 0
    obj1.setMinimumHeight(INT_MAX);

    child1.setMinimumHeight(0);
    QCOMPARE(child1.minimumHeight(), 0);
    child1.setMinimumHeight(INT_MIN);
    QCOMPARE(child1.minimumHeight(), 0); // A widgets height can never be less than 0
    child1.setMinimumHeight(INT_MAX);
    QCOMPARE(child1.minimumHeight(), QWIDGETSIZE_MAX); // The largest minimum size should only be as big as the maximium

    // int QWidget::maximumWidth()
    // void QWidget::setMaximumWidth(int)
    obj1.setMaximumWidth(0);
    QCOMPARE(obj1.maximumWidth(), 0);
    obj1.setMaximumWidth(INT_MIN);
    QCOMPARE(obj1.maximumWidth(), 0); // A widgets width can never be less than 0
    obj1.setMaximumWidth(INT_MAX);
    QCOMPARE(obj1.maximumWidth(), QWIDGETSIZE_MAX); // QWIDGETSIZE_MAX is the abs max, not INT_MAX

    // int QWidget::maximumHeight()
    // void QWidget::setMaximumHeight(int)
    obj1.setMaximumHeight(0);
    QCOMPARE(obj1.maximumHeight(), 0);
    obj1.setMaximumHeight(INT_MIN);
    QCOMPARE(obj1.maximumHeight(), 0); // A widgets height can never be less than 0
    obj1.setMaximumHeight(INT_MAX);
    QCOMPARE(obj1.maximumHeight(), QWIDGETSIZE_MAX); // QWIDGETSIZE_MAX is the abs max, not INT_MAX

    // back to normal
    obj1.setMinimumWidth(0);
    obj1.setMinimumHeight(0);
    obj1.setMaximumWidth(QWIDGETSIZE_MAX);
    obj1.setMaximumHeight(QWIDGETSIZE_MAX);

    // const QPalette & QWidget::palette()
    // void QWidget::setPalette(const QPalette &)
    QPalette var6;
    obj1.setPalette(var6);
    QCOMPARE(var6, obj1.palette());
    obj1.setPalette(QPalette());
    QCOMPARE(QPalette(), obj1.palette());

    // const QFont & QWidget::font()
    // void QWidget::setFont(const QFont &)
    QFont var7;
    obj1.setFont(var7);
    QCOMPARE(var7, obj1.font());
    obj1.setFont(QFont());
    QCOMPARE(QFont(), obj1.font());

    // qreal QWidget::windowOpacity()
    // void QWidget::setWindowOpacity(qreal)
    obj1.setWindowOpacity(0.0);
    QCOMPARE(0.0, obj1.windowOpacity());
    obj1.setWindowOpacity(1.1);
    QCOMPARE(1.0, obj1.windowOpacity()); // 1.0 is the fullest opacity possible

    // QWidget * QWidget::focusProxy()
    // void QWidget::setFocusProxy(QWidget *)
    {
        QScopedPointer<QWidget> var9(new QWidget());
        obj1.setFocusProxy(var9.data());
        QCOMPARE(var9.data(), obj1.focusProxy());
        obj1.setFocusProxy(nullptr);
        QCOMPARE(nullptr, obj1.focusProxy());
    }

    // const QRect & QWidget::geometry()
    // void QWidget::setGeometry(const QRect &)
    QCoreApplication::processEvents();
    QRect var10(10, 10, 100, 100);
    obj1.setGeometry(var10);
    QCoreApplication::processEvents();
    qDebug() << obj1.geometry();
    QCOMPARE(var10, obj1.geometry());
    obj1.setGeometry(QRect(0,0,0,0));
    qDebug() << obj1.geometry();
    QCOMPARE(QRect(0,0,0,0), obj1.geometry());

    // QLayout * QWidget::layout()
    // void QWidget::setLayout(QLayout *)
    QBoxLayout *var11 = new QBoxLayout(QBoxLayout::LeftToRight);
    obj1.setLayout(var11);
    QCOMPARE(static_cast<QLayout *>(var11), obj1.layout());
    obj1.setLayout(nullptr);
    QCOMPARE(static_cast<QLayout *>(var11), obj1.layout()); // You cannot set a 0-pointer layout, that keeps the current
    delete var11; // This will remove the layout from the widget
    QCOMPARE(nullptr, obj1.layout());

    // bool QWidget::acceptDrops()
    // void QWidget::setAcceptDrops(bool)
    obj1.setAcceptDrops(false);
    QCOMPARE(false, obj1.acceptDrops());
    obj1.setAcceptDrops(true);
    QCOMPARE(true, obj1.acceptDrops());

    // bool QWidget::autoFillBackground()
    // void QWidget::setAutoFillBackground(bool)
    obj1.setAutoFillBackground(false);
    QCOMPARE(false, obj1.autoFillBackground());
    obj1.setAutoFillBackground(true);
    QCOMPARE(true, obj1.autoFillBackground());

    var1.reset();
#if defined (Q_OS_WIN) && !defined(Q_OS_WINRT)
    obj1.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    const HWND handle = reinterpret_cast<HWND>(obj1.winId());   // explicitly create window handle
    QVERIFY(GetWindowLong(handle, GWL_STYLE) & LONG(WS_POPUP));
#endif
}

tst_QWidget::tst_QWidget()
    : m_platform(QGuiApplication::platformName().toLower())
    , m_safeCursorPos(0, 0)
    , m_windowsAnimationsEnabled(windowsAnimationsEnabled())
    , m_touchScreen(QTest::createTouchDevice())
    , m_fuzz(int(QGuiApplication::primaryScreen()->devicePixelRatio()))
{
    if (m_windowsAnimationsEnabled) // Disable animations which can interfere with screen grabbing in moveChild(), showAndMoveChild()
        setWindowsAnimationsEnabled(false);
    QFont font;
    font.setBold(true);
    font.setPointSize(42);
    QApplication::setFont(font, "QPropagationTestWidget");

    QPalette palette;
    palette.setColor(QPalette::ToolTipBase, QColor(12, 13, 14));
    palette.setColor(QPalette::Text, QColor(21, 22, 23));
    QApplication::setPalette(palette, "QPropagationTestWidget");
}

tst_QWidget::~tst_QWidget()
{
    if (m_windowsAnimationsEnabled)
        setWindowsAnimationsEnabled(m_windowsAnimationsEnabled);
}

void tst_QWidget::initTestCase()
{
    // Size of reference widget, 200 for < 2000, scale up for larger screens
    // to avoid Windows warnings about minimum size for decorated windows.
    int width = 200;
    const QScreen *screen = QGuiApplication::primaryScreen();
    const QRect availableGeometry = screen->availableGeometry();
    m_availableTopLeft = availableGeometry.topLeft();
    // XCB: Determine "safe" cursor position at bottom/right corner of screen.
    // Pushing the mouse rapidly to the top left corner can trigger KDE / KWin's
    // "Present all Windows" (Ctrl+F9) feature also programmatically.
    if (m_platform == QLatin1String("xcb"))
        m_safeCursorPos = availableGeometry.bottomRight() - QPoint(40, 40);
    const int screenWidth = screen->geometry().width();
    if (screenWidth > 2000)
        width = 100 * ((screenWidth + 500) / 1000);
    m_testWidgetSize = QSize(width, width);
}

void tst_QWidget::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QWidget::fontPropagation()
{
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget.data()));
    QFont font = testWidget->font();
    QWidget* childWidget = new QWidget( testWidget.data() );
    childWidget->show();
    QCOMPARE( font, childWidget->font() );

    font.setBold( true );
    testWidget->setFont( font );
    QCOMPARE( font, testWidget->font() );
    QCOMPARE( font, childWidget->font() );

    QFont newFont = font;
    newFont.setItalic( true );
    childWidget->setFont( newFont );
    QWidget* grandChildWidget = new QWidget( childWidget );
    QCOMPARE( font, testWidget->font() );
    QCOMPARE( newFont, grandChildWidget->font() );

    font.setUnderline( true );
    testWidget->setFont( font );

    // the child and grand child should now have merged bold and
    // underline
    newFont.setUnderline( true );

    QCOMPARE( newFont, childWidget->font() );
    QCOMPARE( newFont, grandChildWidget->font() );

    // make sure font propagation continues working after reparenting
    font = testWidget->font();
    font.setPointSize(font.pointSize() + 2);
    testWidget->setFont(font);

    QWidget *one   = new QWidget(testWidget.data());
    QWidget *two   = new QWidget(one);
    QWidget *three = new QWidget(two);
    QWidget *four  = new QWidget(two);

    four->setParent(three);
    four->move(QPoint(0,0));

    font.setPointSize(font.pointSize() + 2);
    testWidget->setFont(font);

    QCOMPARE(testWidget->font(), one->font());
    QCOMPARE(one->font(), two->font());
    QCOMPARE(two->font(), three->font());
    QCOMPARE(three->font(), four->font());

    QVERIFY(testWidget->testAttribute(Qt::WA_SetFont));
    QVERIFY(! one->testAttribute(Qt::WA_SetFont));
    QVERIFY(! two->testAttribute(Qt::WA_SetFont));
    QVERIFY(! three->testAttribute(Qt::WA_SetFont));
    QVERIFY(! four->testAttribute(Qt::WA_SetFont));

    font.setPointSize(font.pointSize() + 2);
    one->setFont(font);

    QCOMPARE(one->font(), two->font());
    QCOMPARE(two->font(), three->font());
    QCOMPARE(three->font(), four->font());

    QVERIFY(one->testAttribute(Qt::WA_SetFont));
    QVERIFY(! two->testAttribute(Qt::WA_SetFont));
    QVERIFY(! three->testAttribute(Qt::WA_SetFont));
    QVERIFY(! four->testAttribute(Qt::WA_SetFont));

    font.setPointSize(font.pointSize() + 2);
    two->setFont(font);

    QCOMPARE(two->font(), three->font());
    QCOMPARE(three->font(), four->font());

    QVERIFY(two->testAttribute(Qt::WA_SetFont));
    QVERIFY(! three->testAttribute(Qt::WA_SetFont));
    QVERIFY(! four->testAttribute(Qt::WA_SetFont));

    font.setPointSize(font.pointSize() + 2);
    three->setFont(font);

    QCOMPARE(three->font(), four->font());

    QVERIFY(three->testAttribute(Qt::WA_SetFont));
    QVERIFY(! four->testAttribute(Qt::WA_SetFont));

    font.setPointSize(font.pointSize() + 2);
    four->setFont(font);

    QVERIFY(four->testAttribute(Qt::WA_SetFont));
}

class QPropagationTestWidget : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;
};

void tst_QWidget::fontPropagation2()
{
    // ! Note, the code below is executed in tst_QWidget's constructor.
    // QFont font;
    // font.setBold(true);
    // font.setPointSize(42);
    // QApplication::setFont(font, "QPropagationTestWidget");

    QScopedPointer<QWidget> root(new QWidget);
    root->setObjectName(QLatin1String("fontPropagation2"));
    root->setWindowTitle(root->objectName());
    root->resize(200, 200);

    QWidget *child0 = new QWidget(root.data());
    QWidget *child1 = new QWidget(child0);
    QWidget *child2 = new QPropagationTestWidget(child1);
    QWidget *child3 = new QWidget(child2);
    QWidget *child4 = new QWidget(child3);
    QWidget *child5 = new QWidget(child4);
    root->show();

    // Check that only the application fonts apply.
    QCOMPARE(root->font(), QApplication::font());
    QCOMPARE(child0->font(), QApplication::font());
    QCOMPARE(child1->font(), QApplication::font());
    QCOMPARE(child2->font().pointSize(), 42);
    QVERIFY(child2->font().bold());
    QCOMPARE(child3->font().pointSize(), 42);
    QVERIFY(child3->font().bold());
    QCOMPARE(child4->font().pointSize(), 42);
    QVERIFY(child4->font().bold());
    QCOMPARE(child5->font().pointSize(), 42);
    QVERIFY(child5->font().bold());

    // Set child0's font size to 15, and remove bold on child4.
    QFont font;
    font.setPointSize(15);
    child0->setFont(font);
    QFont unboldFont;
    unboldFont.setBold(false);
    child4->setFont(unboldFont);

    // Check that the above settings propagate correctly.
    QCOMPARE(root->font(), QApplication::font());
    QCOMPARE(child0->font().pointSize(), 15);
    QVERIFY(!child0->font().bold());
    QCOMPARE(child1->font().pointSize(), 15);
    QVERIFY(!child1->font().bold());
    QCOMPARE(child2->font().pointSize(), 15);
    QVERIFY(child2->font().bold());
    QCOMPARE(child3->font().pointSize(), 15);
    QVERIFY(child3->font().bold());
    QCOMPARE(child4->font().pointSize(), 15);
    QVERIFY(!child4->font().bold());
    QCOMPARE(child5->font().pointSize(), 15);
    QVERIFY(!child5->font().bold());

    // Replace the app font for child2. Italic should propagate
    // but the size should still be ignored. The previous bold
    // setting is gone.
    QFont italicSizeFont;
    italicSizeFont.setItalic(true);
    italicSizeFont.setPointSize(33);
    QApplication::setFont(italicSizeFont, "QPropagationTestWidget");

    // Check that this propagates correctly.
    QCOMPARE(root->font(), QApplication::font());
    QCOMPARE(child0->font().pointSize(), 15);
    QVERIFY(!child0->font().bold());
    QVERIFY(!child0->font().italic());
    QCOMPARE(child1->font().pointSize(), 15);
    QVERIFY(!child1->font().bold());
    QVERIFY(!child1->font().italic());
    QCOMPARE(child2->font().pointSize(), 15);
    QVERIFY(!child2->font().bold());
    QVERIFY(child2->font().italic());
    QCOMPARE(child3->font().pointSize(), 15);
    QVERIFY(!child3->font().bold());
    QVERIFY(child3->font().italic());
    QCOMPARE(child4->font().pointSize(), 15);
    QVERIFY(!child4->font().bold());
    QVERIFY(child4->font().italic());
    QCOMPARE(child5->font().pointSize(), 15);
    QVERIFY(!child5->font().bold());
    QVERIFY(child5->font().italic());
}

void tst_QWidget::fontPropagation3()
{
    QWidget parent;
    QWidget *child = new QWidget(&parent);
    parent.setFont(QFont("Monospace", 9));
    QImage image(32, 32, QImage::Format_RGB32);
    QPainter p(&image);
    p.setFont(child->font());
    QCOMPARE(p.font().family(), child->font().family());
    QCOMPARE(p.font().pointSize(), child->font().pointSize());
}

/*!
    This tests that children that are added to a widget with an explicitly
    defined font inherit that font correctly, merging (and overriding)
    with the font that might be defined by the platform theme.
*/
void tst_QWidget::fontPropagationDynamic()
{
    // override side effects from previous tests
    QFont themedFont;
    themedFont.setBold(true);
    themedFont.setPointSize(42);
    QApplication::setFont(themedFont, "QPropagationTestWidget");

    QWidget parent;
    QWidget firstChild(&parent);

    const QFont defaultFont = parent.font();
    QFont appFont = defaultFont;
    appFont.setPointSize(72);

    // sanity check
    QVERIFY(themedFont != defaultFont);
    QVERIFY(themedFont != appFont);

    // palette propagates to existing children
    parent.setFont(appFont);
    QCOMPARE(firstChild.font().pointSize(), appFont.pointSize());

    // palatte propagates to children added later
    QWidget secondChild(&parent);
    QCOMPARE(secondChild.font().pointSize(), appFont.pointSize());
    QWidget thirdChild;
    QCOMPARE(thirdChild.font().pointSize(), defaultFont.pointSize());
    thirdChild.setParent(&parent);
    QCOMPARE(thirdChild.font().pointSize(), appFont.pointSize());

    // even if the child has an override in QApplication::font
    QPropagationTestWidget themedChild;
    themedChild.ensurePolished(); // needed for default font to be set up
    QCOMPARE(themedChild.font().pointSize(), themedFont.pointSize());
    QCOMPARE(themedChild.font().bold(), themedFont.bold());
    themedChild.setParent(&parent);
    QCOMPARE(themedChild.font().pointSize(), appFont.pointSize());
    QCOMPARE(themedChild.font().bold(), themedFont.bold());

    // grand children as well
    QPropagationTestWidget themedGrandChild;
    themedGrandChild.setParent(&themedChild);
    QCOMPARE(themedGrandChild.font().pointSize(), appFont.pointSize());
    QCOMPARE(themedGrandChild.font().bold(), themedFont.bold());

    // child with own font attribute does not inherit from parent
    QFont childFont = defaultFont;
    childFont.setPointSize(9);
    QWidget modifiedChild;
    modifiedChild.setFont(childFont);
    modifiedChild.setParent(&parent);
    QCOMPARE(modifiedChild.font().pointSize(), childFont.pointSize());
}

void tst_QWidget::palettePropagation()
{
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget.data()));

    QPalette palette = testWidget->palette();
    QWidget* childWidget = new QWidget( testWidget.data() );
    childWidget->show();
    QCOMPARE( palette, childWidget->palette() );

    palette.setColor( QPalette::Base, Qt::red );
    testWidget->setPalette( palette );
    QCOMPARE( palette, testWidget->palette() );
    QCOMPARE( palette, childWidget->palette() );

    QPalette newPalette = palette;
    newPalette.setColor( QPalette::Highlight, Qt::green );
    childWidget->setPalette( newPalette );
    QWidget* grandChildWidget = new QWidget( childWidget );
    QCOMPARE( palette, testWidget->palette() );
    QCOMPARE( newPalette, grandChildWidget->palette() );

    palette.setColor( QPalette::Text, Qt::blue );
    testWidget->setPalette( palette );

    // the child and grand child should now have merged green
    // highlight and blue text
    newPalette.setColor( QPalette::Text, Qt::blue);

    QCOMPARE( newPalette, childWidget->palette() );
    QCOMPARE( newPalette, grandChildWidget->palette() );
}

void tst_QWidget::palettePropagation2()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // ! Note, the code below is executed in tst_QWidget's constructor.
    // QPalette palette;
    // palette.setColor(QPalette::ToolTipBase, QColor(12, 13, 14));
    // palette.setColor(QPalette::Text, QColor(21, 22, 23));
    // qApp->setPalette(palette, "QPropagationTestWidget");

    QScopedPointer<QWidget> root(new QWidget);
    root->setObjectName(QLatin1String("palettePropagation2"));
    root->setWindowTitle(root->objectName());
    root->resize(200, 200);
    QWidget *child0 = new QWidget(root.data());
    QWidget *child1 = new QWidget(child0);
    QWidget *child2 = new QPropagationTestWidget(child1);
    QWidget *child3 = new QWidget(child2);
    QWidget *child4 = new QWidget(child3);
    QWidget *child5 = new QWidget(child4);
    root->show();
    QVERIFY(QTest::qWaitForWindowExposed(root.data()));

    // These colors are unlikely to be imposed on the default palette of
    // QWidget ;-).
    QColor sysPalText(21, 22, 23);
    QColor sysPalToolTipBase(12, 13, 14);
    QColor overridePalText(42, 43, 44);
    QColor overridePalToolTipBase(45, 46, 47);
    QColor sysPalButton(99, 98, 97);

    // Check that only the application fonts apply.
    QPalette appPal = QApplication::palette();
    QCOMPARE(root->palette(), appPal);
    QCOMPARE(child0->palette(), appPal);
    QCOMPARE(child1->palette(), appPal);
    QCOMPARE(child2->palette().color(QPalette::ToolTipBase), sysPalToolTipBase);
    QCOMPARE(child2->palette().color(QPalette::Text), sysPalText);
    QCOMPARE(child2->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child3->palette().color(QPalette::ToolTipBase), sysPalToolTipBase);
    QCOMPARE(child3->palette().color(QPalette::Text), sysPalText);
    QCOMPARE(child3->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child4->palette().color(QPalette::ToolTipBase), sysPalToolTipBase);
    QCOMPARE(child4->palette().color(QPalette::Text), sysPalText);
    QCOMPARE(child4->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child5->palette().color(QPalette::ToolTipBase), sysPalToolTipBase);
    QCOMPARE(child5->palette().color(QPalette::Text), sysPalText);
    QCOMPARE(child5->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));

    // Set child0's Text, and set ToolTipBase on child4.
    QPalette textPalette;
    textPalette.setColor(QPalette::Text, overridePalText);
    child0->setPalette(textPalette);
    QPalette toolTipPalette;
    toolTipPalette.setColor(QPalette::ToolTipBase, overridePalToolTipBase);
    child4->setPalette(toolTipPalette);

    // Check that the above settings propagate correctly.
    QCOMPARE(root->palette(), appPal);
    QCOMPARE(child0->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child0->palette().color(QPalette::ToolTipBase), appPal.color(QPalette::ToolTipBase));
    QCOMPARE(child0->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child1->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child1->palette().color(QPalette::ToolTipBase), appPal.color(QPalette::ToolTipBase));
    QCOMPARE(child1->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child2->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child2->palette().color(QPalette::ToolTipBase), sysPalToolTipBase);
    QCOMPARE(child2->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child3->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child3->palette().color(QPalette::ToolTipBase), sysPalToolTipBase);
    QCOMPARE(child3->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child4->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child4->palette().color(QPalette::ToolTipBase), overridePalToolTipBase);
    QCOMPARE(child4->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child5->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child5->palette().color(QPalette::ToolTipBase), overridePalToolTipBase);
    QCOMPARE(child5->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));

    // Replace the app palette for child2. Button should propagate but Text
    // should still be ignored. The previous ToolTipBase setting is gone.
    QPalette buttonPalette;
    buttonPalette.setColor(QPalette::ToolTipText, sysPalButton);
    QApplication::setPalette(buttonPalette, "QPropagationTestWidget");

    // Check that the above settings propagate correctly.
    QCOMPARE(root->palette(), appPal);
    QCOMPARE(child0->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child0->palette().color(QPalette::ToolTipBase), appPal.color(QPalette::ToolTipBase));
    QCOMPARE(child0->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child1->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child1->palette().color(QPalette::ToolTipBase), appPal.color(QPalette::ToolTipBase));
    QCOMPARE(child1->palette().color(QPalette::ToolTipText), appPal.color(QPalette::ToolTipText));
    QCOMPARE(child2->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child2->palette().color(QPalette::ToolTipBase), appPal.color(QPalette::ToolTipBase));
    QCOMPARE(child2->palette().color(QPalette::ToolTipText), sysPalButton);
    QCOMPARE(child3->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child3->palette().color(QPalette::ToolTipBase), appPal.color(QPalette::ToolTipBase));
    QCOMPARE(child3->palette().color(QPalette::ToolTipText), sysPalButton);
    QCOMPARE(child4->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child4->palette().color(QPalette::ToolTipBase), overridePalToolTipBase);
    QCOMPARE(child4->palette().color(QPalette::ToolTipText), sysPalButton);
    QCOMPARE(child5->palette().color(QPalette::Text), overridePalText);
    QCOMPARE(child5->palette().color(QPalette::ToolTipBase), overridePalToolTipBase);
    QCOMPARE(child5->palette().color(QPalette::ToolTipText), sysPalButton);
}

/*!
    This tests that children that are added to a widget with an explicitly
    defined palette inherit that palette correctly, merging (and overriding)
    with the palette that might be defined by the platform theme.
*/
void tst_QWidget::palettePropagationDynamic()
{
    // override side effects from previous tests
    QPalette themedPalette;
    themedPalette.setColor(QPalette::ToolTipBase, QColor(12, 13, 14));
    themedPalette.setColor(QPalette::Text, QColor(21, 22, 23));
    QApplication::setPalette(themedPalette, "QPropagationTestWidget");

    QWidget parent;
    QWidget firstChild(&parent);

    const QPalette defaultPalette = parent.palette();
    QPalette appPalette = defaultPalette;
    const QColor appColor(1, 2, 3);
    appPalette.setColor(QPalette::Text, appColor);

    // sanity check
    QVERIFY(themedPalette != defaultPalette);
    QVERIFY(themedPalette != appPalette);

    // palette propagates to existing children
    parent.setPalette(appPalette);
    QCOMPARE(firstChild.palette().color(QPalette::Text), appPalette.color(QPalette::Text));

    // palatte propagates to children added later
    QWidget secondChild(&parent);
    QCOMPARE(secondChild.palette().color(QPalette::Text), appPalette.color(QPalette::Text));
    QWidget thirdChild;
    QCOMPARE(thirdChild.palette().color(QPalette::Text), defaultPalette.color(QPalette::Text));
    thirdChild.setParent(&parent);
    QCOMPARE(thirdChild.palette().color(QPalette::Text), appPalette.color(QPalette::Text));

    // even if the child has an override in QApplication::palette
    QPropagationTestWidget themedChild;
    themedChild.ensurePolished(); // needed for default palette to be set up
    QCOMPARE(themedChild.palette().color(QPalette::Text), themedPalette.color(QPalette::Text));
    QCOMPARE(themedChild.palette().color(QPalette::ToolTipBase), themedPalette.color(QPalette::ToolTipBase));
    themedChild.setParent(&parent);
    QCOMPARE(themedChild.palette().color(QPalette::Text), appPalette.color(QPalette::Text));
    QCOMPARE(themedChild.palette().color(QPalette::ToolTipBase), themedPalette.color(QPalette::ToolTipBase));

    // grand children as well
    QPropagationTestWidget themedGrandChild;
    themedGrandChild.setParent(&themedChild);
    QCOMPARE(themedGrandChild.palette().color(QPalette::Text), appPalette.color(QPalette::Text));
    QCOMPARE(themedGrandChild.palette().color(QPalette::ToolTipBase), themedPalette.color(QPalette::ToolTipBase));

    // child with own color does not inherit from parent
    QPalette childPalette = defaultPalette;
    childPalette.setColor(QPalette::Text, Qt::red);
    QWidget modifiedChild;
    modifiedChild.setPalette(childPalette);
    modifiedChild.setParent(&parent);
    QCOMPARE(modifiedChild.palette().color(QPalette::Text), childPalette.color(QPalette::Text));

}

void tst_QWidget::enabledPropagation()
{
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget.data()));
    QWidget* childWidget = new QWidget( testWidget.data() );
    childWidget->show();
    QVERIFY( testWidget->isEnabled() );
    QVERIFY( childWidget->isEnabled() );

    testWidget->setEnabled( false );
    QVERIFY( !testWidget->isEnabled() );
    QVERIFY( !childWidget->isEnabled() );

    testWidget->setDisabled( false );
    QVERIFY( testWidget->isEnabled() );
    QVERIFY( childWidget->isEnabled() );

    QWidget* grandChildWidget = new QWidget( childWidget );
    QVERIFY( grandChildWidget->isEnabled() );

    testWidget->setDisabled( true );
    QVERIFY( !testWidget->isEnabled() );
    QVERIFY( !childWidget->isEnabled() );
    QVERIFY( !grandChildWidget->isEnabled() );

    grandChildWidget->setEnabled( false );
    testWidget->setEnabled( true );
    QVERIFY( testWidget->isEnabled() );
    QVERIFY( childWidget->isEnabled() );
    QVERIFY( !grandChildWidget->isEnabled() );

    grandChildWidget->setEnabled( true );
    testWidget->setEnabled( false );
    childWidget->setDisabled( true );
    testWidget->setEnabled( true );
    QVERIFY( testWidget->isEnabled() );
    QVERIFY( !childWidget->isEnabled() );
    QVERIFY( !grandChildWidget->isEnabled() );
}

void tst_QWidget::ignoreKeyEventsWhenDisabled_QTBUG27417()
{
    QLineEdit lineEdit;
    lineEdit.setWindowTitle(__FUNCTION__);
    lineEdit.setMinimumWidth(m_testWidgetSize.width());
    centerOnScreen(&lineEdit);
    lineEdit.setDisabled(true);
    lineEdit.show();
    QTest::keyClick(&lineEdit, Qt::Key_A);
    QTRY_VERIFY(lineEdit.text().isEmpty());
}

void tst_QWidget::properTabHandlingWhenDisabled_QTBUG27417()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget widget;
    widget.setWindowTitle(__FUNCTION__);
    widget.setMinimumWidth(m_testWidgetSize.width());
    centerOnScreen(&widget);
    QVBoxLayout *layout = new QVBoxLayout();
    QLineEdit *lineEdit = new QLineEdit();
    layout->addWidget(lineEdit);
    QLineEdit *lineEdit2 = new QLineEdit();
    layout->addWidget(lineEdit2);
    QLineEdit *lineEdit3 = new QLineEdit();
    layout->addWidget(lineEdit3);
    widget.setLayout(layout);
    widget.show();

    lineEdit->setFocus();
    QTRY_VERIFY(lineEdit->hasFocus());
    QTest::keyClick(&widget, Qt::Key_Tab);
    QTRY_VERIFY(lineEdit2->hasFocus());
    QTest::keyClick(&widget, Qt::Key_Tab);
    QTRY_VERIFY(lineEdit3->hasFocus());

    lineEdit2->setDisabled(true);
    lineEdit->setFocus();
    QTRY_VERIFY(lineEdit->hasFocus());
    QTest::keyClick(&widget, Qt::Key_Tab);
    QTRY_VERIFY(!lineEdit2->hasFocus());
    QVERIFY(lineEdit3->hasFocus());
}

// Drag'n drop disabled in this build.
#if QT_CONFIG(draganddrop)
void tst_QWidget::acceptDropsPropagation()
{
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget.data()));
    QWidget *childWidget = new QWidget(testWidget.data());
    childWidget->show();
    QVERIFY(!testWidget->acceptDrops());
    QVERIFY(!childWidget->acceptDrops());

    testWidget->setAcceptDrops(true);
    QVERIFY(testWidget->acceptDrops());
    QVERIFY(!childWidget->acceptDrops());
    QVERIFY(childWidget->testAttribute(Qt::WA_DropSiteRegistered));

    testWidget->setAcceptDrops(false);
    QVERIFY(!testWidget->acceptDrops());
    QVERIFY(!childWidget->acceptDrops());
    QVERIFY(!childWidget->testAttribute(Qt::WA_DropSiteRegistered));

    QWidget *grandChildWidget = new QWidget(childWidget);
    QVERIFY(!grandChildWidget->acceptDrops());
    QVERIFY(!grandChildWidget->testAttribute(Qt::WA_DropSiteRegistered));

    testWidget->setAcceptDrops(true);
    QVERIFY(testWidget->acceptDrops());
    QVERIFY(!childWidget->acceptDrops());
    QVERIFY(childWidget->testAttribute(Qt::WA_DropSiteRegistered));
    QVERIFY(!grandChildWidget->acceptDrops());
    QVERIFY(grandChildWidget->testAttribute(Qt::WA_DropSiteRegistered));

    grandChildWidget->setAcceptDrops(true);
    testWidget->setAcceptDrops(false);
    QVERIFY(!testWidget->acceptDrops());
    QVERIFY(!childWidget->acceptDrops());
    QVERIFY(grandChildWidget->acceptDrops());
    QVERIFY(grandChildWidget->testAttribute(Qt::WA_DropSiteRegistered));

    grandChildWidget->setAcceptDrops(false);
    QVERIFY(!grandChildWidget->testAttribute(Qt::WA_DropSiteRegistered));
    testWidget->setAcceptDrops(true);
    childWidget->setAcceptDrops(true);
    testWidget->setAcceptDrops(false);
    QVERIFY(!testWidget->acceptDrops());
    QVERIFY(childWidget->acceptDrops());
    QVERIFY(!grandChildWidget->acceptDrops());
    QVERIFY(grandChildWidget->testAttribute(Qt::WA_DropSiteRegistered));
}
#endif

void tst_QWidget::isEnabledTo()
{
    QWidget testWidget;
    testWidget.resize(m_testWidgetSize);
    testWidget.setWindowTitle(__FUNCTION__);
    centerOnScreen(&testWidget);
    testWidget.show();
    QWidget* childWidget = new QWidget( &testWidget );
    QWidget* grandChildWidget = new QWidget( childWidget );

    QVERIFY( childWidget->isEnabledTo( &testWidget ) );
    QVERIFY( grandChildWidget->isEnabledTo( &testWidget ) );

    childWidget->setEnabled( false );
    QVERIFY( !childWidget->isEnabledTo( &testWidget ) );
    QVERIFY( grandChildWidget->isEnabledTo( childWidget ) );
    QVERIFY( !grandChildWidget->isEnabledTo( &testWidget ) );

    QScopedPointer<QMainWindow> childDialog(new QMainWindow(&testWidget));
    testWidget.setEnabled(false);
    QVERIFY(!childDialog->isEnabled());
    QVERIFY(childDialog->isEnabledTo(nullptr));
}

void tst_QWidget::visible()
{
    // Ensure that the testWidget is hidden for this test at the
    // start
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    QVERIFY( !testWidget->isVisible() );
    QWidget* childWidget = new QWidget( testWidget.data() );
    QVERIFY( !childWidget->isVisible() );

    testWidget->show();
    QVERIFY(testWidget->screen());
    QVERIFY( testWidget->isVisible() );
    QVERIFY( childWidget->isVisible() );

    QWidget* grandChildWidget = new QWidget( childWidget );
    QVERIFY( !grandChildWidget->isVisible() );
    grandChildWidget->show();
    QVERIFY( grandChildWidget->isVisible() );

    grandChildWidget->hide();
    testWidget->hide();
    testWidget->show();
    QVERIFY( !grandChildWidget->isVisible() );
    QVERIFY( testWidget->isVisible() );
    QVERIFY( childWidget->isVisible() );

    grandChildWidget->show();
    childWidget->hide();
    testWidget->hide();
    testWidget->show();
    QVERIFY( testWidget->isVisible() );
    QVERIFY( !childWidget->isVisible() );
    QVERIFY( !grandChildWidget->isVisible() );

    grandChildWidget->show();
    QVERIFY( !grandChildWidget->isVisible() );
}

void tst_QWidget::setLocale()
{
    QWidget w;
    QCOMPARE(w.locale(), QLocale());

    w.setLocale(QLocale::Italian);
    QCOMPARE(w.locale(), QLocale(QLocale::Italian));

    QWidget child1(&w);
    QCOMPARE(child1.locale(), QLocale(QLocale::Italian));

    w.unsetLocale();
    QCOMPARE(w.locale(), QLocale());
    QCOMPARE(child1.locale(), QLocale());

    w.setLocale(QLocale::French);
    QCOMPARE(w.locale(), QLocale(QLocale::French));
    QCOMPARE(child1.locale(), QLocale(QLocale::French));

    child1.setLocale(QLocale::Italian);
    QCOMPARE(w.locale(), QLocale(QLocale::French));
    QCOMPARE(child1.locale(), QLocale(QLocale::Italian));

    child1.unsetLocale();
    QCOMPARE(w.locale(), QLocale(QLocale::French));
    QCOMPARE(child1.locale(), QLocale(QLocale::French));

    QWidget child2;
    QCOMPARE(child2.locale(), QLocale());
    child2.setParent(&w);
    QCOMPARE(child2.locale(), QLocale(QLocale::French));
}

void tst_QWidget::propagateLocale()
{
    QWidget parent;
    parent.setLocale(QLocale::French);
    // Non-window widget; propagates locale:
    QWidget *child = new QWidget(&parent);
    QVERIFY(!child->isWindow());
    QVERIFY(!child->testAttribute(Qt::WA_WindowPropagation));
    QCOMPARE(child->locale(), QLocale(QLocale::French));
    parent.setLocale(QLocale::Italian);
    QCOMPARE(child->locale(), QLocale(QLocale::Italian));
    delete child;
    // Window: doesn't propagate locale:
    child = new QWidget(&parent, Qt::Window);
    QVERIFY(child->isWindow());
    QVERIFY(!child->testAttribute(Qt::WA_WindowPropagation));
    QCOMPARE(child->locale(), QLocale());
    parent.setLocale(QLocale::French);
    QCOMPARE(child->locale(), QLocale());
    // ... unless we tell it to:
    child->setAttribute(Qt::WA_WindowPropagation, true);
    QVERIFY(child->testAttribute(Qt::WA_WindowPropagation));
    QCOMPARE(child->locale(), QLocale(QLocale::French));
    parent.setLocale(QLocale::Italian);
    QCOMPARE(child->locale(), QLocale(QLocale::Italian));
}

void tst_QWidget::visible_setWindowOpacity()
{
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->winId();

    QVERIFY( !testWidget->isVisible() );
    testWidget->setWindowOpacity(0.5);
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    QVERIFY(!::IsWindowVisible(winHandleOf(testWidget.data())));
#endif
    testWidget->setWindowOpacity(1.0);
}

void tst_QWidget::isVisibleTo()
{
    // Ensure that the testWidget is hidden for this test at the
    // start
    QWidget testWidget;
    testWidget.resize(m_testWidgetSize);
    testWidget.setWindowTitle(__FUNCTION__);
    centerOnScreen(&testWidget);

    QWidget* childWidget = new QWidget( &testWidget );
    QVERIFY( childWidget->isVisibleTo( &testWidget ) );
    childWidget->hide();
    QVERIFY( !childWidget->isVisibleTo( &testWidget ) );

    QWidget* grandChildWidget = new QWidget( childWidget );
    QVERIFY( !grandChildWidget->isVisibleTo( &testWidget ) );
    QVERIFY( grandChildWidget->isVisibleTo( childWidget ) );

    testWidget.show();
    childWidget->show();

    QVERIFY( childWidget->isVisibleTo( &testWidget ) );
    grandChildWidget->hide();
    QVERIFY( !grandChildWidget->isVisibleTo( childWidget ) );
    QVERIFY( !grandChildWidget->isVisibleTo( &testWidget ) );
}

void tst_QWidget::isHidden()
{
    // Ensure that the testWidget is hidden for this test at the
    // start
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());

    QVERIFY( testWidget->isHidden() );
    QWidget* childWidget = new QWidget( testWidget.data() );
    QVERIFY( !childWidget->isHidden() );

    testWidget->show();
    QVERIFY( !testWidget->isHidden() );
    QVERIFY( !childWidget->isHidden() );

    QWidget* grandChildWidget = new QWidget( childWidget );
    QVERIFY( grandChildWidget->isHidden() );
    grandChildWidget->show();
    QVERIFY( !grandChildWidget->isHidden() );

    grandChildWidget->hide();
    testWidget->hide();
    testWidget->show();
    QVERIFY( grandChildWidget->isHidden() );
    QVERIFY( !testWidget->isHidden() );
    QVERIFY( !childWidget->isHidden() );

    grandChildWidget->show();
    childWidget->hide();
    testWidget->hide();
    testWidget->show();
    QVERIFY( !testWidget->isHidden() );
    QVERIFY( childWidget->isHidden() );
    QVERIFY( !grandChildWidget->isHidden() );

    grandChildWidget->show();
    QVERIFY( !grandChildWidget->isHidden() );
}

void tst_QWidget::fonts()
{
    QWidget testWidget;
    testWidget.resize(m_testWidgetSize);
    testWidget.setWindowTitle(__FUNCTION__);
    centerOnScreen(&testWidget);
    testWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&testWidget));

    // Tests setFont(), ownFont() and unsetFont()
    QWidget* cleanTestWidget = new QWidget( &testWidget );
    QFont originalFont = cleanTestWidget->font();

    QVERIFY( !cleanTestWidget->testAttribute(Qt::WA_SetFont) );
    cleanTestWidget->setFont(QFont());
    QVERIFY( !cleanTestWidget->testAttribute(Qt::WA_SetFont) );

    QFont newFont( "times", 18 );
    cleanTestWidget->setFont( newFont );
    newFont = newFont.resolve( testWidget.font() );

    QVERIFY( cleanTestWidget->testAttribute(Qt::WA_SetFont) );
    QVERIFY2( cleanTestWidget->font() == newFont,
              msgComparisonFailed(cleanTestWidget->font(), "==", newFont));

    cleanTestWidget->setFont(QFont());
    QVERIFY( !cleanTestWidget->testAttribute(Qt::WA_SetFont) );
    QVERIFY2( cleanTestWidget->font() == originalFont,
              msgComparisonFailed(cleanTestWidget->font(), "==", originalFont));
}

void tst_QWidget::mapFromAndTo_data()
{
    QTest::addColumn<bool>("windowHidden");
    QTest::addColumn<bool>("subWindow1Hidden");
    QTest::addColumn<bool>("subWindow2Hidden");
    QTest::addColumn<bool>("subSubWindowHidden");
    QTest::addColumn<bool>("windowMinimized");
    QTest::addColumn<bool>("subWindow1Minimized");

    QTest::newRow("window 1 sub1 1 sub2 1 subsub 1") << false << false << false << false << false << false;
    QTest::newRow("window 0 sub1 1 sub2 1 subsub 1") << true << false << false << false << false << false;
    QTest::newRow("window 1 sub1 0 sub2 1 subsub 1") << false << true << false << false << false << false;
    QTest::newRow("window 0 sub1 0 sub2 1 subsub 1") << true << true << false << false << false << false;
    QTest::newRow("window 1 sub1 1 sub2 0 subsub 1") << false << false << true << false << false << false;
    QTest::newRow("window 0 sub1 1 sub2 0 subsub 1") << true << false << true << false << false << false;
    QTest::newRow("window 1 sub1 0 sub2 0 subsub 1") << false << true << true << false << false << false;
    QTest::newRow("window 0 sub1 0 sub2 0 subsub 1") << true << true << true << false << false << false;
    QTest::newRow("window 1 sub1 1 sub2 1 subsub 0") << false << false << false << true << false << false;
    QTest::newRow("window 0 sub1 1 sub2 1 subsub 0") << true << false << false << true << false << false;
    QTest::newRow("window 1 sub1 0 sub2 1 subsub 0") << false << true << false << true << false << false;
    QTest::newRow("window 0 sub1 0 sub2 1 subsub 0") << true << true << false << true << false << false;
    QTest::newRow("window 1 sub1 1 sub2 0 subsub 0") << false << false << true << true << false << false;
    QTest::newRow("window 0 sub1 1 sub2 0 subsub 0") << true << false << true << true << false << false;
    QTest::newRow("window 1 sub1 0 sub2 0 subsub 0") << false << true << true << true << false << false;
    QTest::newRow("window 0 sub1 0 sub2 0 subsub 0") << true << true << true << true << false << false;
    QTest::newRow("window 1 sub1 1 sub2 1 subsub 1 windowMinimized") << false << false << false << false << true << false;
    QTest::newRow("window 0 sub1 1 sub2 1 subsub 1 windowMinimized") << true << false << false << false << true << false;
    QTest::newRow("window 1 sub1 0 sub2 1 subsub 1 windowMinimized") << false << true << false << false << true << false;
    QTest::newRow("window 0 sub1 0 sub2 1 subsub 1 windowMinimized") << true << true << false << false << true << false;
    QTest::newRow("window 1 sub1 1 sub2 0 subsub 1 windowMinimized") << false << false << true << false << true << false;
    QTest::newRow("window 0 sub1 1 sub2 0 subsub 1 windowMinimized") << true << false << true << false << true << false;
    QTest::newRow("window 1 sub1 0 sub2 0 subsub 1 windowMinimized") << false << true << true << false << true << false;
    QTest::newRow("window 0 sub1 0 sub2 0 subsub 1 windowMinimized") << true << true << true << false << true << false;
    QTest::newRow("window 1 sub1 1 sub2 1 subsub 0 windowMinimized") << false << false << false << true << true << false;
    QTest::newRow("window 0 sub1 1 sub2 1 subsub 0 windowMinimized") << true << false << false << true << true << false;
    QTest::newRow("window 1 sub1 0 sub2 1 subsub 0 windowMinimized") << false << true << false << true << true << false;
    QTest::newRow("window 0 sub1 0 sub2 1 subsub 0 windowMinimized") << true << true << false << true << true << false;
    QTest::newRow("window 1 sub1 1 sub2 0 subsub 0 windowMinimized") << false << false << true << true << true << false;
    QTest::newRow("window 0 sub1 1 sub2 0 subsub 0 windowMinimized") << true << false << true << true << true << false;
    QTest::newRow("window 1 sub1 0 sub2 0 subsub 0 windowMinimized") << false << true << true << true << true << false;
    QTest::newRow("window 0 sub1 0 sub2 0 subsub 0 windowMinimized") << true << true << true << true << true << false;
    QTest::newRow("window 1 sub1 1 sub2 1 subsub 1 subWindow1Minimized") << false << false << false << false << false << true;
    QTest::newRow("window 0 sub1 1 sub2 1 subsub 1 subWindow1Minimized") << true << false << false << false << false << true;
    QTest::newRow("window 1 sub1 0 sub2 1 subsub 1 subWindow1Minimized") << false << true << false << false << false << true;
    QTest::newRow("window 0 sub1 0 sub2 1 subsub 1 subWindow1Minimized") << true << true << false << false << false << true;
    QTest::newRow("window 1 sub1 1 sub2 0 subsub 1 subWindow1Minimized") << false << false << true << false << false << true;
    QTest::newRow("window 0 sub1 1 sub2 0 subsub 1 subWindow1Minimized") << true << false << true << false << false << true;
    QTest::newRow("window 1 sub1 0 sub2 0 subsub 1 subWindow1Minimized") << false << true << true << false << false << true;
    QTest::newRow("window 0 sub1 0 sub2 0 subsub 1 subWindow1Minimized") << true << true << true << false << false << true;
    QTest::newRow("window 1 sub1 1 sub2 1 subsub 0 subWindow1Minimized") << false << false << false << true << false << true;
    QTest::newRow("window 0 sub1 1 sub2 1 subsub 0 subWindow1Minimized") << true << false << false << true << false << true;
    QTest::newRow("window 1 sub1 0 sub2 1 subsub 0 subWindow1Minimized") << false << true << false << true << false << true;
    QTest::newRow("window 0 sub1 0 sub2 1 subsub 0 subWindow1Minimized") << true << true << false << true << false << true;
    QTest::newRow("window 1 sub1 1 sub2 0 subsub 0 subWindow1Minimized") << false << false << true << true << false << true;
    QTest::newRow("window 0 sub1 1 sub2 0 subsub 0 subWindow1Minimized") << true << false << true << true << false << true;
    QTest::newRow("window 1 sub1 0 sub2 0 subsub 0 subWindow1Minimized") << false << true << true << true << false << true;
    QTest::newRow("window 0 sub1 0 sub2 0 subsub 0 subWindow1Minimized") << true << true << true << true << false << true;


}

void tst_QWidget::mapFromAndTo()
{
    QFETCH(bool, windowHidden);
    QFETCH(bool, subWindow1Hidden);
    QFETCH(bool, subWindow2Hidden);
    QFETCH(bool, subSubWindowHidden);
    QFETCH(bool, windowMinimized);
    QFETCH(bool, subWindow1Minimized);

    // create a toplevel and two overlapping siblings
    QWidget window;
    window.setObjectName(QStringLiteral("mapFromAndTo"));
    window.setWindowTitle(window.objectName());
    window.setWindowFlags(window.windowFlags() | Qt::X11BypassWindowManagerHint);
    QWidget *subWindow1 = new QWidget(&window);
    QWidget *subWindow2 = new QWidget(&window);
    QWidget *subSubWindow = new QWidget(subWindow1);

    // set their geometries
    window.setGeometry(100, 100, 100, 100);
    subWindow1->setGeometry(50, 50, 100, 100);
    subWindow2->setGeometry(75, 75, 100, 100);
    subSubWindow->setGeometry(10, 10, 10, 10);

#if !defined(Q_OS_QNX) && !defined(Q_OS_WINRT)
    //update visibility
    if (windowMinimized) {
        if (!windowHidden) {
            window.showMinimized();
            QVERIFY(window.isMinimized());
        }
    } else {
        window.setVisible(!windowHidden);
    }
    if (subWindow1Minimized) {
        subWindow1->hide();
        subWindow1->showMinimized();
        QVERIFY(subWindow1->isMinimized());
    } else {
        subWindow1->setVisible(!subWindow1Hidden);
    }
#else
    Q_UNUSED(windowHidden);
    Q_UNUSED(subWindow1Hidden);
    Q_UNUSED(windowMinimized);
    Q_UNUSED(subWindow1Minimized);
#endif

    subWindow2->setVisible(!subWindow2Hidden);
    subSubWindow->setVisible(!subSubWindowHidden);

    // window
    QCOMPARE(window.mapToGlobal(QPoint(0, 0)), QPoint(100, 100));
    QCOMPARE(window.mapToGlobal(QPoint(10, 0)), QPoint(110, 100));
    QCOMPARE(window.mapToGlobal(QPoint(0, 10)), QPoint(100, 110));
    QCOMPARE(window.mapToGlobal(QPoint(-10, 0)), QPoint(90, 100));
    QCOMPARE(window.mapToGlobal(QPoint(0, -10)), QPoint(100, 90));
    QCOMPARE(window.mapToGlobal(QPoint(100, 100)), QPoint(200, 200));
    QCOMPARE(window.mapToGlobal(QPoint(110, 100)), QPoint(210, 200));
    QCOMPARE(window.mapToGlobal(QPoint(100, 110)), QPoint(200, 210));
    QCOMPARE(window.mapFromGlobal(QPoint(100, 100)), QPoint(0, 0));
    QCOMPARE(window.mapFromGlobal(QPoint(110, 100)), QPoint(10, 0));
    QCOMPARE(window.mapFromGlobal(QPoint(100, 110)), QPoint(0, 10));
    QCOMPARE(window.mapFromGlobal(QPoint(90, 100)), QPoint(-10, 0));
    QCOMPARE(window.mapFromGlobal(QPoint(100, 90)), QPoint(0, -10));
    QCOMPARE(window.mapFromGlobal(QPoint(200, 200)), QPoint(100, 100));
    QCOMPARE(window.mapFromGlobal(QPoint(210, 200)), QPoint(110, 100));
    QCOMPARE(window.mapFromGlobal(QPoint(200, 210)), QPoint(100, 110));
    QCOMPARE(window.mapToParent(QPoint(0, 0)), QPoint(100, 100));
    QCOMPARE(window.mapToParent(QPoint(10, 0)), QPoint(110, 100));
    QCOMPARE(window.mapToParent(QPoint(0, 10)), QPoint(100, 110));
    QCOMPARE(window.mapToParent(QPoint(-10, 0)), QPoint(90, 100));
    QCOMPARE(window.mapToParent(QPoint(0, -10)), QPoint(100, 90));
    QCOMPARE(window.mapToParent(QPoint(100, 100)), QPoint(200, 200));
    QCOMPARE(window.mapToParent(QPoint(110, 100)), QPoint(210, 200));
    QCOMPARE(window.mapToParent(QPoint(100, 110)), QPoint(200, 210));
    QCOMPARE(window.mapFromParent(QPoint(100, 100)), QPoint(0, 0));
    QCOMPARE(window.mapFromParent(QPoint(110, 100)), QPoint(10, 0));
    QCOMPARE(window.mapFromParent(QPoint(100, 110)), QPoint(0, 10));
    QCOMPARE(window.mapFromParent(QPoint(90, 100)), QPoint(-10, 0));
    QCOMPARE(window.mapFromParent(QPoint(100, 90)), QPoint(0, -10));
    QCOMPARE(window.mapFromParent(QPoint(200, 200)), QPoint(100, 100));
    QCOMPARE(window.mapFromParent(QPoint(210, 200)), QPoint(110, 100));
    QCOMPARE(window.mapFromParent(QPoint(200, 210)), QPoint(100, 110));

    // first subwindow
    QCOMPARE(subWindow1->mapToGlobal(QPoint(0, 0)), QPoint(150, 150));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(10, 0)), QPoint(160, 150));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(0, 10)), QPoint(150, 160));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(-10, 0)), QPoint(140, 150));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(0, -10)), QPoint(150, 140));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(100, 100)), QPoint(250, 250));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(110, 100)), QPoint(260, 250));
    QCOMPARE(subWindow1->mapToGlobal(QPoint(100, 110)), QPoint(250, 260));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(150, 150)), QPoint(0, 0));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(160, 150)), QPoint(10, 0));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(150, 160)), QPoint(0, 10));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(140, 150)), QPoint(-10, 0));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(150, 140)), QPoint(0, -10));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(250, 250)), QPoint(100, 100));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(260, 250)), QPoint(110, 100));
    QCOMPARE(subWindow1->mapFromGlobal(QPoint(250, 260)), QPoint(100, 110));
    QCOMPARE(subWindow1->mapToParent(QPoint(0, 0)), QPoint(50, 50));
    QCOMPARE(subWindow1->mapToParent(QPoint(10, 0)), QPoint(60, 50));
    QCOMPARE(subWindow1->mapToParent(QPoint(0, 10)), QPoint(50, 60));
    QCOMPARE(subWindow1->mapToParent(QPoint(-10, 0)), QPoint(40, 50));
    QCOMPARE(subWindow1->mapToParent(QPoint(0, -10)), QPoint(50, 40));
    QCOMPARE(subWindow1->mapToParent(QPoint(100, 100)), QPoint(150, 150));
    QCOMPARE(subWindow1->mapToParent(QPoint(110, 100)), QPoint(160, 150));
    QCOMPARE(subWindow1->mapToParent(QPoint(100, 110)), QPoint(150, 160));
    QCOMPARE(subWindow1->mapFromParent(QPoint(50, 50)), QPoint(0, 0));
    QCOMPARE(subWindow1->mapFromParent(QPoint(60, 50)), QPoint(10, 0));
    QCOMPARE(subWindow1->mapFromParent(QPoint(50, 60)), QPoint(0, 10));
    QCOMPARE(subWindow1->mapFromParent(QPoint(40, 50)), QPoint(-10, 0));
    QCOMPARE(subWindow1->mapFromParent(QPoint(50, 40)), QPoint(0, -10));
    QCOMPARE(subWindow1->mapFromParent(QPoint(150, 150)), QPoint(100, 100));
    QCOMPARE(subWindow1->mapFromParent(QPoint(160, 150)), QPoint(110, 100));
    QCOMPARE(subWindow1->mapFromParent(QPoint(150, 160)), QPoint(100, 110));

    // subsubwindow
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(0, 0)), QPoint(160, 160));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(10, 0)), QPoint(170, 160));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(0, 10)), QPoint(160, 170));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(-10, 0)), QPoint(150, 160));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(0, -10)), QPoint(160, 150));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(100, 100)), QPoint(260, 260));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(110, 100)), QPoint(270, 260));
    QCOMPARE(subSubWindow->mapToGlobal(QPoint(100, 110)), QPoint(260, 270));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(160, 160)), QPoint(0, 0));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(170, 160)), QPoint(10, 0));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(160, 170)), QPoint(0, 10));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(150, 160)), QPoint(-10, 0));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(160, 150)), QPoint(0, -10));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(260, 260)), QPoint(100, 100));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(270, 260)), QPoint(110, 100));
    QCOMPARE(subSubWindow->mapFromGlobal(QPoint(260, 270)), QPoint(100, 110));
    QCOMPARE(subSubWindow->mapToParent(QPoint(0, 0)), QPoint(10, 10));
    QCOMPARE(subSubWindow->mapToParent(QPoint(10, 0)), QPoint(20, 10));
    QCOMPARE(subSubWindow->mapToParent(QPoint(0, 10)), QPoint(10, 20));
    QCOMPARE(subSubWindow->mapToParent(QPoint(-10, 0)), QPoint(0, 10));
    QCOMPARE(subSubWindow->mapToParent(QPoint(0, -10)), QPoint(10, 0));
    QCOMPARE(subSubWindow->mapToParent(QPoint(100, 100)), QPoint(110, 110));
    QCOMPARE(subSubWindow->mapToParent(QPoint(110, 100)), QPoint(120, 110));
    QCOMPARE(subSubWindow->mapToParent(QPoint(100, 110)), QPoint(110, 120));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(10, 10)), QPoint(0, 0));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(20, 10)), QPoint(10, 0));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(10, 20)), QPoint(0, 10));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(0, 10)), QPoint(-10, 0));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(10, 0)), QPoint(0, -10));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(110, 110)), QPoint(100, 100));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(120, 110)), QPoint(110, 100));
    QCOMPARE(subSubWindow->mapFromParent(QPoint(110, 120)), QPoint(100, 110));
}

void tst_QWidget::focusChainOnReparent()
{
    QWidget window;
    QWidget *child1 = new QWidget(&window);
    QWidget *child2 = new QWidget(&window);
    QWidget *child3 = new QWidget(&window);
    QWidget *child21 = new QWidget(child2);
    QWidget *child22 = new QWidget(child2);
    QWidget *child4 = new QWidget(&window);

    QWidget *expectedOriginalChain[8] = {&window, child1,  child2,  child3,  child21, child22, child4, &window};
    QWidget *w = &window;
    for (auto expectedOriginal : expectedOriginalChain) {
        QCOMPARE(w, expectedOriginal);
        w = w->nextInFocusChain();
    }
    for (int i = 7; i >= 0; --i) {
        w = w->previousInFocusChain();
        QCOMPARE(w, expectedOriginalChain[i]);
    }

    QWidget window2;
    child2->setParent(&window2);

    QWidget *expectedNewChain[5] = {&window2, child2,  child21, child22, &window2};
    w = &window2;
    for (auto expectedNew : expectedNewChain) {
        QCOMPARE(w, expectedNew);
        w = w->nextInFocusChain();
    }
    for (int i = 4; i >= 0; --i) {
        w = w->previousInFocusChain();
        QCOMPARE(w, expectedNewChain[i]);
    }

    QWidget *expectedOldChain[5] = {&window, child1,  child3, child4, &window};
    w = &window;
    for (auto expectedOld : expectedOldChain) {
        QCOMPARE(w, expectedOld);
        w = w->nextInFocusChain();
    }
    for (int i = 4; i >= 0; --i) {
        w = w->previousInFocusChain();
        QCOMPARE(w, expectedOldChain[i]);
    }
}


void tst_QWidget::focusChainOnHide()
{
    // focus should move to the next widget in the focus chain when we hide it.
    QScopedPointer<QWidget> parent(new QWidget());
    parent->setObjectName(QLatin1String("focusChainOnHide"));
    parent->resize(200, 200);
    parent->setWindowTitle(parent->objectName());
    parent->setFocusPolicy(Qt::StrongFocus);
    QWidget *child = new QWidget(parent.data());
    child->setObjectName(QLatin1String("child"));
    child->setFocusPolicy(Qt::StrongFocus);
    QWidget::setTabOrder(child, parent.data());

    parent->show();
    QApplication::setActiveWindow(parent->window());
    child->activateWindow();
    child->setFocus();

    QTRY_VERIFY(child->hasFocus());
    child->hide();

    QTRY_VERIFY(parent->hasFocus());
    QCOMPARE(parent.data(), QApplication::focusWidget());
}

class Container : public QWidget
{
public:
    QVBoxLayout* box;

    Container()
    {
        box = new QVBoxLayout(this);
        //(new QVBoxLayout(this))->setAutoAdd(true);
    }

    void tab()
    {
        focusNextPrevChild(true);
    }

    void backTab()
    {
        focusNextPrevChild(false);
    }
};

class Composite : public QFrame
{
public:
    explicit Composite(QWidget *parent = nullptr, const QString &name = QString())
        : QFrame(parent)
    {
        setObjectName(name);

        lineEdit1 = new QLineEdit;
        lineEdit2 = new QLineEdit;
        lineEdit3 = new QLineEdit;
        lineEdit3->setEnabled(false);

        QHBoxLayout* hbox = new QHBoxLayout(this);
        hbox->addWidget(lineEdit1);
        hbox->addWidget(lineEdit2);
        hbox->addWidget(lineEdit3);
    }

public:
    QLineEdit *lineEdit1;
    QLineEdit *lineEdit2;
    QLineEdit *lineEdit3;
};

void tst_QWidget::defaultTabOrder()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const int compositeCount = 2;
    Container container;
    Composite *composite[compositeCount];

    QLineEdit *firstEdit = new QLineEdit;
    container.box->addWidget(firstEdit);

    for (int i = 0; i < compositeCount; i++) {
        composite[i] = new Composite();
        container.box->addWidget(composite[i]);
    }

    QLineEdit *lastEdit = new QLineEdit();
    container.box->addWidget(lastEdit);

    container.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    container.show();
    container.activateWindow();
    QApplication::setActiveWindow(&container);
    QVERIFY(QTest::qWaitForWindowActive(&container));

    QTRY_VERIFY(firstEdit->hasFocus());

    // Check that focus moves between the line edits when we tab forward
    for (int i = 0; i < compositeCount; ++i) {
        container.tab();
        QVERIFY(composite[i]->lineEdit1->hasFocus());
        QVERIFY(!composite[i]->lineEdit2->hasFocus());
        container.tab();
        QVERIFY(!composite[i]->lineEdit1->hasFocus());
        QVERIFY(composite[i]->lineEdit2->hasFocus());
    }

    container.tab();
    QVERIFY(lastEdit->hasFocus());

    // Check that focus moves between the line edits in reverse
    // order when we tab backwards
    for (int i = compositeCount - 1; i >= 0; --i) {
        container.backTab();
        QVERIFY(!composite[i]->lineEdit1->hasFocus());
        QVERIFY(composite[i]->lineEdit2->hasFocus());

        container.backTab();
        QVERIFY(composite[i]->lineEdit1->hasFocus());
        QVERIFY(!composite[i]->lineEdit2->hasFocus());
    }

    container.backTab();
    QVERIFY(firstEdit->hasFocus());
}

void tst_QWidget::reverseTabOrder()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const int compositeCount = 2;
    Container container;
    container.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    Composite* composite[compositeCount];

    QLineEdit *firstEdit = new QLineEdit();
    container.box->addWidget(firstEdit);

    for (int i = 0; i < compositeCount; i++) {
        composite[i] = new Composite();
        container.box->addWidget(composite[i]);
    }

    QLineEdit *lastEdit = new QLineEdit();
    container.box->addWidget(lastEdit);

    // Reverse tab order inside each composite
    for (int i = 0; i < compositeCount; ++i)
        QWidget::setTabOrder(composite[i]->lineEdit2, composite[i]->lineEdit1);

    container.show();
    container.activateWindow();
    QApplication::setActiveWindow(&container);
    QVERIFY(QTest::qWaitForWindowActive(&container));

    QTRY_VERIFY(firstEdit->hasFocus());

    // Check that focus moves in reverse order when tabbing inside the composites
    // (but in the correct order when tabbing between them)
    for (int i = 0; i < compositeCount; ++i) {
        container.tab();
        QVERIFY(!composite[i]->lineEdit1->hasFocus());
        QVERIFY(composite[i]->lineEdit2->hasFocus());
        container.tab();
        QVERIFY(composite[i]->lineEdit1->hasFocus());
        QVERIFY(!composite[i]->lineEdit2->hasFocus());
    }

    container.tab();
    QVERIFY(lastEdit->hasFocus());

    // Check that focus moves in "normal" order when tabbing backwards inside the
    // composites (since backwards of reversed order cancels each other out),
    // but in the reverse order when tabbing between them.
    for (int i = compositeCount - 1; i >= 0; --i) {
        container.backTab();
        QVERIFY(composite[i]->lineEdit1->hasFocus());
        QVERIFY(!composite[i]->lineEdit2->hasFocus());
        container.backTab();
        QVERIFY(!composite[i]->lineEdit1->hasFocus());
        QVERIFY(composite[i]->lineEdit2->hasFocus());
    }

    container.backTab();
    QVERIFY(firstEdit->hasFocus());
}

void tst_QWidget::tabOrderWithProxy()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const int compositeCount = 2;
    Container container;
    container.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    Composite* composite[compositeCount];

    QLineEdit *firstEdit = new QLineEdit();
    container.box->addWidget(firstEdit);

    for (int i = 0; i < compositeCount; i++) {
        composite[i] = new Composite();
        container.box->addWidget(composite[i]);

        // Set second child as focus proxy
        composite[i]->setFocusPolicy(Qt::StrongFocus);
        composite[i]->setFocusProxy(composite[i]->lineEdit2);
    }

    QLineEdit *lastEdit = new QLineEdit();
    container.box->addWidget(lastEdit);

    container.show();
    container.activateWindow();
    QApplication::setActiveWindow(&container);
    QVERIFY(QTest::qWaitForWindowActive(&container));

    QTRY_VERIFY(firstEdit->hasFocus());

    // Check that focus moves between the second line edits
    // (the focus proxies) when we tab forward
    for (int i = 0; i < compositeCount; ++i) {
        container.tab();
        QVERIFY(!composite[i]->lineEdit1->hasFocus());
        QVERIFY(composite[i]->lineEdit2->hasFocus());
    }

    container.tab();
    QVERIFY(lastEdit->hasFocus());

    // Check that focus moves between the line edits
    // in reverse order when we tab backwards.
    // Note that in this case, the focus proxies should not
    // be taken into consideration, since they only take
    // effect when tabbing forward
    for (int i = compositeCount - 1; i >= 0; --i) {
        container.backTab();
        QVERIFY(!composite[i]->lineEdit1->hasFocus());
        QVERIFY(composite[i]->lineEdit2->hasFocus());
        container.backTab();
        QVERIFY(composite[i]->lineEdit1->hasFocus());
        QVERIFY(!composite[i]->lineEdit2->hasFocus());
    }

    container.backTab();
    QVERIFY(firstEdit->hasFocus());
}

void tst_QWidget::tabOrderWithCompoundWidgets()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const int compositeCount = 4;
    Container container;
    container.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    Composite *composite[compositeCount];

    QLineEdit *firstEdit = new QLineEdit();
    container.box->addWidget(firstEdit);

    for (int i = 0; i < compositeCount; i++) {
        composite[i] = new Composite(nullptr, QStringLiteral("Composite: ") + QString::number(i));
        container.box->addWidget(composite[i]);

        // Let the composite handle focus, and set a child as focus proxy (use the second child, just
        // to ensure that we don't just tab to the first child by coinsidence). This will make the
        // composite "compound". Also enable the last line edit to have a bit more data to check when
        // tabbing forwards.
        composite[i]->setFocusPolicy(Qt::StrongFocus);
        composite[i]->setFocusProxy(composite[i]->lineEdit2);
        composite[i]->lineEdit3->setEnabled(true);
    }

    QLineEdit *lastEdit = new QLineEdit();
    container.box->addWidget(lastEdit);

    // Reverse tab order between each composite
    // (but not inside them), including first and last line edit.
    // The result should not affect local tab order inside each
    // composite, only between them.
    QWidget::setTabOrder(lastEdit, composite[compositeCount - 1]);
    for (int i = compositeCount - 1; i >= 1; --i)
        QWidget::setTabOrder(composite[i], composite[i-1]);
    QWidget::setTabOrder(composite[0], firstEdit);

    container.show();
    container.activateWindow();
    QApplication::setActiveWindow(&container);
    QVERIFY(QTest::qWaitForWindowActive(&container));

    lastEdit->setFocus();
    QTRY_VERIFY(lastEdit->hasFocus());

    // Check that focus moves between the line edits in the normal
    // order when tabbing inside each compound, but in the reverse
    // order when tabbing between them. Since the composites have
    // lineEdit2 as focus proxy, lineEdit2 will be the first with focus
    // when the compound gets focus, and lineEdit1 will therefore be skipped.
    for (int i = compositeCount - 1; i >= 0; --i) {
        container.tab();
        Composite *c = composite[i];
        QVERIFY(!c->lineEdit1->hasFocus());
        QVERIFY(c->lineEdit2->hasFocus());
        QVERIFY(!c->lineEdit3->hasFocus());
        container.tab();
        QVERIFY(!c->lineEdit1->hasFocus());
        QVERIFY(!c->lineEdit2->hasFocus());
        QVERIFY(c->lineEdit3->hasFocus());
    }

    container.tab();
    QVERIFY(firstEdit->hasFocus());

    // Check that focus moves in reverse order when backTab inside the composites, but
    // in the 'correct' order when backTab between them (since the composites are in reverse tab
    // order from before, which cancels it out). Note that when we backtab into a compound, we start
    // at lineEdit3 rather than the focus proxy, since that is the reverse of what happens when we tab
    // forward. And this time we will also backtab to lineEdit1, since there is no focus proxy that interferes.
    for (int i = 0; i < compositeCount; ++i) {
        container.backTab();
        Composite *c = composite[i];
        QVERIFY(!c->lineEdit1->hasFocus());
        QVERIFY(!c->lineEdit2->hasFocus());
        QVERIFY(c->lineEdit3->hasFocus());
        container.backTab();
        QVERIFY(!c->lineEdit1->hasFocus());
        QVERIFY(c->lineEdit2->hasFocus());
        QVERIFY(!c->lineEdit3->hasFocus());
        container.backTab();
        QVERIFY(c->lineEdit1->hasFocus());
        QVERIFY(!c->lineEdit2->hasFocus());
        QVERIFY(!c->lineEdit3->hasFocus());
    }

    container.backTab();
    QVERIFY(lastEdit->hasFocus());
}

static QVector<QWidget*> getFocusChain(QWidget *start, bool bForward)
{
    QVector<QWidget*> ret;
    QWidget *cur = start;
    do {
        ret += cur;
        auto widgetPrivate = static_cast<QWidgetPrivate *>(qt_widget_private(cur));
        cur = bForward ? widgetPrivate->focus_next : widgetPrivate->focus_prev;
    } while (cur != start);
    return ret;
}

//#define DEBUG_FOCUS_CHAIN
static void dumpFocusChain(QWidget *start, bool bForward, const char *desc = nullptr)
{
#ifdef DEBUG_FOCUS_CHAIN
    qDebug() << "Dump focus chain, start:" << start << "isForward:" << bForward << desc;
    QWidget *cur = start;
    do {
        qDebug() << cur;
        auto widgetPrivate = static_cast<QWidgetPrivate *>(qt_widget_private(cur));
        cur = bForward ? widgetPrivate->focus_next : widgetPrivate->focus_prev;
    } while (cur != start);
#else
    Q_UNUSED(start)
    Q_UNUSED(bForward)
    Q_UNUSED(desc)
#endif
}

void tst_QWidget::tabOrderWithCompoundWidgetsNoFocusPolicy()
{
    Container container;
    container.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QSpinBox spinbox1;
    spinbox1.setObjectName("spinbox1");
    QSpinBox spinbox2;
    spinbox2.setObjectName("spinbox2");
    QSpinBox spinbox3;
    spinbox3.setObjectName("spinbox3");

    spinbox1.setFocusPolicy(Qt::StrongFocus);
    spinbox2.setFocusPolicy(Qt::NoFocus);
    spinbox3.setFocusPolicy(Qt::StrongFocus);
    container.box->addWidget(&spinbox1);
    container.box->addWidget(&spinbox2);
    container.box->addWidget(&spinbox3);

    container.show();
    container.activateWindow();

    QApplication::setActiveWindow(&container);
    if (!QTest::qWaitForWindowActive(&container))
        QSKIP("Window failed to activate, skipping test");

    QVERIFY2(spinbox1.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
    container.tab();
    QVERIFY2(!spinbox2.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
    QVERIFY2(spinbox3.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
    container.tab();
    QVERIFY2(spinbox1.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
    container.backTab();
    QVERIFY2(spinbox3.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
    container.backTab();
    QVERIFY2(!spinbox2.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
    QVERIFY2(spinbox1.hasFocus(),
             qPrintable(QApplication::focusWidget()->objectName()));
}

void tst_QWidget::tabOrderNoChange()
{
    QWidget w;
    auto *verticalLayout = new QVBoxLayout(&w);
    auto *tabWidget = new QTabWidget(&w);
    auto *tv = new QTreeView(tabWidget);
    tabWidget->addTab(tv, QStringLiteral("Tab 1"));
    verticalLayout->addWidget(tabWidget);

    const auto focusChainForward = getFocusChain(&w, true);
    const auto focusChainBackward = getFocusChain(&w, false);
    dumpFocusChain(&w, true);
    QWidget::setTabOrder(tabWidget, tv);
    dumpFocusChain(&w, true);
    QCOMPARE(focusChainForward, getFocusChain(&w, true));
    QCOMPARE(focusChainBackward, getFocusChain(&w, false));
}

void tst_QWidget::tabOrderNoChange2()
{
    QWidget w;
    auto *verticalLayout = new QVBoxLayout(&w);
    auto *tabWidget = new QTabWidget(&w);
    tabWidget->setObjectName("tabWidget");
    verticalLayout->addWidget(tabWidget);

    auto *tab1 = new QWidget(tabWidget);
    tab1->setObjectName("tab1");
    auto *vLay1 = new QVBoxLayout(tab1);
    auto *le1 = new QLineEdit(tab1);
    le1->setObjectName("le1");
    auto *le2 = new QLineEdit(tab1);
    le2->setObjectName("le2");
    vLay1->addWidget(le1);
    vLay1->addWidget(le2);
    tabWidget->addTab(tab1, QStringLiteral("Tab 1"));

    auto *tab2 = new QWidget(tabWidget);
    tab2->setObjectName("tab2");
    auto *vLay2 = new QVBoxLayout(tab2);
    auto *le3 = new QLineEdit(tab2);
    le3->setObjectName("le3");
    auto *le4 = new QLineEdit(tab2);
    le4->setObjectName("le4");
    vLay2->addWidget(le3);
    vLay2->addWidget(le4);
    tabWidget->addTab(tab2, QStringLiteral("Tab 2"));

    const auto focusChainForward = getFocusChain(&w, true);
    const auto focusChainBackward = getFocusChain(&w, false);
    dumpFocusChain(&w, true);
    dumpFocusChain(&w, false);
    // this will screw up the focus chain order without visible changes,
    // so don't call it here for the simplicity of the test
    //QWidget::setTabOrder(tabWidget, le1);

    QWidget::setTabOrder(le1, le2);
    dumpFocusChain(&w, true, "QWidget::setTabOrder(le1, le2)");
    QWidget::setTabOrder(le2, le3);
    dumpFocusChain(&w, true, "QWidget::setTabOrder(le2, le3)");
    QWidget::setTabOrder(le3, le4);
    dumpFocusChain(&w, true, "QWidget::setTabOrder(le3, le4)");
    QWidget::setTabOrder(le4, tabWidget);
    dumpFocusChain(&w, true, "QWidget::setTabOrder(le4, tabWidget)");
    dumpFocusChain(&w, false);

    QCOMPARE(focusChainForward, getFocusChain(&w, true));
    QCOMPARE(focusChainBackward, getFocusChain(&w, false));
}

void tst_QWidget::appFocusWidgetWithFocusProxyLater()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // Given a lineedit without a focus proxy
    QWidget window;
    window.setWindowTitle(QTest::currentTestFunction());
    QLineEdit *lineEditFocusProxy = new QLineEdit(&window);
    QLineEdit *lineEdit = new QLineEdit(&window);
    lineEdit->setFocus();
    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QApplication::focusWidget(), lineEdit);

    // When setting a focus proxy for the focus widget (like QWebEngineView does)
    lineEdit->setFocusProxy(lineEditFocusProxy);

    // Then the focus widget should be updated
    QCOMPARE(QApplication::focusWidget(), lineEditFocusProxy);

    // So that deleting the lineEdit and later the window, doesn't crash
    delete lineEdit;
    QCOMPARE(QApplication::focusWidget(), nullptr);
}

void tst_QWidget::appFocusWidgetWhenLosingFocusProxy()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // Given a lineedit with a focus proxy
    QWidget window;
    window.setWindowTitle(QTest::currentTestFunction());
    QLineEdit *lineEditFocusProxy = new QLineEdit(&window);
    QLineEdit *lineEdit = new QLineEdit(&window);
    lineEdit->setFocusProxy(lineEditFocusProxy);
    lineEdit->setFocus();
    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QCOMPARE(QApplication::focusWidget(), lineEditFocusProxy);
    QVERIFY(lineEdit->hasFocus());
    QVERIFY(lineEditFocusProxy->hasFocus());

    // When unsetting the focus proxy
    lineEdit->setFocusProxy(nullptr);

    // then the focus widget should not change
    QCOMPARE(QApplication::focusWidget(), lineEditFocusProxy);
    QVERIFY(!lineEdit->hasFocus());
    QVERIFY(lineEditFocusProxy->hasFocus());
}

void tst_QWidget::explicitTabOrderWithComplexWidget()
{
    // Check that handling tab/backtab with a widget comprimised of other widgets
    // handles tabbing correctly
    Container window;
    auto lineEditOne = new QLineEdit;
    window.box->addWidget(lineEditOne);
    auto lineEditTwo = new QLineEdit;
    window.box->addWidget(lineEditTwo);
    QWidget::setTabOrder(lineEditOne, lineEditTwo);
    lineEditOne->setFocus();
    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QTRY_COMPARE(QApplication::focusWidget(), lineEditOne);

    window.tab();
    QTRY_COMPARE(QApplication::focusWidget(), lineEditTwo);
    window.tab();
    QTRY_COMPARE(QApplication::focusWidget(), lineEditOne);
    window.backTab();
    QTRY_COMPARE(QApplication::focusWidget(), lineEditTwo);
    window.backTab();
    QTRY_COMPARE(QApplication::focusWidget(), lineEditOne);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
void tst_QWidget::activation()
{
    Q_CHECK_PAINTEVENTS

    QWidget widget1;
    widget1.setObjectName("activation-Widget1");
    widget1.setWindowTitle(widget1.objectName());

    QWidget widget2;
    widget1.setObjectName("activation-Widget2");
    widget1.setWindowTitle(widget2.objectName());

    widget1.show();
    widget2.show();

    QTRY_COMPARE(QApplication::activeWindow(), &widget2);
    widget2.showMinimized();

    QTRY_COMPARE(QApplication::activeWindow(), &widget1);
    widget2.showMaximized();
    QTRY_COMPARE(QApplication::activeWindow(), &widget2);
    widget2.showMinimized();
    QTRY_COMPARE(QApplication::activeWindow(), &widget1);
    widget2.showNormal();
    QTRY_COMPARE(QApplication::activeWindow(), &widget2);
    widget2.hide();
    QTRY_COMPARE(QApplication::activeWindow(), &widget1);
}
#endif // Q_OS_WIN

void tst_QWidget::windowState()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    if (m_platform == QStringLiteral("xcb"))
        QSKIP("X11: Many window managers do not support window state properly, which causes this test to fail.");
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");

    QPoint pos;
    QSize size = m_testWidgetSize;
    if (QGuiApplicationPrivate::platformIntegration()->defaultWindowState(Qt::Widget)
                                                       == Qt::WindowFullScreen
        || m_platform == QStringLiteral("winrt")) {
        size = QGuiApplication::primaryScreen()->size();
    } else {
        pos = QPoint(10, 10);
    }

    QWidget widget1;
    widget1.move(pos);
    widget1.resize(size);
    QCOMPARE(widget1.pos(), pos);
    QCOMPARE(widget1.size(), size);
    QTest::qWait(100);
    widget1.setObjectName(QStringLiteral("windowState-Widget1"));
    widget1.setWindowTitle(widget1.objectName());
    QCOMPARE(widget1.pos(), pos);
    QCOMPARE(widget1.size(), size);

#define VERIFY_STATE(s)                                                                            \
    QCOMPARE(int(widget1.windowState() & stateMask), int(s));                                      \
    QCOMPARE(int(widget1.windowHandle()->windowStates() & stateMask), int(s))

    const auto stateMask = Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowFullScreen;

    widget1.setWindowState(Qt::WindowMaximized);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowMaximized);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMaximized);

    widget1.setVisible(true);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowMaximized);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMaximized);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMaximized);
    QTest::qWait(100);
    QVERIFY(!(widget1.windowState() & Qt::WindowMaximized));
    QTRY_VERIFY2(HighDpi::fuzzyCompare(widget1.pos(), pos, m_fuzz),
                 qPrintable(HighDpi::msgPointMismatch(widget1.pos(), pos)));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowNoState);

    widget1.setWindowState(Qt::WindowMinimized);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowMinimized);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMinimized);

    widget1.setWindowState(widget1.windowState() | Qt::WindowMaximized);
    QTest::qWait(100);
    VERIFY_STATE((Qt::WindowMinimized|Qt::WindowMaximized));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMinimized);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowMaximized);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMaximized);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMaximized);
    QTest::qWait(100);
    QVERIFY(!(widget1.windowState() & (Qt::WindowMinimized|Qt::WindowMaximized)));
    QTRY_VERIFY2(HighDpi::fuzzyCompare(widget1.pos(), pos, m_fuzz),
                 qPrintable(HighDpi::msgPointMismatch(widget1.pos(), pos)));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowNoState);

    widget1.setWindowState(Qt::WindowFullScreen);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowFullScreen);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowFullScreen);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(100);
    VERIFY_STATE((Qt::WindowFullScreen|Qt::WindowMinimized));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMinimized);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowFullScreen);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowFullScreen);

    widget1.setWindowState(Qt::WindowNoState);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowNoState);
    QTRY_VERIFY2(HighDpi::fuzzyCompare(widget1.pos(), pos, m_fuzz),
                 qPrintable(HighDpi::msgPointMismatch(widget1.pos(), pos)));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowNoState);

    widget1.setWindowState(Qt::WindowFullScreen);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowFullScreen);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowFullScreen);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMaximized);
    QTest::qWait(100);
    VERIFY_STATE((Qt::WindowFullScreen|Qt::WindowMaximized));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowFullScreen);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(100);
    VERIFY_STATE((Qt::WindowFullScreen|Qt::WindowMaximized|Qt::WindowMinimized));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMinimized);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(100);
    VERIFY_STATE((Qt::WindowFullScreen|Qt::WindowMaximized));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowFullScreen);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowFullScreen);
    QTest::qWait(100);
    VERIFY_STATE(Qt::WindowMaximized);
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowMaximized);

    widget1.setWindowState(widget1.windowState() ^ Qt::WindowMaximized);
    QTest::qWait(100);
    QVERIFY(!(widget1.windowState() & stateMask));
    QCOMPARE(widget1.windowHandle()->windowState(), Qt::WindowNoState);

    QTRY_VERIFY2(HighDpi::fuzzyCompare(widget1.pos(), pos, m_fuzz),
                 qPrintable(HighDpi::msgPointMismatch(widget1.pos(), pos)));
    QTRY_COMPARE(widget1.size(), size);
}

void tst_QWidget::showMaximized()
{
    QWidget plain;
    QHBoxLayout *layout;
    layout = new QHBoxLayout;
    QWidget layouted;
    QLineEdit le;
    QLineEdit le2;
    QLineEdit le3;

    layout->addWidget(&le);
    layout->addWidget(&le2);
    layout->addWidget(&le3);

    layouted.setLayout(layout);

    plain.showMaximized();
    QVERIFY(plain.windowState() & Qt::WindowMaximized);

    plain.showNormal();
    QVERIFY(!(plain.windowState() & Qt::WindowMaximized));

    layouted.showMaximized();
    QVERIFY(layouted.windowState() & Qt::WindowMaximized);

    layouted.showNormal();
    QVERIFY(!(layouted.windowState() & Qt::WindowMaximized));

    // ### fixme: embedded may choose a different size to fit on the screen.
    if (layouted.size() != layouted.sizeHint())
        QEXPECT_FAIL("", "QTBUG-22326", Continue);
    QCOMPARE(layouted.size(), layouted.sizeHint());

    layouted.showMaximized();
    QVERIFY(layouted.isMaximized());
    QVERIFY(layouted.isVisible());

    layouted.hide();
    QVERIFY(layouted.isMaximized());
    QVERIFY(!layouted.isVisible());

    layouted.showMaximized();
    QVERIFY(layouted.isMaximized());
    QVERIFY(layouted.isVisible());

    layouted.showMinimized();
    QVERIFY(layouted.isMinimized());
    QVERIFY(layouted.isMaximized());

    layouted.showMaximized();
    QVERIFY(!layouted.isMinimized());
    QVERIFY(layouted.isMaximized());
    QVERIFY(layouted.isVisible());

    layouted.showMinimized();
    QVERIFY(layouted.isMinimized());
    QVERIFY(layouted.isMaximized());

    layouted.showMaximized();
    QVERIFY(!layouted.isMinimized());
    QVERIFY(layouted.isMaximized());
    QVERIFY(layouted.isVisible());

    {
        QWidget frame;
        QWidget widget(&frame);
        widget.showMaximized();
        QVERIFY(widget.isMaximized());
    }

    {
        QWidget widget;
        setFrameless(&widget);
        widget.setGeometry(0, 0, 10, 10);
        widget.showMaximized();
        QTRY_VERIFY(widget.size().width() > 20 && widget.size().height() > 20);
    }
}

void tst_QWidget::showFullScreen()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT: This fails. QTBUG-68297");
    QWidget plain;
    QHBoxLayout *layout;
    QWidget layouted;
    QLineEdit le;
    QLineEdit le2;
    QLineEdit le3;
    layout = new QHBoxLayout;

    layout->addWidget(&le);
    layout->addWidget(&le2);
    layout->addWidget(&le3);

    layouted.setLayout(layout);

    plain.showFullScreen();
    QVERIFY(plain.windowState() & Qt::WindowFullScreen);
    QVERIFY(plain.windowHandle());
    QVERIFY(plain.windowHandle()->screen());
    const QRect expectedFullScreenGeometry = plain.windowHandle()->screen()->geometry();
    QTRY_COMPARE(plain.geometry(), expectedFullScreenGeometry);

    plain.showNormal();
    QVERIFY(!(plain.windowState() & Qt::WindowFullScreen));

    layouted.showFullScreen();
    QVERIFY(layouted.windowState() & Qt::WindowFullScreen);
    QTRY_COMPARE(layouted.geometry(), expectedFullScreenGeometry);

    layouted.showNormal();
    QVERIFY(!(layouted.windowState() & Qt::WindowFullScreen));

    // ### fixme: embedded may choose a different size to fit on the screen.
    if (layouted.size() != layouted.sizeHint())
        QEXPECT_FAIL("", "QTBUG-22326", Continue);
    QCOMPARE(layouted.size(), layouted.sizeHint());

    layouted.showFullScreen();
    QVERIFY(layouted.isFullScreen());
    QVERIFY(layouted.isVisible());
    QTRY_COMPARE(layouted.geometry(), expectedFullScreenGeometry);

    layouted.hide();
    QVERIFY(layouted.isFullScreen());
    QVERIFY(!layouted.isVisible());

    layouted.showFullScreen();
    QVERIFY(layouted.isFullScreen());
    QVERIFY(layouted.isVisible());
    QTRY_COMPARE(layouted.geometry(), expectedFullScreenGeometry);

    layouted.showMinimized();
    QVERIFY(layouted.isMinimized());
    QVERIFY(layouted.isFullScreen());

    layouted.showFullScreen();
    QVERIFY(!layouted.isMinimized());
    QVERIFY(layouted.isFullScreen());
    QVERIFY(layouted.isVisible());
    QTRY_COMPARE(layouted.geometry(), expectedFullScreenGeometry);

    layouted.showMinimized();
    QVERIFY(layouted.isMinimized());
    QVERIFY(layouted.isFullScreen());

    layouted.showFullScreen();
    QVERIFY(!layouted.isMinimized());
    QVERIFY(layouted.isFullScreen());
    QVERIFY(layouted.isVisible());
    QTRY_COMPARE(layouted.geometry(), expectedFullScreenGeometry);

    {
        QWidget frame;
        QWidget widget(&frame);
        widget.showFullScreen();
        QVERIFY(widget.isFullScreen());
        QTRY_COMPARE(layouted.geometry(), expectedFullScreenGeometry);
    }
}

class ResizeWidget : public QWidget {
public:
    explicit ResizeWidget(QWidget *p = nullptr) : QWidget(p)
    {
        setObjectName(QLatin1String("ResizeWidget"));
        setWindowTitle(objectName());
    }
protected:
    void resizeEvent(QResizeEvent *e) override
    {
        QCOMPARE(size(), e->size());
        ++m_resizeEventCount;
    }

public:
    int m_resizeEventCount = 0;
};

void tst_QWidget::resizeEvent()
{
    {
        QWidget wParent;
        wParent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        wParent.resize(m_testWidgetSize);
        ResizeWidget wChild(&wParent);
        wParent.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wParent));
        QCOMPARE (wChild.m_resizeEventCount, 1); // initial resize event before paint
        wParent.hide();
        QSize safeSize(640,480);
        if (wChild.size() == safeSize)
            safeSize.setWidth(639);
        wChild.resize(safeSize);
        QCOMPARE (wChild.m_resizeEventCount, 1);
        wParent.show();
        QCOMPARE (wChild.m_resizeEventCount, 2);
    }

    {
        ResizeWidget wTopLevel;
        wTopLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        wTopLevel.resize(m_testWidgetSize);
        wTopLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wTopLevel));
        if (m_platform == QStringLiteral("winrt"))
            QEXPECT_FAIL("", "WinRT does not support resize", Abort);
        QCOMPARE (wTopLevel.m_resizeEventCount, 1); // initial resize event before paint for toplevels
        wTopLevel.hide();
        QSize safeSize(640,480);
        if (wTopLevel.size() == safeSize)
            safeSize.setWidth(639);
        wTopLevel.resize(safeSize);
        QCOMPARE (wTopLevel.m_resizeEventCount, 1);
        wTopLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wTopLevel));
        QCOMPARE (wTopLevel.m_resizeEventCount, 2);
    }
}

void tst_QWidget::showMinimized()
{
    if (m_platform == QStringLiteral("wayland")) {
        QSKIP("Wayland: Neither xdg_shell, wl_shell or ivi_application support "
              "letting a client know whether it's minimized. So on these shells "
              "Qt Wayland will always report that it's unmimized.");
    }

    QWidget plain;
    plain.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    plain.move(100, 100);
    plain.resize(200, 200);
    QPoint pos = plain.pos();

    plain.showMinimized();
    QVERIFY(plain.isMinimized());
    QVERIFY(plain.isVisible());
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Winrt does not support move and resize", Abort);
#endif
    QVERIFY2(HighDpi::fuzzyCompare(plain.pos(), pos, m_fuzz),
             qPrintable(HighDpi::msgPointMismatch(plain.pos(), pos)));

    plain.showNormal();
    QVERIFY(!plain.isMinimized());
    QVERIFY(plain.isVisible());
    QVERIFY2(HighDpi::fuzzyCompare(plain.pos(), pos, m_fuzz),
             qPrintable(HighDpi::msgPointMismatch(plain.pos(), pos)));

    plain.showMinimized();
    QVERIFY(plain.isMinimized());
    QVERIFY(plain.isVisible());
    QVERIFY2(HighDpi::fuzzyCompare(plain.pos(), pos, m_fuzz),
             qPrintable(HighDpi::msgPointMismatch(plain.pos(), pos)));

    plain.hide();
    QVERIFY(plain.isMinimized());
    QVERIFY(!plain.isVisible());

    plain.showMinimized();
    QVERIFY(plain.isMinimized());
    QVERIFY(plain.isVisible());

    plain.setGeometry(200, 200, 300, 300);
    plain.showNormal();
    QCOMPARE(plain.geometry(), QRect(200, 200, 300, 300));

    {
        QWidget frame;
        QWidget widget(&frame);
        widget.showMinimized();
        QVERIFY(widget.isMinimized());
    }
}

void tst_QWidget::showMinimizedKeepsFocus()
{
    if (m_platform == QStringLiteral("xcb"))
        QSKIP("QTBUG-26424");
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported.");
    if (m_platform == QStringLiteral("offscreen"))
        QSKIP("Platform offscreen does not support showMinimized()");

    //here we test that minimizing a widget and restoring it doesn't change the focus inside of it
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget child1(&window), child2(&window);
        child1.setFocusPolicy(Qt::StrongFocus);
        child2.setFocusPolicy(Qt::StrongFocus);
        window.show();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
        child2.setFocus();

        QTRY_COMPARE(window.focusWidget(), &child2);
        QTRY_COMPARE(QApplication::focusWidget(), &child2);

        window.showMinimized();
        QTRY_VERIFY(window.isMinimized());
        QTRY_COMPARE(window.focusWidget(), &child2);

        window.showNormal();
        QTest::qWait(10);
        QTRY_COMPARE(window.focusWidget(), &child2);
    }

    //testing deletion of the focusWidget
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget *child = new QWidget(&window);
        child->setFocusPolicy(Qt::StrongFocus);
        window.show();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
        child->setFocus();
        QTRY_COMPARE(window.focusWidget(), child);
        QTRY_COMPARE(QApplication::focusWidget(), child);

        delete child;
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);
    }

    //testing reparenting the focus widget
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget *child = new QWidget(&window);
        child->setFocusPolicy(Qt::StrongFocus);
        window.show();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
        child->setFocus();
        QTRY_COMPARE(window.focusWidget(), child);
        QTRY_COMPARE(QApplication::focusWidget(), child);

        child->setParent(nullptr);
        QScopedPointer<QWidget> childGuard(child);
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);
    }

    //testing setEnabled(false)
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget *child = new QWidget(&window);
        child->setFocusPolicy(Qt::StrongFocus);
        window.show();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
        child->setFocus();
        QTRY_COMPARE(window.focusWidget(), child);
        QTRY_COMPARE(QApplication::focusWidget(), child);

        child->setEnabled(false);
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);
    }

    //testing clearFocus
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget *firstchild = new QWidget(&window);
        firstchild->setFocusPolicy(Qt::StrongFocus);
        QWidget *child = new QWidget(&window);
        child->setFocusPolicy(Qt::StrongFocus);
        window.show();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
        child->setFocus();
        QTRY_COMPARE(window.focusWidget(), child);
        QTRY_COMPARE(QApplication::focusWidget(), child);

        child->clearFocus();
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        window.showMinimized();
        QTest::qWait(30);
        QTRY_VERIFY(window.isMinimized());
        QCOMPARE(window.focusWidget(), nullptr);
        QTRY_COMPARE(QApplication::focusWidget(), nullptr);

        window.showNormal();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
#ifdef Q_OS_MACOS
        if (!macHasAccessToWindowsServer())
            QEXPECT_FAIL("", "When not having WindowServer access, we lose focus.", Continue);
#elif defined(Q_OS_WINRT)
        QEXPECT_FAIL("", "Winrt fails here - QTBUG-68297", Continue);
#endif
        QTRY_COMPARE(window.focusWidget(), firstchild);
#ifdef Q_OS_MACOS
        if (!macHasAccessToWindowsServer())
            QEXPECT_FAIL("", "When not having WindowServer access, we lose focus.", Continue);
#elif defined(Q_OS_WINRT)
        QEXPECT_FAIL("", "Winrt fails here - QTBUG-68297", Continue);
#endif
        QTRY_COMPARE(QApplication::focusWidget(), firstchild);
    }
}


void tst_QWidget::reparent()
{
    QWidget parent;
    parent.setWindowTitle(QStringLiteral("Toplevel ") + __FUNCTION__);
    const QPoint parentPosition = m_availableTopLeft + QPoint(300, 300);
    parent.setGeometry(QRect(parentPosition, m_testWidgetSize));

    QWidget child;
    child.setObjectName("child");
    child.setGeometry(10, 10, 180, 130);
    QPalette pal1;
    pal1.setColor(child.backgroundRole(), Qt::white);
    child.setPalette(pal1);

    QWidget childTLW(&child, Qt::Window);
    childTLW.setObjectName(QStringLiteral("childTLW ") +  __FUNCTION__);
    childTLW.setWindowTitle(childTLW.objectName());
    childTLW.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWidgetSize));
    QPalette pal2;
    pal2.setColor(childTLW.backgroundRole(), Qt::yellow);
    childTLW.setPalette(pal2);

    parent.show();
    childTLW.show();
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "WinRT does not support more than 1 top level widget", Abort);
#endif
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    parent.move(parentPosition);

    QPoint childPos = parent.mapToGlobal(child.pos());
    QPoint tlwPos = childTLW.pos();

    child.setParent(nullptr, child.windowFlags() & ~Qt::WindowType_Mask);
    child.setGeometry(childPos.x(), childPos.y(), child.width(), child.height());
    child.show();

#if 0   // QTBUG-26424
    if (m_platform == QStringLiteral("xcb"))
        QEXPECT_FAIL("", "On X11, the window manager will apply NorthWestGravity rules to 'child', which"
                         " means the top-left corner of the window frame will be placed at 'childPos'"
                         " causing this test to fail.", Continue);
#endif

    QCOMPARE(child.geometry().topLeft(), childPos);
    QTRY_COMPARE(childTLW.pos(), tlwPos);
}

// Qt/Embedded does it differently.
void tst_QWidget::icon()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    QPixmap p(20,20);
    p.fill(Qt::red);
    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget.data()));
    testWidget->setWindowIcon(p);

    QVERIFY(!testWidget->windowIcon().isNull());
    testWidget->show();
    QVERIFY(!testWidget->windowIcon().isNull());
    testWidget->showFullScreen();
    QVERIFY(!testWidget->windowIcon().isNull());
    testWidget->showNormal();
    QVERIFY(!testWidget->windowIcon().isNull());
}

void tst_QWidget::hideWhenFocusWidgetIsChild()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported.");

    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->setWindowTitle(__FUNCTION__);
    testWidget->resize(m_testWidgetSize);
    centerOnScreen(testWidget.data());
    QWidget *parentWidget(new QWidget(testWidget.data()));
    parentWidget->setObjectName("parentWidget");
    parentWidget->setGeometry(0, 0, 100, 100);
    QLineEdit *edit = new QLineEdit(parentWidget);
    edit->setObjectName("edit1");
    QLineEdit *edit3 = new QLineEdit(parentWidget);
    edit3->setObjectName("edit3");
    edit3->move(0,50);
    QLineEdit *edit2 = new QLineEdit(testWidget.data());
    edit2->setObjectName("edit2");
    edit2->move(110, 100);
    edit->setFocus();
    testWidget->show();
    testWidget->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(testWidget.data()));

    QString actualFocusWidget, expectedFocusWidget;
    if (!QApplication::focusWidget() && m_platform == QStringLiteral("xcb"))
        QSKIP("X11: Your window manager is too broken for this test");

    QVERIFY(QApplication::focusWidget());
    actualFocusWidget = QString::asprintf("%p %s %s", QApplication::focusWidget(), QApplication::focusWidget()->objectName().toLatin1().constData(), QApplication::focusWidget()->metaObject()->className());
    expectedFocusWidget = QString::asprintf("%p %s %s", edit, edit->objectName().toLatin1().constData(), edit->metaObject()->className());
    QCOMPARE(actualFocusWidget, expectedFocusWidget);

    parentWidget->hide();
    QCoreApplication::processEvents();
    actualFocusWidget = QString::asprintf("%p %s %s", QApplication::focusWidget(), QApplication::focusWidget()->objectName().toLatin1().constData(), QApplication::focusWidget()->metaObject()->className());
    expectedFocusWidget = QString::asprintf("%p %s %s", edit2, edit2->objectName().toLatin1().constData(), edit2->metaObject()->className());
    QCOMPARE(actualFocusWidget, expectedFocusWidget);
}

void tst_QWidget::normalGeometry()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    else if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT: This fails. Figure out why - QTBUG-68297.");
    QWidget parent;
    parent.setWindowTitle("NormalGeometry parent");
    QWidget *child = new QWidget(&parent);

    QCOMPARE(parent.normalGeometry(), parent.geometry());
    QCOMPARE(child->normalGeometry(), QRect());

    const QRect testGeom = QRect(m_availableTopLeft + QPoint(100 ,100), m_testWidgetSize);
    parent.setGeometry(testGeom);
    parent.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QApplication::processEvents();

    QRect geom = parent.geometry();
    // ### the window manager places the top-left corner at
    // ### 100,100... making geom something like 102,124 (offset by
    // ### the frame/frame)... this indicates a rather large different
    // ### between how X11 and Windows works
    // QCOMPARE(geom, QRect(100, 100, 200, 200));
    QCOMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowMaximized);
    QTRY_VERIFY(parent.windowState() & Qt::WindowMaximized);
    QTRY_VERIFY(parent.geometry() != geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowMaximized);
    QTRY_VERIFY(!(parent.windowState() & Qt::WindowMaximized));
    QTRY_COMPARE(parent.geometry(), geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.showMaximized();
    QTRY_VERIFY(parent.windowState() & Qt::WindowMaximized);
    QTRY_VERIFY(parent.geometry() != geom);
    QCOMPARE(parent.normalGeometry(), geom);

    parent.showNormal();
    QTRY_VERIFY(!(parent.windowState() & Qt::WindowMaximized));
    QTRY_COMPARE(parent.geometry(), geom);
    QCOMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(10);
    parent.setWindowState(parent.windowState() ^ Qt::WindowMaximized);
    QTest::qWait(10);
    if (m_platform == QStringLiteral("xcb"))
        QSKIP("QTBUG-26424");
    QTRY_VERIFY(parent.windowState() & (Qt::WindowMinimized|Qt::WindowMaximized));
    // ### when minimized and maximized at the same time, the geometry
    // ### does *NOT* have to be the normal geometry, it could be the
    // ### maximized geometry.
    // QCOMPARE(parent.geometry(), geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowMinimized);
    QTest::qWait(10);
    QTRY_VERIFY(!(parent.windowState() & Qt::WindowMinimized));
    QTRY_VERIFY(parent.windowState() & Qt::WindowMaximized);
    QTRY_VERIFY(parent.geometry() != geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowMaximized);
    QTest::qWait(10);
    QTRY_VERIFY(!(parent.windowState() & Qt::WindowMaximized));
    QTRY_COMPARE(parent.geometry(), geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowFullScreen);
    QTest::qWait(10);
    QTRY_VERIFY(parent.windowState() & Qt::WindowFullScreen);
    QTRY_VERIFY(parent.geometry() != geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.setWindowState(parent.windowState() ^ Qt::WindowFullScreen);
    QTest::qWait(10);
    QVERIFY(!(parent.windowState() & Qt::WindowFullScreen));
    QTRY_COMPARE(parent.geometry(), geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.showFullScreen();
    QTest::qWait(10);
    QTRY_VERIFY(parent.windowState() & Qt::WindowFullScreen);
    QTRY_VERIFY(parent.geometry() != geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.showNormal();
    QTest::qWait(10);
    QTRY_VERIFY(!(parent.windowState() & Qt::WindowFullScreen));
    QTRY_COMPARE(parent.geometry(), geom);
    QTRY_COMPARE(parent.normalGeometry(), geom);

    parent.showNormal();
    parent.setWindowState(Qt:: WindowFullScreen | Qt::WindowMaximized);
    parent.setWindowState(Qt::WindowMinimized | Qt:: WindowFullScreen | Qt::WindowMaximized);
    parent.setWindowState(Qt:: WindowFullScreen | Qt::WindowMaximized);
    QTest::qWait(10);
    QTRY_COMPARE(parent.normalGeometry(), geom);
}

void tst_QWidget::setGeometry()
{
    QWidget tlw;
    tlw.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QWidget child(&tlw);

    const QPoint topLeft = QGuiApplication::primaryScreen()->availableGeometry().topLeft();
    const QSize initialSize = 2 * m_testWidgetSize;
    QRect tr(topLeft + QPoint(100,100), initialSize);
    QRect cr(50,50,50,50);
    tlw.setGeometry(tr);
    child.setGeometry(cr);
    tlw.showNormal();
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT does not support setGeometry", Abort);
    QTRY_COMPARE(tlw.geometry().size(), tr.size());
    QCOMPARE(child.geometry(), cr);

    tlw.setParent(nullptr, Qt::Window|Qt::FramelessWindowHint);
    tr = QRect(topLeft, initialSize / 2);
    tlw.setGeometry(tr);
    QCOMPARE(tlw.geometry(), tr);
    tlw.showNormal();
    QTest::qWait(50);
    if (tlw.frameGeometry() != tlw.geometry())
        QSKIP("Your window manager is too broken for this test");
    if (m_platform == QStringLiteral("xcb"))
        QSKIP("QTBUG-26424");
    QCOMPARE(tlw.geometry(), tr);
}

void tst_QWidget::setGeometryHidden()
{
    if (QGuiApplication::styleHints()->showIsMaximized())
        QSKIP("Platform does not support QWidget::setGeometry() - skipping");

    QWidget tlw;
    tlw.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QWidget child(&tlw);

    const QRect tr(m_availableTopLeft + QPoint(100, 100), 2 * m_testWidgetSize);
    const QRect cr(QPoint(50, 50), m_testWidgetSize);
    tlw.setGeometry(tr);
    child.setGeometry(cr);
    tlw.showNormal();

    tlw.hide();
    QTRY_VERIFY(tlw.isHidden());
    tlw.setGeometry(cr);
    QVERIFY(tlw.testAttribute(Qt::WA_PendingMoveEvent));
    QVERIFY(tlw.testAttribute(Qt::WA_PendingResizeEvent));
    QImage img(tlw.size(), QImage::Format_ARGB32);  // just needed to call QWidget::render()
    tlw.render(&img);
    QVERIFY(!tlw.testAttribute(Qt::WA_PendingMoveEvent));
    QVERIFY(!tlw.testAttribute(Qt::WA_PendingResizeEvent));
    tlw.setGeometry(cr);
    QVERIFY(!tlw.testAttribute(Qt::WA_PendingMoveEvent));
    QVERIFY(!tlw.testAttribute(Qt::WA_PendingResizeEvent));
    tlw.resize(cr.size());
    QVERIFY(!tlw.testAttribute(Qt::WA_PendingMoveEvent));
    QVERIFY(!tlw.testAttribute(Qt::WA_PendingResizeEvent));
}

void tst_QWidget::windowOpacity()
{
    QWidget widget;
    QWidget child(&widget);

    // Initial value should be 1.0
    QCOMPARE(widget.windowOpacity(), 1.0);
    // children should always return 1.0
    QCOMPARE(child.windowOpacity(), 1.0);

    widget.setWindowOpacity(0.0);
    QCOMPARE(widget.windowOpacity(), 0.0);
    child.setWindowOpacity(0.0);
    QCOMPARE(child.windowOpacity(), 1.0);

    widget.setWindowOpacity(1.0);
    QCOMPARE(widget.windowOpacity(), 1.0);
    child.setWindowOpacity(1.0);
    QCOMPARE(child.windowOpacity(), 1.0);

    widget.setWindowOpacity(2.0);
    QCOMPARE(widget.windowOpacity(), 1.0);
    child.setWindowOpacity(2.0);
    QCOMPARE(child.windowOpacity(), 1.0);

    widget.setWindowOpacity(-1.0);
    QCOMPARE(widget.windowOpacity(), 0.0);
    child.setWindowOpacity(-1.0);
    QCOMPARE(child.windowOpacity(), 1.0);
}

class UpdateWidget : public QWidget
{
public:
    explicit UpdateWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName(QLatin1String("UpdateWidget"));
        reset();
    }

    void paintEvent(QPaintEvent *e) override
    {
        paintedRegion += e->region();
        ++numPaintEvents;
        if (resizeInPaintEvent) {
            resizeInPaintEvent = false;
            resize(size() + QSize(2, 2));
        }
    }

    bool event(QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::ZOrderChange:
            ++numZOrderChangeEvents;
            break;
        case QEvent::UpdateRequest:
            ++numUpdateRequestEvents;
            break;
        case QEvent::ActivationChange:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
            if (!updateOnActivationChangeAndFocusIn)
                return true; // Filter out to avoid update() calls in QWidget.
            break;
        default:
            break;
        }
        return QWidget::event(event);
    }

    void reset()
    {
        numPaintEvents = numZOrderChangeEvents = numUpdateRequestEvents = 0;
        updateOnActivationChangeAndFocusIn = resizeInPaintEvent = false;
        paintedRegion = QRegion();
    }

    int numPaintEvents;
    int numZOrderChangeEvents;
    int numUpdateRequestEvents;
    bool updateOnActivationChangeAndFocusIn;
    bool resizeInPaintEvent;
    QRegion paintedRegion;
};

void tst_QWidget::lostUpdatesOnHide()
{
#ifndef Q_OS_MACOS
    UpdateWidget widget;
    widget.setAttribute(Qt::WA_DontShowOnScreen);
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.show();
    widget.hide();
    QTest::qWait(50);
    widget.show();
    QTest::qWait(50);

    QCOMPARE(widget.numPaintEvents, 1);
#endif
}

void tst_QWidget::raise()
{
    QScopedPointer<QWidget> parentPtr(new QWidget);
    parentPtr->resize(200, 200);
    parentPtr->setObjectName(QLatin1String("raise"));
    parentPtr->setWindowTitle(parentPtr->objectName());
    QList<UpdateWidget *> allChildren;

    UpdateWidget *child1 = new UpdateWidget(parentPtr.data());
    child1->setAutoFillBackground(true);
    allChildren.append(child1);

    UpdateWidget *child2 = new UpdateWidget(parentPtr.data());
    child2->setAutoFillBackground(true);
    allChildren.append(child2);

    UpdateWidget *child3 = new UpdateWidget(parentPtr.data());
    child3->setAutoFillBackground(true);
    allChildren.append(child3);

    UpdateWidget *child4 = new UpdateWidget(parentPtr.data());
    child4->setAutoFillBackground(true);
    allChildren.append(child4);

    parentPtr->show();
    QVERIFY(QTest::qWaitForWindowExposed(parentPtr.data()));

#ifdef Q_OS_MACOS
    if (child1->internalWinId()) {
        QSKIP("Cocoa has no Z-Order for views, we hack it, but it results in paint events.");
    }
#endif

    QObjectList list1{child1, child2, child3, child4};
    QCOMPARE(parentPtr->children(), list1);
    QCOMPARE(allChildren.count(), list1.count());

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = child == child4 ? 1 : 0;
        if (expectedPaintEvents == 0) {
            QCOMPARE(child->numPaintEvents, 0);
        } else {
            // show() issues multiple paint events on some window managers
            QTRY_VERIFY(child->numPaintEvents >= expectedPaintEvents);
        }
        QCOMPARE(child->numZOrderChangeEvents, 0);
        child->reset();
    }

    for (int i = 0; i < 5; ++i)
        child2->raise();
    QTest::qWait(50);

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = child == child2 ? 1 : 0;
        int expectedZOrderChangeEvents = child == child2 ? 1 : 0;
        QTRY_COMPARE(child->numPaintEvents, expectedPaintEvents);
        QCOMPARE(child->numZOrderChangeEvents, expectedZOrderChangeEvents);
        child->reset();
    }

    QList<QObject *> list2;
    list2 << child1 << child3 << child4 << child2;
    QCOMPARE(parentPtr->children(), list2);

    // Creates a widget on top of all the children and checks that raising one of
    // the children underneath doesn't trigger a repaint on the covering widget.
    QWidget topLevel;
    topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QWidget *parent = parentPtr.take();
    parent->setParent(&topLevel);
    topLevel.show();

    UpdateWidget *onTop = new UpdateWidget(&topLevel);
    onTop->reset();
    onTop->resize(topLevel.size());
    onTop->setAutoFillBackground(true);
    onTop->show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTRY_VERIFY(onTop->numPaintEvents > 0);
    onTop->reset();

    // Reset all the children.
    for (UpdateWidget *child : qAsConst(allChildren))
        child->reset();

    for (int i = 0; i < 5; ++i)
        child3->raise();
    QTest::qWait(50);

    QCOMPARE(onTop->numPaintEvents, 0);
    QCOMPARE(onTop->numZOrderChangeEvents, 0);

    QObjectList list3{child1, child4, child2, child3};
    QCOMPARE(parent->children(), list3);

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = 0;
        int expectedZOrderChangeEvents = child == child3 ? 1 : 0;
        QTRY_COMPARE(child->numPaintEvents, expectedPaintEvents);
        QCOMPARE(child->numZOrderChangeEvents, expectedZOrderChangeEvents);
        child->reset();
    }
}

void tst_QWidget::lower()
{
    QScopedPointer<QWidget> parent(new QWidget);
    parent->setObjectName(QLatin1String("lower"));
    parent->setWindowTitle(parent->objectName());
    parent->resize(200, 200);
    QList<UpdateWidget *> allChildren;

    UpdateWidget *child1 = new UpdateWidget(parent.data());
    child1->setAutoFillBackground(true);
    allChildren.append(child1);

    UpdateWidget *child2 = new UpdateWidget(parent.data());
    child2->setAutoFillBackground(true);
    allChildren.append(child2);

    UpdateWidget *child3 = new UpdateWidget(parent.data());
    child3->setAutoFillBackground(true);
    allChildren.append(child3);

    UpdateWidget *child4 = new UpdateWidget(parent.data());
    child4->setAutoFillBackground(true);
    allChildren.append(child4);

    parent->show();
    QVERIFY(QTest::qWaitForWindowExposed(parent.data()));

    QObjectList list1{child1, child2, child3, child4};
    QCOMPARE(parent->children(), list1);
    QCOMPARE(allChildren.count(), list1.count());

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = child == child4 ? 1 : 0;
        if (expectedPaintEvents == 0) {
            QCOMPARE(child->numPaintEvents, 0);
        } else {
            // show() issues multiple paint events on some window managers
            QTRY_VERIFY(child->numPaintEvents >= expectedPaintEvents);
        }
        QCOMPARE(child->numZOrderChangeEvents, 0);
        child->reset();
    }

    for (int i = 0; i < 5; ++i)
        child4->lower();

    QTest::qWait(100);

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = child == child3 ? 1 : 0;
        int expectedZOrderChangeEvents = child == child4 ? 1 : 0;
        QTRY_COMPARE(child->numZOrderChangeEvents, expectedZOrderChangeEvents);
        QTRY_COMPARE(child->numPaintEvents, expectedPaintEvents);
        child->reset();
    }

    QList<QObject *> list2;
    list2 << child4 << child1 << child2 << child3;
    QCOMPARE(parent->children(), list2);
}

void tst_QWidget::stackUnder()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974: Cocoa has no Z-Order for views, we hack it, but it results in paint events.");
#endif

    QScopedPointer<QWidget> parent(new QWidget);
    parent->setObjectName(QLatin1String("stackUnder"));
    parent->setWindowTitle(parent->objectName());
    parent->resize(200, 200);
    QList<UpdateWidget *> allChildren;

    UpdateWidget *child1 = new UpdateWidget(parent.data());
    child1->setAutoFillBackground(true);
    allChildren.append(child1);

    UpdateWidget *child2 = new UpdateWidget(parent.data());
    child2->setAutoFillBackground(true);
    allChildren.append(child2);

    UpdateWidget *child3 = new UpdateWidget(parent.data());
    child3->setAutoFillBackground(true);
    allChildren.append(child3);

    UpdateWidget *child4 = new UpdateWidget(parent.data());
    child4->setAutoFillBackground(true);
    allChildren.append(child4);

    parent->show();
    QVERIFY(QTest::qWaitForWindowExposed(parent.data()));
    QObjectList list1{child1, child2, child3, child4};
    QCOMPARE(parent->children(), list1);

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = child == child4 ? 1 : 0;
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        if (expectedPaintEvents == 1 && child->numPaintEvents == 2)
            QEXPECT_FAIL(0, "Mac and Windows issues double repaints for Z-Order change", Continue);
#endif
        QTRY_COMPARE(child->numPaintEvents, expectedPaintEvents);
        QCOMPARE(child->numZOrderChangeEvents, 0);
        child->reset();
    }

    for (int i = 0; i < 5; ++i)
        child4->stackUnder(child2);
    QTest::qWait(10);

    QObjectList list2{child1, child4, child2, child3};
    QCOMPARE(parent->children(), list2);

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedPaintEvents = child == child3 ? 1 : 0;
        int expectedZOrderChangeEvents = child == child4 ? 1 : 0;
        QTRY_COMPARE(child->numPaintEvents, expectedPaintEvents);
        QTRY_COMPARE(child->numZOrderChangeEvents, expectedZOrderChangeEvents);
        child->reset();
    }

    for (int i = 0; i < 5; ++i)
        child1->stackUnder(child3);
    QTest::qWait(10);

    QObjectList list3{child4, child2, child1, child3};
    QCOMPARE(parent->children(), list3);

    for (UpdateWidget *child : qAsConst(allChildren)) {
        int expectedZOrderChangeEvents = child == child1 ? 1 : 0;
        if (child == child3) {
#ifndef Q_OS_MACOS
            QEXPECT_FAIL(0, "See QTBUG-493", Continue);
#endif
            QCOMPARE(child->numPaintEvents, 0);
        } else {
            QCOMPARE(child->numPaintEvents, 0);
        }
        QTRY_COMPARE(child->numZOrderChangeEvents, expectedZOrderChangeEvents);
        child->reset();
    }
}

void drawPolygon(QPaintDevice *dev, int w, int h)
{
    QPainter p(dev);
    p.fillRect(0, 0, w, h, Qt::white);

    QPolygon a;
    a << QPoint(0, 0) << QPoint(w/2, h/2) << QPoint(w, 0)
      << QPoint(w/2, h) << QPoint(0, 0);

    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::DiagCrossPattern);
    p.drawPolygon(a);
}

class ContentsPropagationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ContentsPropagationWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName(QLatin1String("ContentsPropagationWidget"));
        setWindowTitle(objectName());
        QWidget *child = this;
        for (int i = 0; i < 32; ++i) {
            child = new QWidget(child);
            child->setGeometry(i, i, 400 - i * 2, 400 - i * 2);
        }
    }

    void setContentsPropagation(bool enable)
    {
        for (QObject *child : children())
            qobject_cast<QWidget *>(child)->setAutoFillBackground(!enable);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        int w = width(), h = height();
        drawPolygon(this, w, h);
    }

    QSize sizeHint() const override { return {500, 500}; }
};

// Scale to remove devicePixelRatio should scaling be active.
static QPixmap grabFromWidget(QWidget *w, const QRect &rect)
{
    QPixmap pixmap = w->grab(rect);
    const qreal devicePixelRatio = pixmap.devicePixelRatioF();
    if (!qFuzzyCompare(devicePixelRatio, qreal(1))) {
        pixmap = pixmap.scaled((QSizeF(pixmap.size()) / devicePixelRatio).toSize(),
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pixmap.setDevicePixelRatio(1);
    }
    return pixmap;
}

void tst_QWidget::testContentsPropagation()
{
    if (!qFuzzyCompare(qApp->devicePixelRatio(), qreal(1)))
        QSKIP("This test does not work with scaling.");
    ContentsPropagationWidget widget;
    widget.setFixedSize(500, 500);
    widget.setContentsPropagation(false);
    QPixmap widgetSnapshot = widget.grab(QRect(QPoint(0, 0), QSize(-1, -1)));

    QPixmap correct(500, 500);
    drawPolygon(&correct, 500, 500);
    //correct.save("correct.png", "PNG");

    //widgetSnapshot.save("snap1.png", "PNG");
    QVERIFY(widgetSnapshot.toImage() != correct.toImage());

    widget.setContentsPropagation(true);
    widgetSnapshot = widgetSnapshot = widget.grab(QRect(QPoint(0, 0), QSize(-1, -1)));
    //widgetSnapshot.save("snap2.png", "PNG");

    QCOMPARE(widgetSnapshot, correct);
}

/*
    Test that saving and restoring window geometry with
    saveGeometry() and restoreGeometry() works.
*/

void tst_QWidget::saveRestoreGeometry()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    const QPoint position = m_availableTopLeft + QPoint(100, 100);
    const QSize size = m_testWidgetSize;

    QByteArray savedGeometry;

    {
        QWidget widget;
        widget.move(position);
        widget.resize(size);
        widget.showNormal();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QApplication::processEvents();

        if (m_platform == QStringLiteral("winrt"))
            QEXPECT_FAIL("", "WinRT does not support move/resize", Abort);
        QTRY_VERIFY2(HighDpi::fuzzyCompare(widget.pos(), position, m_fuzz),
                     qPrintable(HighDpi::msgPointMismatch(widget.pos(), position)));
        QCOMPARE(widget.size(), size);
        savedGeometry = widget.saveGeometry();
    }

    {
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));

        const QByteArray empty;
        const QByteArray one("a");
        const QByteArray two("ab");
        const QByteArray three("abc");
        const QByteArray four("abca");
        const QByteArray garbage("abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc");

        QVERIFY(!widget.restoreGeometry(empty));
        QVERIFY(!widget.restoreGeometry(one));
        QVERIFY(!widget.restoreGeometry(two));
        QVERIFY(!widget.restoreGeometry(three));
        QVERIFY(!widget.restoreGeometry(four));
        QVERIFY(!widget.restoreGeometry(garbage));

        QVERIFY(widget.restoreGeometry(savedGeometry));
        widget.showNormal();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QApplication::processEvents();

        QVERIFY2(HighDpi::fuzzyCompare(widget.pos(), position, m_fuzz),
                 qPrintable(HighDpi::msgPointMismatch(widget.pos(), position)));
        QCOMPARE(widget.size(), size);
        widget.show();
        QVERIFY2(HighDpi::fuzzyCompare(widget.pos(), position, m_fuzz),
                 qPrintable(HighDpi::msgPointMismatch(widget.pos(), position)));
        QCOMPARE(widget.size(), size);
    }

    {
        QWidget widget;
        widget.move(position);
        widget.resize(size);
        widget.showNormal();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QTRY_COMPARE(widget.geometry().size(), size);

        QRect geom;

        //Restore from Full screen
        savedGeometry = widget.saveGeometry();
        geom = widget.geometry();
        widget.setWindowState(widget.windowState() | Qt::WindowFullScreen);
        QTRY_VERIFY((widget.windowState() & Qt::WindowFullScreen));
        QTest::qWait(500);
        QVERIFY(widget.restoreGeometry(savedGeometry));
        QTest::qWait(120);
        QTRY_VERIFY(!(widget.windowState() & Qt::WindowFullScreen));
        QTRY_COMPARE(widget.geometry(), geom);

        //Restore to full screen
        widget.setWindowState(widget.windowState() | Qt::WindowFullScreen);
        QTest::qWait(120);
        QTRY_VERIFY((widget.windowState() & Qt::WindowFullScreen));
        QTest::qWait(500);
        savedGeometry = widget.saveGeometry();
        geom = widget.geometry();
        widget.setWindowState(widget.windowState() ^ Qt::WindowFullScreen);
        QTest::qWait(120);
        QTRY_VERIFY(!(widget.windowState() & Qt::WindowFullScreen));
        QTest::qWait(400);
        QVERIFY(widget.restoreGeometry(savedGeometry));
        QTest::qWait(120);
        QTRY_VERIFY((widget.windowState() & Qt::WindowFullScreen));
        QTRY_COMPARE(widget.geometry(), geom);
        QVERIFY((widget.windowState() & Qt::WindowFullScreen));
        widget.setWindowState(widget.windowState() ^ Qt::WindowFullScreen);
        QTest::qWait(120);
        QTRY_VERIFY(!(widget.windowState() & Qt::WindowFullScreen));
        QTest::qWait(120);

        //Restore from Maximised
        widget.move(position);
        widget.resize(size);
        QTest::qWait(10);
        QTRY_COMPARE(widget.size(), size);
        QTest::qWait(500);
        savedGeometry = widget.saveGeometry();
        geom = widget.geometry();
        widget.setWindowState(widget.windowState() | Qt::WindowMaximized);
        QTest::qWait(120);
        QTRY_VERIFY((widget.windowState() & Qt::WindowMaximized));
        QTRY_VERIFY(widget.geometry() != geom);
        QTest::qWait(500);
        QVERIFY(widget.restoreGeometry(savedGeometry));
        QTest::qWait(120);
        QTRY_COMPARE(widget.geometry(), geom);

        QVERIFY(!(widget.windowState() & Qt::WindowMaximized));

        //Restore to maximised
        widget.setWindowState(widget.windowState() | Qt::WindowMaximized);
        QTest::qWait(120);
        QTRY_VERIFY((widget.windowState() & Qt::WindowMaximized));
        QTest::qWait(500);
        geom = widget.geometry();
        savedGeometry = widget.saveGeometry();
        widget.setWindowState(widget.windowState() ^ Qt::WindowMaximized);
        QTest::qWait(120);
        QTRY_VERIFY(!(widget.windowState() & Qt::WindowMaximized));
        QTest::qWait(500);
        QVERIFY(widget.restoreGeometry(savedGeometry));
        QTest::qWait(120);
        QTRY_VERIFY((widget.windowState() & Qt::WindowMaximized));
        QTRY_COMPARE(widget.geometry(), geom);
    }
}

void tst_QWidget::restoreVersion1Geometry_data()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<Qt::WindowState>("expectedWindowState");
    QTest::addColumn<QPoint>("expectedPosition");
    QTest::addColumn<QSize>("expectedSize");
    QTest::addColumn<QRect>("expectedNormalGeometry");
    const QPoint position(100, 100);
    const QSize size(200, 200);
    const QRect normalGeometry(102, 124, 200, 200);

    QTest::newRow("geometry.dat") << ":geometry.dat" << Qt::WindowNoState << position << size << normalGeometry;
    QTest::newRow("geometry-maximized.dat") << ":geometry-maximized.dat" << Qt::WindowMaximized << position << size << normalGeometry;
    QTest::newRow("geometry-fullscreen.dat") << ":geometry-fullscreen.dat" << Qt::WindowFullScreen << position << size << normalGeometry;
}

/*
    Test that the current version of restoreGeometry() can restore geometry
    saved width saveGeometry() version 1.0.
*/
void tst_QWidget::restoreVersion1Geometry()
{
    QFETCH(QString, fileName);
    QFETCH(Qt::WindowState, expectedWindowState);
    QFETCH(QPoint, expectedPosition);
    Q_UNUSED(expectedPosition);
    QFETCH(QSize, expectedSize);
    QFETCH(QRect, expectedNormalGeometry);

    if (m_platform == QLatin1String("windows") && QGuiApplication::primaryScreen()->geometry().width() > 2000)
        QSKIP("Skipping due to minimum decorated window size on Windows");

    // WindowActive is uninteresting for this test
    const Qt::WindowStates WindowStateMask = Qt::WindowFullScreen | Qt::WindowMaximized | Qt::WindowMinimized;

    QFile f(fileName);
    QVERIFY(f.exists());
    f.open(QIODevice::ReadOnly);
    const QByteArray savedGeometry = f.readAll();
    QCOMPARE(savedGeometry.count(), 46);
    f.close();

    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("::")
                          + QLatin1String(QTest::currentDataTag()));

    QVERIFY(widget.restoreGeometry(savedGeometry));

    QCOMPARE(widget.windowState() & WindowStateMask, expectedWindowState);
    if (expectedWindowState == Qt::WindowNoState) {
        QTRY_COMPARE(widget.geometry(), expectedNormalGeometry);
        QCOMPARE(widget.size(), expectedSize);
    }

    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTest::qWait(100);

    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT does not support restoreGeometry", Abort);

    if (expectedWindowState == Qt::WindowNoState) {
        QTRY_COMPARE(widget.size(), expectedSize);
        QCOMPARE(widget.geometry(), expectedNormalGeometry);
    }

    widget.showNormal();
    QTest::qWait(10);

    QTRY_COMPARE(widget.geometry(), expectedNormalGeometry);
    if (expectedWindowState == Qt::WindowNoState)
        QCOMPARE(widget.size(), expectedSize);

#if 0
    // Code for saving a new geometry*.dat files
    {
        QWidget widgetToSave;
        widgetToSave.move(expectedPosition);
        widgetToSave.resize(expectedSize);
        widgetToSave.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QTest::qWait(500); // stabilize
        widgetToSave.setWindowState(Qt::WindowStates(expectedWindowState));
        QTest::qWait(500); // stabilize

        QByteArray geometryToSave = widgetToSave.saveGeometry();

        // Code for saving a new geometry.dat file.
        f.setFileName(fileName.mid(1));
        QVERIFY(f.open(QIODevice::WriteOnly)); // did you forget to 'p4 edit *.dat'? :)
        f.write(geometryToSave);
        f.close();
    }
#endif
}

void tst_QWidget::widgetAt()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    if (m_platform == QStringLiteral("offscreen"))
        QSKIP("Platform offscreen does not support lower()/raise() or WindowMasks");
    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT does not support more than 1 top level widget");

    Q_CHECK_PAINTEVENTS

    const QPoint referencePos = m_availableTopLeft + QPoint(100, 100);
    QScopedPointer<QWidget> w1(new QWidget(nullptr, Qt::X11BypassWindowManagerHint));
    w1->setGeometry(QRect(referencePos, QSize(m_testWidgetSize.width(), 150)));
    w1->setObjectName(QLatin1String("w1"));
    w1->setWindowTitle(w1->objectName());
    QScopedPointer<QWidget> w2(new QWidget(nullptr, Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint));
    w2->setGeometry(QRect(referencePos + QPoint(50, 50), QSize(m_testWidgetSize.width(), 100)));
    w2->setObjectName(QLatin1String("w2"));
    w2->setWindowTitle(w2->objectName());
    w1->showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(w1.data()));
    const QPoint testPos = referencePos + QPoint(100, 100);
    QWidget *wr;
    QTRY_VERIFY((wr = QApplication::widgetAt((testPos))));
    QCOMPARE(wr->objectName(), QString("w1"));

    w2->showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(w2.data()));
    QTRY_VERIFY((wr = QApplication::widgetAt(testPos)));
    QCOMPARE(wr->objectName(), QString("w2"));

    w2->lower();
    QTRY_VERIFY((wr = QApplication::widgetAt(testPos)) && wr->objectName() == QString("w1"));
    w2->raise();

    QTRY_VERIFY((wr = QApplication::widgetAt(testPos)) && wr->objectName() == QString("w2"));

    QWidget *w3 = new QWidget(w2.data());
    w3->setGeometry(10,10,50,50);
    w3->setObjectName("w3");
    w3->showNormal();
    QTRY_VERIFY((wr = QApplication::widgetAt(testPos)) && wr->objectName() == QString("w3"));

    w3->setAttribute(Qt::WA_TransparentForMouseEvents);
    QTRY_VERIFY((wr = QApplication::widgetAt(testPos)) && wr->objectName() == QString("w2"));

    if (!QGuiApplicationPrivate::platformIntegration()
                               ->hasCapability(QPlatformIntegration::WindowMasks)) {
        QSKIP("Platform does not support WindowMasks");
    }

    QRegion rgn = QRect(QPoint(0,0), w2->size());
    QPoint point = w2->mapFromGlobal(testPos);
    rgn -= QRect(point, QSize(1,1));
    w2->setMask(rgn);

    QTRY_VERIFY((wr = QApplication::widgetAt(testPos)));
    QTRY_COMPARE(wr->objectName(), w1->objectName());
    QTRY_VERIFY((wr = QApplication::widgetAt(testPos + QPoint(1, 1))));
    QTRY_COMPARE(wr->objectName(), w2->objectName());

    QBitmap bitmap(w2->size());
    QPainter p(&bitmap);
    p.fillRect(bitmap.rect(), Qt::color1);
    p.setPen(Qt::color0);
    p.drawPoint(w2->mapFromGlobal(testPos));
    p.end();
    w2->setMask(bitmap);
    QTRY_COMPARE(QApplication::widgetAt(testPos), w1.data());
    QTRY_VERIFY(QApplication::widgetAt(testPos + QPoint(1, 1)) == w2.data());
}

void tst_QWidget::task110173()
{
    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));

    QPushButton *pb1 = new QPushButton("click", &w);
    pb1->setFocusPolicy(Qt::ClickFocus);
    pb1->move(100, 100);

    QPushButton *pb2 = new QPushButton("push", &w);
    pb2->setFocusPolicy(Qt::ClickFocus);
    pb2->move(300, 300);

    QTest::keyClick( &w, Qt::Key_Tab );
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
}

class Widget : public QWidget
{
public:
    Widget() { setFocusPolicy(Qt::StrongFocus); }
    void actionEvent(QActionEvent *) override { if (deleteThis) delete this; }
    void changeEvent(QEvent *) override { if (deleteThis) delete this; }
    void closeEvent(QCloseEvent *) override { if (deleteThis) delete this; }
    void hideEvent(QHideEvent *) override { if (deleteThis) delete this; }
    void focusOutEvent(QFocusEvent *) override { if (deleteThis) delete this; }
    void keyPressEvent(QKeyEvent *) override { if (deleteThis) delete this; }
    void keyReleaseEvent(QKeyEvent *) override { if (deleteThis) delete this; }
    void mouseDoubleClickEvent(QMouseEvent *) override { if (deleteThis) delete this; }
    void mousePressEvent(QMouseEvent *) override { if (deleteThis) delete this; }
    void mouseReleaseEvent(QMouseEvent *) override { if (deleteThis) delete this; }
    void mouseMoveEvent(QMouseEvent *) override { if (deleteThis) delete this; }

    bool deleteThis = false;
};

void tst_QWidget::testDeletionInEventHandlers()
{
    // closeEvent
    QPointer<Widget> w = new Widget;
    w->deleteThis = true;
    w->close();
    QVERIFY(w.isNull());
    delete w;

    // focusOut (crashes)
    //w = new Widget;
    //w->show();
    //w->setFocus();
    //QCOMPARE(qApp->focusWidget(), w);
    //w->deleteThis = true;
    //w->clearFocus();
    //QVERIFY(w.isNull());

    // key press
    w = new Widget;
    w->show();
    w->deleteThis = true;
    QTest::keyPress(w, Qt::Key_A);
    QVERIFY(w.isNull());
    delete w;

    // key release
    w = new Widget;
    w->show();
    w->deleteThis = true;
    QTest::keyRelease(w, Qt::Key_A);
    QVERIFY(w.isNull());
    delete w;

    // mouse press
    w = new Widget;
    w->show();
    w->deleteThis = true;
    QTest::mousePress(w, Qt::LeftButton);
    QVERIFY(w.isNull());
    delete w;

    // mouse release
    w = new Widget;
    w->show();
    w->deleteThis = true;
    QMouseEvent me(QEvent::MouseButtonRelease, QPoint(1, 1), Qt::LeftButton, Qt::LeftButton, Qt::KeyboardModifiers());
    qApp->notify(w, &me);
    QVERIFY(w.isNull());
    delete w;

    // mouse double click
    w = new Widget;
    w->show();
    w->deleteThis = true;
    QTest::mouseDClick(w, Qt::LeftButton);
    QVERIFY(w.isNull());
    delete w;

    // hide event (crashes)
    //w = new Widget;
    //w->show();
    //w->deleteThis = true;
    //w->hide();
    //QVERIFY(w.isNull());

    // action event
    w = new Widget;
    w->deleteThis = true;
    w->addAction(new QAction(w));
    QVERIFY(w.isNull());
    delete w;

    // change event
    w = new Widget;
    w->show();
    w->deleteThis = true;
    w->setMouseTracking(true);
    QVERIFY(w.isNull());
    delete w;

    w = new Widget;
    w->setMouseTracking(true);
    w->show();
    w->deleteThis = true;
    me = QMouseEvent(QEvent::MouseMove, QPoint(0, 0), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(w, &me);
    QVERIFY(w.isNull());
    delete w;
}

#ifdef Q_OS_MACOS
class MaskedPainter : public QWidget
{
public:
    QRect mask;

    MaskedPainter()
    : mask(20, 20, 50, 50)
    {
        setMask(mask);
    }

    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.fillRect(mask, QColor(Qt::red));
    }
};

/*
    Verifies that the entire area inside the mask is painted red.
*/
bool verifyWidgetMask(QWidget *widget, QRect mask)
{
    const QImage image = widget->grab(QRect(QPoint(0, 0), widget->size())).toImage();

    const QImage masked = image.copy(mask);
    QImage red(masked);
    red.fill(QColor(Qt::red).rgb());

    return (masked == red);
}

void tst_QWidget::setMask()
{
    {
        MaskedPainter w;
        w.resize(200, 200);
        w.show();
        QTest::qWait(100);
        QVERIFY(verifyWidgetMask(&w, w.mask));
    }
    {
        MaskedPainter w;
        w.resize(200, 200);
        w.setWindowFlags(w.windowFlags() | Qt::FramelessWindowHint);
        w.show();
        QTest::qWait(100);
        QRect mask = w.mask;

        QVERIFY(verifyWidgetMask(&w, mask));
    }
}
#endif

class StaticWidget : public QWidget
{
Q_OBJECT
public:
    bool partial = false;
    bool gotPaintEvent = false;
    QRegion paintedRegion;

    explicit StaticWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_StaticContents);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setPalette(Qt::red); // Make sure we have an opaque palette.
        setAutoFillBackground(true);
    }

    void paintEvent(QPaintEvent *e) override
    {
        paintedRegion += e->region();
        gotPaintEvent = true;
//        qDebug() << "paint" << e->region();
        // Look for a full update, set partial to false if found.
        for (QRect r : e->region()) {
            partial = (r != rect());
            if (!partial)
                break;
        }
    }
};

/*
    Test that widget resizes and moves can be done with minimal repaints when WA_StaticContents
    and WA_OpaquePaintEvent is set. Test is mac-only for now.
*/
void tst_QWidget::optimizedResizeMove()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    QWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    parent.resize(400, 400);

    StaticWidget staticWidget(&parent);
    staticWidget.gotPaintEvent = false;
    staticWidget.move(150, 150);
    staticWidget.resize(150, 150);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QTRY_VERIFY(staticWidget.gotPaintEvent);

    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(10, 10));
    QTest::qWait(20);
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT does not support move/resize", Abort);
    QCOMPARE(staticWidget.gotPaintEvent, false);

    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(-10, -10));
    QTest::qWait(20);
    QCOMPARE(staticWidget.gotPaintEvent, false);

    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(-10, 10));
    QTest::qWait(20);
    QCOMPARE(staticWidget.gotPaintEvent, false);

    staticWidget.gotPaintEvent = false;
    staticWidget.resize(staticWidget.size() + QSize(10, 10));
    QTRY_VERIFY(staticWidget.gotPaintEvent);
    QCOMPARE(staticWidget.partial, true);

    staticWidget.gotPaintEvent = false;
    staticWidget.resize(staticWidget.size() + QSize(-10, -10));
    QTest::qWait(20);
    QCOMPARE(staticWidget.gotPaintEvent, false);

    staticWidget.gotPaintEvent = false;
    staticWidget.resize(staticWidget.size() + QSize(10, -10));
    QTRY_VERIFY(staticWidget.gotPaintEvent);
    QCOMPARE(staticWidget.partial, true);

    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(10, 10));
    staticWidget.resize(staticWidget.size() + QSize(-10, -10));
    QTest::qWait(20);
    QCOMPARE(staticWidget.gotPaintEvent, false);

    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(10, 10));
    staticWidget.resize(staticWidget.size() + QSize(10, 10));
    QTRY_VERIFY(staticWidget.gotPaintEvent);
    QCOMPARE(staticWidget.partial, true);

    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(-10, -10));
    staticWidget.resize(staticWidget.size() + QSize(-10, -10));
    QTest::qWait(20);
    QCOMPARE(staticWidget.gotPaintEvent, false);

    staticWidget.setAttribute(Qt::WA_StaticContents, false);
    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(-10, -10));
    staticWidget.resize(staticWidget.size() + QSize(-10, -10));
    QTRY_VERIFY(staticWidget.gotPaintEvent);
    QCOMPARE(staticWidget.partial, false);
    staticWidget.setAttribute(Qt::WA_StaticContents, true);

    staticWidget.setAttribute(Qt::WA_StaticContents, false);
    staticWidget.gotPaintEvent = false;
    staticWidget.move(staticWidget.pos() + QPoint(10, 10));
    QTest::qWait(20);
    QCOMPARE(staticWidget.gotPaintEvent, false);
    staticWidget.setAttribute(Qt::WA_StaticContents, true);
}

void tst_QWidget::optimizedResize_topLevel()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    if (QHighDpiScaling::isActive())
        QSKIP("Skip due to rounding errors in the regions.");
    StaticWidget topLevel;
    topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    topLevel.gotPaintEvent = false;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTRY_VERIFY(topLevel.gotPaintEvent);

    topLevel.gotPaintEvent = false;
    topLevel.partial = false;
    topLevel.paintedRegion = QRegion();

#if !defined(Q_OS_WIN32)
    topLevel.resize(topLevel.size() + QSize(10, 10));
#else
    // Static contents does not work when programmatically resizing
    // top-levels with QWidget::resize. We do some funky stuff in
    // setGeometry_sys. However, resizing it with the mouse or with
    // a native function call works (it basically has to go through
    // WM_RESIZE in QApplication). This is a corner case, though.
    // See task 243708
    RECT rect;
    GetWindowRect(winHandleOf(&topLevel), &rect);
    MoveWindow(winHandleOf(&topLevel), rect.left, rect.top,
               rect.right - rect.left + 10, rect.bottom - rect.top + 10,
               true);
    QTest::qWait(100);
#endif

    // Expected update region: New rect - old rect.
    QRegion expectedUpdateRegion(topLevel.rect());
    expectedUpdateRegion -= QRect(QPoint(), topLevel.size() - QSize(10, 10));

    QTRY_VERIFY(topLevel.gotPaintEvent);
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("offscreen"))
        QSKIP("QTBUG-26424");
    else if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT does not support move/resize", Abort);
    QCOMPARE(topLevel.partial, true);
    QCOMPARE(topLevel.paintedRegion, expectedUpdateRegion);
}

class SiblingDeleter : public QWidget
{
public:
    inline SiblingDeleter(QWidget *sibling, QWidget *parent)
        : QWidget(parent), sibling(sibling) {}
    inline ~SiblingDeleter() { delete sibling; }

private:
    QPointer<QWidget> sibling;
};


void tst_QWidget::childDeletesItsSibling()
{
    auto commonParent = new QWidget(nullptr);
    QPointer<QWidget> child(new QWidget(nullptr));
    QPointer<QWidget> siblingDeleter = new SiblingDeleter(child, commonParent);
    child->setParent(commonParent);
    delete commonParent; // don't crash
    QVERIFY(!child);
    QVERIFY(!siblingDeleter);

}

void tst_QWidget::setMinimumSize()
{
    QWidget w;
    QSize defaultSize = w.size();

    w.setMinimumSize(defaultSize + QSize(100, 100));
    QCOMPARE(w.size(), defaultSize + QSize(100, 100));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setMinimumSize(defaultSize + QSize(50, 50));
    QCOMPARE(w.size(), defaultSize + QSize(100, 100));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setMinimumSize(defaultSize + QSize(200, 200));
    QCOMPARE(w.size(), defaultSize + QSize(200, 200));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    QSize nonDefaultSize = defaultSize + QSize(5,5);
    w.setMinimumSize(nonDefaultSize);
    w.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY2(w.height() >= nonDefaultSize.height(),
             msgComparisonFailed(w.height(), ">=", nonDefaultSize.height()));
    QVERIFY2(w.width() >= nonDefaultSize.width(),
             msgComparisonFailed(w.width(), ">=", nonDefaultSize.width()));
}

void tst_QWidget::setMaximumSize()
{
    QWidget w;
    QSize defaultSize = w.size();

    w.setMinimumSize(defaultSize + QSize(100, 100));
    QCOMPARE(w.size(), defaultSize + QSize(100, 100));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));
    w.setMinimumSize(defaultSize);

    w.setMaximumSize(defaultSize + QSize(200, 200));
    QCOMPARE(w.size(), defaultSize + QSize(100, 100));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setMaximumSize(defaultSize + QSize(50, 50));
    QCOMPARE(w.size(), defaultSize + QSize(50, 50));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));
}

void tst_QWidget::setFixedSize()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget w;
    QSize defaultSize = w.size();

    w.setFixedSize(defaultSize + QSize(100, 100));
    QCOMPARE(w.size(), defaultSize + QSize(100, 100));
    QVERIFY(w.testAttribute(Qt::WA_Resized));

    w.setFixedSize(defaultSize + QSize(200, 200));

    QCOMPARE(w.minimumSize(), defaultSize + QSize(200,200));
    QCOMPARE(w.maximumSize(), defaultSize + QSize(200,200));
    QCOMPARE(w.size(), defaultSize + QSize(200, 200));
    QVERIFY(w.testAttribute(Qt::WA_Resized));

    w.setFixedSize(defaultSize + QSize(50, 50));
    QCOMPARE(w.size(), defaultSize + QSize(50, 50));
    QVERIFY(w.testAttribute(Qt::WA_Resized));

    w.setAttribute(Qt::WA_Resized, false);
    w.setFixedSize(defaultSize + QSize(50, 50));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setFixedSize(defaultSize + QSize(150, 150));
    w.showNormal();
    QVERIFY(QTest::qWaitForWindowActive(&w));
    if (m_platform == QStringLiteral("xcb"))
        QSKIP("QTBUG-26424");
    else if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT does not support move/resize", Abort);
    QCOMPARE(w.size(), defaultSize + QSize(150,150));
}

void tst_QWidget::ensureCreated()
{
    {
        QWidget widget;
        WId widgetWinId = widget.winId();
        Q_UNUSED(widgetWinId);
        QVERIFY(widget.testAttribute(Qt::WA_WState_Created));
    }

    {
        QWidget window;

        QDialog dialog(&window);
        dialog.setWindowModality(Qt::NonModal);

        WId dialogWinId = dialog.winId();
        Q_UNUSED(dialogWinId);
        QVERIFY(dialog.testAttribute(Qt::WA_WState_Created));
        QVERIFY(window.testAttribute(Qt::WA_WState_Created));
    }

    {
        QWidget window;

        QDialog dialog(&window);
        dialog.setWindowModality(Qt::WindowModal);

        WId dialogWinId = dialog.winId();
        Q_UNUSED(dialogWinId);
        QVERIFY(dialog.testAttribute(Qt::WA_WState_Created));
        QVERIFY(window.testAttribute(Qt::WA_WState_Created));
    }

    {
        QWidget window;

        QDialog dialog(&window);
        dialog.setWindowModality(Qt::ApplicationModal);

        WId dialogWinId = dialog.winId();
        Q_UNUSED(dialogWinId);
        QVERIFY(dialog.testAttribute(Qt::WA_WState_Created));
        QVERIFY(window.testAttribute(Qt::WA_WState_Created));
    }
}

class WinIdChangeWidget : public QWidget
{
public:
    using QWidget::QWidget;
protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::WinIdChange) {
            m_winIdList.append(internalWinId());
            return true;
        }
        return QWidget::event(e);
    }
public:
    QList<WId> m_winIdList;
    int winIdChangeEventCount() const { return m_winIdList.count(); }
};

class CreateDestroyWidget : public WinIdChangeWidget
{
public:
    void create() { QWidget::create(); }
    void destroy() { QWidget::destroy(); }
};

void tst_QWidget::createAndDestroy()
{
    CreateDestroyWidget widget;

    // Create and destroy via QWidget
    widget.create();
    QVERIFY(widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 1);
    QVERIFY(widget.internalWinId());

    widget.destroy();
    QVERIFY(!widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 2);
    QVERIFY(!widget.internalWinId());

    // Create via QWidget, destroy via QWindow
    widget.create();
    QVERIFY(widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 3);
    QVERIFY(widget.internalWinId());

    widget.windowHandle()->destroy();
    QVERIFY(!widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 4);
    QVERIFY(!widget.internalWinId());

    // Create via QWidget again
    widget.create();
    QVERIFY(widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 5);
    QVERIFY(widget.internalWinId());

    // Destroy via QWindow, create via QWindow
    widget.windowHandle()->destroy();
    QVERIFY(widget.windowHandle());
    QVERIFY(!widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 6);
    QVERIFY(!widget.internalWinId());

    widget.windowHandle()->create();
    QVERIFY(widget.testAttribute(Qt::WA_WState_Created));
    QCOMPARE(widget.winIdChangeEventCount(), 7);
    QVERIFY(widget.internalWinId());
}

void tst_QWidget::winIdChangeEvent()
{
    {
        // Transforming an alien widget into a native widget
        WinIdChangeWidget widget;
        const WId winIdBefore = widget.internalWinId();
        const WId winIdAfter = widget.winId();
        QVERIFY(winIdBefore != winIdAfter);
        QCOMPARE(widget.winIdChangeEventCount(), 1);
    }

    {
        // Changing parent of a native widget
        QWidget parent1, parent2;
        WinIdChangeWidget child(&parent1);
        const WId winIdBefore = child.winId();
        QCOMPARE(child.winIdChangeEventCount(), 1);
        child.setParent(&parent2);
        const WId winIdAfter = child.internalWinId();
        QCOMPARE(winIdBefore, winIdAfter);
        QCOMPARE(child.winIdChangeEventCount(), 3);
        // winId is set to zero during reparenting
        QCOMPARE(WId(0), child.m_winIdList[1]);
    }

    {
        // Changing grandparent of a native widget
        QWidget grandparent1, grandparent2;
        QWidget parent(&grandparent1);
        WinIdChangeWidget child(&parent);
        const WId winIdBefore = child.winId();
        QCOMPARE(child.winIdChangeEventCount(), 1);
        parent.setParent(&grandparent2);
        const WId winIdAfter = child.internalWinId();
        QCOMPARE(winIdBefore, winIdAfter);
        QCOMPARE(child.winIdChangeEventCount(), 1);
    }

    {
        // Changing parent of an alien widget
        QWidget parent1, parent2;
        WinIdChangeWidget child(&parent1);
        const WId winIdBefore = child.internalWinId();
        child.setParent(&parent2);
        const WId winIdAfter = child.internalWinId();
        QCOMPARE(winIdBefore, winIdAfter);
        QCOMPARE(child.winIdChangeEventCount(), 0);
    }

    {
        // Making native child widget into a top-level window
        QWidget parent;
        WinIdChangeWidget child(&parent);
        child.winId();
        const WId winIdBefore = child.internalWinId();
        QCOMPARE(child.winIdChangeEventCount(), 1);
        const Qt::WindowFlags flags = child.windowFlags();
        child.setWindowFlags(flags | Qt::Window);
        const WId winIdAfter = child.internalWinId();
        QCOMPARE(winIdBefore, winIdAfter);
        QCOMPARE(child.winIdChangeEventCount(), 3);
        // winId is set to zero during reparenting
        QCOMPARE(WId(0), child.m_winIdList[1]);
    }
}

void tst_QWidget::persistentWinId()
{
    QScopedPointer<QWidget> parent(new QWidget);
    QWidget *w1 = new QWidget;
    QWidget *w2 = new QWidget;
    QWidget *w3 = new QWidget;
    w1->setParent(parent.data());
    w2->setParent(w1);
    w3->setParent(w2);

    WId winId1 = w1->winId();
    WId winId2 = w2->winId();
    WId winId3 = w3->winId();

    // reparenting should preserve the winId of the widget being reparented and of its children
    w1->setParent(nullptr);
    QCOMPARE(w1->winId(), winId1);
    QCOMPARE(w2->winId(), winId2);
    QCOMPARE(w3->winId(), winId3);

    w1->setParent(parent.data());
    QCOMPARE(w1->winId(), winId1);
    QCOMPARE(w2->winId(), winId2);
    QCOMPARE(w3->winId(), winId3);

    w2->setParent(nullptr);
    QCOMPARE(w2->winId(), winId2);
    QCOMPARE(w3->winId(), winId3);

    w2->setParent(parent.data());
    QCOMPARE(w2->winId(), winId2);
    QCOMPARE(w3->winId(), winId3);

    w2->setParent(w1);
    QCOMPARE(w2->winId(), winId2);
    QCOMPARE(w3->winId(), winId3);

    w3->setParent(nullptr);
    QCOMPARE(w3->winId(), winId3);

    w3->setParent(w1);
    QCOMPARE(w3->winId(), winId3);

    w3->setParent(w2);
    QCOMPARE(w3->winId(), winId3);
}

void tst_QWidget::transientParent()
{
    QWidget topLevel;
    topLevel.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWidgetSize));
    topLevel.setWindowTitle(__FUNCTION__);
    QWidget *child = new QWidget(&topLevel);
    QMenu *menu = new QMenu(child); // QTBUG-41898: Use top level as transient parent for native widgets as well.
    QToolButton *toolButton = new QToolButton(child);
    toolButton->setMenu(menu);
    toolButton->winId();
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QCOMPARE(menu->windowHandle()->transientParent(), topLevel.windowHandle());
}

void tst_QWidget::showNativeChild()
{
    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT does not support setGeometry");
    QWidget topLevel;
    topLevel.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWidgetSize));
    topLevel.setWindowTitle(__FUNCTION__);
    QWidget child(&topLevel);
    child.winId();
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
}

class ShowHideEventWidget : public QWidget
{
public:
    int numberOfShowEvents = 0, numberOfHideEvents = 0;
    int numberOfSpontaneousShowEvents = 0, numberOfSpontaneousHideEvents = 0;

    using QWidget::QWidget;
    using QWidget::create;

    void showEvent(QShowEvent *e) override
    {
        ++numberOfShowEvents;
        if (e->spontaneous())
            ++numberOfSpontaneousShowEvents;
    }

    void hideEvent(QHideEvent *e) override
    {
        ++numberOfHideEvents;
        if (e->spontaneous())
            ++numberOfSpontaneousHideEvents;
    }
};

void tst_QWidget::showHideEvent_data()
{
    QTest::addColumn<bool>("show");
    QTest::addColumn<bool>("hide");
    QTest::addColumn<bool>("create");
    QTest::addColumn<int>("expectedShowEvents");
    QTest::addColumn<int>("expectedHideEvents");

    QTest::newRow("window: only show")
            << true
            << false
            << false
            << 1
            << 0;
    QTest::newRow("window: show/hide")
            << true
            << true
            << false
            << 1
            << 1;
    QTest::newRow("window: show/hide/create")
            << true
            << true
            << true
            << 1
            << 1;
    QTest::newRow("window: hide/create")
            << false
            << true
            << true
            << 0
            << 0;
    QTest::newRow("window: only hide")
            << false
            << true
            << false
            << 0
            << 0;
    QTest::newRow("window: nothing")
            << false
            << false
            << false
            << 0
            << 0;
}

void tst_QWidget::showHideEvent()
{
    QFETCH(bool, show);
    QFETCH(bool, hide);
    QFETCH(bool, create);
    QFETCH(int, expectedShowEvents);
    QFETCH(int, expectedHideEvents);

    ShowHideEventWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    if (show)
        widget.show();
    if (hide)
        widget.hide();
    if (create && !widget.testAttribute(Qt::WA_WState_Created))
        widget.create();

    QCOMPARE(widget.numberOfShowEvents, expectedShowEvents);
    QCOMPARE(widget.numberOfHideEvents, expectedHideEvents);
}

void tst_QWidget::showHideEventWhileMinimize()
{
    const QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    if (!pi->hasCapability(QPlatformIntegration::MultipleWindows)
        || !pi->hasCapability(QPlatformIntegration::NonFullScreenWindows)
        || !pi->hasCapability(QPlatformIntegration::WindowManagement)) {
        QSKIP("This test requires window management capabilities");
    }
    // QTBUG-41312, hide, show events should be received during minimized.
    ShowHideEventWidget widget;
    widget.setWindowTitle(__FUNCTION__);
    widget.resize(m_testWidgetSize);
    centerOnScreen(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const int showEventsBeforeMinimize = widget.numberOfShowEvents;
    const int hideEventsBeforeMinimize = widget.numberOfHideEvents;
    widget.showMinimized();
    QTRY_COMPARE(widget.numberOfHideEvents, hideEventsBeforeMinimize + 1);
    widget.showNormal();
    QTRY_COMPARE(widget.numberOfShowEvents, showEventsBeforeMinimize + 1);
}

void tst_QWidget::showHideChildrenWhileMinimize_QTBUG50589()
{
    const QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    if (!pi->hasCapability(QPlatformIntegration::MultipleWindows)
        || !pi->hasCapability(QPlatformIntegration::NonFullScreenWindows)
        || !pi->hasCapability(QPlatformIntegration::WindowManagement)) {
        QSKIP("This test requires window management capabilities");
    }

    QWidget parent;
    ShowHideEventWidget child(&parent);

    parent.setWindowTitle(QTest::currentTestFunction());
    parent.resize(m_testWidgetSize);
    centerOnScreen(&parent);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    const int showEventsBeforeMinimize = child.numberOfSpontaneousShowEvents;
    const int hideEventsBeforeMinimize = child.numberOfSpontaneousHideEvents;
    parent.showMinimized();
    QTRY_COMPARE(child.numberOfSpontaneousHideEvents, hideEventsBeforeMinimize + 1);
    parent.showNormal();
    QTRY_COMPARE(child.numberOfSpontaneousShowEvents, showEventsBeforeMinimize + 1);
}

void tst_QWidget::update()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    Q_CHECK_PAINTEVENTS

    UpdateWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.resize(100, 100);
    centerOnScreen(&w);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_COMPARE(w.numPaintEvents, 1);

    QCOMPARE(w.visibleRegion(), QRegion(w.rect()));
    QCOMPARE(w.paintedRegion, w.visibleRegion());
    w.reset();

    UpdateWidget child(&w);
    child.setGeometry(10, 10, 80, 80);
    child.show();

    QPoint childOffset = child.mapToParent(QPoint());

    // widgets are transparent by default, so both should get repaints
    {
        if (m_platform == QStringLiteral("winrt"))
            QEXPECT_FAIL("", "WinRT does not support setGeometry", Abort);
        QApplication::processEvents();
        QApplication::processEvents();
        QCOMPARE(child.numPaintEvents, 1);
        QCOMPARE(child.visibleRegion(), QRegion(child.rect()));
        QCOMPARE(child.paintedRegion, child.visibleRegion());
        QCOMPARE(w.numPaintEvents, 1);
        QCOMPARE(w.visibleRegion(), QRegion(w.rect()));
        QCOMPARE(w.paintedRegion, child.visibleRegion().translated(childOffset));

        w.reset();
        child.reset();

        w.update();
        QApplication::processEvents();
        QApplication::processEvents();
        QCOMPARE(child.numPaintEvents, 1);
        QCOMPARE(child.visibleRegion(), QRegion(child.rect()));
        QCOMPARE(child.paintedRegion, child.visibleRegion());
        QCOMPARE(w.numPaintEvents, 1);
        QCOMPARE(w.visibleRegion(), QRegion(w.rect()));
        QCOMPARE(w.paintedRegion, w.visibleRegion());
    }

    QPalette opaquePalette = child.palette();
    opaquePalette.setColor(child.backgroundRole(), QColor(Qt::red));

    // setting an opaque background on the child should prevent paint-events
    // for the parent in the child area
    {
        child.setPalette(opaquePalette);
        child.setAutoFillBackground(true);
        QApplication::processEvents();

        w.reset();
        child.reset();

        w.update();
        QApplication::processEvents();
        QApplication::processEvents();

        QCOMPARE(w.numPaintEvents, 1);
        QRegion expectedVisible = QRegion(w.rect())
                                  - child.visibleRegion().translated(childOffset);
        QCOMPARE(w.visibleRegion(), expectedVisible);
        QCOMPARE(w.paintedRegion, expectedVisible);
        QCOMPARE(child.numPaintEvents, 0);

        w.reset();
        child.reset();

        child.update();
        QApplication::processEvents();
        QApplication::processEvents();

        QCOMPARE(w.numPaintEvents, 0);
        QCOMPARE(child.numPaintEvents, 1);
        QCOMPARE(child.paintedRegion, child.visibleRegion());

        w.reset();
        child.reset();
    }

    // overlapping sibling
    UpdateWidget sibling(&w);
    child.setGeometry(10, 10, 20, 20);
    sibling.setGeometry(15, 15, 20, 20);
    sibling.show();

    QApplication::processEvents();
    w.reset();
    child.reset();
    sibling.reset();

    const QPoint siblingOffset = sibling.mapToParent(QPoint());

    sibling.update();
    QApplication::processEvents();
    QApplication::processEvents();

    // child is opaque, sibling transparent
    {
        QCOMPARE(sibling.numPaintEvents, 1);
        QCOMPARE(sibling.paintedRegion, sibling.visibleRegion());

        QCOMPARE(child.numPaintEvents, 1);
        QCOMPARE(child.paintedRegion.translated(childOffset),
                 child.visibleRegion().translated(childOffset)
                 & sibling.visibleRegion().translated(siblingOffset));

        QCOMPARE(w.numPaintEvents, 1);
        QCOMPARE(w.paintedRegion,
                 w.visibleRegion() & sibling.visibleRegion().translated(siblingOffset));
        QCOMPARE(w.paintedRegion,
                 (w.visibleRegion() - child.visibleRegion().translated(childOffset))
                 & sibling.visibleRegion().translated(siblingOffset));

    }
    w.reset();
    child.reset();
    sibling.reset();

    sibling.setPalette(opaquePalette);
    sibling.setAutoFillBackground(true);

    sibling.update();
    QApplication::processEvents();
    QApplication::processEvents();

    // child opaque, sibling opaque
    {
        QCOMPARE(sibling.numPaintEvents, 1);
        QCOMPARE(sibling.paintedRegion, sibling.visibleRegion());

#ifdef Q_OS_MACOS
        if (child.internalWinId()) // child is native
            QEXPECT_FAIL(0, "Cocoa compositor paints child and sibling", Continue);
#endif
        QCOMPARE(child.numPaintEvents, 0);
        QCOMPARE(child.visibleRegion(),
                 QRegion(child.rect())
                 - sibling.visibleRegion().translated(siblingOffset - childOffset));

        QCOMPARE(w.numPaintEvents, 0);
        QCOMPARE(w.visibleRegion(),
                 QRegion(w.rect())
                 - child.visibleRegion().translated(childOffset)
                 - sibling.visibleRegion().translated(siblingOffset));
    }
}

#ifndef Q_OS_MACOS
static inline bool isOpaque(QWidget *widget)
{
    if (!widget)
        return false;
    return qt_widget_private(widget)->isOpaque;
}
#endif

void tst_QWidget::isOpaque()
{
#ifndef Q_OS_MACOS
    QWidget w;
    QVERIFY(::isOpaque(&w));

    QWidget child(&w);
    QVERIFY(!::isOpaque(&child));

    child.setAutoFillBackground(true);
    QVERIFY(::isOpaque(&child));

    QPalette palette;

    // background color

    palette = child.palette();
    palette.setColor(child.backgroundRole(), QColor(255, 0, 0, 127));
    child.setPalette(palette);
    QVERIFY(!::isOpaque(&child));

    palette.setColor(child.backgroundRole(), QColor(255, 0, 0, 255));
    child.setPalette(palette);
    QVERIFY(::isOpaque(&child));

    palette.setColor(QPalette::Window, QColor(0, 0, 255, 127));
    w.setPalette(palette);

    QVERIFY(!::isOpaque(&w));

    child.setAutoFillBackground(false);
    QVERIFY(!::isOpaque(&child));

    // Qt::WA_OpaquePaintEvent

    child.setAttribute(Qt::WA_OpaquePaintEvent);
    QVERIFY(::isOpaque(&child));

    child.setAttribute(Qt::WA_OpaquePaintEvent, false);
    QVERIFY(!::isOpaque(&child));

    // Qt::WA_NoSystemBackground

    child.setAttribute(Qt::WA_NoSystemBackground);
    QVERIFY(!::isOpaque(&child));

    child.setAttribute(Qt::WA_NoSystemBackground, false);
    QVERIFY(!::isOpaque(&child));

    palette.setColor(QPalette::Window, QColor(0, 0, 255, 255));
    w.setPalette(palette);
    QVERIFY(::isOpaque(&w));

    w.setAttribute(Qt::WA_NoSystemBackground);
    QVERIFY(!::isOpaque(&w));

    w.setAttribute(Qt::WA_NoSystemBackground, false);
    QVERIFY(::isOpaque(&w));

    {
        QPalette palette = QApplication::palette();
        QPalette old = palette;
        palette.setColor(QPalette::Window, Qt::transparent);
        QApplication::setPalette(palette);

        QWidget widget;
        QVERIFY(!::isOpaque(&widget));

        QApplication::setPalette(old);
        QCOMPARE(::isOpaque(&widget), old.color(QPalette::Window).alpha() == 255);
    }
#endif
}

#ifndef Q_OS_MACOS
/*
    Test that scrolling of a widget invalidates the correct regions
*/
void tst_QWidget::scroll()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    QScreen *screen = QGuiApplication::primaryScreen();
    const int w = qMin(500, screen->availableGeometry().width() / 2);
    const int h = qMin(500, screen->availableGeometry().height() / 2);

    UpdateWidget updateWidget;
    updateWidget.resize(w, h);
    updateWidget.reset();
    updateWidget.move(QGuiApplication::primaryScreen()->geometry().center() - QPoint(250, 250));
    updateWidget.showNormal();
    QApplication::setActiveWindow(&updateWidget);
    QVERIFY(QTest::qWaitForWindowActive(&updateWidget));
    QVERIFY(updateWidget.numPaintEvents > 0);

    {
        updateWidget.reset();
        updateWidget.scroll(10, 10);
        QCoreApplication::processEvents();
        QRegion dirty(QRect(0, 0, w, 10));
        dirty += QRegion(QRect(0, 10, 10, h - 10));
        if (m_platform == QStringLiteral("winrt"))
            QEXPECT_FAIL("", "WinRT does not support move/resize", Abort);
        QTRY_COMPARE(updateWidget.paintedRegion, dirty);
    }

    {
        updateWidget.reset();
        updateWidget.update(0, 0, 10, 10);
        updateWidget.scroll(0, 10);
        QCoreApplication::processEvents();
        QRegion dirty(QRect(0, 0, w, 10));
        dirty += QRegion(QRect(0, 10, 10, 10));
        QTRY_COMPARE(updateWidget.paintedRegion, dirty);
    }

    if (updateWidget.width() < 200 || updateWidget.height() < 200)
         QSKIP("Skip this test due to too small screen geometry.");

    {
        updateWidget.reset();
        updateWidget.update(0, 0, 100, 100);
        updateWidget.scroll(10, 10, QRect(50, 50, 100, 100));
        QCoreApplication::processEvents();
        QRegion dirty(QRect(0, 0, 100, 50));
        dirty += QRegion(QRect(0, 50, 150, 10));
        dirty += QRegion(QRect(0, 60, 110, 40));
        dirty += QRegion(QRect(50, 100, 60, 10));
        dirty += QRegion(QRect(50, 110, 10, 40));
        QTRY_COMPARE(updateWidget.paintedRegion, dirty);
    }

    {
        updateWidget.reset();
        updateWidget.update(0, 0, 100, 100);
        updateWidget.scroll(10, 10, QRect(100, 100, 100, 100));
        QCoreApplication::processEvents();
        QRegion dirty(QRect(0, 0, 100, 100));
        dirty += QRegion(QRect(100, 100, 100, 10));
        dirty += QRegion(QRect(100, 110, 10, 90));
        QTRY_COMPARE(updateWidget.paintedRegion, dirty);
    }
}

// QTBUG-38999, scrolling a widget with native child widgets should move the children.
void tst_QWidget::scrollNativeChildren()
{
    QWidget parent;
    parent.setWindowTitle(QLatin1String(__FUNCTION__));
    parent.resize(400, 400);
    centerOnScreen(&parent);
    QLabel *nativeLabel = new QLabel(QStringLiteral("nativeLabel"), &parent);
    const QPoint oldLabelPos(100, 100);
    nativeLabel->move(oldLabelPos);
    QVERIFY(nativeLabel->winId());
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    const QPoint delta(50, 50);
    parent.scroll(delta.x(), delta.y());
    const QPoint newLabelPos = oldLabelPos + delta;
    QWindow *labelWindow = nativeLabel->windowHandle();
    QVERIFY(labelWindow);
    QTRY_COMPARE(labelWindow->geometry().topLeft(), newLabelPos);
    QTRY_COMPARE(nativeLabel->geometry().topLeft(), newLabelPos);
}

#endif // Mac OS

class DestroyedSlotChecker : public QObject
{
    Q_OBJECT

public:
    bool wasQWidget = false;

public slots:
    void destroyedSlot(QObject *object)
    {
        wasQWidget = (qobject_cast<QWidget *>(object) != nullptr || object->isWidgetType());
    }
};

/*
    Test that qobject_cast<QWidget*> returns 0 in a slot
    connected to QObject::destroyed.
*/
void tst_QWidget::qobject_castInDestroyedSlot()
{
    DestroyedSlotChecker checker;

    QWidget *widget = new QWidget();

    QObject::connect(widget, &QObject::destroyed, &checker, &DestroyedSlotChecker::destroyedSlot);
    delete widget;

    QVERIFY(checker.wasQWidget);
}

// Since X11 WindowManager operations are all async, and we have no way to know if the window
// manager has finished playing with the window geometry, this test can't be reliable on X11.

using Rects = QVector<QRect>;

void tst_QWidget::setWindowGeometry_data()
{
    QTest::addColumn<Rects>("rects");
    QTest::addColumn<int>("windowFlags");

    QVector<Rects> rects;
    const int width = m_testWidgetSize.width();
    const int height = m_testWidgetSize.height();
    const QRect availableAdjusted = QGuiApplication::primaryScreen()->availableGeometry().adjusted(100, 100, -100, -100);
    rects << Rects{QRect(m_availableTopLeft + QPoint(100, 100), m_testWidgetSize),
                   availableAdjusted,
                   QRect(m_availableTopLeft + QPoint(130, 100), QSize(0, height)),
                   QRect(m_availableTopLeft + QPoint(100, 50), QSize(width, 0)),
                   QRect(m_availableTopLeft + QPoint(130, 50), QSize(0, 0))}
          << Rects{availableAdjusted,
                   QRect(m_availableTopLeft + QPoint(130, 100), QSize(0, height)),
                   QRect(m_availableTopLeft + QPoint(100, 50), QSize(width, 0)),
                   QRect(m_availableTopLeft + QPoint(130, 50), QSize(0, 0)),
                   QRect(m_availableTopLeft + QPoint(100, 100), QSize(width, height))}
          << Rects{QRect(m_availableTopLeft + QPoint(130, 100), QSize(0, height)),
                   QRect(m_availableTopLeft + QPoint(100, 50), QSize(width, 0)),
                   QRect(m_availableTopLeft + QPoint(130, 50), QSize(0, 0)),
                   QRect(m_availableTopLeft + QPoint(100, 100), QSize(width, height)),
                   availableAdjusted}
          << Rects{QRect(m_availableTopLeft + QPoint(100, 50), QSize(width, 0)),
                   QRect(m_availableTopLeft + QPoint(130, 50), QSize(0, 0)),
                   QRect(m_availableTopLeft + QPoint(100, 100), QSize(width, height)),
                   availableAdjusted,
                   QRect(m_availableTopLeft + QPoint(130, 100), QSize(0, height))}
          << Rects{QRect(m_availableTopLeft + QPoint(130, 50), QSize(0, 0)),
                   QRect(m_availableTopLeft + QPoint(100, 100), QSize(width, height)),
                   availableAdjusted,
                   QRect(m_availableTopLeft + QPoint(130, 100), QSize(0, height)),
                   QRect(m_availableTopLeft + QPoint(100, 50), QSize(width, 0))};

    const Qt::WindowFlags windowFlags[] = {Qt::WindowFlags(), Qt::FramelessWindowHint};

    const bool skipEmptyRects = (m_platform == QStringLiteral("windows"));
    for (Rects l : qAsConst(rects)) {
        if (skipEmptyRects) {
            l.erase(std::remove_if(l.begin(), l.end(), [] (const QRect &r) { return r.isEmpty(); }),
                    l.end());
        }
        const QRect &rect = l.constFirst();
        for (int windowFlag : windowFlags) {
            QTest::newRow(QString("%1,%2 %3x%4, flags %5")
                          .arg(rect.x())
                          .arg(rect.y())
                          .arg(rect.width())
                          .arg(rect.height())
                          .arg(windowFlag, 0, 16).toLatin1())
                << l
                << windowFlag;
        }
    }
}

void tst_QWidget::setWindowGeometry()
{
    if (m_platform == QStringLiteral("xcb"))
         QSKIP("X11: Skip this test due to Window manager positioning issues.");
    else if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT does not support setWindowGeometry");

    QFETCH(Rects, rects);
    QFETCH(int, windowFlags);
    QRect rect = rects.takeFirst();

    {
        // test setGeometry() without actually showing the window
        QWidget widget;
        if (windowFlags != 0)
            widget.setWindowFlags(Qt::WindowFlags(windowFlags));

        widget.setGeometry(rect);
        QTest::qWait(100);
        QCOMPARE(widget.geometry(), rect);

        // setGeometry() without showing
        for (const QRect &r : qAsConst(rects)) {
            widget.setGeometry(r);
            QTest::qWait(100);
            QCOMPARE(widget.geometry(), r);
        }
    }

    {
        // setGeometry() first, then show()
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        if (windowFlags != 0)
            widget.setWindowFlags(Qt::WindowFlags(windowFlags));

        widget.setGeometry(rect);
        widget.showNormal();
        if (rect.isValid()) {
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        } else {
            // in case of an invalid rect, wait for the geometry to become
            // adjusted to the actual (valid) value.
            QApplication::processEvents();
        }
        QTRY_COMPARE(widget.geometry(), rect);

        // setGeometry() while shown
        for (const QRect &r : qAsConst(rects)) {
            widget.setGeometry(r);
            QTest::qWait(10);
            QTRY_COMPARE(widget.geometry(), r);
        }
        widget.setGeometry(rect);
        QTest::qWait(20);
        QTRY_COMPARE(widget.geometry(), rect);

        // now hide
        widget.hide();
        QTest::qWait(20);
        QTRY_COMPARE(widget.geometry(), rect);

        // setGeometry() after hide()
        for (const QRect &r : qAsConst(rects)) {
            widget.setGeometry(r);
            QTest::qWait(10);
            QTRY_COMPARE(widget.geometry(), r);
        }
        widget.setGeometry(rect);
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);

        // show() again, geometry() should still be the same
        widget.show();
        if (rect.isValid())
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QTRY_COMPARE(widget.geometry(), rect);

        // final hide(), again geometry() should be unchanged
        widget.hide();
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);
    }

    {
        // show() first, then setGeometry()
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        if (windowFlags != 0)
            widget.setWindowFlags(Qt::WindowFlags(windowFlags));

        widget.showNormal();
        if (rect.isValid())
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        widget.setGeometry(rect);
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);

        // setGeometry() while shown
        for (const QRect &r : qAsConst(rects)) {
            widget.setGeometry(r);
            QTest::qWait(10);
            QTRY_COMPARE(widget.geometry(), r);
        }
        widget.setGeometry(rect);
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);

        // now hide
        widget.hide();
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);

        // setGeometry() after hide()
        for (const QRect &r : qAsConst(rects)) {
            widget.setGeometry(r);
            QTest::qWait(10);
            QTRY_COMPARE(widget.geometry(), r);
        }
        widget.setGeometry(rect);
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);

        // show() again, geometry() should still be the same
        widget.show();
        if (rect.isValid())
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);

        // final hide(), again geometry() should be unchanged
        widget.hide();
        QTest::qWait(10);
        QTRY_COMPARE(widget.geometry(), rect);
    }
}

#if defined (Q_OS_WIN) && !defined(Q_OS_WINRT)
void tst_QWidget::setGeometry_win()
{
    QWidget widget;

    setFrameless(&widget);
    widget.setGeometry(QRect(m_availableTopLeft + QPoint(0, 600), QSize(100, 100)));
    widget.show();
    widget.setWindowState(widget.windowState() | Qt::WindowMaximized);
    QRect geom = widget.normalGeometry();
    widget.close();
    widget.setGeometry(geom);
    widget.setWindowState(widget.windowState() | Qt::WindowMaximized);
    widget.show();
    RECT rt;
    ::GetWindowRect(winHandleOf(&widget), &rt);
    QVERIFY2(rt.left <= m_availableTopLeft.x(),
             msgComparisonFailed(int(rt.left), "<=", m_availableTopLeft.x()));
    QVERIFY2(rt.top <= m_availableTopLeft.y(),
             msgComparisonFailed(int(rt.top), "<=", m_availableTopLeft.y()));
}
#endif // defined (Q_OS_WIN) && !defined(Q_OS_WINRT)

// Since X11 WindowManager operation are all async, and we have no way to know if the window
// manager has finished playing with the window geometry, this test can't be reliable on X11.

void tst_QWidget::windowMoveResize_data()
{
    setWindowGeometry_data();
}

void tst_QWidget::windowMoveResize()
{
    if (m_platform == QStringLiteral("xcb"))
         QSKIP("X11: Skip this test due to Window manager positioning issues.");
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT does not support move/resize");

    QFETCH(Rects, rects);
    QFETCH(int, windowFlags);

    QRect rect = rects.takeFirst();

    {
        // test setGeometry() without actually showing the window
        QWidget widget;
        if (windowFlags != 0)
            widget.setWindowFlags(Qt::WindowFlags(windowFlags));

        widget.move(rect.topLeft());
        widget.resize(rect.size());
        QTest::qWait(10);
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // move() without showing
        for (const QRect &r : qAsConst(rects)) {
            widget.move(r.topLeft());
            widget.resize(r.size());
            QApplication::processEvents();
            QTRY_COMPARE(widget.pos(), r.topLeft());
            QTRY_COMPARE(widget.size(), r.size());
        }
    }

    {
        // move() first, then show()
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        if (windowFlags != 0)
            widget.setWindowFlags(Qt::WindowFlags(windowFlags));

        widget.move(rect.topLeft());
        widget.resize(rect.size());
        widget.showNormal();

        QTest::qWait(10);
        QTRY_VERIFY2(HighDpi::fuzzyCompare(widget.pos(), rect.topLeft(), m_fuzz),
                     qPrintable(HighDpi::msgPointMismatch(widget.pos(), rect.topLeft())));
        // Windows: Minimum size of decorated windows.
        const bool expectResizeFail = (!windowFlags && (rect.width() < 160 || rect.height() < 40))
            && m_platform == QStringLiteral("windows");
        if (!expectResizeFail)
            QTRY_COMPARE(widget.size(), rect.size());

        // move() while shown
        for (const QRect &r : qAsConst(rects)) {
            // XCB: First resize after show of zero-sized gets wrong win_gravity.
            const bool expectMoveFail = !windowFlags
                && ((widget.width() == 0 || widget.height() == 0) && r.width() != 0 && r.height() != 0)
                && m_platform == QStringLiteral("xcb")
                && (rect == QRect(QPoint(130, 100), QSize(0, 200))
                    || rect == QRect(QPoint(100, 50), QSize(200, 0))
                    || rect == QRect(QPoint(130, 50), QSize(0, 0)));
            widget.move(r.topLeft());
            widget.resize(r.size());
            QApplication::processEvents();
            if (!expectMoveFail) {
                QTRY_COMPARE(widget.pos(), r.topLeft());
                QTRY_COMPARE(widget.size(), r.size());
            }
        }
        widget.move(rect.topLeft());
        widget.resize(rect.size());
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // now hide
        widget.hide();
        QTest::qWait(10);
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // move() after hide()
        for (const QRect &r : qAsConst(rects)) {
            widget.move(r.topLeft());
            widget.resize(r.size());
            QApplication::processEvents();
#if defined(Q_OS_MACOS)
            if (r.width() == 0 && r.height() > 0) {
                widget.move(r.topLeft());
                widget.resize(r.size());
             }
#endif
            QTRY_COMPARE(widget.pos(), r.topLeft());
            QTRY_COMPARE(widget.size(), r.size());
        }
        widget.move(rect.topLeft());
        widget.resize(rect.size());
        QTest::qWait(10);
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // show() again, pos() should be the same
        widget.show();
        if (rect.isValid())
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // final hide(), again pos() should be unchanged
        widget.hide();
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());
    }

    {
        // show() first, then move()
        QWidget widget;
        if (windowFlags != 0)
            widget.setWindowFlags(Qt::WindowFlags(windowFlags));

        widget.showNormal();
        if (rect.isValid())
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QApplication::processEvents();
        widget.move(rect.topLeft());
        widget.resize(rect.size());
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // move() while shown
        for (const QRect &r : qAsConst(rects)) {
            widget.move(r.topLeft());
            widget.resize(r.size());
            QApplication::processEvents();
            QTRY_COMPARE(widget.pos(), r.topLeft());
            QTRY_COMPARE(widget.size(), r.size());
        }
        widget.move(rect.topLeft());
        widget.resize(rect.size());
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // now hide
        widget.hide();
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // move() after hide()
        for (const QRect &r : qAsConst(rects)) {
            widget.move(r.topLeft());
            widget.resize(r.size());
            QApplication::processEvents();
#if defined(Q_OS_MACOS)
            if (r.width() == 0 && r.height() > 0) {
                widget.move(r.topLeft());
                widget.resize(r.size());
             }
#endif
            QTRY_COMPARE(widget.pos(), r.topLeft());
            QTRY_COMPARE(widget.size(), r.size());
        }
        widget.move(rect.topLeft());
        widget.resize(rect.size());
        QApplication::processEvents();
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // show() again, pos() should be the same
        widget.show();
        if (rect.isValid())
            QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QTest::qWait(10);
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());

        // final hide(), again pos() should be unchanged
        widget.hide();
        QTest::qWait(10);
        QTRY_COMPARE(widget.pos(), rect.topLeft());
        QTRY_COMPARE(widget.size(), rect.size());
    }
}

class ColorWidget : public QWidget
{
public:
    explicit ColorWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags(),
                         const QColor &c = QColor(Qt::red))
        : QWidget(parent, f), color(c)
    {
        QPalette opaquePalette = palette();
        opaquePalette.setColor(backgroundRole(), color);
        setPalette(opaquePalette);
        setAutoFillBackground(true);
    }

    void paintEvent(QPaintEvent *e) override
    {
        r += e->region();
    }

    void reset()
    {
        r = QRegion();
    }

    void enterEvent(QEvent *) override { ++enters; }
    void leaveEvent(QEvent *) override { ++leaves; }

    void resetCounts()
    {
        enters = 0;
        leaves = 0;
    }

    QColor color;
    QRegion r;
    int enters = 0;
    int leaves = 0;
};

static inline QByteArray msgRgbMismatch(unsigned actual, unsigned expected)
{
    return QByteArrayLiteral("Color mismatch, 0x") + QByteArray::number(actual, 16) +
           QByteArrayLiteral(" != 0x") + QByteArray::number(expected, 16);
}

static QPixmap grabWindow(QWindow *window, int x, int y, int width, int height)
{
    QScreen *screen = window->screen();
    Q_ASSERT(screen);
    return screen->grabWindow(window->winId(), x, y, width, height);
}

#define VERIFY_COLOR(child, region, color) verifyColor(child, region, color, __LINE__)

bool verifyColor(QWidget &child, const QRegion &region, const QColor &color, int callerLine)
{
    QWindow *window = child.window()->windowHandle();
    Q_ASSERT(window);
    const QPoint offset = child.mapTo(child.window(), QPoint(0,0));
    bool grabBackingStore = false;
    for (QRect r : region) {
        QRect rect = r.translated(offset);
        for (int t = 0; t < 6; t++) {
            const QPixmap pixmap = grabBackingStore
                ? child.grab(rect)
                : grabWindow(window, rect.left(), rect.top(), rect.width(), rect.height());
            const QSize actualSize = pixmap.size() / pixmap.devicePixelRatioF();
            if (!QTest::qCompare(actualSize, rect.size(), "pixmap.size()", "rect.size()", __FILE__, callerLine))
                return false;
            QPixmap expectedPixmap(pixmap); /* ensure equal formats */
            expectedPixmap.detach();
            expectedPixmap.fill(color);
            QImage image = pixmap.toImage();
            uint alphaCorrection = image.format() == QImage::Format_RGB32 ? 0xff000000 : 0;
            uint firstPixel = image.pixel(0,0) | alphaCorrection;
            if (t < 5) {
                /* Normal run.
                   If it succeeds: return success
                   If it fails:    do not return, but wait a bit and reiterate (retry)
                */
                if (firstPixel == QColor(color).rgb() && image == expectedPixmap.toImage())
                    return true;
                if (t == 4) {
                    grabBackingStore = true;
                    rect = r;
                } else {
                    QTest::qWait(200);
                }
            } else {
                // Last run, report failure if it still fails
                if (!QTest::qVerify(firstPixel == QColor(color).rgb(),
                                   "firstPixel == QColor(color).rgb()",
                                    qPrintable(msgRgbMismatch(firstPixel, QColor(color).rgb())),
                                    __FILE__, callerLine))
                    return false;
                if (!QTest::qVerify(image == expectedPixmap.toImage(),
                                    "image == expectedPixmap.toImage()",
                                    "grabbed pixmap differs from expected pixmap",
                                    __FILE__, callerLine))
                    return false;
            }
        }
    }
    return true;
}

void tst_QWidget::moveChild_data()
{
    QTest::addColumn<QPoint>("offset");

    QTest::newRow("right") << QPoint(20, 0);
    QTest::newRow("down") << QPoint(0, 20);
    QTest::newRow("left") << QPoint(-20, 0);
    QTest::newRow("up") << QPoint(0, -20);
}

void tst_QWidget::moveChild()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    QFETCH(QPoint, offset);

    ColorWidget parent(nullptr, Qt::Window | Qt::WindowStaysOnTopHint);
    // prevent custom styles
    const QScopedPointer<QStyle> style(QStyleFactory::create(QLatin1String("Windows")));
    parent.setStyle(style.data());
    ColorWidget child(&parent, Qt::Widget, Qt::blue);

    parent.setGeometry(QRect(parent.screen()->availableGeometry().topLeft() + QPoint(50, 50),
                             QSize(200, 200)));
    child.setGeometry(25, 25, 50, 50);
#ifndef QT_NO_CURSOR // Try to make sure the cursor is not in a taskbar area to prevent tooltips or window highlighting
    QCursor::setPos(parent.geometry().topRight() + QPoint(50 , 50));
#endif
    parent.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    QTRY_COMPARE(parent.r, QRegion(parent.rect()) - child.geometry());
    QTRY_COMPARE(child.r, QRegion(child.rect()));

    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT does not support setGeometry (and we cannot use QEXPECT_FAIL because of VERIFY_COLOR)");
    VERIFY_COLOR(child, child.rect(),
                 child.color);
    VERIFY_COLOR(parent, QRegion(parent.rect()) - child.geometry(), parent.color);
    parent.reset();
    child.reset();

    // move

    const QRect oldGeometry = child.geometry();

    QPoint pos = child.pos() + offset;
    child.move(pos);
    QTRY_COMPARE(pos, child.pos());

    QTRY_COMPARE(parent.r, QRegion(oldGeometry) - child.geometry());
#if !defined(Q_OS_MACOS)
    // should be scrolled in backingstore
    QCOMPARE(child.r, QRegion());
#endif
    VERIFY_COLOR(child, child.rect(), child.color);
    VERIFY_COLOR(parent, QRegion(parent.rect()) - child.geometry(), parent.color);
}

void tst_QWidget::showAndMoveChild()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    QWidget parent(nullptr, Qt::Window | Qt::WindowStaysOnTopHint);
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    // prevent custom styles
    const QScopedPointer<QStyle> style(QStyleFactory::create(QLatin1String("Windows")));
    parent.setStyle(style.data());

    QRect desktopDimensions = parent.screen()->availableGeometry();
    desktopDimensions = desktopDimensions.adjusted(64, 64, -64, -64);

#ifndef QT_NO_CURSOR // Try to make sure the cursor is not in a taskbar area to prevent tooltips or window highlighting
    QCursor::setPos(desktopDimensions.topRight() + QPoint(40, 40));
#endif
    parent.setGeometry(desktopDimensions);
    parent.setPalette(Qt::red);
    parent.show();
    QApplication::setActiveWindow(&parent);
    QVERIFY(QTest::qWaitForWindowActive(&parent));

    QWidget child(&parent);
    child.resize(desktopDimensions.width()/2, desktopDimensions.height()/2);
    child.setPalette(Qt::blue);
    child.setAutoFillBackground(true);

    // Ensure that the child is repainted correctly when moved right after show.
    // NB! Do NOT processEvents() (or qWait()) in between show() and move().
    child.show();
    child.move(desktopDimensions.width()/2, desktopDimensions.height()/2);
    QCoreApplication::processEvents();

    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT does not support setGeometry (and we cannot use QEXPECT_FAIL because of VERIFY_COLOR)");
    VERIFY_COLOR(child, child.rect(), Qt::blue);
    VERIFY_COLOR(parent, QRegion(parent.rect()) - child.geometry(), Qt::red);
}


void tst_QWidget::subtractOpaqueSiblings()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974: Cocoa only has rect granularity.");
#endif

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setGeometry(50, 50, 300, 300);

    ColorWidget *large = new ColorWidget(&w, Qt::Widget, Qt::red);
    large->setGeometry(50, 50, 200, 200);

    ColorWidget *medium = new ColorWidget(large, Qt::Widget, Qt::gray);
    medium->setGeometry(50, 50, 100, 100);

    ColorWidget *tall = new ColorWidget(&w, Qt::Widget, Qt::blue);
    tall->setGeometry(100, 30, 50, 100);

    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    large->reset();
    medium->reset();
    tall->reset();

    medium->update();

    // QWidgetPrivate::subtractOpaqueSiblings() should prevent parts of medium
    // to be repainted and tall from be repainted at all.

    QTRY_COMPARE(large->r, QRegion());
    QTRY_COMPARE(tall->r, QRegion());
    QTRY_COMPARE(medium->r.translated(medium->mapTo(&w, QPoint())),
             QRegion(medium->geometry().translated(large->pos()))
             - tall->geometry());
}

void tst_QWidget::deleteStyle()
{
    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.setStyle(QStyleFactory::create(QLatin1String("Windows")));
    widget.show();
    delete widget.style();
    QCoreApplication::processEvents();
}

class TopLevelFocusCheck: public QWidget
{
    Q_OBJECT
public:
    QLineEdit* edit;
    explicit TopLevelFocusCheck(QWidget *parent = nullptr)
        : QWidget(parent), edit(new QLineEdit(this))
    {
        edit->hide();
        edit->installEventFilter(this);
    }

public slots:
    void mouseDoubleClickEvent ( QMouseEvent * /*event*/ ) override
    {
        edit->show();
        edit->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();
    }
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == edit && event->type()== QEvent::FocusOut) {
            edit->hide();
            return true;
        }
        return false;
    }
};

void tst_QWidget::multipleToplevelFocusCheck()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");
    else if (m_platform == QStringLiteral("winrt"))
        QSKIP("Winrt: Sometimes crashes in QTextLayout. - QTBUG-68297");
    TopLevelFocusCheck w1;
    TopLevelFocusCheck w2;

    const QString title = QLatin1String(QTest::currentTestFunction());
    w1.setWindowTitle(title + QLatin1String("_W1"));
    w1.move(m_availableTopLeft + QPoint(20, 20));
    w1.resize(200, 200);
    w1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w1));
    w2.setWindowTitle(title + QLatin1String("_W2"));
    w2.move(w1.frameGeometry().topRight() + QPoint(20, 0));
    w2.resize(200,200);
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));

    QApplication::setActiveWindow(&w1);
    w1.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&w1));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&w1));
    QTest::mouseDClick(&w1, Qt::LeftButton);
    QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget *>(w1.edit));

    w2.activateWindow();
    QApplication::setActiveWindow(&w2);
    QVERIFY(QTest::qWaitForWindowActive(&w2));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&w2));
    QTest::mouseClick(&w2, Qt::LeftButton);
    QTRY_COMPARE(QApplication::focusWidget(), nullptr);

    QTest::mouseDClick(&w2, Qt::LeftButton);
    QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget *>(w2.edit));

    w1.activateWindow();
    QApplication::setActiveWindow(&w1);
    QVERIFY(QTest::qWaitForWindowActive(&w1));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&w1));
    QTest::mouseDClick(&w1, Qt::LeftButton);
    QTRY_COMPARE(QApplication::focusWidget(), static_cast<QWidget *>(w1.edit));

    w2.activateWindow();
    QApplication::setActiveWindow(&w2);
    QVERIFY(QTest::qWaitForWindowActive(&w2));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&w2));
    QTest::mouseClick(&w2, Qt::LeftButton);
    QTRY_COMPARE(QApplication::focusWidget(), nullptr);
}

class FocusWidget: public QWidget
{
protected:
    bool event(QEvent *ev) override
    {
        if (ev->type() == QEvent::FocusAboutToChange)
            widgetDuringFocusAboutToChange = QApplication::focusWidget();
        return QWidget::event(ev);
    }
    void focusInEvent(QFocusEvent *) override
    {
        focusObjectDuringFocusIn = QGuiApplication::focusObject();
        detectedBadEventOrdering = focusObjectDuringFocusIn != mostRecentFocusObjectChange;
    }
    void focusOutEvent(QFocusEvent *) override
    {
        focusObjectDuringFocusOut = QGuiApplication::focusObject();
        widgetDuringFocusOut = QApplication::focusWidget();
        detectedBadEventOrdering = focusObjectDuringFocusOut != mostRecentFocusObjectChange;
    }

    void focusObjectChanged(QObject *focusObject)
    {
        mostRecentFocusObjectChange = focusObject;
    }

public:
    explicit FocusWidget(QWidget *parent) : QWidget(parent)
    {
        connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &FocusWidget::focusObjectChanged);
    }

    QWidget *widgetDuringFocusAboutToChange = nullptr;
    QWidget *widgetDuringFocusOut = nullptr;

    QObject *focusObjectDuringFocusIn = nullptr;
    QObject *focusObjectDuringFocusOut = nullptr;

    QObject *mostRecentFocusObjectChange = nullptr;
    bool detectedBadEventOrdering = false;
};

void tst_QWidget::setFocus()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QScopedPointer<QWidget> testWidget(new QWidget);
    testWidget->resize(m_testWidgetSize);
    testWidget->setWindowTitle(__FUNCTION__);
    centerOnScreen(testWidget.data());
    testWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWidget.data()));

    const QPoint windowPos = testWidget->geometry().topRight() + QPoint(50, 0);
    {
        // move focus to another window
        testWidget->activateWindow();
        QApplication::setActiveWindow(testWidget.data());
        if (testWidget->focusWidget())
            testWidget->focusWidget()->clearFocus();
        else
            testWidget->clearFocus();

        // window and children never shown, nobody gets focus
        QWidget window;
        window.setWindowTitle(QStringLiteral("#1 ") + __FUNCTION__);
        window.resize(m_testWidgetSize);
        window.move(windowPos);

        QWidget child1(&window);
        child1.setFocusPolicy(Qt::StrongFocus);

        QWidget child2(&window);
        child2.setFocusPolicy(Qt::StrongFocus);

        child1.setFocus();
        QVERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), &child1);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child2.setFocus();
        QVERIFY(!child2.hasFocus());
        QCOMPARE(window.focusWidget(), &child2);
        QCOMPARE(QApplication::focusWidget(), nullptr);
    }

    {
        // window and children show, but window not active, nobody gets focus
        QWidget window;
        window.setWindowTitle(QStringLiteral("#2 ") + __FUNCTION__);
        window.resize(m_testWidgetSize);
        window.move(windowPos);

        QWidget child1(&window);
        child1.setFocusPolicy(Qt::StrongFocus);

        QWidget child2(&window);
        child2.setFocusPolicy(Qt::StrongFocus);

        window.show();

        // note: window may be active, but we don't want it to be
        testWidget->activateWindow();
        QApplication::setActiveWindow(testWidget.data());
        if (testWidget->focusWidget())
            testWidget->focusWidget()->clearFocus();
        else
            testWidget->clearFocus();

        child1.setFocus();
        if (m_platform == QStringLiteral("winrt"))
            QEXPECT_FAIL("", "WinRT fails here - QTBUG-68297", Abort);
        QVERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), &child1);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child2.setFocus();
        QVERIFY(!child2.hasFocus());
        QCOMPARE(window.focusWidget(), &child2);
        QCOMPARE(QApplication::focusWidget(), nullptr);
    }

    {
        // window and children show, but window *is* active, children get focus
        QWidget window;
        window.setWindowTitle(QStringLiteral("#3 ") + __FUNCTION__);
        window.resize(m_testWidgetSize);
        window.move(windowPos);

        FocusWidget child1(&window);
        child1.setFocusPolicy(Qt::StrongFocus);

        QWidget child2(&window);
        child2.setFocusPolicy(Qt::StrongFocus);

        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(QGuiApplication::focusWindow());

        child1.setFocus();
        QTRY_VERIFY(child1.hasFocus());
        QCOMPARE(window.focusWidget(), &child1);
        QCOMPARE(QApplication::focusWidget(), &child1);

        child2.setFocus();
        QVERIFY(child2.hasFocus());
        QCOMPARE(window.focusWidget(), &child2);
        QCOMPARE(QApplication::focusWidget(), &child2);

        // focus changed in between the events
        QCOMPARE(child1.widgetDuringFocusAboutToChange, &child1);
        QCOMPARE(child1.widgetDuringFocusOut, &child2);
    }

    {
        // window shown and active, children created, don't get focus, but get focus when shown
        QWidget window;
        window.setWindowTitle(QStringLiteral("#4 ") + __FUNCTION__);
        window.resize(m_testWidgetSize);
        window.move(windowPos);

        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(QGuiApplication::focusWindow());

        QWidget child1(&window);
        child1.setFocusPolicy(Qt::StrongFocus);

        QWidget child2(&window);
        child2.setFocusPolicy(Qt::StrongFocus);

        child1.setFocus();
        QVERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child1.show();
        QApplication::processEvents();
        QTRY_VERIFY(child1.hasFocus());
        QCOMPARE(window.focusWidget(), &child1);
        QCOMPARE(QApplication::focusWidget(), &child1);

        child2.setFocus();
        QVERIFY(!child2.hasFocus());
        QCOMPARE(window.focusWidget(), &child1);
        QCOMPARE(QApplication::focusWidget(), &child1);

        child2.show();
        QVERIFY(child2.hasFocus());
        QCOMPARE(window.focusWidget(), &child2);
        QCOMPARE(QApplication::focusWidget(), &child2);
    }

    {
        // window shown and active, children created, don't get focus,
        // even after setFocus(), hide(), then show()
        QWidget window;
        window.setWindowTitle(QStringLiteral("#5 ") + __FUNCTION__);
        window.resize(m_testWidgetSize);
        window.move(windowPos);

        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(QGuiApplication::focusWindow());

        QWidget child1(&window);
        child1.setFocusPolicy(Qt::StrongFocus);

        QWidget child2(&window);
        child2.setFocusPolicy(Qt::StrongFocus);

        child1.setFocus();
        QVERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child1.hide();
        QVERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child1.show();
        QVERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child2.setFocus();
        QVERIFY(!child2.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child2.hide();
        QVERIFY(!child2.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);

        child2.show();
        QVERIFY(!child2.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);
    }

    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(m_testWidgetSize);
        window.move(windowPos);

        FocusWidget child1(&window);
        QWidget child2(&window);

        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(QApplication::focusWindow());

        QCOMPARE(QApplication::focusObject(), &window);

        child1.setFocus();
        QTRY_VERIFY(child1.hasFocus());
        QCOMPARE(window.focusWidget(), &child1);
        QCOMPARE(QApplication::focusWidget(), &child1);
        QCOMPARE(QApplication::focusObject(), &child1);
        QCOMPARE(child1.focusObjectDuringFocusIn, &child1);
        QVERIFY2(!child1.detectedBadEventOrdering,
            "focusObjectChanged should be delivered before widget focus events on setFocus");

        child1.clearFocus();
        QTRY_VERIFY(!child1.hasFocus());
        QCOMPARE(window.focusWidget(), nullptr);
        QCOMPARE(QApplication::focusWidget(), nullptr);
        QCOMPARE(QApplication::focusObject(), &window);
        QVERIFY(child1.focusObjectDuringFocusOut != &child1);
        QVERIFY2(!child1.detectedBadEventOrdering,
            "focusObjectChanged should be delivered before widget focus events on clearFocus");
    }
}

template<class T> class EventSpy : public QObject
{
public:
    EventSpy(T *widget, QEvent::Type event)
        : m_widget(widget), eventToSpy(event)
    {
        if (m_widget)
            m_widget->installEventFilter(this);
    }

    T *widget() const { return m_widget; }
    int count() const { return m_count; }
    void clear() { m_count = 0; }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == eventToSpy)
            ++m_count;
        return  QObject::eventFilter(object, event);
    }

private:
    T *m_widget;
    const QEvent::Type eventToSpy;
    int m_count = 0;
};

#ifndef QT_NO_CURSOR
void tst_QWidget::setCursor()
{
    {
        QWidget window;
        window.resize(200, 200);
        QWidget child(&window);

        QVERIFY(!window.testAttribute(Qt::WA_SetCursor));
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));

        window.setCursor(window.cursor());
        QVERIFY(window.testAttribute(Qt::WA_SetCursor));
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window.cursor().shape());
    }

    // do it again, but with window show()n
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget child(&window);
        window.show();

        QVERIFY(!window.testAttribute(Qt::WA_SetCursor));
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));

        window.setCursor(window.cursor());
        QVERIFY(window.testAttribute(Qt::WA_SetCursor));
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window.cursor().shape());
    }


    {
        QWidget window;
        window.resize(200, 200);
        QWidget child(&window);

        window.setCursor(Qt::WaitCursor);
        QVERIFY(window.testAttribute(Qt::WA_SetCursor));
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window.cursor().shape());
    }

    // same thing again, just with window show()n
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget child(&window);

        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.setCursor(Qt::WaitCursor);
        QVERIFY(window.testAttribute(Qt::WA_SetCursor));
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window.cursor().shape());
    }

    // reparenting child should not cause the WA_SetCursor to become set
    {
        QWidget window;
        window.resize(200, 200);
        QWidget window2;
        window2.resize(200, 200);
        QWidget child(&window);

        window.setCursor(Qt::WaitCursor);

        child.setParent(nullptr);
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), QCursor().shape());

        child.setParent(&window2);
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window2.cursor().shape());

            window2.setCursor(Qt::WaitCursor);
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window2.cursor().shape());
    }

    // again, with windows show()n
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        QWidget window2;
        window2.resize(200, 200);
        QWidget child(&window);

        window.setCursor(Qt::WaitCursor);
        window.show();

        child.setParent(nullptr);
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), QCursor().shape());

        child.setParent(&window2);
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window2.cursor().shape());

        window2.show();
        window2.setCursor(Qt::WaitCursor);
        QVERIFY(!child.testAttribute(Qt::WA_SetCursor));
        QCOMPARE(child.cursor().shape(), window2.cursor().shape());
    }

    // test if CursorChange is sent
    {
        QWidget widget;
        EventSpy<QWidget> spy(&widget, QEvent::CursorChange);
        QCOMPARE(spy.count(), 0);
        widget.setCursor(QCursor(Qt::WaitCursor));
        QCOMPARE(spy.count(), 1);
        widget.unsetCursor();
        QCOMPARE(spy.count(), 2);
    }
}
#endif

void tst_QWidget::setToolTip()
{
    QWidget widget;
    widget.resize(200, 200);
    // Showing the widget is not required for the tooltip event count test
    // to work. It should just prevent the application from becoming inactive
    // which would cause it to close all popups, interfering with the test
    // in the loop below.
    widget.setObjectName(QLatin1String("tst_qwidget setToolTip"));
    widget.setWindowTitle(widget.objectName());
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    EventSpy<QWidget> spy(&widget, QEvent::ToolTipChange);
    QCOMPARE(spy.count(), 0);

    QCOMPARE(widget.toolTip(), QString());
    widget.setToolTip(QString("Hello"));
    QCOMPARE(widget.toolTip(), QString("Hello"));
    QCOMPARE(spy.count(), 1);
    widget.setToolTip(QString());
    QCOMPARE(widget.toolTip(), QString());
    QCOMPARE(spy.count(), 2);

    for (int pass = 0; pass < 2; ++pass) {
        QCursor::setPos(m_safeCursorPos);
        QScopedPointer<QWidget> popup(new QWidget(nullptr, Qt::Popup));
        popup->setObjectName(QLatin1String("tst_qwidget setToolTip #") + QString::number(pass));
        popup->setWindowTitle(popup->objectName());
        popup->setGeometry(50, 50, 150, 50);
        QFrame *frame = new QFrame(popup.data());
        frame->setGeometry(0, 0, 50, 50);
        frame->setFrameStyle(QFrame::Box | QFrame::Plain);
        EventSpy<QWidget> spy1(frame, QEvent::ToolTip);
        EventSpy<QWidget> spy2(popup.data(), QEvent::ToolTip);
        frame->setMouseTracking(pass != 0);
        frame->setToolTip(QLatin1String("TOOLTIP FRAME"));
        popup->setToolTip(QLatin1String("TOOLTIP POPUP"));
        popup->show();
        QVERIFY(QTest::qWaitForWindowExposed(popup.data()));
        QWindow *popupWindow = popup->windowHandle();
        QTest::qWait(10);
        QTest::mouseMove(popupWindow, QPoint(25, 25));
        QTest::qWait(900);          // delay is 700

        QCOMPARE(spy1.count(), 1);
        QCOMPARE(spy2.count(), 0);
        if (pass == 0)
            QTest::qWait(2200);     // delay is 2000
        QTest::mouseMove(popupWindow);
    }

    QTRY_COMPARE(QApplication::topLevelWidgets().size(), 1);
}

void tst_QWidget::testWindowIconChangeEventPropagation()
{
    typedef QSharedPointer<EventSpy<QWidget> > EventSpyPtr;
    typedef QSharedPointer<EventSpy<QWindow> > WindowEventSpyPtr;
    // Create widget hierarchy.
    QWidget topLevelWidget;
    topLevelWidget.setWindowTitle(QStringLiteral("TopLevel ") + __FUNCTION__);
    topLevelWidget.resize(m_testWidgetSize);
    topLevelWidget.move(m_availableTopLeft + QPoint(100, 100));
    QWidget topLevelChild(&topLevelWidget);

    QDialog dialog(&topLevelWidget);
    dialog.resize(m_testWidgetSize);
    dialog.move(topLevelWidget.geometry().topRight() + QPoint(100, 0));
    dialog.setWindowTitle(QStringLiteral("Dialog ") + __FUNCTION__);
    QWidget dialogChild(&dialog);

    QWidgetList widgets;
    widgets << &topLevelWidget << &topLevelChild
            << &dialog << &dialogChild;
    QCOMPARE(widgets.count(), 4);

    topLevelWidget.show();
    dialog.show();

    QWindowList windows;
    windows << topLevelWidget.windowHandle() << dialog.windowHandle();
    QWindow otherWindow;
    windows << &otherWindow;
    const int lastWidgetWindow = 1; // 0 and 1 are qwidgetwindow, 2 is a pure qwindow

    // Create spy lists.
    QList <EventSpyPtr> applicationEventSpies;
    QList <EventSpyPtr> widgetEventSpies;
    for (QWidget *widget : qAsConst(widgets)) {
        applicationEventSpies.append(EventSpyPtr::create(widget, QEvent::ApplicationWindowIconChange));
        widgetEventSpies.append(EventSpyPtr::create(widget, QEvent::WindowIconChange));
    }
    QList <WindowEventSpyPtr> appWindowEventSpies;
    QList <WindowEventSpyPtr> windowEventSpies;
    for (QWindow *window : qAsConst(windows)) {
        appWindowEventSpies.append(WindowEventSpyPtr::create(window, QEvent::ApplicationWindowIconChange));
        windowEventSpies.append(WindowEventSpyPtr::create(window, QEvent::WindowIconChange));
    }

    // QApplication::setWindowIcon
    const QIcon windowIcon = qApp->style()->standardIcon(QStyle::SP_TitleBarMenuButton);
    qApp->setWindowIcon(windowIcon);

    for (int i = 0; i < widgets.count(); ++i) {
        // Check QEvent::ApplicationWindowIconChange
        EventSpyPtr spy = applicationEventSpies.at(i);
        QWidget *widget = spy->widget();
        if (widget->isWindow()) {
            QCOMPARE(spy->count(), 1);
            QCOMPARE(widget->windowIcon(), windowIcon);
        } else {
            QCOMPARE(spy->count(), 0);
        }
        spy->clear();

        // Check QEvent::WindowIconChange
        spy = widgetEventSpies.at(i);
        QCOMPARE(spy->count(), 1);
        spy->clear();
    }
    for (int i = 0; i < windows.count(); ++i) {
        // Check QEvent::ApplicationWindowIconChange (sent to QWindow)
        // QWidgetWindows don't get this event, since the widget takes care of changing the icon
        WindowEventSpyPtr spy = appWindowEventSpies.at(i);
        QWindow *window = spy->widget();
        QCOMPARE(spy->count(), i > lastWidgetWindow ? 1 : 0);
        QCOMPARE(window->icon(), windowIcon);
        spy->clear();

        // Check QEvent::WindowIconChange (sent to QWindow)
        spy = windowEventSpies.at(i);
        QCOMPARE(spy->count(), 1);
        spy->clear();
    }

    // Set icon on a top-level widget.
    topLevelWidget.setWindowIcon(QIcon());

    for (int i = 0; i < widgets.count(); ++i) {
        // Check QEvent::ApplicationWindowIconChange
        EventSpyPtr spy = applicationEventSpies.at(i);
        QCOMPARE(spy->count(), 0);
        spy->clear();

        // Check QEvent::WindowIconChange
        spy = widgetEventSpies.at(i);
        QWidget *widget = spy->widget();
        if (widget == &topLevelWidget) {
            QCOMPARE(widget->windowIcon(), QIcon());
            QCOMPARE(spy->count(), 1);
        } else if (topLevelWidget.isAncestorOf(widget)) {
            QCOMPARE(spy->count(), 1);
        } else {
            QCOMPARE(spy->count(), 0);
        }
        spy->clear();
    }

    // Cleanup.
    qApp->setWindowIcon(QIcon());
}

void tst_QWidget::minAndMaxSizeWithX11BypassWindowManagerHint()
{
    if (m_platform != QStringLiteral("xcb"))
        QSKIP("This test is for X11 only.");
    // Same size as in QWidgetPrivate::create.
    const QSize desktopSize = QApplication::desktop()->size();
    const QSize originalSize(desktopSize.width() / 2, desktopSize.height() * 4 / 10);

    { // Maximum size.
    QWidget widget(nullptr, Qt::X11BypassWindowManagerHint);
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));

    const QSize newMaximumSize = widget.size().boundedTo(originalSize) - QSize(10, 10);
    widget.setMaximumSize(newMaximumSize);
    QCOMPARE(widget.size(), newMaximumSize);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.size(), newMaximumSize);
    }

    { // Minimum size.
    QWidget widget(nullptr, Qt::X11BypassWindowManagerHint);
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));

    const QSize newMinimumSize = widget.size().expandedTo(originalSize) + QSize(10, 10);
    widget.setMinimumSize(newMinimumSize);
    QCOMPARE(widget.size(), newMinimumSize);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.size(), newMinimumSize);
    }
}

class ShowHideShowWidget : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

    int state = 0;
public:
    bool gotExpectedMapNotify = false;
    bool gotExpectedGlobalEvent = false;

    ShowHideShowWidget()
    {
        startTimer(1000);
    }

    void timerEvent(QTimerEvent *) override
    {
        switch (state++) {
        case 0:
            show();
            break;
        case 1:
            emit done();
            break;
        }
    }

    bool isMapNotify(const QByteArray &eventType, void *message)
    {
        enum { XCB_MAP_NOTIFY = 19 };
        if (state == 1 && eventType == QByteArrayLiteral("xcb_generic_event_t")) {
            // XCB events have a uint8 response_type member at the beginning.
            const auto responseType = *reinterpret_cast<const unsigned char *>(message);
            return ((responseType & ~0x80) == XCB_MAP_NOTIFY);
        }
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *) override
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *) override
#endif
    {
        if (isMapNotify(eventType, message))
            gotExpectedMapNotify = true;
        return false;
    }

    // QAbstractNativeEventFilter interface
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *) override
#endif
    {
        if (isMapNotify(eventType, message))
            gotExpectedGlobalEvent = true;
        return false;
    }

signals:
    void done();
};

void tst_QWidget::showHideShowX11()
{
    if (m_platform != QStringLiteral("xcb"))
        QSKIP("This test is for X11 only.");

    ShowHideShowWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    qApp->installNativeEventFilter(&w);

    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    w.hide();

    QEventLoop eventLoop;
    connect(&w, &ShowHideShowWidget::done, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QVERIFY(w.gotExpectedGlobalEvent);
    QVERIFY(w.gotExpectedMapNotify);
}

void tst_QWidget::clean_qt_x11_enforce_cursor()
{
    if (m_platform != QStringLiteral("xcb"))
        QSKIP("This test is for X11 only.");

    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        QWidget *w = new QWidget(&window);
        QWidget *child = new QWidget(w);
        child->setAttribute(Qt::WA_SetCursor, true);

        window.show();
        QApplication::setActiveWindow(&window);
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::qWait(100);
        QCursor::setPos(window.geometry().center());
        QTest::qWait(100);

        child->setFocus();
        QApplication::processEvents();
        QTest::qWait(100);

        delete w;
    }

    QGraphicsScene scene;
    QLineEdit *edit = new QLineEdit;
    scene.addWidget(edit);

    // If the test didn't crash, then it passed.
}

class EventRecorder : public QObject
{
    Q_OBJECT

public:
    typedef QPair<QWidget *, QEvent::Type> WidgetEventTypePair;
    typedef QList<WidgetEventTypePair> EventList;

    using QObject::QObject;

    EventList eventList() const
    {
        return events;
    }

    void clear()
    {
        events.clear();
    }

    bool eventFilter(QObject *object, QEvent *event) override
    {
        QWidget *widget = qobject_cast<QWidget *>(object);
        if (widget && !event->spontaneous())
            events.append(qMakePair(widget, event->type()));
        return false;
    }

    static QByteArray msgEventListMismatch(const EventList &expected, const EventList &actual);
    static QByteArray msgExpectFailQtBug26424(const EventList &expected, const EventList &actual)
    { return QByteArrayLiteral("QTBUG-26424: ") + msgEventListMismatch(expected, actual); }

private:
    static inline void formatEventList(const EventList &l, QDebug &d);

    EventList events;
};

void EventRecorder::formatEventList(const EventList &l, QDebug &d)
{
    QWidget *lastWidget = nullptr;
    for (const WidgetEventTypePair &p : l) {
        if (p.first != lastWidget) {
            d << p.first << ':';
            lastWidget = p.first;
        }
        d << p.second << ' ';
    }
}

QByteArray EventRecorder::msgEventListMismatch(const EventList &expected, const EventList &actual)
{
    QString result;
    QDebug d = QDebug(&result).nospace();
    d << "Event list mismatch, expected " << expected.size() << " (";
    EventRecorder::formatEventList(expected, d);
    d << "), actual " << actual.size() << " (";
    EventRecorder::formatEventList(actual, d);
    d << ')';
    return result.toLocal8Bit();
}

void tst_QWidget::childEvents()
{
    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT: This fails. QTBUG-68297.");
    EventRecorder::EventList expected;

    // Move away the cursor; otherwise it might result in an enter event if it's
    // inside the widget when the widget is shown.
    QCursor::setPos(m_safeCursorPos);
    QTest::qWait(100);

    {
        // no children created, not shown
        QWidget widget;
        widget.resize(200, 200);
        EventRecorder spy;
        widget.installEventFilter(&spy);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 1)));

        QCoreApplication::sendPostedEvents();

        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::PolishRequest)
            << qMakePair(&widget, QEvent::Polish)
            << qMakePair(&widget, QEvent::Type(QEvent::User + 1));
        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
    }

    {
        // no children, shown
        QWidget widget;
        widget.resize(200, 200);
        EventRecorder spy;
        widget.installEventFilter(&spy);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 1)));

        widget.showNormal();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::Polish)
            << qMakePair(&widget, QEvent::PlatformSurface)
            << qMakePair(&widget, QEvent::WinIdChange)
            << qMakePair(&widget, QEvent::WindowIconChange)
            << qMakePair(&widget, QEvent::Move)
            << qMakePair(&widget, QEvent::Resize)
            << qMakePair(&widget, QEvent::Show)
            << qMakePair(&widget, QEvent::CursorChange)
            << qMakePair(&widget, QEvent::ShowToParent);

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
        spy.clear();

        QCoreApplication::sendPostedEvents();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::PolishRequest)
            << qMakePair(&widget, QEvent::Type(QEvent::User + 1))
            << qMakePair(&widget, QEvent::UpdateLater)
            << qMakePair(&widget, QEvent::UpdateRequest);

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
    }

    {
        // 2 children, not shown
        QWidget widget;
        widget.resize(200, 200);
        EventRecorder spy;
        widget.installEventFilter(&spy);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 1)));

        QWidget child1(&widget);
        QWidget child2;
        child2.setParent(&widget);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 2)));

        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::ChildAdded)
            << qMakePair(&widget, QEvent::ChildAdded);
        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
        spy.clear();

        QCoreApplication::sendPostedEvents();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::PolishRequest)
            << qMakePair(&widget, QEvent::Polish)
            << qMakePair(&widget, QEvent::ChildPolished)
            << qMakePair(&widget, QEvent::ChildPolished)
            << qMakePair(&widget, QEvent::Type(QEvent::User + 1))
            << qMakePair(&widget, QEvent::Type(QEvent::User + 2));
        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
    }

    {
        // 2 children, widget shown
        QWidget widget;
        widget.resize(200, 200);
        EventRecorder spy;
        widget.installEventFilter(&spy);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 1)));

        QWidget child1(&widget);
        QWidget child2;
        child2.setParent(&widget);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 2)));

        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::ChildAdded)
            << qMakePair(&widget, QEvent::ChildAdded);
        QCOMPARE(spy.eventList(), expected);
        spy.clear();

        widget.showNormal();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::Polish)
            << qMakePair(&widget, QEvent::ChildPolished)
            << qMakePair(&widget, QEvent::ChildPolished)
            << qMakePair(&widget, QEvent::PlatformSurface)
            << qMakePair(&widget, QEvent::WinIdChange)
            << qMakePair(&widget, QEvent::WindowIconChange)
            << qMakePair(&widget, QEvent::Move)
            << qMakePair(&widget, QEvent::Resize)
            << qMakePair(&widget, QEvent::Show)
            << qMakePair(&widget, QEvent::CursorChange)
            << qMakePair(&widget, QEvent::ShowToParent);

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
        spy.clear();

        QCoreApplication::sendPostedEvents();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::PolishRequest)
            << qMakePair(&widget, QEvent::Type(QEvent::User + 1))
            << qMakePair(&widget, QEvent::Type(QEvent::User + 2))
            << qMakePair(&widget, QEvent::UpdateLater)
            << qMakePair(&widget, QEvent::UpdateRequest);

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
    }

    {
        // 2 children, but one is reparented away, not shown
        QWidget widget;
        widget.resize(200, 200);
        EventRecorder spy;
        widget.installEventFilter(&spy);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 1)));

        QWidget child1(&widget);
        QWidget child2;
        child2.setParent(&widget);
        child2.setParent(nullptr);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 2)));

        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::ChildAdded)
            << qMakePair(&widget, QEvent::ChildAdded)
            << qMakePair(&widget, QEvent::ChildRemoved);
        QCOMPARE(spy.eventList(), expected);
        spy.clear();

        QCoreApplication::sendPostedEvents();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::PolishRequest)
            << qMakePair(&widget, QEvent::Polish)
            << qMakePair(&widget, QEvent::ChildPolished)
            << qMakePair(&widget, QEvent::Type(QEvent::User + 1))
            << qMakePair(&widget, QEvent::Type(QEvent::User + 2));

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
    }

    {
        // 2 children, but one is reparented away, then widget is shown
        QWidget widget;
        widget.resize(200, 200);
        EventRecorder spy;
        widget.installEventFilter(&spy);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 1)));

        QWidget child1(&widget);
        QWidget child2;
        child2.setParent(&widget);
        child2.setParent(nullptr);

        QCoreApplication::postEvent(&widget, new QEvent(QEvent::Type(QEvent::User + 2)));

        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::ChildAdded)
            << qMakePair(&widget, QEvent::ChildAdded)
            << qMakePair(&widget, QEvent::ChildRemoved);
        QCOMPARE(spy.eventList(), expected);
        spy.clear();

        widget.showNormal();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::Polish)
            << qMakePair(&widget, QEvent::ChildPolished)
            << qMakePair(&widget, QEvent::PlatformSurface)
            << qMakePair(&widget, QEvent::WinIdChange)
            << qMakePair(&widget, QEvent::WindowIconChange)
            << qMakePair(&widget, QEvent::Move)
            << qMakePair(&widget, QEvent::Resize)
            << qMakePair(&widget, QEvent::Show)
            << qMakePair(&widget, QEvent::CursorChange)
            << qMakePair(&widget, QEvent::ShowToParent);

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
        spy.clear();

        QCoreApplication::sendPostedEvents();
        expected =
            EventRecorder::EventList()
            << qMakePair(&widget, QEvent::PolishRequest)
            << qMakePair(&widget, QEvent::Type(QEvent::User + 1))
            << qMakePair(&widget, QEvent::Type(QEvent::User + 2))
            << qMakePair(&widget, QEvent::UpdateLater)
            << qMakePair(&widget, QEvent::UpdateRequest);

        QVERIFY2(spy.eventList() == expected,
                 EventRecorder::msgEventListMismatch(expected, spy.eventList()).constData());
    }
}

class RenderWidget : public QWidget
{
public:
    RenderWidget(QWidget *source)
        : source(source), ellipse(false) {}

    void setEllipseEnabled(bool enable = true)
    {
        ellipse = enable;
        update();
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        if (ellipse) {
            QPainter painter(this);
            painter.fillRect(rect(), Qt::red);
            painter.end();
            QRegion regionToRender = QRegion(0, 0, source->width(), source->height() / 2,
                                             QRegion::Ellipse);
            source->render(this, QPoint(0, 30), regionToRender);
        } else {
            source->render(this);
        }
    }

private:
    QWidget *source;
    bool ellipse;
};

void tst_QWidget::render()
{
    return;
    QCalendarWidget source;
    source.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    // disable anti-aliasing to eliminate potential differences when subpixel antialiasing
    // is enabled on the screen
    QFont f;
    f.setStyleStrategy(QFont::NoAntialias);
    source.setFont(f);
    source.show();
    QVERIFY(QTest::qWaitForWindowExposed(&source));

    // Render the entire source into target.
    RenderWidget target(&source);
    target.resize(source.size());
    target.show();

    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents();
    QTest::qWait(250);

    const QImage sourceImage = source.grab(QRect(QPoint(0, 0), QSize(-1, -1))).toImage();
    QCoreApplication::processEvents();
    QImage targetImage = target.grab(QRect(QPoint(0, 0), QSize(-1, -1))).toImage();
    QCoreApplication::processEvents();
    QCOMPARE(sourceImage, targetImage);

    // Fill target.rect() will Qt::red and render
    // QRegion(0, 0, source->width(), source->height() / 2, QRegion::Ellipse)
    // of source into target with offset (0, 30).
    target.setEllipseEnabled();
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents();

    targetImage = target.grab(QRect(QPoint(0, 0), QSize(-1, -1))).toImage();
    QVERIFY(sourceImage != targetImage);

    QCOMPARE(targetImage.pixel(target.width() / 2, 29), QColor(Qt::red).rgb());
    QCOMPARE(targetImage.pixel(target.width() / 2, 30), sourceImage.pixel(source.width() / 2, 0));

    // Test that a child widget properly fills its background
    {
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(100, 100);
        // prevent custom styles
        window.setStyle(QStyleFactory::create(QLatin1String("Windows")));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QWidget child(&window);
        child.resize(window.size());
        child.show();

        QCoreApplication::processEvents();
        const QPixmap childPixmap = child.grab(QRect(QPoint(0, 0), QSize(-1, -1)));
        const QPixmap windowPixmap = window.grab(QRect(QPoint(0, 0), QSize(-1, -1)));
        QCOMPARE(childPixmap, windowPixmap);
    }

    { // Check that the target offset is correct.
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        widget.resize(200, 200);
        widget.setAutoFillBackground(true);
        widget.setPalette(Qt::red);
        // prevent custom styles
        widget.setStyle(QStyleFactory::create(QLatin1String("Windows")));
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QImage image(widget.size(), QImage::Format_RGB32);
        image.fill(QColor(Qt::blue).rgb());

        // Target offset (0, 0)
        widget.render(&image, QPoint(), QRect(20, 20, 100, 100));
        QCOMPARE(image.pixel(0, 0), QColor(Qt::red).rgb());
        QCOMPARE(image.pixel(99, 99), QColor(Qt::red).rgb());
        QCOMPARE(image.pixel(100, 100), QColor(Qt::blue).rgb());

        // Target offset (20, 20).
        image.fill(QColor(Qt::blue).rgb());
        widget.render(&image, QPoint(20, 20), QRect(20, 20, 100, 100));
        QCOMPARE(image.pixel(0, 0), QColor(Qt::blue).rgb());
        QCOMPARE(image.pixel(19, 19), QColor(Qt::blue).rgb());
        QCOMPARE(image.pixel(20, 20), QColor(Qt::red).rgb());
        QCOMPARE(image.pixel(119, 119), QColor(Qt::red).rgb());
        QCOMPARE(image.pixel(120, 120), QColor(Qt::blue).rgb());
    }
}

// On Windows the active palette is used instead of the inactive palette even
// though the widget is invisible. This is probably related to task 178507/168682,
// but for the renderInvisible test it doesn't matter, we're mostly interested
// in testing the geometry so just workaround the palette issue for now.
static void workaroundPaletteIssue(QWidget *widget)
{
#ifndef Q_OS_WIN
    return;
#endif
    if (!widget)
        return;

    QWidget *navigationBar = widget->findChild<QWidget *>(QLatin1String("qt_calendar_navigationbar"));
    QVERIFY(navigationBar);

    QPalette palette = navigationBar->palette();
    const QColor background = palette.color(QPalette::Inactive, navigationBar->backgroundRole());
    const QColor highlightedText = palette.color(QPalette::Inactive, QPalette::HighlightedText);
    palette.setColor(QPalette::Active, navigationBar->backgroundRole(), background);
    palette.setColor(QPalette::Active, QPalette::HighlightedText, highlightedText);
    navigationBar->setPalette(palette);
}

//#define RENDER_DEBUG
void tst_QWidget::renderInvisible()
{
    if (m_platform == QStringLiteral("xcb"))
        QSKIP("QTBUG-26424");
    if (m_platform == QStringLiteral("winrt"))
        QSKIP("WinRT: This fails. QTBUG-68297.");

    QScopedPointer<QCalendarWidget> calendar(new QCalendarWidget);
    calendar->move(m_availableTopLeft + QPoint(100, 100));
    calendar->setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    // disable anti-aliasing to eliminate potential differences when subpixel antialiasing
    // is enabled on the screen
    QFont f;
    f.setStyleStrategy(QFont::NoAntialias);
    calendar->setFont(f);
    calendar->showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(calendar.data()));

    // Create a dummy focus widget to get rid of focus rect in reference image.
    QLineEdit dummyFocusWidget;
    dummyFocusWidget.setMinimumWidth(m_testWidgetSize.width());
    dummyFocusWidget.move(calendar->geometry().bottomLeft() + QPoint(0, 100));
    dummyFocusWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dummyFocusWidget));
    QCoreApplication::processEvents();
    QTest::qWait(120);

    // Create normal reference image.
    const QSize calendarSize = calendar->size();
    QImage referenceImage(calendarSize, QImage::Format_ARGB32);
    calendar->render(&referenceImage);
#ifdef RENDER_DEBUG
    referenceImage.save("referenceImage.png");
#endif
    QVERIFY(!referenceImage.isNull());

    // Create resized reference image.
    const QSize calendarSizeResized = calendar->size() + QSize(50, 50);
    calendar->resize(calendarSizeResized);
    QCoreApplication::processEvents();
    QTest::qWait(30);
    QImage referenceImageResized(calendarSizeResized, QImage::Format_ARGB32);
    calendar->render(&referenceImageResized);
#ifdef RENDER_DEBUG
    referenceImageResized.save("referenceImageResized.png");
#endif
    QVERIFY(!referenceImageResized.isNull());

    // Explicitly hide the calendar.
    calendar->hide();
    QCoreApplication::processEvents();
    QTest::qWait(30);
    workaroundPaletteIssue(calendar.data());

    { // Make sure we get the same image when the calendar is explicitly hidden.
    QImage testImage(calendarSizeResized, QImage::Format_ARGB32);
    calendar->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("explicitlyHiddenCalendarResized.png");
#endif
    QCOMPARE(testImage, referenceImageResized);
    }

    // Now that we have reference images we can delete the source and re-create
    // the calendar and check that we get the same images from a calendar which has never
    // been visible, laid out or created (Qt::WA_WState_Created).
    calendar.reset(new QCalendarWidget);
    calendar->setFont(f);
    workaroundPaletteIssue(calendar.data());

    { // Never been visible, created or laid out.
    QImage testImage(calendarSize, QImage::Format_ARGB32);
    calendar->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("neverBeenVisibleCreatedOrLaidOut.png");
#endif
    QCOMPARE(testImage, referenceImage);
    }

    calendar->hide();
    QCoreApplication::processEvents();
    QTest::qWait(30);

    { // Calendar explicitly hidden.
    QImage testImage(calendarSize, QImage::Format_ARGB32);
    calendar->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("explicitlyHiddenCalendar.png");
#endif
    QCOMPARE(testImage, referenceImage);
    }

    // Get navigation bar and explicitly hide it.
    QWidget *navigationBar = calendar.data()->findChild<QWidget *>(QLatin1String("qt_calendar_navigationbar"));
    QVERIFY(navigationBar);
    navigationBar->hide();

    { // Check that the navigation bar isn't drawn when rendering the entire calendar.
    QImage testImage(calendarSize, QImage::Format_ARGB32);
    calendar->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("calendarWithoutNavigationBar.png");
#endif
    QVERIFY(testImage != referenceImage);
    }

    { // Make sure the navigation bar renders correctly even though it's hidden.
    QImage testImage(navigationBar->size(), QImage::Format_ARGB32);
    navigationBar->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("explicitlyHiddenNavigationBar.png");
#endif
    QCOMPARE(testImage, referenceImage.copy(navigationBar->rect()));
    }

    // Get next month button.
    QWidget *nextMonthButton = navigationBar->findChild<QWidget *>(QLatin1String("qt_calendar_nextmonth"));
    QVERIFY(nextMonthButton);

    { // Render next month button.
    // Fill test image with correct background color.
    QImage testImage(nextMonthButton->size(), QImage::Format_ARGB32);
    navigationBar->render(&testImage, QPoint(), QRegion(), QWidget::RenderFlags());
#ifdef RENDER_DEBUG
    testImage.save("nextMonthButtonBackground.png");
#endif

    // Set the button's background color to Qt::transparent; otherwise it will fill the
    // background with QPalette::Window.
    const QPalette originalPalette = nextMonthButton->palette();
    QPalette palette = originalPalette;
    palette.setColor(QPalette::Window, Qt::transparent);
    nextMonthButton->setPalette(palette);

    // Render the button on top of the background.
    nextMonthButton->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("nextMonthButton.png");
#endif
    const QRect buttonRect(nextMonthButton->mapTo(calendar.data(), QPoint()), nextMonthButton->size());
    QCOMPARE(testImage, referenceImage.copy(buttonRect));

    // Restore palette.
    nextMonthButton->setPalette(originalPalette);
    }

    // Navigation bar isn't explicitly hidden anymore.
    navigationBar->show();
    QCoreApplication::processEvents();
    QTest::qWait(30);
    QVERIFY(!calendar->isVisible());

    // Now, completely mess up the layout. This will trigger an update on the layout
    // when the calendar is visible or shown, but it's not. QWidget::render must therefore
    // make sure the layout is activated before rendering.
    QVERIFY(!calendar->isVisible());
    calendar->resize(calendarSizeResized);
    QCoreApplication::processEvents();

    { // Make sure we get an image equal to the resized reference image.
    QImage testImage(calendarSizeResized, QImage::Format_ARGB32);
    calendar->render(&testImage);
#ifdef RENDER_DEBUG
    testImage.save("calendarResized.png");
#endif
    QCOMPARE(testImage, referenceImageResized);
    }

    { // Make sure we lay out the widget correctly the first time it's rendered.
    QCalendarWidget calendar;
    const QSize calendarSize = calendar.sizeHint();

    QImage image(2 * calendarSize, QImage::Format_ARGB32);
    image.fill(QColor(Qt::red).rgb());
    calendar.render(&image);

    for (int i = calendarSize.height(); i < 2 * calendarSize.height(); ++i)
        for (int j = calendarSize.width(); j < 2 * calendarSize.width(); ++j)
            QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
    }

    { // Ensure that we don't call adjustSize() on invisible top-levels if render() is called
      // right after widgets have been added/removed to/from its layout.
    QWidget topLevel;
    topLevel.setLayout(new QVBoxLayout);

    QWidget *widget = new QLineEdit;
    topLevel.layout()->addWidget(widget);

    const QSize initialSize = topLevel.size();
    QPixmap pixmap(topLevel.sizeHint());
    topLevel.render(&pixmap); // triggers adjustSize()
    const QSize finalSize = topLevel.size();
    QVERIFY2(finalSize != initialSize,
             msgComparisonFailed(finalSize, "!=", initialSize));

    topLevel.layout()->removeWidget(widget);
    QCOMPARE(topLevel.size(), finalSize);
    topLevel.render(&pixmap);
    QCOMPARE(topLevel.size(), finalSize);

    topLevel.layout()->addWidget(widget);
    QCOMPARE(topLevel.size(), finalSize);
    topLevel.render(&pixmap);
    QCOMPARE(topLevel.size(), finalSize);
    }
}

void tst_QWidget::renderWithPainter()
{
    QWidget widget(nullptr, Qt::Tool);
    // prevent custom styles

    const QScopedPointer<QStyle> style(QStyleFactory::create(QLatin1String("Windows")));
    widget.setStyle(style.data());
    widget.show();
    widget.resize(70, 50);
    widget.setAutoFillBackground(true);
    widget.setPalette(Qt::black);

    // Render the entire widget onto the image.
    QImage image(QSize(70, 50), QImage::Format_ARGB32);
    image.fill(QColor(Qt::red).rgb());
    QPainter painter(&image);
    widget.render(&painter);

    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j)
            QCOMPARE(image.pixel(j, i), QColor(Qt::black).rgb());
    }

    // Translate painter (10, 10).
    painter.save();
    image.fill(QColor(Qt::red).rgb());
    painter.translate(10, 10);
    widget.render(&painter);
    painter.restore();

    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (i < 10 || j < 10)
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::black).rgb());
        }
    }

    // Pass target offset (10, 10) (the same as QPainter::translate).
    image.fill(QColor(Qt::red).rgb());
    widget.render(&painter, QPoint(10, 10));

    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (i < 10 || j < 10)
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::black).rgb());
        }
    }

    // Translate (10, 10) and pass target offset (10, 10).
    painter.save();
    image.fill(QColor(Qt::red).rgb());
    painter.translate(10, 10);
    widget.render(&painter, QPoint(10, 10));
    painter.restore();

    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (i < 20 || j < 20)
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::black).rgb());
        }
    }

    // Rotate painter 90 degrees.
    painter.save();
    image.fill(QColor(Qt::red).rgb());
    painter.rotate(90);
    widget.render(&painter);
    painter.restore();

    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j)
            QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
    }

    // Translate and rotate.
    image.fill(QColor(Qt::red).rgb());
    widget.resize(40, 10);
    painter.translate(10, 10);
    painter.rotate(90);
    widget.render(&painter);

    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (i >= 10 && j >= 0 && j < 10)
                QCOMPARE(image.pixel(j, i), QColor(Qt::black).rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
        }
    }

    // Make sure QWidget::render does not modify the render hints set on the painter.
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform
                           | QPainter::TextAntialiasing);
    QPainter::RenderHints oldRenderHints = painter.renderHints();
    widget.render(&painter);
    QCOMPARE(painter.renderHints(), oldRenderHints);
}

void tst_QWidget::render_task188133()
{
    QMainWindow mainWindow;

    // Make sure QWidget::render does not trigger QWidget::repaint/update
    // and asserts for Qt::WA_WState_Created.
    const QPixmap pixmap = mainWindow.grab(QRect(QPoint(0, 0), QSize(-1, -1)));
    Q_UNUSED(pixmap)
}

void tst_QWidget::render_task211796()
{
    class MyWidget : public QWidget
    {
        void resizeEvent(QResizeEvent *) override
        {
            QPixmap pixmap(size());
            render(&pixmap);
        }
    };

    { // Please don't die in a resize recursion.
        MyWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        widget.resize(m_testWidgetSize);
        centerOnScreen(&widget);
        widget.show();
    }

    { // Same check with a deeper hierarchy.
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        widget.resize(m_testWidgetSize);
        centerOnScreen(&widget);
        widget.show();
        QWidget child(&widget);
        MyWidget grandChild;
        grandChild.setParent(&child);
        grandChild.resize(100, 100);
        child.show();
    }
}

void tst_QWidget::render_task217815()
{
    // Make sure we don't change the size of the widget when calling
    // render() and the widget has an explicit size set.
    // This was a problem on Windows because we called createWinId(),
    // which in turn enforced the size to be bigger than the smallest
    // possible native window size (which is (115,something) on WinXP).
    QWidget widget;
    const QSize explicitSize(80, 20);
    widget.resize(explicitSize);
    QCOMPARE(widget.size(), explicitSize);

    QPixmap pixmap(explicitSize);
    widget.render(&pixmap);

    QCOMPARE(widget.size(), explicitSize);
}

// Window Opacity is not supported on Windows CE.
void tst_QWidget::render_windowOpacity()
{
    if (m_platform == QStringLiteral("offscreen"))
        QSKIP("Platform offscreen does not support setting opacity");

    const qreal opacity = 0.5;

    { // Check that the painter opacity effects the widget drawing.
    QWidget topLevel;
    QWidget child(&topLevel);
    child.resize(50, 50);
    child.setPalette(Qt::red);
    child.setAutoFillBackground(true);

    QPixmap expected(child.size());

    if (m_platform == QStringLiteral("xcb") && expected.depth() < 24)
        QSKIP("This test won't give correct results with dithered pixmaps");

    expected.fill(Qt::green);
    QPainter painter(&expected);
    painter.setOpacity(opacity);
    painter.fillRect(QRect(QPoint(0, 0), child.size()), Qt::red);
    painter.end();

    QPixmap result(child.size());
    result.fill(Qt::green);
    painter.begin(&result);
    painter.setOpacity(opacity);
    child.render(&painter);
    painter.end();
    QCOMPARE(result, expected);
    }

    { // Combine the opacity set on the painter with the widget opacity.
    class MyWidget : public QWidget
    {
    public:
        explicit MyWidget(qreal opacityIn) : opacity(opacityIn) {}
        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            painter.setOpacity(opacity);
            QCOMPARE(painter.opacity(), opacity);
            painter.fillRect(rect(), Qt::red);
        }
        const qreal opacity;
    };

    MyWidget widget(opacity);
    widget.resize(50, 50);
    widget.setPalette(Qt::blue);
    widget.setAutoFillBackground(true);

    QPixmap expected(widget.size());
    expected.fill(Qt::green);
    QPainter painter(&expected);
    painter.setOpacity(opacity);
    QPixmap pixmap(widget.size());
    pixmap.fill(Qt::blue);
    QPainter pixmapPainter(&pixmap);
    pixmapPainter.setOpacity(opacity);
    pixmapPainter.fillRect(QRect(QPoint(), widget.size()), Qt::red);
    painter.drawPixmap(QPoint(), pixmap);
    painter.end();

    QPixmap result(widget.size());
    result.fill(Qt::green);
    painter.begin(&result);
    painter.setOpacity(opacity);
    widget.render(&painter);
    painter.end();
    QCOMPARE(result, expected);
    }
}

void tst_QWidget::render_systemClip()
{
    QWidget widget;
    widget.setPalette(Qt::blue);
    widget.resize(100, 100);

    QImage image(widget.size(), QImage::Format_RGB32);
    image.fill(QColor(Qt::red).rgb());

    QPaintEngine *paintEngine = image.paintEngine();
    QVERIFY(paintEngine);
    paintEngine->setSystemClip(QRegion(0, 0, 50, 50));

    QPainter painter(&image);
    // Make sure we're using the same paint engine and has the right clip set.
    QCOMPARE(painter.paintEngine(), paintEngine);
    QCOMPARE(paintEngine->systemClip(), QRegion(0, 0, 50, 50));

    // Translate painter outside system clip.
    painter.translate(50, 0);
    widget.render(&painter);

#ifdef RENDER_DEBUG
    image.save("outside_systemclip.png");
#endif

    // All pixels should be red.
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j)
            QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
    }

    // Restore painter and refill image with red.
    image.fill(QColor(Qt::red).rgb());
    painter.translate(-50, 0);

    // Set transform on the painter.
    QTransform transform;
    transform.shear(0, 1);
    painter.setTransform(transform);
    widget.render(&painter);

#ifdef RENDER_DEBUG
    image.save("blue_triangle.png");
#endif

    // We should now have a blue triangle starting at scan line 1, and the rest should be red.
    // rrrrrrrrrr
    // brrrrrrrrr
    // bbrrrrrrrr
    // bbbrrrrrrr
    // bbbbrrrrrr
    // rrrrrrrrrr
    // ...

#ifndef Q_OS_MACOS
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (i < 50 && j < i)
                QCOMPARE(image.pixel(j, i), QColor(Qt::blue).rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
        }
    }
#else
    // We don't paint directly on the image on the Mac, so we cannot do the pixel comparison
    // as above due to QPainter::SmoothPixmapTransform. We therefore need to generate an
    // expected image by first painting on a pixmap, and then draw the pixmap onto
    // the image using QPainter::SmoothPixmapTransform. Then we can compare pixels :)
    // The check is basically the same, except that it takes the smoothening into account.
    QPixmap pixmap(50, 50);
    const QRegion sysClip(0, 0, 50, 50);
    widget.render(&pixmap, QPoint(), sysClip);

    QImage expectedImage(widget.size(), QImage::Format_RGB32);
    expectedImage.fill(QColor(Qt::red).rgb());
    expectedImage.paintEngine()->setSystemClip(sysClip);

    QPainter expectedImagePainter(&expectedImage);
    expectedImagePainter.setTransform(QTransform().shear(0, 1));
    // NB! This is the important part (SmoothPixmapTransform).
    expectedImagePainter.setRenderHints(QPainter::SmoothPixmapTransform);
    expectedImagePainter.drawPixmap(QPoint(0, 0), pixmap);
    expectedImagePainter.end();

    QCOMPARE(image, expectedImage);
#endif
}

void tst_QWidget::render_systemClip2_data()
{
    QTest::addColumn<bool>("autoFillBackground");
    QTest::addColumn<bool>("usePaintEvent");
    QTest::addColumn<QColor>("expectedColor");

    QTest::newRow("Only auto-fill background") << true << false << QColor(Qt::blue);
    QTest::newRow("Only draw in paintEvent") << false << true << QColor(Qt::green);
    QTest::newRow("Auto-fill background and draw in paintEvent") << true << true << QColor(Qt::green);
}

void tst_QWidget::render_systemClip2()
{
    QFETCH(bool, autoFillBackground);
    QFETCH(bool, usePaintEvent);
    QFETCH(QColor, expectedColor);

    QVERIFY2(expectedColor != QColor(Qt::red), "Qt::red is the reference color for the image, pick another color");

    class MyWidget : public QWidget
    {
    public:
        explicit MyWidget(bool usePaintEventIn) : usePaintEvent(usePaintEventIn) {}
        const bool usePaintEvent;
        void paintEvent(QPaintEvent *) override
        {
            if (usePaintEvent)
                QPainter(this).fillRect(rect(), Qt::green);
        }
    };

    MyWidget widget(usePaintEvent);
    widget.setPalette(Qt::blue);
    // NB! widget.setAutoFillBackground(autoFillBackground) won't do the
    // trick here since the widget is a top-level. The background is filled
    // regardless, unless Qt::WA_OpaquePaintEvent or Qt::WA_NoSystemBackground
    // is set. We therefore use the opaque attribute to turn off auto-fill.
    if (!autoFillBackground)
        widget.setAttribute(Qt::WA_OpaquePaintEvent);
    widget.resize(100, 100);

    QImage image(widget.size(), QImage::Format_RGB32);
    image.fill(QColor(Qt::red).rgb());

    QPaintEngine *paintEngine = image.paintEngine();
    QVERIFY(paintEngine);

    QRegion systemClip(QRegion(50, 0, 50, 10));
    systemClip += QRegion(90, 10, 10, 40);
    paintEngine->setSystemClip(systemClip);

    // Render entire widget directly onto device.
    widget.render(&image);

#ifdef RENDER_DEBUG
    image.save("systemclip_with_device.png");
#endif
    // All pixels within the system clip should now be
    // the expectedColor, and the rest should be red.
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (systemClip.contains(QPoint(j, i)))
                QCOMPARE(image.pixel(j, i), expectedColor.rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
        }
    }

    // Refill image with red.
    image.fill(QColor(Qt::red).rgb());
    paintEngine->setSystemClip(systemClip);

    // Do the same with an untransformed painter.
    QPainter painter(&image);
    //Make sure we're using the same paint engine and has the right clip set.
    QCOMPARE(painter.paintEngine(), paintEngine);
    QCOMPARE(paintEngine->systemClip(), systemClip);

    widget.render(&painter);

#ifdef RENDER_DEBUG
    image.save("systemclip_with_untransformed_painter.png");
#endif
    // All pixels within the system clip should now be
    // the expectedColor, and the rest should be red.
    for (int i = 0; i < image.height(); ++i) {
        for (int j = 0; j < image.width(); ++j) {
            if (systemClip.contains(QPoint(j, i)))
                QCOMPARE(image.pixel(j, i), expectedColor.rgb());
            else
                QCOMPARE(image.pixel(j, i), QColor(Qt::red).rgb());
        }
    }
}

void tst_QWidget::render_systemClip3_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<bool>("useSystemClip");

    // Reference: http://en.wikipedia.org/wiki/Flag_of_Norway
    QTest::newRow("Norwegian Civil Flag") << QSize(220, 160) << false;
    QTest::newRow("Norwegian War Flag") << QSize(270, 160) << true;
}

// This test ensures that the current engine clip (systemClip + painter clip)
// is preserved after QPainter::setClipRegion(..., Qt::ReplaceClip);
void tst_QWidget::render_systemClip3()
{
    QFETCH(QSize, size);
    QFETCH(bool, useSystemClip);

    // Calculate the inner/outer cross of the flag.
    QRegion outerCross(0, 0, size.width(), size.height());
    outerCross -= QRect(0, 0, 60, 60);
    outerCross -= QRect(100, 0, size.width() - 100, 60);
    outerCross -= QRect(0, 100, 60, 60);
    outerCross -= QRect(100, 100, size.width() - 100, 60);

    QRegion innerCross(0, 0, size.width(), size.height());
    innerCross -= QRect(0, 0, 70, 70);
    innerCross -= QRect(90, 0, size.width() - 90, 70);
    innerCross -= QRect(0, 90, 70, 70);
    innerCross -= QRect(90, 90, size.width() - 90, 70);

    const QRegion redArea(QRegion(0, 0, size.width(), size.height()) - outerCross);
    const QRegion whiteArea(outerCross - innerCross);
    QRegion systemClip;

    // Okay, here's the image that should look like a Norwegian civil/war flag in the end.
    QImage flag(size, QImage::Format_ARGB32);
    flag.fill(QColor(Qt::transparent).rgba());

    if (useSystemClip) {
        QPainterPath warClip(QPoint(size.width(), 0));
        warClip.lineTo(size.width() - 110, 60);
        warClip.lineTo(size.width(), 80);
        warClip.lineTo(size.width() - 110, 100);
        warClip.lineTo(size.width(), 160);
        warClip.closeSubpath();
        systemClip = QRegion(0, 0, size.width(), size.height()) - QRegion(warClip.toFillPolygon().toPolygon());
        flag.paintEngine()->setSystemClip(systemClip);
    }

    QPainter painter(&flag);
    painter.fillRect(QRect(QPoint(), size), Qt::red); // Fill image background with red.
    painter.setClipRegion(outerCross); // Limit widget painting to inside the outer cross.

    // Here's the widget that's supposed to draw the inner/outer cross of the flag.
    // The outer cross (white) should be drawn when the background is auto-filled, and
    // the inner cross (blue) should be drawn in the paintEvent.
    class MyWidget : public QWidget
    {
    public:
        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            // Be evil and try to paint outside the outer cross. This should not be
            // possible since the shared painter is clipped to the outer cross.
            painter.setClipRect(0, 0, 60, 60, Qt::ReplaceClip);
            painter.fillRect(rect(), Qt::green);
            painter.setClipRegion(clip, Qt::ReplaceClip);
            painter.fillRect(rect(), Qt::blue);
        }
        QRegion clip;
    };

    MyWidget widget;
    widget.clip = innerCross;
    widget.setFixedSize(size);
    widget.setPalette(Qt::white);
    widget.setAutoFillBackground(true);
    widget.render(&painter);

#ifdef RENDER_DEBUG
    flag.save("flag.png");
#endif

    // Let's make sure we got a Norwegian flag.
    for (int i = 0; i < flag.height(); ++i) {
        for (int j = 0; j < flag.width(); ++j) {
            const QPoint pixel(j, i);
            const QRgb pixelValue = flag.pixel(pixel);
            if (useSystemClip && !systemClip.contains(pixel))
                QCOMPARE(pixelValue, QColor(Qt::transparent).rgba());
            else if (redArea.contains(pixel))
                QCOMPARE(pixelValue, QColor(Qt::red).rgba());
            else if (whiteArea.contains(pixel))
                QCOMPARE(pixelValue, QColor(Qt::white).rgba());
            else
                QCOMPARE(pixelValue, QColor(Qt::blue).rgba());
        }
    }
}

void tst_QWidget::render_task252837()
{
    QWidget widget;
    widget.resize(200, 200);

    QPixmap pixmap(widget.size());
    QPainter painter(&pixmap);
    // Please do not crash.
    widget.render(&painter);
}

void tst_QWidget::render_worldTransform()
{
    class MyWidget : public QWidget
    {
    public:
        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            // Make sure world transform is identity.
            QCOMPARE(painter.worldTransform(), QTransform());

            // Make sure device transform is correct.
            const QPoint widgetOffset = geometry().topLeft();
            QTransform expectedDeviceTransform = QTransform::fromTranslate(105, 5);
            expectedDeviceTransform.rotate(90);
            expectedDeviceTransform.translate(widgetOffset.x(), widgetOffset.y());
            QCOMPARE(painter.deviceTransform(), expectedDeviceTransform);

            // Set new world transform.
            QTransform newWorldTransform = QTransform::fromTranslate(10, 10);
            newWorldTransform.rotate(90);
            painter.setWorldTransform(newWorldTransform);
            QCOMPARE(painter.worldTransform(), newWorldTransform);

            // Again, check device transform.
            expectedDeviceTransform.translate(10, 10);
            expectedDeviceTransform.rotate(90);
            QCOMPARE(painter.deviceTransform(), expectedDeviceTransform);

            painter.fillRect(QRect(0, 0, 20, 10), Qt::green);
        }
    };

    MyWidget widget;
    widget.setFixedSize(100, 100);
    widget.setPalette(Qt::red);
    widget.setAutoFillBackground(true);

    MyWidget child;
    child.setParent(&widget);
    child.move(50, 50);
    child.setFixedSize(50, 50);
    child.setPalette(Qt::blue);
    child.setAutoFillBackground(true);

    QImage image(QSize(110, 110), QImage::Format_RGB32);
    image.fill(QColor(Qt::black).rgb());

    QPainter painter(&image);
    painter.translate(105, 5);
    painter.rotate(90);

    // Render widgets onto image.
    widget.render(&painter);
#ifdef RENDER_DEBUG
    image.save("render_worldTransform_image.png");
#endif

    // Ensure the transforms are unchanged after render.
    QCOMPARE(painter.worldTransform(), painter.worldTransform());
    QCOMPARE(painter.deviceTransform(), painter.deviceTransform());
    painter.end();

    // Paint expected image.
    QImage expected(QSize(110, 110), QImage::Format_RGB32);
    expected.fill(QColor(Qt::black).rgb());

    QPainter expectedPainter(&expected);
    expectedPainter.translate(105, 5);
    expectedPainter.rotate(90);
    expectedPainter.save();
    expectedPainter.fillRect(widget.rect(),Qt::red);
    expectedPainter.translate(10, 10);
    expectedPainter.rotate(90);
    expectedPainter.fillRect(QRect(0, 0, 20, 10), Qt::green);
    expectedPainter.restore();
    expectedPainter.translate(50, 50);
    expectedPainter.fillRect(child.rect(),Qt::blue);
    expectedPainter.translate(10, 10);
    expectedPainter.rotate(90);
    expectedPainter.fillRect(QRect(0, 0, 20, 10), Qt::green);
    expectedPainter.end();

#ifdef RENDER_DEBUG
    expected.save("render_worldTransform_expected.png");
#endif

    QCOMPARE(image, expected);
}

void tst_QWidget::setContentsMargins()
{
    QLabel label("why does it always rain on me?");
    QSize oldSize = label.sizeHint();
    label.setFrameStyle(QFrame::Sunken | QFrame::Box);
    QSize newSize = label.sizeHint();
    QVERIFY2(oldSize != newSize, msgComparisonFailed(oldSize, "!=", newSize));

    QLabel label2("why does it always rain on me?");
    label2.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    label2.show();
    label2.setFrameStyle(QFrame::Sunken | QFrame::Box);
    QCOMPARE(newSize, label2.sizeHint());

    QLabel label3("why does it always rain on me?");
    label3.setFrameStyle(QFrame::Sunken | QFrame::Box);
    QCOMPARE(newSize, label3.sizeHint());
}

void tst_QWidget::moveWindowInShowEvent_data()
{
    QTest::addColumn<QPoint>("initial");
    QTest::addColumn<QPoint>("position");

    QPoint p = QGuiApplication::primaryScreen()->availableGeometry().topLeft();

    QTest::newRow("1") << p << (p + QPoint(10, 10));
    QTest::newRow("2") << (p + QPoint(10,10)) << p;
}

void tst_QWidget::moveWindowInShowEvent()
{
    if (m_platform == QStringLiteral("xcb"))
        QSKIP("QTBUG-26424");

    QFETCH(QPoint, initial);
    QFETCH(QPoint, position);

    class MoveWindowInShowEventWidget : public QWidget
    {
    public:
        QPoint position;
        void showEvent(QShowEvent *) override
        {
            move(position);
        }
    };

    MoveWindowInShowEventWidget widget;
    QScreen *screen = QGuiApplication::primaryScreen();
    widget.resize(QSize(screen->availableGeometry().size() / 3).expandedTo(QSize(1, 1)));
    // move to this position in showEvent()
    widget.position = position;

    // put the widget in it's starting position
    widget.move(initial);
    QCOMPARE(widget.pos(), initial);

    // show it
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTest::qWait(100);
    // it should have moved
    QCOMPARE(widget.pos(), position);
}

void tst_QWidget::repaintWhenChildDeleted()
{
#ifdef Q_OS_WIN
    QTest::qWait(1000);
#endif
    ColorWidget w(nullptr, Qt::FramelessWindowHint, Qt::red);
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QPoint startPoint = w.screen()->availableGeometry().topLeft();
    startPoint.rx() += 50;
    startPoint.ry() += 50;
    w.setGeometry(QRect(startPoint, QSize(100, 100)));
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_COMPARE(w.r, QRegion(w.rect()));
    w.r = QRegion();

    {
        ColorWidget child(&w, Qt::Widget, Qt::blue);
        child.setGeometry(10, 10, 10, 10);
        child.show();
        QTRY_COMPARE(child.r, QRegion(child.rect()));
        w.r = QRegion();
    }

    QTRY_COMPARE(w.r, QRegion(10, 10, 10, 10));
}

// task 175114
void tst_QWidget::hideOpaqueChildWhileHidden()
{
    ColorWidget w(nullptr, Qt::FramelessWindowHint, Qt::red);
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QPoint startPoint = w.screen()->availableGeometry().topLeft();
    startPoint.rx() += 50;
    startPoint.ry() += 50;
    w.setGeometry(QRect(startPoint, QSize(100, 100)));

    ColorWidget child(&w, Qt::Widget, Qt::blue);
    child.setGeometry(10, 10, 80, 80);

    ColorWidget child2(&child, Qt::Widget, Qt::white);
    child2.setGeometry(10, 10, 60, 60);

    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_COMPARE(child2.r, QRegion(child2.rect()));
    child.r = QRegion();
    child2.r = QRegion();
    w.r = QRegion();

    child.hide();
    child2.hide();

    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    QTRY_COMPARE(w.r, QRegion(child.geometry()));

    child.show();
    QTRY_COMPARE(child.r, QRegion(child.rect()));
    QCOMPARE(child2.r, QRegion());
}

// This test doesn't make sense without support for showMinimized().
void tst_QWidget::updateWhileMinimized()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    if (m_platform == QStringLiteral("offscreen"))
        QSKIP("Platform offscreen does not support showMinimized()");

#if defined(Q_OS_QNX)
    QSKIP("Platform does not support showMinimized()");
#endif
    UpdateWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
   // Filter out activation change and focus events to avoid update() calls in QWidget.
    widget.updateOnActivationChangeAndFocusIn = false;
    widget.reset();
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_VERIFY(widget.numPaintEvents > 0);
    QTest::qWait(150);

    // Minimize window.
    widget.showMinimized();
    QTest::qWait(110);

    widget.reset();

    // The widget is not visible on the screen (but isVisible() still returns true).
    // Make sure update requests are discarded until the widget is shown again.
    widget.update(0, 0, 50, 50);
    QTest::qWait(10);
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    int count = 0;
    // mutter/GNOME Shell doesn't unmap when minimizing window.
    // More details at https://gitlab.gnome.org/GNOME/mutter/issues/185
    if (m_platform == QStringLiteral("xcb")) {
        const QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
        qDebug() << "xcb: XDG_CURRENT_DESKTOP=" << desktop;
        if (desktop == QStringLiteral("ubuntu:GNOME")
            || desktop == QStringLiteral("GNOME-Classic:GNOME")
            || desktop == QStringLiteral("GNOME"))
            count = 1;
    }
    QCOMPARE(widget.numPaintEvents, count);

    // Restore window.
    widget.showNormal();
    QTRY_COMPARE(widget.numPaintEvents, 1);
    QCOMPARE(widget.paintedRegion, QRegion(0, 0, 50, 50));
}

class PaintOnScreenWidget: public QWidget
{
public:
    using QWidget::QWidget;
#if defined(Q_OS_WIN)
    // This is the only way to enable PaintOnScreen on Windows.
    QPaintEngine *paintEngine() const override { return nullptr; }
#endif
};

void tst_QWidget::alienWidgets()
{
    if (m_platform != QStringLiteral("xcb") && m_platform != QStringLiteral("windows"))
        QSKIP("This test is only for X11/Windows.");

    qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    parent.resize(m_testWidgetSize);
    QWidget child(&parent);
    QWidget grandChild(&child);
    QWidget greatGrandChild(&grandChild);
    parent.show();

    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    // Verify that the WA_WState_Created attribute is set
    // and the top-level is the only native window.
    QVERIFY(parent.testAttribute(Qt::WA_WState_Created));
    QVERIFY(parent.internalWinId());

    QVERIFY(child.testAttribute(Qt::WA_WState_Created));
    QVERIFY(!child.internalWinId());

    QVERIFY(grandChild.testAttribute(Qt::WA_WState_Created));
    QVERIFY(!grandChild.internalWinId());

    QVERIFY(greatGrandChild.testAttribute(Qt::WA_WState_Created));
    QVERIFY(!greatGrandChild.internalWinId());

    // Enforce native windows all the way up in the parent hierarchy
    // if not WA_DontCreateNativeAncestors is set.
    grandChild.setAttribute(Qt::WA_DontCreateNativeAncestors);
    greatGrandChild.setAttribute(Qt::WA_NativeWindow);
    QVERIFY(greatGrandChild.internalWinId());
    QVERIFY(grandChild.internalWinId());
    QVERIFY(!child.internalWinId());

    {
        // Ensure that hide() on an ancestor of a widget with
        // Qt::WA_DontCreateNativeAncestors still gets unmapped
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(m_testWidgetSize);
        QWidget widget(&window);
        QWidget child(&widget);
        child.setAttribute(Qt::WA_NativeWindow);
        child.setAttribute(Qt::WA_DontCreateNativeAncestors);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(child.testAttribute(Qt::WA_Mapped));
        widget.hide();
        QTRY_VERIFY(!child.testAttribute(Qt::WA_Mapped));
    }

    // Enforce a native window when calling QWidget::winId.
    QVERIFY(child.winId());
    QVERIFY(child.internalWinId());

    // Check that paint on screen widgets (incl. children) are native.
    PaintOnScreenWidget paintOnScreen(&parent);
    QWidget paintOnScreenChild(&paintOnScreen);
    paintOnScreen.show();
    QVERIFY(paintOnScreen.testAttribute(Qt::WA_WState_Created));
    QVERIFY(!paintOnScreen.testAttribute(Qt::WA_NativeWindow));
    QVERIFY(!paintOnScreen.internalWinId());
    QVERIFY(!paintOnScreenChild.testAttribute(Qt::WA_NativeWindow));
    QVERIFY(!paintOnScreenChild.internalWinId());

    paintOnScreen.setAttribute(Qt::WA_PaintOnScreen);
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
        QEXPECT_FAIL("", "QTBUG-26424", Continue);
    QVERIFY(paintOnScreen.testAttribute(Qt::WA_NativeWindow));
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
        QEXPECT_FAIL("", "QTBUG-26424", Continue);
    QVERIFY(paintOnScreen.internalWinId());
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
        QEXPECT_FAIL("", "QTBUG-26424", Continue);
    QVERIFY(paintOnScreenChild.testAttribute(Qt::WA_NativeWindow));
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
        QEXPECT_FAIL("", "QTBUG-26424", Continue);
    QVERIFY(paintOnScreenChild.internalWinId());

    // Check that widgets with the Qt::MSWindowsOwnDC attribute set
    // are native.
    QWidget msWindowsOwnDC(&parent, Qt::MSWindowsOwnDC);
    msWindowsOwnDC.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    msWindowsOwnDC.show();
    QVERIFY(msWindowsOwnDC.testAttribute(Qt::WA_WState_Created));
    QVERIFY(msWindowsOwnDC.testAttribute(Qt::WA_NativeWindow));
    QVERIFY(msWindowsOwnDC.internalWinId());

    { // Enforce a native window when calling QWidget::handle() (on X11) or QWidget::getDC() (on Windows).
        QWidget widget(&parent);
        widget.show();
        QVERIFY(widget.testAttribute(Qt::WA_WState_Created));
        QVERIFY(!widget.internalWinId());

        widget.winId();
        QVERIFY(widget.internalWinId());
    }

    if (m_platform == QStringLiteral("xcb")) {
        // Make sure we don't create native windows when setting Qt::WA_X11NetWmWindowType attributes
        // on alien widgets (see task 194231).
        QWidget dummy;
        dummy.resize(m_testWidgetSize);
        QVERIFY(dummy.winId());
        QWidget widget(&dummy);
        widget.setAttribute(Qt::WA_X11NetWmWindowTypeToolBar);
        QVERIFY(!widget.internalWinId());
    }

    { // Make sure we create native ancestors when setting Qt::WA_PaintOnScreen before show().
        QWidget topLevel;
        topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        topLevel.resize(m_testWidgetSize);
        QWidget child(&topLevel);
        QWidget grandChild(&child);
        PaintOnScreenWidget greatGrandChild(&grandChild);

        greatGrandChild.setAttribute(Qt::WA_PaintOnScreen);
        QVERIFY(!child.internalWinId());
        QVERIFY(!grandChild.internalWinId());
        QVERIFY(!greatGrandChild.internalWinId());

        topLevel.show();
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(child.internalWinId());
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(grandChild.internalWinId());
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(greatGrandChild.internalWinId());
    }

    { // Ensure that widgets reparented into Qt::WA_PaintOnScreen widgets become native.
        QWidget topLevel;
        topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        topLevel.resize(m_testWidgetSize);
        QWidget *widget = new PaintOnScreenWidget(&topLevel);
        widget->setAttribute(Qt::WA_PaintOnScreen);
        QWidget *child = new QWidget;
        QWidget *dummy = new QWidget(child);
        QWidget *grandChild = new QWidget(child);
        QWidget *dummy2 = new QWidget(grandChild);

        child->setParent(widget);

        QVERIFY(!topLevel.internalWinId());
        QVERIFY(!child->internalWinId());
        QVERIFY(!dummy->internalWinId());
        QVERIFY(!grandChild->internalWinId());
        QVERIFY(!dummy2->internalWinId());

        topLevel.show();
        QVERIFY(topLevel.internalWinId());
        QVERIFY(widget->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(child->internalWinId());
        QVERIFY(child->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(!child->testAttribute(Qt::WA_PaintOnScreen));
        QVERIFY(!dummy->internalWinId());
        QVERIFY(!dummy->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(!grandChild->internalWinId());
        QVERIFY(!grandChild->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(!dummy2->internalWinId());
        QVERIFY(!dummy2->testAttribute(Qt::WA_NativeWindow));
    }

    { // Ensure that ancestors of a Qt::WA_PaintOnScreen widget stay native
      // if they are re-created (typically in QWidgetPrivate::setParent_sys) (task 210822).
        QWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(m_testWidgetSize);
        QWidget child(&window);

        QWidget grandChild;
        grandChild.setWindowTitle("This causes the widget to be created");

        PaintOnScreenWidget paintOnScreenWidget;
        paintOnScreenWidget.setAttribute(Qt::WA_PaintOnScreen);
        paintOnScreenWidget.setParent(&grandChild);

        grandChild.setParent(&child);

        window.show();

        QVERIFY(window.internalWinId());
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(child.internalWinId());
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(child.testAttribute(Qt::WA_NativeWindow));
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(grandChild.internalWinId());
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(grandChild.testAttribute(Qt::WA_NativeWindow));
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(paintOnScreenWidget.internalWinId());
        if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
            QEXPECT_FAIL("", "QTBUG-26424", Continue);
        QVERIFY(paintOnScreenWidget.testAttribute(Qt::WA_NativeWindow));
    }

    { // Ensure that all siblings are native unless Qt::AA_DontCreateNativeWidgetSiblings is set.
        qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, false);
        QWidget mainWindow;
        mainWindow.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        QWidget *toolBar = new QWidget(&mainWindow);
        QWidget *dockWidget = new QWidget(&mainWindow);
        QWidget *centralWidget = new QWidget(&mainWindow);
        centralWidget->setMinimumSize(m_testWidgetSize);

        QWidget *button = new QWidget(centralWidget);
        QWidget *mdiArea = new QWidget(centralWidget);

        QWidget *horizontalScroll = new QWidget(mdiArea);
        QWidget *verticalScroll = new QWidget(mdiArea);
        QWidget *viewport = new QWidget(mdiArea);

        viewport->setAttribute(Qt::WA_NativeWindow);
        mainWindow.show();

        // Ensure that the viewport and its siblings are native:
        QVERIFY(verticalScroll->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(verticalScroll->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(horizontalScroll->testAttribute(Qt::WA_NativeWindow));

        // Ensure that the mdi area and its siblings are native:
        QVERIFY(mdiArea->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(button->testAttribute(Qt::WA_NativeWindow));

        // Ensure that the central widget and its siblings are native:
        QVERIFY(centralWidget->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(dockWidget->testAttribute(Qt::WA_NativeWindow));
        QVERIFY(toolBar->testAttribute(Qt::WA_NativeWindow));
    }
}

using WidgetAttributes = QVector<Qt::WidgetAttribute>;

void tst_QWidget::nativeWindowPosition_data()
{
    QTest::addColumn<WidgetAttributes>("attributes");

    QTest::newRow("non-native all the way")
        << WidgetAttributes{};
    QTest::newRow("native all the way")
        << WidgetAttributes{ Qt::WA_NativeWindow };
    QTest::newRow("native with non-native ancestor")
        << WidgetAttributes{ Qt::WA_NativeWindow, Qt::WA_DontCreateNativeAncestors };
}

void tst_QWidget::nativeWindowPosition()
{
    QWidget topLevel;
    QWidget child(&topLevel);
    child.move(5, 5);

    QWidget grandChild(&child);
    grandChild.move(10, 10);

    QFETCH(WidgetAttributes, attributes);
    for (auto attribute : attributes)
        grandChild.setAttribute(attribute);

    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    QCOMPARE(child.pos(), QPoint(5, 5));
    QCOMPARE(grandChild.pos(), QPoint(10, 10));
}

class ASWidget : public QWidget
{
public:
    ASWidget(QSize sizeHint, QSizePolicy sizePolicy, bool layout, bool hfwLayout, QWidget *parent = nullptr)
        : QWidget(parent), mySizeHint(sizeHint)
    {
        setObjectName(QStringLiteral("ASWidget"));
        setWindowTitle(objectName());
        setSizePolicy(sizePolicy);
        if (layout) {
            QSizePolicy sp = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            sp.setHeightForWidth(hfwLayout);

            QVBoxLayout *vbox = new QVBoxLayout;
            vbox->setContentsMargins(0, 0, 0, 0);
            vbox->addWidget(new ASWidget(sizeHint + QSize(30, 20), sp, false, false));
            setLayout(vbox);
        }
    }

    QSize sizeHint() const override
    {
        if (layout())
            return layout()->totalSizeHint();
        return mySizeHint;
    }
    int heightForWidth(int width) const override
    {
        return sizePolicy().hasHeightForWidth() ? width * 2 : -1;
    }

    QSize mySizeHint;
};

void tst_QWidget::adjustSize_data()
{
    const int MagicW = 200;
    const int MagicH = 100;

    QTest::addColumn<QSize>("sizeHint");
    QTest::addColumn<int>("hPolicy");
    QTest::addColumn<int>("vPolicy");
    QTest::addColumn<bool>("hfwSP");
    QTest::addColumn<bool>("layout");
    QTest::addColumn<bool>("hfwLayout");
    QTest::addColumn<bool>("haveParent");
    QTest::addColumn<QSize>("expectedSize");

    QTest::newRow("1") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << false << false << false << QSize(5, qMax(6, MagicH));
    QTest::newRow("2") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << true << false << false << false << QSize(5, qMax(10, MagicH));
    QTest::newRow("3") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << false << false << QSize(35, 26);
    QTest::newRow("4") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << true << false << QSize(35, 70);
    QTest::newRow("5") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << false << false << false << QSize(100000, 100000);
    QTest::newRow("6") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << true << false << false << false << QSize(100000, 100000);
    QTest::newRow("7") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << false << false << QSize(100000, 100000);
    QTest::newRow("8") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << true << false << QSize(100000, 100000);
    QTest::newRow("9") << QSize(5, 6) << int(QSizePolicy::Expanding) << int(QSizePolicy::Minimum)
        << true << false << false << false << QSize(qMax(5, MagicW), 10);

    QTest::newRow("1c") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << false << false << true << QSize(5, 6);
    QTest::newRow("2c") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << true << false << false << true << QSize(5, 6 /* or 10 would be OK too, since hfw contradicts sizeHint() */);
    QTest::newRow("3c") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << false << true << QSize(35, 26);
    QTest::newRow("4c") << QSize(5, 6) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << true << true << QSize(35, 70);
    QTest::newRow("5c") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << false << false << true << QSize(40001, 30001);
    QTest::newRow("6c") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << true << false << false << true << QSize(40001, 30001 /* or 80002 would be OK too, since hfw contradicts sizeHint() */);
    QTest::newRow("7c") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << false << true << QSize(40001 + 30, 30001 + 20);
    QTest::newRow("8c") << QSize(40001, 30001) << int(QSizePolicy::Minimum) << int(QSizePolicy::Expanding)
        << false << true << true << true << QSize(40001 + 30, 80002 + 60);
    QTest::newRow("9c") << QSize(5, 6) << int(QSizePolicy::Expanding) << int(QSizePolicy::Minimum)
        << true << false << false << true << QSize(5, 6);
}

void tst_QWidget::adjustSize()
{
    QFETCH(QSize, sizeHint);
    QFETCH(int, hPolicy);
    QFETCH(int, vPolicy);
    QFETCH(bool, hfwSP);
    QFETCH(bool, layout);
    QFETCH(bool, hfwLayout);
    QFETCH(bool, haveParent);
    QFETCH(QSize, expectedSize);

    QScopedPointer<QWidget> parent(new QWidget);

    QSizePolicy sp = QSizePolicy(QSizePolicy::Policy(hPolicy), QSizePolicy::Policy(vPolicy));
    sp.setHeightForWidth(hfwSP);

    QWidget *child = new ASWidget(sizeHint, sp, layout, hfwLayout, haveParent ? parent.data() : nullptr);
    child->resize(123, 456);
    child->adjustSize();
    if (expectedSize == QSize(100000, 100000)) {
        QVERIFY2(child->size().width() < sizeHint.width(),
                 msgComparisonFailed(child->size().width(), "<", sizeHint.width()));
        QVERIFY2(child->size().height() < sizeHint.height(),
                 msgComparisonFailed(child->size().height(), "<", sizeHint.height()));
    } else {
        QCOMPARE(child->size(), expectedSize);
    }
    if (!haveParent)
        delete child;
}

class TestLayout : public QVBoxLayout
{
    Q_OBJECT
public:
    using QVBoxLayout::QVBoxLayout;

    void invalidate() override
    {
        invalidated = true;
    }

    bool invalidated = false;
};

void tst_QWidget::updateGeometry_data()
{
    QTest::addColumn<QSize>("minSize");
    QTest::addColumn<bool>("shouldInvalidate");
    QTest::addColumn<QSize>("maxSize");
    QTest::addColumn<bool>("shouldInvalidate2");
    QTest::addColumn<QSizePolicy::Policy>("verticalSizePolicy");
    QTest::addColumn<bool>("shouldInvalidate3");
    QTest::addColumn<bool>("setVisible");
    QTest::addColumn<bool>("shouldInvalidate4");

    QTest::newRow("setMinimumSize")
        << QSize(100, 100) << true
        << QSize() << false
        << QSizePolicy::Preferred << false
        << true << false;
    QTest::newRow("setMaximumSize")
        << QSize() << false
        << QSize(100, 100) << true
        << QSizePolicy::Preferred << false
        << true << false;
    QTest::newRow("setMinimumSize, then maximumSize to a different size")
        << QSize(100, 100) << true
        << QSize(300, 300) << true
        << QSizePolicy::Preferred << false
        << true << false;
    QTest::newRow("setMinimumSize, then maximumSize to the same size")
        << QSize(100, 100) << true
        << QSize(100, 100) << true
        << QSizePolicy::Preferred << false
        << true << false;
    QTest::newRow("setMinimumSize, then maximumSize to the same size and then hide it")
        << QSize(100, 100) << true
        << QSize(100, 100) << true
        << QSizePolicy::Preferred << false
        << false << true;
    QTest::newRow("Change sizePolicy")
        << QSize() << false
        << QSize() << false
        << QSizePolicy::Minimum << true
        << true << false;

}

void tst_QWidget::updateGeometry()
{
    QFETCH(QSize, minSize);
    QFETCH(bool, shouldInvalidate);
    QFETCH(QSize, maxSize);
    QFETCH(bool, shouldInvalidate2);
    QFETCH(QSizePolicy::Policy, verticalSizePolicy);
    QFETCH(bool, shouldInvalidate3);
    QFETCH(bool, setVisible);
    QFETCH(bool, shouldInvalidate4);
    QWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("::")
                          + QLatin1String(QTest::currentDataTag()));
    parent.resize(200, 200);
    TestLayout *lout = new TestLayout();
    parent.setLayout(lout);
    QWidget *child = new QWidget(&parent);
    lout->addWidget(child);
    parent.show();
    QApplication::processEvents();

    lout->invalidated = false;
    if (minSize.isValid())
        child->setMinimumSize(minSize);
    QCOMPARE(lout->invalidated, shouldInvalidate);

    lout->invalidated = false;
    if (maxSize.isValid())
        child->setMaximumSize(maxSize);
    QCOMPARE(lout->invalidated, shouldInvalidate2);

    lout->invalidated = false;
    child->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, verticalSizePolicy));
    if (shouldInvalidate3)
        QCOMPARE(lout->invalidated, true);

    lout->invalidated = false;
    if (!setVisible)
        child->setVisible(false);
    QCOMPARE(lout->invalidated, shouldInvalidate4);
}

void tst_QWidget::sendUpdateRequestImmediately()
{
    UpdateWidget updateWidget;
    updateWidget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    updateWidget.show();

    QVERIFY(QTest::qWaitForWindowExposed(&updateWidget));

    QCoreApplication::processEvents();
    updateWidget.reset();

    QCOMPARE(updateWidget.numUpdateRequestEvents, 0);
    updateWidget.repaint();
    QCOMPARE(updateWidget.numUpdateRequestEvents, 1);
}

void tst_QWidget::doubleRepaint()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974");
#endif

#if defined(Q_OS_MACOS)
    if (!macHasAccessToWindowsServer())
        QSKIP("Not having window server access causes the wrong number of repaints to be issues");
#endif
   UpdateWidget widget;
   widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
   centerOnScreen(&widget);
   widget.setFocusPolicy(Qt::StrongFocus);
   // Filter out activation change and focus events to avoid update() calls in QWidget.
   widget.updateOnActivationChangeAndFocusIn = false;

   // Show: 1 repaint
   int expectedRepaints = 1;
   widget.show();
   QVERIFY(QTest::qWaitForWindowExposed(&widget));
   QTRY_COMPARE(widget.numPaintEvents, expectedRepaints);
   widget.numPaintEvents = 0;

   // Minmize: Should not trigger a repaint.
   widget.showMinimized();
   QTest::qWait(10);
#if defined(Q_OS_QNX)
    QEXPECT_FAIL("", "Platform does not support showMinimized()", Continue);
#endif
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
   QCOMPARE(widget.numPaintEvents, 0);
   widget.numPaintEvents = 0;

   // Restore: Should not trigger a repaint.
   widget.showNormal();
   QVERIFY(QTest::qWaitForWindowExposed(&widget));
   QTest::qWait(10);
   QCOMPARE(widget.numPaintEvents, 0);
}

void tst_QWidget::resizeInPaintEvent()
{
    QWidget window;
    window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    UpdateWidget widget(&window);
    window.resize(200, 200);
    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY(widget.numPaintEvents > 0);

    widget.reset();
    QCOMPARE(widget.numPaintEvents, 0);

    widget.resizeInPaintEvent = true;
    // This will call resize in the paintEvent, which in turn will call
    // invalidateBackingStore() and a new update request should be posted.
    // the resize triggers another update.
    widget.update();
    QTRY_COMPARE(widget.numPaintEvents, 2);
}

void tst_QWidget::opaqueChildren()
{
    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.resize(200, 200);

    QWidget child(&widget);
    child.setGeometry(-700, -700, 200, 200);

    QWidget grandChild(&child);
    grandChild.resize(200, 200);

    QWidget greatGrandChild(&grandChild);
    greatGrandChild.setGeometry(50, 50, 200, 200);
    greatGrandChild.setPalette(Qt::red);
    greatGrandChild.setAutoFillBackground(true); // Opaque child widget.

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    // Child, grandChild and greatGrandChild are outside the ancestor clip.
    QRegion expectedOpaqueRegion(50, 50, 150, 150);
    QCOMPARE(qt_widget_private(&grandChild)->getOpaqueChildren(), expectedOpaqueRegion);

    // Now they are all inside the ancestor clip.
    child.setGeometry(50, 50, 150, 150);
    QCOMPARE(qt_widget_private(&grandChild)->getOpaqueChildren(), expectedOpaqueRegion);

    // Set mask on greatGrandChild.
    const QRegion mask(10, 10, 50, 50);
    greatGrandChild.setMask(mask);
    expectedOpaqueRegion &= mask.translated(50, 50);
    QCOMPARE(qt_widget_private(&grandChild)->getOpaqueChildren(), expectedOpaqueRegion);

    // Make greatGrandChild "transparent".
    greatGrandChild.setAutoFillBackground(false);
    QCOMPARE(qt_widget_private(&grandChild)->getOpaqueChildren(), QRegion());
}


class MaskSetWidget : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;

    void paintEvent(QPaintEvent *event) override
    {
        QPainter p(this);

        paintedRegion += event->region();
        for (const QRect &r : event->region())
            p.fillRect(r, Qt::red);
    }

    void resizeEvent(QResizeEvent *) override
    {
        setMask(QRegion(QRect(0, 0, width(), 10).normalized()));
    }

    QRegion paintedRegion;

public slots:
    void resizeDown() { setGeometry(QRect(0, 50, 50, 50)); }
    void resizeUp() { setGeometry(QRect(0, 50, 150, 50)); }
};

void tst_QWidget::setMaskInResizeEvent()
{
    UpdateWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.reset();
    w.resize(200, 200);
    centerOnScreen(&w);
    w.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    w.raise();

    MaskSetWidget testWidget(&w);
    testWidget.setGeometry(0, 0, 100, 100);
    testWidget.setMask(QRegion(QRect(0,0,100,10)));
    testWidget.show();
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_VERIFY(w.numPaintEvents > 0);

    w.reset();
    testWidget.paintedRegion = QRegion();
    QTimer::singleShot(0, &testWidget, SLOT(resizeDown()));
    QTest::qWait(100);

    QRegion expectedParentUpdate(0, 0, 100, 10); // Old testWidget area.
    expectedParentUpdate += testWidget.geometry(); // New testWidget area.
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    QTRY_COMPARE(w.paintedRegion, expectedParentUpdate);
    QTRY_COMPARE(testWidget.paintedRegion, testWidget.mask());

    testWidget.paintedRegion = QRegion();
    // Now resize the widget again, but in the oposite direction
    QTimer::singleShot(0, &testWidget, SLOT(resizeUp()));
    QTest::qWait(100);

    QTRY_COMPARE(testWidget.paintedRegion, testWidget.mask());
}

class MoveInResizeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MoveInResizeWidget(QWidget *p = nullptr)
        : QWidget(p)
    {
        setWindowFlags(Qt::FramelessWindowHint);
    }

    void resizeEvent(QResizeEvent *) override
    {
        move(QPoint(100,100));

        static bool firstTime = true;
        if (firstTime)
            QTimer::singleShot(250, this, &MoveInResizeWidget::resizeMe);

        firstTime = false;
    }

public slots:
    void resizeMe() {
        resize(100, 100);
    }
};

void tst_QWidget::moveInResizeEvent()
{
    MoveInResizeWidget testWidget;
    testWidget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    testWidget.setGeometry(50, 50, 200, 200);
    testWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&testWidget));

    QRect expectedGeometry(100,100, 100, 100);
    QTRY_COMPARE(testWidget.geometry(), expectedGeometry);
}

#ifdef QT_BUILD_INTERNAL
void tst_QWidget::immediateRepaintAfterInvalidateBackingStore()
{
    if (m_platform != QStringLiteral("xcb") && m_platform != QStringLiteral("windows"))
        QSKIP("We don't support immediate repaint right after show on other platforms.");

    QScopedPointer<UpdateWidget> widget(new UpdateWidget);
    widget->setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    centerOnScreen(widget.data());
    widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(widget.data()));

    widget->numPaintEvents = 0;

    // Marks the area covered by the widget as dirty in the backing store and
    // posts an UpdateRequest event.
    qt_widget_private(widget.data())->invalidateBackingStore(widget->rect());
    QCOMPARE(widget->numPaintEvents, 0);

    // The entire widget is already dirty, but this time we want to update immediately
    // by calling repaint(), and thus we have to repaint the widget and not wait for
    // the UpdateRequest to be sent when we get back to the event loop.
    widget->update();
    QTRY_COMPARE(widget->numPaintEvents, 1);
}
#endif

void tst_QWidget::effectiveWinId()
{
    QWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    parent.resize(200, 200);
    QWidget child(&parent);

    // Shouldn't crash.
    QVERIFY(!parent.effectiveWinId());
    QVERIFY(!child.effectiveWinId());

    parent.show();

    QVERIFY(parent.effectiveWinId());
    QVERIFY(child.effectiveWinId());
}

void tst_QWidget::effectiveWinId2()
{
    QWidget parent;

    class MyWidget : public QWidget
    {
        bool event(QEvent *e) override
        {
            if (e->type() == QEvent::WinIdChange) {
                // Shouldn't crash.
                effectiveWinId();
            }

            return QWidget::event(e);
        }
    };

    MyWidget child;
    child.setParent(&parent);
    parent.show();

    child.setParent(nullptr);
    child.setParent(&parent);
}

class CustomWidget : public QWidget
{
public:
    mutable int metricCallCount = 0;

    using QWidget::QWidget;

    int metric(PaintDeviceMetric metric) const override
    {
        ++metricCallCount;
        return QWidget::metric(metric);
    }
};

void tst_QWidget::customDpi()
{
    QScopedPointer<QWidget> topLevel(new QWidget);
    CustomWidget *custom = new CustomWidget(topLevel.data());
    QWidget *child = new QWidget(custom);

    custom->metricCallCount = 0;
    topLevel->logicalDpiX();
    QCOMPARE(custom->metricCallCount, 0);
    custom->logicalDpiX();
    QCOMPARE(custom->metricCallCount, 1);
    child->logicalDpiX();
    QCOMPARE(custom->metricCallCount, 1);
}

void tst_QWidget::customDpiProperty()
{
    QScopedPointer<QWidget> topLevel(new QWidget);
    QWidget *middle = new CustomWidget(topLevel.data());
    QWidget *child = new QWidget(middle);

    const int initialDpiX = topLevel->logicalDpiX();
    const int initialDpiY = topLevel->logicalDpiY();

    middle->setProperty("_q_customDpiX", 300);
    middle->setProperty("_q_customDpiY", 400);

    QCOMPARE(topLevel->logicalDpiX(), initialDpiX);
    QCOMPARE(topLevel->logicalDpiY(), initialDpiY);

    QCOMPARE(middle->logicalDpiX(), 300);
    QCOMPARE(middle->logicalDpiY(), 400);

    QCOMPARE(child->logicalDpiX(), 300);
    QCOMPARE(child->logicalDpiY(), 400);

    middle->setProperty("_q_customDpiX", QVariant());
    middle->setProperty("_q_customDpiY", QVariant());

    QCOMPARE(topLevel->logicalDpiX(), initialDpiX);
    QCOMPARE(topLevel->logicalDpiY(), initialDpiY);

    QCOMPARE(middle->logicalDpiX(), initialDpiX);
    QCOMPARE(middle->logicalDpiY(), initialDpiY);

    QCOMPARE(child->logicalDpiX(), initialDpiX);
    QCOMPARE(child->logicalDpiY(), initialDpiY);
}

void tst_QWidget::quitOnCloseAttribute()
{
    QWidget w;
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), true);
    w.setAttribute(Qt::WA_QuitOnClose, false);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);

    w.setAttribute(Qt::WA_QuitOnClose);
    w.setWindowFlags(Qt::Tool);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);

    w.setAttribute(Qt::WA_QuitOnClose);
    w.setWindowFlags(Qt::Popup);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);

    w.setAttribute(Qt::WA_QuitOnClose);
    w.setWindowFlags(Qt::ToolTip);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);

    w.setAttribute(Qt::WA_QuitOnClose);
    w.setWindowFlags(Qt::SplashScreen);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);

    w.setAttribute(Qt::WA_QuitOnClose);
    w.setWindowFlags(Qt::SubWindow);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);

    w.setAttribute(Qt::WA_QuitOnClose);
    w.setWindowFlags(Qt::Dialog);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), true);
    w.show();
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), true);
    w.setWindowFlags(Qt::Tool);
    QCOMPARE(w.testAttribute(Qt::WA_QuitOnClose), false);
}

void tst_QWidget::moveRect()
{
    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.resize(200, 200);
    widget.setUpdatesEnabled(false);
    QWidget child(&widget);
    child.setUpdatesEnabled(false);
    child.setAttribute(Qt::WA_OpaquePaintEvent);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    child.move(10, 10); // Don't crash.
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
class GDIWidget : public QDialog
{
    Q_OBJECT
public:
    GDIWidget() {
        setAttribute(Qt::WA_PaintOnScreen);
        timer.setSingleShot(true);
        timer.setInterval(0);
    }
    QPaintEngine *paintEngine() const override { return nullptr; }

    void paintEvent(QPaintEvent *) override
    {
        QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();
        const auto hdc = reinterpret_cast<HDC>(ni->nativeResourceForWindow(QByteArrayLiteral("getDC"), windowHandle()));
        if (hdc) {
            const HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
            SelectObject(hdc, brush);
            Rectangle(hdc, 0, 0, 10, 10);
            DeleteObject(brush);
            ni->nativeResourceForWindow(QByteArrayLiteral("releaseDC"), windowHandle());
        } else {
            qWarning("%s: Unable to obtain native DC.", Q_FUNC_INFO);
        }
        if (!timer.isActive()) {
            connect(&timer, &QTimer::timeout, this,
                    hdc ? &GDIWidget::slotTimer : &QDialog::reject);
            timer.start();
        }
    }

    QSize sizeHint() const override { return {400, 300}; };

private slots:
    void slotTimer() {
        QScreen *screen = windowHandle()->screen();
        const QImage im = screen->grabWindow(internalWinId(), 0, 0, -1, -1).toImage();
        color = im.pixel(1, 1);
        accept();
    }

public:
    QColor color;
    QTimer timer;
};

void tst_QWidget::gdiPainting()
{
    GDIWidget w;
    w.exec();

    QCOMPARE(w.color, QColor(255, 0, 0));

}

void tst_QWidget::paintOnScreenPossible()
{
    QWidget w1;
    w1.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w1.setAttribute(Qt::WA_PaintOnScreen);
    QVERIFY(!w1.testAttribute(Qt::WA_PaintOnScreen));

    GDIWidget w2;
    w2.setAttribute(Qt::WA_PaintOnScreen);
    QVERIFY(w2.testAttribute(Qt::WA_PaintOnScreen));
}
#endif // Q_OS_WIN && !Q_OS_WINRT

void tst_QWidget::reparentStaticWidget()
{
    QWidget window1;
    window1.setWindowTitle(QStringLiteral("window1 ") + __FUNCTION__);
    window1.resize(m_testWidgetSize);
    window1.move(m_availableTopLeft + QPoint(100, 100));

    QWidget *child = new QWidget(&window1);
    child->setPalette(Qt::red);
    child->setAutoFillBackground(true);
    child->setAttribute(Qt::WA_StaticContents);
    child->resize(window1.width() - 40,  window1.height() - 40);
    child->setWindowTitle(QStringLiteral("child ") + __FUNCTION__);

    QWidget *grandChild = new QWidget(child);
    grandChild->setPalette(Qt::blue);
    grandChild->setAutoFillBackground(true);
    grandChild->resize(50, 50);
    grandChild->setAttribute(Qt::WA_StaticContents);
    window1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window1));

    QWidget window2;
    window2.setWindowTitle(QStringLiteral("window2 ") + __FUNCTION__);
    window2.resize(m_testWidgetSize);
    window2.move(window1.geometry().topRight() + QPoint(100, 0));
    window2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window2));

    // Reparent into another top-level.
    child->setParent(&window2);
    child->show();

    // Please don't crash.
    window1.resize(window1.size() + QSize(2, 2));
    QTest::qWait(20);

    // Make sure we move all static children even though
    // the reparented widget itself is non-static.
    child->setAttribute(Qt::WA_StaticContents, false);
    child->setParent(&window1);
    child->show();

    // Please don't crash.
    window2.resize(window2.size() + QSize(2, 2));
    QTest::qWait(20);

    child->setParent(nullptr);
    child->show();
    QTest::qWait(20);

    // Please don't crash.
    child->resize(child->size() + QSize(2, 2));
    window2.resize(window2.size() + QSize(2, 2));
    QTest::qWait(20);

    QWidget *siblingOfGrandChild = new QWidget(child);
    siblingOfGrandChild->show();
    QTest::qWait(20);

    // Nothing should happen when reparenting within the same top-level.
    grandChild->setParent(siblingOfGrandChild);
    grandChild->show();
    QTest::qWait(20);

    QWidget paintOnScreen;
    paintOnScreen.setWindowTitle(QStringLiteral("paintOnScreen ") + __FUNCTION__);
    paintOnScreen.resize(m_testWidgetSize);
    paintOnScreen.move(window1.geometry().bottomLeft() + QPoint(0, 50));

    paintOnScreen.setAttribute(Qt::WA_PaintOnScreen);
    paintOnScreen.show();
    QVERIFY(QTest::qWaitForWindowExposed(&paintOnScreen));
    QTest::qWait(20);

    child->setParent(&paintOnScreen);
    child->show();
    QTest::qWait(20);

    // Please don't crash.
    paintOnScreen.resize(paintOnScreen.size() + QSize(2, 2));
    QTest::qWait(20);

}

void tst_QWidget::QTBUG6883_reparentStaticWidget2()
{
    QMainWindow mw;
    mw.setWindowTitle(QStringLiteral("MainWindow ") + __FUNCTION__);
    mw.move(m_availableTopLeft + QPoint(100, 100));

    QDockWidget *one = new QDockWidget(QStringLiteral("Dock ") + __FUNCTION__, &mw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, one , Qt::Vertical);

    QWidget *child = new QWidget();
    child->setPalette(Qt::red);
    child->setAutoFillBackground(true);
    child->setAttribute(Qt::WA_StaticContents);
    child->resize(m_testWidgetSize);
    one->setWidget(child);

    QToolBar *mainTools = mw.addToolBar("Main Tools");
    QLineEdit *le = new QLineEdit;
    le->setMinimumWidth(m_testWidgetSize.width());
    mainTools->addWidget(le);

    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    one->setFloating(true);
    QTest::qWait(20);
    //do not crash
}

class ColorRedWidget : public QWidget
{
public:
    explicit ColorRedWidget(QWidget *parent = nullptr)
        : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::ToolTip)
    {
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(rect(),Qt::red);
    }
};

void tst_QWidget::translucentWidget()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QPixmap pm(16,16);
    pm.fill(Qt::red);
    ColorRedWidget label;
    label.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    label.setFixedSize(16,16);
    label.setAttribute(Qt::WA_TranslucentBackground);
    const QPoint labelPos = QGuiApplication::primaryScreen()->availableGeometry().topLeft();
    label.move(labelPos);
    label.show();
    QVERIFY(QTest::qWaitForWindowExposed(&label));

    QPixmap widgetSnapshot;

#ifdef Q_OS_WIN
    QWidget *desktopWidget = QApplication::desktop()->screen(0);
    widgetSnapshot = grabWindow(desktopWidget->windowHandle(), labelPos.x(), labelPos.y(), label.width(), label.height());
#else
    widgetSnapshot = label.grab(QRect(QPoint(0, 0), label.size()));
#endif
    const QImage actual = widgetSnapshot.toImage().convertToFormat(QImage::Format_RGB32);
    QImage expected = pm.toImage().scaled(label.devicePixelRatioF() * pm.size());
    expected.setDevicePixelRatio(label.devicePixelRatioF());
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    QCOMPARE(actual.size(),expected.size());
    QCOMPARE(actual,expected);

    const QWindow *window = label.windowHandle();
    const QSurfaceFormat translucentFormat = window->requestedFormat();
    label.setAttribute(Qt::WA_TranslucentBackground, false);
    const QSurfaceFormat opaqueFormat = window->requestedFormat();
    QVERIFY(translucentFormat != opaqueFormat);
}

class MaskResizeTestWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaskResizeTestWidget(QWidget* p = nullptr) : QWidget(p)
    {
        setMask(QRegion(QRect(0, 0, 100, 100).normalized()));
    }

    void paintEvent(QPaintEvent* event) override
    {
        QPainter p(this);

        paintedRegion += event->region();
        for (const QRect &r : event->region())
            p.fillRect(r, Qt::red);
    }

    QRegion paintedRegion;

public slots:
    void enlargeMask() {
        QRegion newMask(QRect(0, 0, 150, 150).normalized());
        setMask(newMask);
    }

    void shrinkMask() {
        QRegion newMask(QRect(0, 0, 50, 50).normalized());
        setMask(newMask);
    }

};

void tst_QWidget::setClearAndResizeMask()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    UpdateWidget topLevel;
    topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    topLevel.resize(160, 160);
    centerOnScreen(&topLevel);
    topLevel.show();
    QApplication::setActiveWindow(&topLevel);
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTRY_VERIFY(topLevel.numPaintEvents > 0);
    topLevel.reset();

    // Mask top-level widget
    const QRegion topLevelMask(0, 0, 100, 100, QRegion::Ellipse);
    topLevel.setMask(topLevelMask);
    QCOMPARE(topLevel.mask(), topLevelMask);
    // Ensure that the top-level doesn't get any update.
    // We don't control what's happening on platforms other than X11, Windows
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows"))
        QCOMPARE(topLevel.numPaintEvents, 0);

    topLevel.reset();

    // Clear top-level mask
    topLevel.clearMask();
    QCOMPARE(topLevel.mask(), QRegion());
    QTest::qWait(10);
    QRegion outsideOldMask(topLevel.rect());
    outsideOldMask -= topLevelMask;
    // Ensure that the top-level gets an update for the area outside the old mask.
    // We don't control what's happening on platforms other than X11, Windows
    if (m_platform == QStringLiteral("xcb") || m_platform == QStringLiteral("windows")) {
        QTRY_VERIFY(topLevel.numPaintEvents > 0);
        QTRY_COMPARE(topLevel.paintedRegion, outsideOldMask);
    }

    UpdateWidget child(&topLevel);
    child.setAutoFillBackground(true); // NB! Opaque child.
    child.setPalette(Qt::red);
    child.resize(100, 100);
    child.show();
    QTest::qWait(10);

    child.reset();
    topLevel.reset();

    // Mask child widget with a mask that is smaller than the rect
    const QRegion childMask(0, 0, 50, 50);
    child.setMask(childMask);
    QTRY_COMPARE(child.mask(), childMask);
    // and ensure that the child widget doesn't get any update.
#ifdef Q_OS_MACOS
    // Mac always issues a full update when calling setMask, and we cannot force it to not do so.
    if (child.internalWinId())
        QCOMPARE(child.numPaintEvents, 1);
    else
#endif
    QCOMPARE(child.numPaintEvents, 0);
    // and the parent widget gets an update for the newly exposed area.
    QTRY_COMPARE(topLevel.numPaintEvents, 1);
    QRegion expectedParentExpose(child.rect());
    expectedParentExpose -= childMask;
    QCOMPARE(topLevel.paintedRegion, expectedParentExpose);

    child.reset();
    topLevel.reset();

    // Clear child widget mask
    child.clearMask();
    QTRY_COMPARE(child.mask(), QRegion());
    // and ensure that that the child widget gets an update for the area outside the old mask.
    QTRY_COMPARE(child.numPaintEvents, 1);
    outsideOldMask = child.rect();
#ifdef Q_OS_MACOS
    // Mac always issues a full update when calling setMask, and we cannot force it to not do so.
    if (!child.internalWinId())
#endif
    outsideOldMask -= childMask;
    QCOMPARE(child.paintedRegion, outsideOldMask);
    // and the parent widget doesn't get any update.
    QCOMPARE(topLevel.numPaintEvents, 0);

    child.reset();
    topLevel.reset();

    // Mask child widget with a mask that is bigger than the rect
    child.setMask(QRegion(0, 0, 1000, 1000));
#ifdef Q_OS_MACOS
    // Mac always issues a full update when calling setMask, and we cannot force it to not do so.
    if (child.internalWinId())
        QTRY_COMPARE(child.numPaintEvents, 1);
    else
#endif
    // and ensure that we don't get any updates at all.
    QTRY_COMPARE(child.numPaintEvents, 0);
    QCOMPARE(topLevel.numPaintEvents, 0);

    // ...and the same applies when clearing the mask.
    child.clearMask();
    QTest::qWait(100);
#ifdef Q_OS_MACOS
    // Mac always issues a full update when calling setMask, and we cannot force it to not do so.
    if (child.internalWinId())
        QTRY_VERIFY(child.numPaintEvents > 0);
    else
#endif
    QCOMPARE(child.numPaintEvents, 0);
    QCOMPARE(topLevel.numPaintEvents, 0);

    QWidget resizeParent;
    MaskResizeTestWidget resizeChild(&resizeParent);

    resizeParent.resize(300,300);
    resizeParent.raise();
    resizeParent.setWindowFlags(Qt::WindowStaysOnTopHint);
    resizeChild.setGeometry(50,50,200,200);
    QPalette pal = resizeParent.palette();
    pal.setColor(QPalette::Window, QColor(Qt::white));
    resizeParent.setPalette(pal);

    resizeParent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&resizeParent));
    // Disable the size grip on the Mac; otherwise it'll be included when grabbing the window.
    resizeParent.setFixedSize(resizeParent.size());
    resizeChild.show();
    QTest::qWait(100);
    resizeChild.paintedRegion = QRegion();

    QTimer::singleShot(100, &resizeChild, SLOT(shrinkMask()));
    QTest::qWait(200);
#ifdef Q_OS_MACOS
    // Mac always issues a full update when calling setMask, and we cannot force it to not do so.
    if (child.internalWinId())
        QTRY_COMPARE(resizeChild.paintedRegion, resizeChild.mask());
    else
#endif
    QTRY_COMPARE(resizeChild.paintedRegion, QRegion());

    resizeChild.paintedRegion = QRegion();
    const QRegion oldMask = resizeChild.mask();
    QTimer::singleShot(0, &resizeChild, SLOT(enlargeMask()));
    QTest::qWait(100);
#ifdef Q_OS_MACOS
    // Mac always issues a full update when calling setMask, and we cannot force it to not do so.
    if (child.internalWinId())
        QTRY_COMPARE(resizeChild.paintedRegion, resizeChild.mask());
    else
#endif
    QTRY_COMPARE(resizeChild.paintedRegion, resizeChild.mask() - oldMask);
}

void tst_QWidget::maskedUpdate()
{
    UpdateWidget topLevel;
    topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    topLevel.resize(200, 200);
    centerOnScreen(&topLevel);
    const QRegion topLevelMask(50, 50, 70, 70);
    topLevel.setMask(topLevelMask);

    UpdateWidget child(&topLevel);
    child.setGeometry(20, 20, 180, 180);
    const QRegion childMask(60, 60, 30, 30);
    child.setMask(childMask);

    UpdateWidget grandChild(&child);
    grandChild.setGeometry(50, 50, 100, 100);
    const QRegion grandChildMask(20, 20, 10, 10);
    grandChild.setMask(grandChildMask);

    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTRY_VERIFY(topLevel.numPaintEvents > 0);


#define RESET_WIDGETS \
    topLevel.reset(); \
    child.reset(); \
    grandChild.reset();

#define CLEAR_MASK(widget) \
    widget.clearMask(); \
    QTest::qWait(100); \
    RESET_WIDGETS;

    // All widgets are transparent at this point, so any call to update() will result
    // in composition, i.e. the update propagates to ancestors and children.

    // TopLevel update.
    RESET_WIDGETS;
    topLevel.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, topLevelMask);
    QTRY_COMPARE(child.paintedRegion, childMask);
    QTRY_COMPARE(grandChild.paintedRegion, grandChildMask);

    // Child update.
    RESET_WIDGETS;
    child.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, childMask.translated(child.pos()));
    QTRY_COMPARE(child.paintedRegion, childMask);
    QTRY_COMPARE(grandChild.paintedRegion, grandChildMask);

    // GrandChild update.
    RESET_WIDGETS;
    grandChild.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, grandChildMask.translated(grandChild.mapTo(&topLevel, QPoint())));
    QTRY_COMPARE(child.paintedRegion, grandChildMask.translated(grandChild.pos()));
    QTRY_COMPARE(grandChild.paintedRegion, grandChildMask);

    topLevel.setAttribute(Qt::WA_OpaquePaintEvent);
    child.setAttribute(Qt::WA_OpaquePaintEvent);
    grandChild.setAttribute(Qt::WA_OpaquePaintEvent);

    // All widgets are now opaque, which means no composition, i.e.
    // the update does not propate to ancestors and children.

    // TopLevel update.
    RESET_WIDGETS;
    topLevel.update();
    QTest::qWait(10);

    QRegion expectedTopLevelUpdate = topLevelMask;
    expectedTopLevelUpdate -= childMask.translated(child.pos()); // Subtract opaque children.
    QTRY_COMPARE(topLevel.paintedRegion, expectedTopLevelUpdate);
    QTRY_COMPARE(child.paintedRegion, QRegion());
    QTRY_COMPARE(grandChild.paintedRegion, QRegion());

    // Child update.
    RESET_WIDGETS;
    child.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, QRegion());
    QRegion expectedChildUpdate = childMask;
    expectedChildUpdate -= grandChildMask.translated(grandChild.pos()); // Subtract oapque children.
    QTRY_COMPARE(child.paintedRegion, expectedChildUpdate);
    QTRY_COMPARE(grandChild.paintedRegion, QRegion());

    // GrandChild update.
    RESET_WIDGETS;
    grandChild.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, QRegion());
    QTRY_COMPARE(child.paintedRegion, QRegion());
    QTRY_COMPARE(grandChild.paintedRegion, grandChildMask);

    // GrandChild update.
    CLEAR_MASK(grandChild);
    grandChild.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, QRegion());
    QTRY_COMPARE(child.paintedRegion, QRegion());
    QRegion expectedGrandChildUpdate = grandChild.rect();
    // Clip with parent's mask.
    expectedGrandChildUpdate &= childMask.translated(-grandChild.pos());
    QCOMPARE(grandChild.paintedRegion, expectedGrandChildUpdate);

    // GrandChild update.
    CLEAR_MASK(child);
    grandChild.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, QRegion());
    QTRY_COMPARE(child.paintedRegion, QRegion());
    expectedGrandChildUpdate = grandChild.rect();
    // Clip with parent's mask.
    expectedGrandChildUpdate &= topLevelMask.translated(-grandChild.mapTo(&topLevel, QPoint()));
    QTRY_COMPARE(grandChild.paintedRegion, expectedGrandChildUpdate);

    // Child update.
    RESET_WIDGETS;
    child.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, QRegion());
    expectedChildUpdate = child.rect();
    // Clip with parent's mask.
    expectedChildUpdate &= topLevelMask.translated(-child.pos());
    expectedChildUpdate -= grandChild.geometry(); // Subtract opaque children.
    QTRY_COMPARE(child.paintedRegion, expectedChildUpdate);
    QTRY_COMPARE(grandChild.paintedRegion, QRegion());

    // GrandChild update.
    CLEAR_MASK(topLevel);
    grandChild.update();
    QTest::qWait(10);

    QTRY_COMPARE(topLevel.paintedRegion, QRegion());
    QTRY_COMPARE(child.paintedRegion, QRegion());
    QTRY_COMPARE(grandChild.paintedRegion, QRegion(grandChild.rect())); // Full update.
}

#ifndef QT_NO_CURSOR
void tst_QWidget::syntheticEnterLeave()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: This fails. Figure out why.");
    class MyWidget : public QWidget
    {
    public:
        using QWidget::QWidget;
        void enterEvent(QEvent *) override { ++numEnterEvents; }
        void leaveEvent(QEvent *) override { ++numLeaveEvents; }
        int numEnterEvents = 0;
        int numLeaveEvents = 0;
    };

    QCursor::setPos(m_safeCursorPos);

    MyWidget window;
    window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window.setWindowFlags(Qt::WindowStaysOnTopHint);
    window.move(200, 200);
    window.resize(200, 200);

    MyWidget *child1 = new MyWidget(&window);
    child1->setPalette(Qt::blue);
    child1->setAutoFillBackground(true);
    child1->resize(200, 200);
    child1->setCursor(Qt::OpenHandCursor);

    MyWidget *child2 = new MyWidget(&window);
    child2->resize(200, 200);

    MyWidget *grandChild = new MyWidget(child2);
    grandChild->setPalette(Qt::red);
    grandChild->setAutoFillBackground(true);
    grandChild->resize(200, 200);
    grandChild->setCursor(Qt::WaitCursor);

    window.show();
    window.raise();

    QVERIFY(QTest::qWaitForWindowExposed(&window));

#define RESET_EVENT_COUNTS \
    window.numEnterEvents = 0; \
    window.numLeaveEvents = 0; \
    child1->numEnterEvents = 0; \
    child1->numLeaveEvents = 0; \
    child2->numEnterEvents = 0; \
    child2->numLeaveEvents = 0; \
    grandChild->numEnterEvents = 0; \
    grandChild->numLeaveEvents = 0;

    // Position the cursor in the middle of the window.
    const QPoint globalPos = window.mapToGlobal(QPoint(100, 100));
    QCursor::setPos(globalPos); // Enter child2 and grandChild.
    QTest::qWait(300);

    QCOMPARE(window.numLeaveEvents, 0);
    QCOMPARE(child2->numLeaveEvents, 0);
    QCOMPARE(grandChild->numLeaveEvents, 0);
    QCOMPARE(child1->numLeaveEvents, 0);

    // This event arrives asynchronously
    QTRY_COMPARE(window.numEnterEvents, 1);
    QCOMPARE(child2->numEnterEvents, 1);
    QCOMPARE(grandChild->numEnterEvents, 1);
    QCOMPARE(child1->numEnterEvents, 0);

    RESET_EVENT_COUNTS;
    child2->hide(); // Leave child2 and grandChild, enter child1.

    QCOMPARE(window.numLeaveEvents, 0);
    QCOMPARE(child2->numLeaveEvents, 1);
    QCOMPARE(grandChild->numLeaveEvents, 1);
    QCOMPARE(child1->numLeaveEvents, 0);

    QCOMPARE(window.numEnterEvents, 0);
    QCOMPARE(child2->numEnterEvents, 0);
    QCOMPARE(grandChild->numEnterEvents, 0);
    QCOMPARE(child1->numEnterEvents, 1);

    RESET_EVENT_COUNTS;
    child2->show(); // Leave child1, enter child2 and grandChild.

    QCOMPARE(window.numLeaveEvents, 0);
    QCOMPARE(child2->numLeaveEvents, 0);
    QCOMPARE(grandChild->numLeaveEvents, 0);
    QCOMPARE(child1->numLeaveEvents, 1);

    QCOMPARE(window.numEnterEvents, 0);
    QCOMPARE(child2->numEnterEvents, 1);
    QCOMPARE(grandChild->numEnterEvents, 1);
    QCOMPARE(child1->numEnterEvents, 0);

    RESET_EVENT_COUNTS;
    delete child2; // Enter child1 (and do not send leave events to child2 and grandChild).

    QCOMPARE(window.numLeaveEvents, 0);
    QCOMPARE(child1->numLeaveEvents, 0);

    QCOMPARE(window.numEnterEvents, 0);
    QCOMPARE(child1->numEnterEvents, 1);
}
#endif

#ifndef QT_NO_CURSOR
void tst_QWidget::taskQTBUG_4055_sendSyntheticEnterLeave()
{
    if (m_platform == QStringLiteral("wayland"))
        QSKIP("Wayland: Clients can't set cursor position on wayland.");
    class SELParent : public QWidget
    {
    public:
        using QWidget::QWidget;

        void mousePressEvent(QMouseEvent *) override { child->show(); }
        QWidget *child = nullptr;
    };

    class SELChild : public QWidget
    {
    public:
        using QWidget::QWidget;
        void enterEvent(QEvent *) override { ++numEnterEvents; }
        void mouseMoveEvent(QMouseEvent *event) override
        {
            QCOMPARE(event->button(), Qt::NoButton);
            QCOMPARE(event->buttons(), QApplication::mouseButtons());
            QCOMPARE(event->modifiers(), QApplication::keyboardModifiers());
            ++numMouseMoveEvents;
        }
        void reset() { numEnterEvents = numMouseMoveEvents = 0; }
        int numEnterEvents = 0, numMouseMoveEvents = 0;
    };

     QCursor::setPos(m_safeCursorPos);

     SELParent parent;
     parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
     parent.move(200, 200);
     parent.resize(200, 200);
     SELChild child(&parent);
     child.resize(200, 200);
     parent.show();
     QVERIFY(QTest::qWaitForWindowActive(&parent));

     QCursor::setPos(child.mapToGlobal(QPoint(100, 100)));
     // Make sure the cursor has entered the child.
     QTRY_VERIFY(child.numEnterEvents > 0);

     child.hide();
     child.reset();
     child.show();

     // Make sure the child gets enter event and no mouse move event.
     QTRY_COMPARE(child.numEnterEvents, 1);
     QCOMPARE(child.numMouseMoveEvents, 0);

     child.hide();
     child.reset();
     child.setMouseTracking(true);
     child.show();

     // Make sure the child gets enter event.
     // Note that we verify event->button() and event->buttons()
     // in SELChild::mouseMoveEvent().
     QTRY_COMPARE(child.numEnterEvents, 1);
     QCOMPARE(child.numMouseMoveEvents, 0);

     // Sending synthetic enter/leave trough the parent's mousePressEvent handler.
     parent.child = &child;

     child.hide();
     child.reset();
     QTest::mouseClick(&parent, Qt::LeftButton);

     // Make sure the child gets enter event.
     QTRY_COMPARE(child.numEnterEvents, 1);
     QCOMPARE(child.numMouseMoveEvents, 0);

     child.hide();
     child.reset();
     QTest::keyPress(&parent, Qt::Key_Shift);
     QTest::mouseClick(&parent, Qt::LeftButton);

     // Make sure the child gets enter event
     QTRY_COMPARE(child.numEnterEvents, 1);
     QCOMPARE(child.numMouseMoveEvents, 0);
     QTest::keyRelease(&child, Qt::Key_Shift);
     child.hide();
     child.reset();
     child.setMouseTracking(false);
     QTest::mouseClick(&parent, Qt::LeftButton);

     // Make sure the child gets enter event and no mouse move event.
     QTRY_COMPARE(child.numEnterEvents, 1);
     QCOMPARE(child.numMouseMoveEvents, 0);
 }
#endif

void tst_QWidget::windowFlags()
{
    QWidget w;
    const auto baseFlags = w.windowFlags();
    w.setWindowFlags(w.windowFlags() | Qt::FramelessWindowHint);
    QVERIFY(w.windowFlags() & Qt::FramelessWindowHint);
    w.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    QCOMPARE(w.windowFlags(), baseFlags | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    w.setWindowFlag(Qt::FramelessWindowHint, false);
    QCOMPARE(w.windowFlags(), baseFlags | Qt::WindowStaysOnTopHint);
}

void tst_QWidget::initialPosForDontShowOnScreenWidgets()
{
    { // Check default position.
        const QPoint expectedPos(0, 0);
        QWidget widget;
        widget.setAttribute(Qt::WA_DontShowOnScreen);
        widget.winId(); // Make sure QWidgetPrivate::create is called.
        QCOMPARE(widget.pos(), expectedPos);
        QCOMPARE(widget.geometry().topLeft(), expectedPos);
    }

    { // Explicitly move to a position.
        const QPoint expectedPos(100, 100);
        QWidget widget;
        widget.setAttribute(Qt::WA_DontShowOnScreen);
        widget.move(expectedPos);
        widget.winId(); // Make sure QWidgetPrivate::create is called.
        QCOMPARE(widget.pos(), expectedPos);
        QCOMPARE(widget.geometry().topLeft(), expectedPos);
    }
}

class MyEvilObject : public QObject
{
    Q_OBJECT
public:
    explicit MyEvilObject(QWidget *widgetToCrash) : QObject(), widget(widgetToCrash)
    {
        connect(widget, &QObject::destroyed, this, &MyEvilObject::beEvil);
        delete widget;
    }
    QWidget *widget;

private slots:
    void beEvil(QObject *) { widget->update(0, 0, 150, 150); }
};

void tst_QWidget::updateOnDestroyedSignal()
{
    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));

    QWidget *child = new QWidget(&widget);
    child->resize(m_testWidgetSize);
    child->setAutoFillBackground(true);
    child->setPalette(Qt::red);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    // Please do not crash.
    MyEvilObject evil(child);
    QTest::qWait(200);
}

void tst_QWidget::toplevelLineEditFocus()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QLineEdit w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setMinimumWidth(m_testWidgetSize.width());
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_COMPARE(QApplication::activeWindow(), static_cast<const QWidget *>(&w));
    QTRY_COMPARE(QApplication::focusWidget(), static_cast<const QWidget *>(&w));
}

void tst_QWidget::focusWidget_task254563()
{
    //having different visibility for widget is important
    QWidget top;
    top.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    top.show();
    QWidget container(&top);
    QWidget *widget = new QWidget(&container);
    widget->show();

    widget->setFocus(); //set focus (will set the focus widget up to the toplevel to be 'widget')
    container.setFocus();
    delete widget; // will call clearFocus but that doesn't help
    QVERIFY(top.focusWidget() != widget); //dangling pointer
}

// This test case relies on developer build (AUTOTEST_EXPORT).
#ifdef QT_BUILD_INTERNAL
void tst_QWidget::destroyBackingStore()
{
    UpdateWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    centerOnScreen(&w);
    w.reset();
    w.show();

    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QApplication::processEvents();
    QTRY_VERIFY(w.numPaintEvents > 0);
    w.reset();
    w.update();
    qt_widget_private(&w)->topData()->repaintManager.reset(new QWidgetRepaintManager(&w));

    w.update();
    QApplication::processEvents();

    QCOMPARE(w.numPaintEvents, 1);

    // Check one more time, because the second time around does more caching.
    w.update();
    QApplication::processEvents();
    QCOMPARE(w.numPaintEvents, 2);
}
#endif // QT_BUILD_INTERNAL

// Helper function
QWidgetRepaintManager* repaintManager(QWidget &widget)
{
    QWidgetRepaintManager *repaintManager = nullptr;
#ifdef QT_BUILD_INTERNAL
    if (QTLWExtra *topExtra = qt_widget_private(&widget)->maybeTopData())
        repaintManager = topExtra->repaintManager.get();
#endif
    return repaintManager;
}

// Tables of 5000 elements do not make sense on Windows Mobile.
void tst_QWidget::rectOutsideCoordinatesLimit_task144779()
{
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(Qt::BlankCursor); //keep the cursor out of screen grabs
#endif
    QWidget main(nullptr, Qt::FramelessWindowHint); //don't get confused by the size of the window frame
    main.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QPalette palette;
    palette.setColor(QPalette::Window, Qt::red);
    main.setPalette(palette);

    QRect desktopDimensions = main.screen()->availableGeometry();
    QSize mainSize(400, 400);
    mainSize = mainSize.boundedTo(desktopDimensions.size());
    main.resize(mainSize);

    QWidget *offsetWidget = new QWidget(&main);
    offsetWidget->setGeometry(0, -(15000 - mainSize.height()), mainSize.width(), 15000);

    // big widget is too big for the coordinates, it must be limited by wrect
    // if wrect is not at the right position because of offsetWidget, bigwidget
    // is not painted correctly
    QWidget *bigWidget = new QWidget(offsetWidget);
    bigWidget->setGeometry(0, 0, mainSize.width(), 50000);
    palette.setColor(QPalette::Window, Qt::green);
    bigWidget->setPalette(palette);
    bigWidget->setAutoFillBackground(true);

    main.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&main));

    QPixmap correct(main.size());
    correct.fill(Qt::green);
    const QPixmap mainPixmap = grabFromWidget(&main, QRect(QPoint(0, 0), QSize(-1, -1)));

    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    QTRY_COMPARE(mainPixmap.toImage().convertToFormat(QImage::Format_RGB32),
                 correct.toImage().convertToFormat(QImage::Format_RGB32));
#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif
}

void tst_QWidget::setGraphicsEffect()
{
    // Check that we don't have any effect by default.
    QScopedPointer<QWidget> widget(new QWidget);
    widget->setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QVERIFY(!widget->graphicsEffect());

    // SetGet check.
    QPointer<QGraphicsEffect> blurEffect = new QGraphicsBlurEffect;
    widget->setGraphicsEffect(blurEffect);
    QCOMPARE(widget->graphicsEffect(), static_cast<QGraphicsEffect *>(blurEffect));

    // Ensure the existing effect is deleted when setting a new one.
    QPointer<QGraphicsEffect> shadowEffect = new QGraphicsDropShadowEffect;
    widget->setGraphicsEffect(shadowEffect);
    QVERIFY(!blurEffect);
    QCOMPARE(widget->graphicsEffect(), static_cast<QGraphicsEffect *>(shadowEffect));
    blurEffect = new QGraphicsBlurEffect;

    // Ensure the effect is uninstalled when setting it on a new target.
    QScopedPointer<QWidget> anotherWidget(new QWidget);
    anotherWidget->setGraphicsEffect(blurEffect);
    widget->setGraphicsEffect(blurEffect);
    QVERIFY(!anotherWidget->graphicsEffect());
    QVERIFY(!shadowEffect);

    // Ensure the existing effect is deleted when deleting the widget.
    widget.reset();
    QVERIFY(!blurEffect);
    anotherWidget.reset();

    // Ensure the effect is uninstalled when deleting it
    widget.reset(new QWidget);
    blurEffect = new QGraphicsBlurEffect;
    widget->setGraphicsEffect(blurEffect);
    delete blurEffect;
    QVERIFY(!widget->graphicsEffect());

    // Ensure the existing effect is uninstalled and deleted when setting a null effect
    blurEffect = new QGraphicsBlurEffect;
    widget->setGraphicsEffect(blurEffect);
    widget->setGraphicsEffect(nullptr);
    QVERIFY(!widget->graphicsEffect());
    QVERIFY(!blurEffect);
}

void tst_QWidget::activateWindow()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported.");

    // Test case for QTBUG-26711

    // Create first mainwindow and set it active
    QScopedPointer<QMainWindow> mainwindow(new QMainWindow);
    mainwindow->setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QLabel* label = new QLabel(mainwindow.data());
    label->setMinimumWidth(m_testWidgetSize.width());
    mainwindow->setWindowTitle(QStringLiteral("#1 ") + __FUNCTION__);
    mainwindow->setCentralWidget(label);
    mainwindow->move(m_availableTopLeft + QPoint(100, 100));
    mainwindow->setVisible(true);
    mainwindow->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(mainwindow.data()));
    QVERIFY(mainwindow->isActiveWindow());

    // Create second mainwindow and set it active
    QScopedPointer<QMainWindow> mainwindow2(new QMainWindow);
    mainwindow2->setWindowTitle(QStringLiteral("#2 ") + __FUNCTION__);
    QLabel* label2 = new QLabel(mainwindow2.data());
    label2->setMinimumWidth(m_testWidgetSize.width());
    mainwindow2->setCentralWidget(label2);
    mainwindow2->move(mainwindow->geometry().bottomLeft() + QPoint(0, 50));
    mainwindow2->setVisible(true);
    mainwindow2->activateWindow();
    QCoreApplication::processEvents();

    QTRY_VERIFY(!mainwindow->isActiveWindow());
    QTRY_VERIFY(mainwindow2->isActiveWindow());

    // Revert first mainwindow back to visible active
    mainwindow->setVisible(true);
    mainwindow->activateWindow();
    QCoreApplication::processEvents();

    QTRY_VERIFY(mainwindow->isActiveWindow());
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    QTRY_VERIFY(!mainwindow2->isActiveWindow());
}

void tst_QWidget::openModal_taskQTBUG_5804()
{
    class Widget : public QWidget
    {
    public:
        Widget(QWidget *parent) : QWidget(parent) {}
        ~Widget()
        {
            QMessageBox msgbox;
            QTimer::singleShot(10, &msgbox, SLOT(accept()));
            msgbox.exec(); //open a modal dialog
        }
    };

    QScopedPointer<QWidget> win(new QWidget);
    win->resize(m_testWidgetSize);
    win->setWindowTitle(__FUNCTION__);
    centerOnScreen(win.data());

    new Widget(win.data());
    win->show();
    QVERIFY(QTest::qWaitForWindowExposed(win.data()));
}

/*!
    Test that the focus proxy receives focus, and that changing the
    focus proxy of a widget that has focus passes focus on correctly.

    The test uses a single window, so we can rely on the window's focus
    widget and the QApplication focus widget to be the same.
*/
void tst_QWidget::focusProxy()
{
    QWidget window;
    window.setFocusPolicy(Qt::StrongFocus);
    class Container : public QWidget
    {
    public:
        Container()
        {
            edit = new QLineEdit;
            edit->installEventFilter(this);
            setFocusProxy(edit);
            QHBoxLayout *layout = new QHBoxLayout;
            layout->addWidget(edit);
            setLayout(layout);
        }

        QLineEdit *edit;
        int focusInCount = 0;
        int focusOutCount = 0;

    protected:
        bool eventFilter(QObject *receiver, QEvent *event)
        {
            if (receiver == edit) {
                switch (event->type()) {
                case QEvent::FocusIn:
                    ++focusInCount;
                    break;
                case QEvent::FocusOut:
                    ++focusOutCount;
                    break;
                default:
                    break;
                }
            }

            return QWidget::eventFilter(receiver, event);
        }
    };

    auto container1 = new Container;
    container1->edit->setObjectName("edit1");
    auto container2 = new Container;
    container2->edit->setObjectName("edit2");

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(container1);
    layout->addWidget(container2);
    window.setLayout(layout);

    window.setFocus();
    window.show();
    window.activateWindow();
    if (!QTest::qWaitForWindowExposed(&window) || !QTest::qWaitForWindowActive(&window))
        QSKIP("Window activation failed");

    // given a widget without focus proxy
    QVERIFY(window.hasFocus());
    QCOMPARE(&window, QApplication::focusWidget());
    QVERIFY(!container1->hasFocus());
    QVERIFY(!container2->hasFocus());
    QCOMPARE(container1->focusInCount, 0);
    QCOMPARE(container1->focusOutCount, 0);

    // setting a (nested) focus proxy moves focus
    window.setFocusProxy(container1);
    QCOMPARE(window.focusWidget(), container1->edit);
    QCOMPARE(window.focusWidget(), QApplication::focusWidget());
    QVERIFY(window.hasFocus()); // and redirects hasFocus correctly
    QVERIFY(container1->edit->hasFocus());
    QCOMPARE(container1->focusInCount, 1);

    // changing the focus proxy should not move focus
    window.setFocusProxy(container2);
    QCOMPARE(window.focusWidget(), container1->edit);
    QCOMPARE(window.focusWidget(), QApplication::focusWidget());
    QVERIFY(!window.hasFocus());
    QCOMPARE(container1->focusOutCount, 0);

    // but setting focus again does
    window.setFocus();
    QCOMPARE(window.focusWidget(), container2->edit);
    QCOMPARE(window.focusWidget(), QApplication::focusWidget());
    QVERIFY(window.hasFocus());
    QVERIFY(!container1->edit->hasFocus());
    QVERIFY(container2->edit->hasFocus());
    QCOMPARE(container1->focusInCount, 1);
    QCOMPARE(container1->focusOutCount, 1);
    QCOMPARE(container2->focusInCount, 1);
    QCOMPARE(container2->focusOutCount, 0);

    // clearing the focus proxy does not move focus
    window.setFocusProxy(nullptr);
    QCOMPARE(window.focusWidget(), container2->edit);
    QCOMPARE(window.focusWidget(), QApplication::focusWidget());
    QVERIFY(!window.hasFocus());
    QCOMPARE(container1->focusInCount, 1);
    QCOMPARE(container1->focusOutCount, 1);
    QCOMPARE(container2->focusInCount, 1);
    QCOMPARE(container2->focusOutCount, 0);

    // but clearing focus does
    window.focusWidget()->clearFocus();
    QCOMPARE(QApplication::focusWidget(), nullptr);
    QVERIFY(!window.hasFocus());
    QVERIFY(!container2->hasFocus());
    QVERIFY(!container2->edit->hasFocus());
    QCOMPARE(container2->focusOutCount, 1);
}

void tst_QWidget::focusProxyAndInputMethods()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported.");
    QScopedPointer<QWidget> toplevel(new QWidget(nullptr, Qt::X11BypassWindowManagerHint));
    toplevel->setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    toplevel->resize(200, 200);
    toplevel->setAttribute(Qt::WA_InputMethodEnabled, true);

    QWidget *child = new QWidget(toplevel.data());
    child->setFocusProxy(toplevel.data());
    child->setAttribute(Qt::WA_InputMethodEnabled, true);

    toplevel->setFocusPolicy(Qt::WheelFocus);
    child->setFocusPolicy(Qt::WheelFocus);

    QVERIFY(!child->hasFocus());
    QVERIFY(!toplevel->hasFocus());

    toplevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(toplevel.data()));
    QApplication::setActiveWindow(toplevel.data());
    QVERIFY(QTest::qWaitForWindowActive(toplevel.data()));
    QVERIFY(toplevel->hasFocus());
    QVERIFY(child->hasFocus());
    QCOMPARE(qApp->focusObject(), toplevel.data());
}

#ifdef QT_BUILD_INTERNAL
class scrollWidgetWBS : public QWidget
{
public:
    void deleteBackingStore()
    {
        static_cast<QWidgetPrivate*>(d_ptr.data())->topData()->repaintManager.reset(nullptr);
    }
    void enableBackingStore()
    {
        if (!static_cast<QWidgetPrivate*>(d_ptr.data())->maybeRepaintManager()) {
            static_cast<QWidgetPrivate*>(d_ptr.data())->topData()->repaintManager.reset(new QWidgetRepaintManager(this));
            static_cast<QWidgetPrivate*>(d_ptr.data())->invalidateBackingStore(this->rect());
            update();
        }
    }
};
#endif

// Test case relies on developer build (AUTOTEST_EXPORT).
#ifdef QT_BUILD_INTERNAL
void tst_QWidget::scrollWithoutBackingStore()
{
    scrollWidgetWBS scrollable;
    scrollable.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    scrollable.resize(200, 200);
    QLabel child(QString("@"),&scrollable);
    child.resize(50,50);
    scrollable.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scrollable));
    scrollable.scroll(50,50);
    QCOMPARE(child.pos(),QPoint(50,50));
    scrollable.deleteBackingStore();
    scrollable.scroll(-25,-25);
    QCOMPARE(child.pos(),QPoint(25,25));
    scrollable.enableBackingStore();
    QTRY_COMPARE(child.pos(),QPoint(25,25));
}
#endif

void tst_QWidget::taskQTBUG_7532_tabOrderWithFocusProxy()
{
    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setFocusPolicy(Qt::TabFocus);
    QWidget *fp = new QWidget(&w);
    fp->setFocusPolicy(Qt::TabFocus);
    w.setFocusProxy(fp);
    QWidget::setTabOrder(&w, fp);

    // In debug mode, no assertion failure means it's alright.
}

void tst_QWidget::movedAndResizedAttributes()
{
    // Use Qt::Tool as fully decorated windows have a minimum width of 160 on
    QWidget w(nullptr, Qt::Tool);
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.show();

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setWindowState(Qt::WindowFullScreen);

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setWindowState(Qt::WindowMaximized);

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.setWindowState(Qt::WindowMinimized);

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.showNormal();

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.showMaximized();

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.showFullScreen();

    QVERIFY(!w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.showNormal();
    w.move(10,10);
    QVERIFY(w.testAttribute(Qt::WA_Moved));
    QVERIFY(!w.testAttribute(Qt::WA_Resized));

    w.resize(100, 100);
    QVERIFY(w.testAttribute(Qt::WA_Moved));
    QVERIFY(w.testAttribute(Qt::WA_Resized));
}

void tst_QWidget::childAt()
{
    QWidget parent(nullptr, Qt::FramelessWindowHint);
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    parent.resize(200, 200);

    QWidget *child = new QWidget(&parent);
    child->setPalette(Qt::red);
    child->setAutoFillBackground(true);
    child->setGeometry(20, 20, 160, 160);

    QWidget *grandChild = new QWidget(child);
    grandChild->setPalette(Qt::blue);
    grandChild->setAutoFillBackground(true);
    grandChild->setGeometry(-20, -20, 220, 220);

    QVERIFY(!parent.childAt(19, 19));
    QVERIFY(!parent.childAt(180, 180));
    QCOMPARE(parent.childAt(20, 20), grandChild);
    QCOMPARE(parent.childAt(179, 179), grandChild);

    grandChild->setAttribute(Qt::WA_TransparentForMouseEvents);
    QCOMPARE(parent.childAt(20, 20), child);
    QCOMPARE(parent.childAt(179, 179), child);
    grandChild->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    child->setMask(QRect(50, 50, 60, 60));

    QVERIFY(!parent.childAt(69, 69));
    QVERIFY(!parent.childAt(130, 130));
    QCOMPARE(parent.childAt(70, 70), grandChild);
    QCOMPARE(parent.childAt(129, 129), grandChild);

    child->setAttribute(Qt::WA_MouseNoMask);
    QCOMPARE(parent.childAt(69, 69), grandChild);
    QCOMPARE(parent.childAt(130, 130), grandChild);
    child->setAttribute(Qt::WA_MouseNoMask, false);

    grandChild->setAttribute(Qt::WA_TransparentForMouseEvents);
    QCOMPARE(parent.childAt(70, 70), child);
    QCOMPARE(parent.childAt(129, 129), child);
    grandChild->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    grandChild->setMask(QRect(80, 80, 40, 40));

    QCOMPARE(parent.childAt(79, 79), child);
    QCOMPARE(parent.childAt(120, 120), child);
    QCOMPARE(parent.childAt(80, 80), grandChild);
    QCOMPARE(parent.childAt(119, 119), grandChild);

    grandChild->setAttribute(Qt::WA_MouseNoMask);

    QCOMPARE(parent.childAt(79, 79), grandChild);
    QCOMPARE(parent.childAt(120, 120), grandChild);
}

#ifdef Q_OS_MACOS

void tst_QWidget::taskQTBUG_11373()
{
    QSKIP("QTBUG-52974");

    QScopedPointer<QMainWindow> myWindow(new QMainWindow);
    QWidget * center = new QWidget();
    myWindow -> setCentralWidget(center);
    QWidget * drawer = new QWidget(myWindow.data(), Qt::Drawer);
    drawer -> hide();
    QCOMPARE(drawer->isVisible(), false);
    myWindow -> show();
    myWindow -> raise();
    // The drawer shouldn't be visible now.
    QCOMPARE(drawer->isVisible(), false);
    myWindow -> setWindowState(Qt::WindowFullScreen);
    myWindow -> setWindowState(Qt::WindowNoState);
    // The drawer should still not be visible, since we haven't shown it.
    QCOMPARE(drawer->isVisible(), false);
}

#endif

void tst_QWidget::taskQTBUG_17333_ResizeInfiniteRecursion()
{
    QTableView tb;
    tb.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    const char *s = "border: 1px solid;";
    tb.setStyleSheet(s);
    tb.show();

    QVERIFY(QTest::qWaitForWindowExposed(&tb));
    tb.setGeometry(QRect(100, 100, 0, 100));
    // No crash, it works.
}

void tst_QWidget::nativeChildFocus()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setMinimumWidth(m_testWidgetSize.width());
    w.setWindowTitle(__FUNCTION__);
    QLayout *layout = new QVBoxLayout;
    w.setLayout(layout);
    QLineEdit *p1 = new QLineEdit;
    QLineEdit *p2 = new QLineEdit;
    layout->addWidget(p1);
    layout->addWidget(p2);
    p1->setObjectName("p1");
    p2->setObjectName("p2");
    centerOnScreen(&w);
    w.show();
    w.activateWindow();
    p1->setFocus();
    p1->setAttribute(Qt::WA_NativeWindow);
    p2->setAttribute(Qt::WA_NativeWindow);
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QCOMPARE(QApplication::activeWindow(), &w);
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(p1));
}

static bool lenientCompare(const QPixmap &actual, const QPixmap &expected)
{
    QImage expectedImage = expected.toImage().convertToFormat(QImage::Format_RGB32);
    QImage actualImage = actual.toImage().convertToFormat(QImage::Format_RGB32);

    if (expectedImage.size() != actualImage.size()) {
        qWarning("Image size comparison failed: expected: %dx%d, got %dx%d",
                 expectedImage.size().width(), expectedImage.size().height(),
                 actualImage.size().width(), actualImage.size().height());
        return false;
    }

    const int size = actual.width() * actual.height();
    const int threshold = QPixmap::defaultDepth() == 16 ? 10 : 2;

    auto a = reinterpret_cast<const QRgb *>(actualImage.bits());
    auto e = reinterpret_cast<const QRgb *>(expectedImage.bits());
    for (int i = 0; i < size; ++i) {
        const QColor ca(a[i]);
        const QColor ce(e[i]);
        if (qAbs(ca.red() - ce.red()) > threshold
            || qAbs(ca.green() - ce.green()) > threshold
            || qAbs(ca.blue() - ce.blue()) > threshold) {
            qWarning("Color mismatch at pixel #%d: Expected: %d,%d,%d, got %d,%d,%d",
                     i, ce.red(), ce.green(), ce.blue(), ca.red(), ca.green(), ca.blue());
            return false;
        }
    }

    return true;
}

void tst_QWidget::grab()
{
    for (int opaque = 0; opaque < 2; ++opaque) {
        QWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        QImage image(128, 128, opaque ? QImage::Format_RGB32 : QImage::Format_ARGB32_Premultiplied);
        for (int row = 0; row < image.height(); ++row) {
            QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(row));
            for (int col = 0; col < image.width(); ++col)
                line[col] = qRgba(QRandomGenerator::global()->bounded(255), row, col, opaque ? 255 : 127);
        }

        QPalette pal = widget.palette();
        pal.setBrush(QPalette::Window, QBrush(image));
        widget.setPalette(pal);
        widget.resize(128, 128);

        QPixmap expected(64, 64);
        if (!opaque)
            expected.fill(Qt::transparent);

        QPainter p(&expected);
        p.translate(-64, -64);
        p.drawTiledPixmap(0, 0, 128, 128, pal.brush(QPalette::Window).texture(), 0, 0);
        p.end();

        QPixmap actual = grabFromWidget(&widget, QRect(64, 64, 64, 64));
        QVERIFY(lenientCompare(actual, expected));

        actual = grabFromWidget(&widget, QRect(64, 64, -1, -1));
        QVERIFY(lenientCompare(actual, expected));

        // Make sure a widget that is not yet shown is grabbed correctly.
        QTreeWidget widget2;
        actual = widget2.grab(QRect());
        widget2.show();
        expected = widget2.grab(QRect());

        QVERIFY(lenientCompare(actual, expected));
    }
}

/* grabMouse() tests whether mouse grab for a widget without window handle works.
 * It creates a top level widget with another nested widget inside. The inner widget grabs
 * the mouse and a series of mouse presses moving over the top level's window is simulated.
 * Only the inner widget should receive events. */

static inline QString mouseEventLogEntry(const QString &objectName, QEvent::Type t, const QPoint &p, Qt::MouseButtons b)
{
    QString result;
    QDebug(&result).nospace() << objectName << " Mouse event " << t << " at " << p << " buttons " << b;
    return result;
}

class GrabLoggerWidget : public QWidget
{
public:
    explicit GrabLoggerWidget(QStringList *log, QWidget *parent = nullptr)  : QWidget(parent), m_log(log) {}

protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease: {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);
            m_log->push_back(mouseEventLogEntry(objectName(), me->type(), me->pos(), me->buttons()));
            me->accept();
            return true;
        }
        default:
            break;
        }
        return QWidget::event(e);
    }
private:
    QStringList *m_log;
};

void tst_QWidget::grabMouse()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStringList log;
    GrabLoggerWidget w(&log);
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setObjectName(QLatin1String("tst_qwidget_grabMouse"));
    w.setWindowTitle(w.objectName());
    QLayout *layout = new QVBoxLayout(&w);
    layout->setContentsMargins(50, 50, 50, 50);
    GrabLoggerWidget *grabber = new GrabLoggerWidget(&log, &w);
    const QString grabberObjectName = QLatin1String("tst_qwidget_grabMouse_grabber");
    grabber->setObjectName(grabberObjectName);
    grabber->setMinimumSize(m_testWidgetSize);
    layout->addWidget(grabber);
    centerOnScreen(&w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QStringList expectedLog;
    QPoint mousePos = QPoint(w.width() / 2, 10);
    QTest::mouseMove(w.windowHandle(), mousePos);
    grabber->grabMouse();
    const int step = w.height() / 5;
    for ( ; mousePos.y() < w.height() ; mousePos.ry() += step) {
        QTest::mouseClick(w.windowHandle(), Qt::LeftButton, Qt::KeyboardModifiers(), mousePos);
        // Events should go to the grabber child using its coordinates.
        const QPoint expectedPos = grabber->mapFromParent(mousePos);
        expectedLog.push_back(mouseEventLogEntry(grabberObjectName, QEvent::MouseButtonPress, expectedPos, Qt::LeftButton));
        expectedLog.push_back(mouseEventLogEntry(grabberObjectName, QEvent::MouseButtonRelease, expectedPos, Qt::NoButton));
    }
    grabber->releaseMouse();
    QCOMPARE(log, expectedLog);
}

void tst_QWidget::grabKeyboard()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setObjectName(QLatin1String("tst_qwidget_grabKeyboard"));
    w.setWindowTitle(w.objectName());
    QLayout *layout = new QVBoxLayout(&w);
    QLineEdit *grabber = new QLineEdit(&w);
    grabber->setMinimumWidth(m_testWidgetSize.width());
    layout->addWidget(grabber);
    QLineEdit *nonGrabber = new QLineEdit(&w);
    nonGrabber->setMinimumWidth(m_testWidgetSize.width());
    layout->addWidget(nonGrabber);
    centerOnScreen(&w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    nonGrabber->setFocus();
    grabber->grabKeyboard();
    QTest::keyClick(w.windowHandle(), Qt::Key_A);
    grabber->releaseKeyboard();
    QCOMPARE(grabber->text().toLower(), QStringLiteral("a"));
    QVERIFY2(nonGrabber->text().isEmpty(), qPrintable(nonGrabber->text()));
}

class TouchMouseWidget : public QWidget {
public:
    explicit TouchMouseWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        resize(200, 200);
    }

    void setAcceptTouch(bool accept)
    {
        m_acceptTouch = accept;
        setAttribute(Qt::WA_AcceptTouchEvents, accept);
    }

    void setAcceptMouse(bool accept)
    {
        m_acceptMouse = accept;
    }

protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            if (e->type() == QEvent::TouchBegin)
                ++m_touchBeginCount;
            else if (e->type() == QEvent::TouchUpdate)
                ++m_touchUpdateCount;
            else if (e->type() == QEvent::TouchEnd)
                ++m_touchEndCount;
            ++m_touchEventCount;
            if (m_acceptTouch)
                e->accept();
            else
                e->ignore();
            return true;
        case QEvent::Gesture:
            ++m_gestureEventCount;
            return true;

        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
            ++m_mouseEventCount;
            m_lastMouseEventPos = static_cast<QMouseEvent *>(e)->localPos();
            if (m_acceptMouse)
                e->accept();
            else
                e->ignore();
            return true;

        default:
            return QWidget::event(e);
        }
    }

public:
    int m_touchBeginCount = 0;
    int m_touchUpdateCount = 0;
    int m_touchEndCount = 0;
    int m_touchEventCount = 0;
    int m_gestureEventCount = 0;
    bool m_acceptTouch = false;
    int m_mouseEventCount = 0;
    bool m_acceptMouse = true;
    QPointF m_lastMouseEventPos;
};

void tst_QWidget::touchEventSynthesizedMouseEvent()
{
    {
        // Simple case, we ignore the touch events, we get mouse events instead
        TouchMouseWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(widget.windowHandle()));
        QCOMPARE(widget.m_touchEventCount, 0);
        QCOMPARE(widget.m_mouseEventCount, 0);

        QTest::touchEvent(&widget, m_touchScreen).press(0, QPoint(10, 10), &widget);
        QCOMPARE(widget.m_touchEventCount, 0);
        QCOMPARE(widget.m_mouseEventCount, 1);
        QCOMPARE(widget.m_lastMouseEventPos, QPointF(10, 10));
        QTest::touchEvent(&widget, m_touchScreen).move(0, QPoint(15, 15), &widget);
        QCOMPARE(widget.m_touchEventCount, 0);
        QCOMPARE(widget.m_mouseEventCount, 2);
        QCOMPARE(widget.m_lastMouseEventPos, QPointF(15, 15));
        QTest::touchEvent(&widget, m_touchScreen).release(0, QPoint(20, 20), &widget);
        QCOMPARE(widget.m_touchEventCount, 0);
        QCOMPARE(widget.m_mouseEventCount, 4); // we receive extra mouse move event
        QCOMPARE(widget.m_lastMouseEventPos, QPointF(20, 20));
    }

    {
        // We accept the touch events, no mouse event is generated
        TouchMouseWidget widget;
        widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        widget.setAcceptTouch(true);
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(widget.windowHandle()));
        QCOMPARE(widget.m_touchEventCount, 0);
        QCOMPARE(widget.m_mouseEventCount, 0);

        QTest::touchEvent(&widget, m_touchScreen).press(0, QPoint(10, 10), &widget);
        QCOMPARE(widget.m_touchEventCount, 1);
        QCOMPARE(widget.m_mouseEventCount, 0);
        QTest::touchEvent(&widget, m_touchScreen).move(0, QPoint(15, 15), &widget);
        QCOMPARE(widget.m_touchEventCount, 2);
        QCOMPARE(widget.m_mouseEventCount, 0);
        QTest::touchEvent(&widget, m_touchScreen).release(0, QPoint(20, 20), &widget);
        QCOMPARE(widget.m_touchEventCount, 3);
        QCOMPARE(widget.m_mouseEventCount, 0);
    }

    {
        // Parent accepts touch events, child ignore both mouse and touch
        // We should see propagation of the TouchBegin
        TouchMouseWidget parent;
        parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        parent.setAcceptTouch(true);
        TouchMouseWidget child(&parent);
        child.move(5, 5);
        child.setAcceptMouse(false);
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(parent.windowHandle()));
        QCOMPARE(parent.m_touchEventCount, 0);
        QCOMPARE(parent.m_mouseEventCount, 0);
        QCOMPARE(child.m_touchEventCount, 0);
        QCOMPARE(child.m_mouseEventCount, 0);

        QTest::touchEvent(parent.window(), m_touchScreen).press(0, QPoint(10, 10), &child);
        QCOMPARE(parent.m_touchEventCount, 1);
        QCOMPARE(parent.m_mouseEventCount, 0);
        QCOMPARE(child.m_touchEventCount, 0);
        QCOMPARE(child.m_mouseEventCount, 0);
    }

    {
        // Parent accepts mouse events, child ignore both mouse and touch
        // We should see propagation of the TouchBegin into a MouseButtonPress
        TouchMouseWidget parent;
        parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        TouchMouseWidget child(&parent);
        const QPoint childPos(5, 5);
        child.move(childPos);
        child.setAcceptMouse(false);
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(parent.windowHandle()));
        QCOMPARE(parent.m_touchEventCount, 0);
        QCOMPARE(parent.m_mouseEventCount, 0);
        QCOMPARE(child.m_touchEventCount, 0);
        QCOMPARE(child.m_mouseEventCount, 0);

        const QPoint touchPos(20, 20);
        QTest::touchEvent(parent.window(), m_touchScreen).press(0, touchPos, &child);
        QCOMPARE(parent.m_touchEventCount, 0);
        QCOMPARE(parent.m_mouseEventCount, 1);
        QCOMPARE(parent.m_lastMouseEventPos, childPos + touchPos);
        QCOMPARE(child.m_touchEventCount, 0);
        QCOMPARE(child.m_mouseEventCount, 1); // Attempt at mouse event before propagation
        QCOMPARE(child.m_lastMouseEventPos, touchPos);
    }
}

void tst_QWidget::touchUpdateOnNewTouch()
{
    TouchMouseWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.setAcceptTouch(true);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QWidget);
    widget.setLayout(layout);
    widget.show();

    QWindow* window = widget.windowHandle();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(widget.m_touchBeginCount, 0);
    QCOMPARE(widget.m_touchUpdateCount, 0);
    QCOMPARE(widget.m_touchEndCount, 0);
    QTest::touchEvent(window, m_touchScreen).press(0, QPoint(20, 20), window);
    QCOMPARE(widget.m_touchBeginCount, 1);
    QCOMPARE(widget.m_touchUpdateCount, 0);
    QCOMPARE(widget.m_touchEndCount, 0);
    QTest::touchEvent(window, m_touchScreen).move(0, QPoint(25, 25), window);
    QCOMPARE(widget.m_touchBeginCount, 1);
    QCOMPARE(widget.m_touchUpdateCount, 1);
    QCOMPARE(widget.m_touchEndCount, 0);
    QTest::touchEvent(window, m_touchScreen).stationary(0).press(1, QPoint(40, 40), window);
    QCOMPARE(widget.m_touchBeginCount, 1);
    QCOMPARE(widget.m_touchUpdateCount, 2);
    QCOMPARE(widget.m_touchEndCount, 0);
    QTest::touchEvent(window, m_touchScreen).stationary(1).release(0, QPoint(25, 25), window);
    QCOMPARE(widget.m_touchBeginCount, 1);
    QCOMPARE(widget.m_touchUpdateCount, 3);
    QCOMPARE(widget.m_touchEndCount, 0);
    QTest::touchEvent(window, m_touchScreen).release(1, QPoint(40, 40), window);
    QCOMPARE(widget.m_touchBeginCount, 1);
    QCOMPARE(widget.m_touchUpdateCount, 3);
    QCOMPARE(widget.m_touchEndCount, 1);
}

void tst_QWidget::touchEventsForGesturePendingWidgets()
{
    TouchMouseWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    TouchMouseWidget child(&parent);
    parent.grabGesture(Qt::TapAndHoldGesture);
    parent.show();

    QWindow* window = parent.windowHandle();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(500); // needed for QApplication::topLevelAt(), which is used by QGestureManager
    QCOMPARE(child.m_touchEventCount, 0);
    QCOMPARE(child.m_gestureEventCount, 0);
    QCOMPARE(parent.m_touchEventCount, 0);
    QCOMPARE(parent.m_gestureEventCount, 0);
    QTest::touchEvent(window, m_touchScreen).press(0, QPoint(20, 20), window);
    QCOMPARE(child.m_touchEventCount, 0);
    QCOMPARE(child.m_gestureEventCount, 0);
    QCOMPARE(parent.m_touchBeginCount, 1); // QTapAndHoldGestureRecognizer::create() sets Qt::WA_AcceptTouchEvents
    QCOMPARE(parent.m_touchUpdateCount, 0);
    QCOMPARE(parent.m_touchEndCount, 0);
    QCOMPARE(parent.m_gestureEventCount, 0);
    QTest::touchEvent(window, m_touchScreen).move(0, QPoint(25, 25), window);
    QCOMPARE(child.m_touchEventCount, 0);
    QCOMPARE(child.m_gestureEventCount, 0);
    QCOMPARE(parent.m_touchBeginCount, 1);
    QCOMPARE(parent.m_touchUpdateCount, 0);
    QCOMPARE(parent.m_touchEndCount, 0);
    QCOMPARE(parent.m_gestureEventCount, 0);
    QTest::qWait(1000);
    QTest::touchEvent(window, m_touchScreen).release(0, QPoint(25, 25), window);
    QCOMPARE(child.m_touchEventCount, 0);
    QCOMPARE(child.m_gestureEventCount, 0);
    QCOMPARE(parent.m_touchBeginCount, 1);
    QCOMPARE(parent.m_touchUpdateCount, 0);
    QCOMPARE(parent.m_touchEndCount, 0);
    QVERIFY(parent.m_gestureEventCount > 0);
}

void tst_QWidget::styleSheetPropagation()
{
    QTableView tw;
    tw.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    tw.setStyleSheet("background-color: red;");
    for (QObject *child : tw.children()) {
        if (QWidget *w = qobject_cast<QWidget *>(child))
            QCOMPARE(w->style(), tw.style());
    }
}

class DestroyTester : public QObject
{
    Q_OBJECT
public:
    explicit DestroyTester(QObject *parent = nullptr) : QObject(parent) { parentDestroyed = 0; }
    static int parentDestroyed;
public slots:
    void parentDestroyedSlot() {
        ++parentDestroyed;
    }
};

int DestroyTester::parentDestroyed = 0;

void tst_QWidget::destroyedSignal()
{
    {
        QWidget *w = new QWidget;
        DestroyTester *t = new DestroyTester(w);
        connect(w, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete w;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
    }

    {
        QWidget *w = new QWidget;
        DestroyTester *t = new DestroyTester(w);
        connect(w, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        w->blockSignals(true);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete w;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
    }

    {
        QObject *o = new QWidget;
        DestroyTester *t = new DestroyTester(o);
        connect(o, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete o;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
    }

    {
        QObject *o = new QWidget;
        auto t = new DestroyTester;
        connect(o, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        o->blockSignals(true);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete o;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
    }

    {
        QWidget *w = new QWidget;
        auto t = new DestroyTester;
        connect(w, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete w;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
        delete t;
    }

    {
        QWidget *w = new QWidget;
        auto t = new DestroyTester;
        connect(w, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        w->blockSignals(true);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete w;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
        delete t;
    }

    {
        QObject *o = new QWidget;
        auto t = new DestroyTester;
        connect(o, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete o;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
        delete t;
    }

    {
        QObject *o = new QWidget;
        auto t = new DestroyTester;
        connect(o, &QObject::destroyed, t, &DestroyTester::parentDestroyedSlot);
        o->blockSignals(true);
        QCOMPARE(DestroyTester::parentDestroyed, 0);
        delete o;
        QCOMPARE(DestroyTester::parentDestroyed, 1);
        delete t;
    }

}

#ifndef QT_NO_CURSOR
void tst_QWidget::underMouse()
{
    // Move the mouse cursor to a safe location
    QCursor::setPos(m_safeCursorPos);

    ColorWidget topLevelWidget(nullptr, Qt::FramelessWindowHint, Qt::blue);
    topLevelWidget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    ColorWidget childWidget1(&topLevelWidget, Qt::Widget, Qt::yellow);
    ColorWidget childWidget2(&topLevelWidget, Qt::Widget, Qt::black);
    ColorWidget popupWidget(nullptr, Qt::Popup, Qt::green);

    topLevelWidget.setObjectName("topLevelWidget");
    childWidget1.setObjectName("childWidget1");
    childWidget2.setObjectName("childWidget2");
    popupWidget.setObjectName("popupWidget");

    topLevelWidget.setGeometry(100, 100, 300, 300);
    childWidget1.setGeometry(20, 20, 100, 100);
    childWidget2.setGeometry(20, 120, 100, 100);
    popupWidget.setGeometry(50, 100, 50, 50);

    topLevelWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevelWidget));
    QWindow *window = topLevelWidget.windowHandle();

    QPoint outsideWindowPoint(30, -10);
    QPoint inWindowPoint(30, 10);
    QPoint child1Point(30, 50);
    QPoint child2PointA(30, 150);
    QPoint child2PointB(31, 151);

    // Outside window
    QTest::mouseMove(window, outsideWindowPoint);
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());

    // Enter window, outside children
    // Note: QTest::mouseMove will not generate enter events for windows, so send one explicitly
    QWindowSystemInterface::handleEnterEvent(window, inWindowPoint, window->mapToGlobal(inWindowPoint));
    QTest::mouseMove(window, inWindowPoint);
    QVERIFY(topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());

    // In childWidget1
    QTest::mouseMove(window, child1Point);
    QVERIFY(topLevelWidget.underMouse());
    QVERIFY(childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());

    // In childWidget2
    QTest::mouseMove(window, child2PointA);
    QVERIFY(topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(childWidget2.underMouse());

    topLevelWidget.resetCounts();
    childWidget1.resetCounts();
    childWidget2.resetCounts();
    popupWidget.resetCounts();

    // Throw up a popup window
    popupWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&popupWidget));
    QWindow *popupWindow = popupWidget.windowHandle();
    QVERIFY(popupWindow);
    QCOMPARE(QApplication::activePopupWidget(), &popupWidget);

    // Send an artificial leave event for window, as it won't get generated automatically
    // due to cursor not actually being over the window.
    QWindowSystemInterface::handleLeaveEvent(window);
    QApplication::processEvents();

    // If there is an active popup, undermouse should not be reported (QTBUG-27478),
    // but opening a popup causes leave for widgets under mouse.
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(!popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 0);
    QCOMPARE(popupWidget.leaves, 0);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 1);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 1);
    topLevelWidget.resetCounts();
    childWidget2.resetCounts();

    // Moving around while popup active should not change undermouse or cause
    // enter and leave events for widgets.
    QTest::mouseMove(popupWindow, popupWindow->mapFromGlobal(window->mapToGlobal(child2PointB)));
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(!popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 0);
    QCOMPARE(popupWidget.leaves, 0);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 0);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 0);

    QTest::mouseMove(popupWindow, popupWindow->mapFromGlobal(window->mapToGlobal(inWindowPoint)));
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(!popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 0);
    QCOMPARE(popupWidget.leaves, 0);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 0);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 0);

    QTest::mouseMove(popupWindow, popupWindow->mapFromGlobal(window->mapToGlobal(child1Point)));
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(!popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 0);
    QCOMPARE(popupWidget.leaves, 0);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 0);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 0);

    // Note: Mouse moving off-application while there is an active popup cannot be simulated
    // without actually moving the cursor so it is not tested.

    // Mouse enters popup, should cause enter to popup.
    // Once again, need to create artificial enter event.
    const QPoint popupCenter = popupWindow->geometry().center();
    QWindowSystemInterface::handleEnterEvent(popupWindow, popupWindow->mapFromGlobal(popupCenter), popupCenter);
    QTest::mouseMove(popupWindow, popupCenter);
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 1);
    QCOMPARE(popupWidget.leaves, 0);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 0);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 0);
    popupWidget.resetCounts();

    // Mouse moves around inside popup, no changes
    QTest::mouseMove(popupWindow, QPoint(5, 5));
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 0);
    QCOMPARE(popupWidget.leaves, 0);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 0);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 0);

    // Mouse leaves popup and enters topLevelWidget, should cause leave for popup
    // but no enter to topLevelWidget.
#ifdef Q_OS_DARWIN
    // Artificial leave event needed for Cocoa.
    QWindowSystemInterface::handleLeaveEvent(popupWindow);
#endif
    QTest::mouseMove(popupWindow, popupWindow->mapFromGlobal(window->mapToGlobal(inWindowPoint)));
    QApplication::processEvents();
    QVERIFY(!topLevelWidget.underMouse());
    QVERIFY(!childWidget1.underMouse());
    QVERIFY(!childWidget2.underMouse());
    QVERIFY(!popupWidget.underMouse());
    QCOMPARE(popupWidget.enters, 0);
    QCOMPARE(popupWidget.leaves, 1);
    QCOMPARE(topLevelWidget.enters, 0);
    QCOMPARE(topLevelWidget.leaves, 0);
    QCOMPARE(childWidget1.enters, 0);
    QCOMPARE(childWidget1.leaves, 0);
    QCOMPARE(childWidget2.enters, 0);
    QCOMPARE(childWidget2.leaves, 0);
}

class EnterTestModalDialog : public QDialog
{
    Q_OBJECT
public:
    EnterTestModalDialog()
    {
        setGeometry(100, 300, 150, 100);
        button = new QPushButton(this);
        button->setGeometry(10, 10, 50, 30);
    }

    QPushButton *button;
};

class EnterTestMainDialog : public QDialog
{
    Q_OBJECT
public:

public slots:
    void buttonPressed()
    {
        qApp->installEventFilter(this);
        modal = new EnterTestModalDialog();
        QTimer::singleShot(2000, modal, SLOT(close())); // Failsafe
        QTimer::singleShot(100, this, SLOT(doMouseMoves()));
        modal->exec();
        delete modal;
        modal = nullptr;
    }

    void doMouseMoves()
    {
        QPoint point1(15, 15);
        QPoint point2(15, 20);
        QPoint point3(20, 20);
        QWindow *window = modal->windowHandle();
        const QPoint nativePoint1 = QHighDpi::toNativePixels(point1, window->screen());
        QWindowSystemInterface::handleEnterEvent(window, nativePoint1);
        QTest::mouseMove(window, point1);
        QTest::mouseMove(window, point2);
        QTest::mouseMove(window, point3);
        modal->close();
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Enter:
            if (modal && modal->button && o == modal->button)
                enters++;
            break;
        default:
            break;
        }
        return QDialog::eventFilter(o, e);
    }

public:
    EnterTestModalDialog *modal = nullptr;
    int enters = 0;
};

// A modal dialog launched by clicking a button should not trigger excess enter events
// when mousing over it.
void tst_QWidget::taskQTBUG_27643_enterEvents()
{
#ifdef Q_OS_MACOS
    QSKIP("QTBUG-52974: this test can crash!");
#endif
    // Move the mouse cursor to a safe location so it won't interfere
    QCursor::setPos(m_safeCursorPos);

    EnterTestMainDialog dialog;
    dialog.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QPushButton button(&dialog);

    connect(&button, &QAbstractButton::clicked, &dialog, &EnterTestMainDialog::buttonPressed);

    dialog.setGeometry(100, 100, 150, 100);
    button.setGeometry(10, 10, 100, 50);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    QWindow *window = dialog.windowHandle();
    QPoint overButton(25, 25);

    QWindowSystemInterface::handleEnterEvent(window, overButton, window->mapToGlobal(overButton));
    QTest::mouseMove(window, overButton);
    QTest::mouseClick(window, Qt::LeftButton, Qt::KeyboardModifiers(), overButton, 0);

    // Modal dialog opened in EnterTestMainDialog::buttonPressed()...

    // Must only register only single enter on modal dialog's button after all said and done
    QCOMPARE(dialog.enters, 1);
}
#endif // QT_NO_CURSOR

class KeyboardWidget : public QWidget
{
public:
    using QWidget::QWidget;
    void mousePressEvent(QMouseEvent* ev) override
    {
        m_modifiers = ev->modifiers();
        m_appModifiers = QApplication::keyboardModifiers();
        ++m_eventCounter;
    }
    Qt::KeyboardModifiers m_modifiers;
    Qt::KeyboardModifiers m_appModifiers;
    int m_eventCounter = 0;
};

void tst_QWidget::keyboardModifiers()
{
    KeyboardWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.resize(300, 300);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTest::mouseClick(&w, Qt::LeftButton, Qt::ControlModifier);
    QCOMPARE(w.m_eventCounter, 1);
    QCOMPARE(int(w.m_modifiers), int(Qt::ControlModifier));
    QCOMPARE(int(w.m_appModifiers), int(Qt::ControlModifier));
}

class DClickWidget : public QWidget
{
public:
    void mouseDoubleClickEvent(QMouseEvent *) override
    {
        triggered = true;
    }
    bool triggered = false;
};

void tst_QWidget::mouseDoubleClickBubbling_QTBUG29680()
{
    DClickWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QWidget child(&parent);
    parent.resize(200, 200);
    child.resize(200, 200);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    QTest::mouseDClick(&child, Qt::LeftButton);

    QTRY_VERIFY(parent.triggered);
}

void tst_QWidget::largerThanScreen_QTBUG30142()
{
    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.resize(200, 4000);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY2(widget.frameGeometry().y() >= 0,
             msgComparisonFailed(widget.frameGeometry().y(), " >=", 0));

    QWidget widget2;
    widget2.resize(10000, 400);
    widget2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget2));
    QVERIFY2(widget2.frameGeometry().x() >= 0,
             msgComparisonFailed(widget.frameGeometry().x(), " >=", 0));
}

void tst_QWidget::resizeStaticContentsChildWidget_QTBUG35282()
{
    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    widget.resize(200,200);

    UpdateWidget childWidget(&widget);
    childWidget.setAttribute(Qt::WA_StaticContents);
    childWidget.setAttribute(Qt::WA_OpaquePaintEvent);
    childWidget.setGeometry(250, 250, 500, 500);

    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    if (m_platform == QStringLiteral("winrt"))
        QEXPECT_FAIL("", "WinRT: This fails. QTBUG-68297.", Abort);
    QCOMPARE(childWidget.numPaintEvents, 0);
    childWidget.reset();

    widget.resize(1000,1000);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QGuiApplication::sync();
    QVERIFY2(childWidget.numPaintEvents >= 1,
             msgComparisonFailed(childWidget.numPaintEvents, ">=", 1));
}

void tst_QWidget::qmlSetParentHelper()
{
#ifdef QT_BUILD_INTERNAL
    QWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QWidget child;
    QVERIFY(QAbstractDeclarativeData::setWidgetParent);
    QAbstractDeclarativeData::setWidgetParent(&child, &parent);
    QCOMPARE(child.parentWidget(), &parent);
    QAbstractDeclarativeData::setWidgetParent(&child, nullptr);
    QVERIFY(!child.parentWidget());
#else
    QSKIP("Needs QT_BUILD_INTERNAL");
#endif
}

void tst_QWidget::testForOutsideWSRangeFlag()
{
    QSKIP("Test assumes QWindows can have 0x0 size, see QTBUG-61953");

    // QTBUG-49445
    {
        QWidget widget;
        widget.resize(0, 0);
        widget.show();
        QTest::qWait(100); // Wait for a while...
        QVERIFY(!widget.windowHandle()->isExposed()); // The window should not be visible
        QVERIFY(widget.isVisible()); // The widget should be in visible state
    }
    {
        QWidget widget;

        QWidget native(&widget);
        native.setAttribute(Qt::WA_NativeWindow);
        native.resize(0, 0);

        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QVERIFY(!native.windowHandle()->isExposed());
    }
    {
        QWidget widget;
        QWidget native(&widget);

        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QVERIFY(native.isVisible());

        native.resize(0, 0);
        native.setAttribute(Qt::WA_NativeWindow);
        QTest::qWait(100); // Wait for a while...
        QVERIFY(!native.windowHandle()->isExposed());
    }

    // QTBUG-48321
    {
        QWidget widget;

        QWidget native(&widget);
        native.setAttribute(Qt::WA_NativeWindow);

        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QVERIFY(native.windowHandle()->isExposed());

        native.resize(0, 0);
        QTest::qWait(100); // Wait for a while...
        QVERIFY(!native.windowHandle()->isExposed());
    }

    // QTBUG-51788
    {
        QWidget widget;
        widget.setLayout(new QGridLayout);
        widget.layout()->addWidget(new QLineEdit);
        widget.resize(0, 0);
        widget.show();
        // The layout should change the size, so the widget must be visible!
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
    }
}

class TabletWidget : public QWidget
{
public:
    TabletWidget(QWidget *parent) : QWidget(parent) { }

    int tabletEventCount = 0;
    int pressEventCount = 0;
    int moveEventCount = 0;
    int releaseEventCount = 0;
    int trackingChangeEventCount = 0;
    qint64 uid = -1;

protected:
    void tabletEvent(QTabletEvent *event) override {
        ++tabletEventCount;
        uid = event->uniqueId();
        switch (event->type()) {
        case QEvent::TabletMove:
            ++moveEventCount;
            break;
        case QEvent::TabletPress:
            ++pressEventCount;
            break;
        case QEvent::TabletRelease:
            ++releaseEventCount;
            break;
        default:
            break;
        }
    }

    bool event(QEvent *ev) override {
        if (ev->type() == QEvent::TabletTrackingChange)
            ++trackingChangeEventCount;
        return QWidget::event(ev);
    }
};

void tst_QWidget::tabletTracking()
{
    QWidget parent;
    parent.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    parent.resize(200,200);
    // QWidgetWindow::handleTabletEvent doesn't deliver tablet events to the window's widget, only to a child.
    // So it doesn't do any good to show a TabletWidget directly: it needs a parent.
    TabletWidget widget(&parent);
    widget.resize(200,200);
    parent.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    widget.setAttribute(Qt::WA_TabletTracking);
    QTRY_COMPARE(widget.trackingChangeEventCount, 1);
    QVERIFY(widget.hasTabletTracking());

    QWindow *window = parent.windowHandle();
    QPointF local(10, 10);
    QPointF global = window->mapToGlobal(local.toPoint());
    QPointF deviceLocal = QHighDpi::toNativeLocalPosition(local, window);
    QPointF deviceGlobal = QHighDpi::toNativePixels(global, window->screen());
    qint64 uid = 1234UL;

    QWindowSystemInterface::handleTabletEvent(window, ulong(QDateTime::currentMSecsSinceEpoch()), deviceLocal, deviceGlobal,
        QTabletEvent::Stylus, QTabletEvent::Pen, Qt::NoButton, 0, 0, 0, 0, 0, 0, uid, Qt::NoModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.moveEventCount, 1);
    QCOMPARE(widget.uid, uid);

    local += QPoint(10, 10);
    deviceLocal += QPoint(10, 10);
    deviceGlobal += QPoint(10, 10);
    QWindowSystemInterface::handleTabletEvent(window, ulong(QDateTime::currentMSecsSinceEpoch()), deviceLocal, deviceGlobal,
        QTabletEvent::Stylus, QTabletEvent::Pen, Qt::NoButton, 0, 0, 0, 0, 0, 0, uid, Qt::NoModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.moveEventCount, 2);

    widget.setTabletTracking(false);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.trackingChangeEventCount, 2);

    QWindowSystemInterface::handleTabletEvent(window, ulong(QDateTime::currentMSecsSinceEpoch()), deviceLocal, deviceGlobal,
        QTabletEvent::Stylus, QTabletEvent::Pen, Qt::LeftButton, 0, 0, 0, 0, 0, 0, uid, Qt::NoModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.pressEventCount, 1);

    local += QPoint(10, 10);
    deviceLocal += QPoint(10, 10);
    deviceGlobal += QPoint(10, 10);
    QWindowSystemInterface::handleTabletEvent(window, ulong(QDateTime::currentMSecsSinceEpoch()), deviceLocal, deviceGlobal,
        QTabletEvent::Stylus, QTabletEvent::Pen, Qt::LeftButton, 0, 0, 0, 0, 0, 0, uid, Qt::NoModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.moveEventCount, 3);

    QWindowSystemInterface::handleTabletEvent(window, ulong(QDateTime::currentMSecsSinceEpoch()), deviceLocal, deviceGlobal,
        QTabletEvent::Stylus, QTabletEvent::Pen, Qt::NoButton, 0, 0, 0, 0, 0, 0, uid, Qt::NoModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.releaseEventCount, 1);

    local += QPoint(10, 10);
    deviceLocal += QPoint(10, 10);
    deviceGlobal += QPoint(10, 10);
    QWindowSystemInterface::handleTabletEvent(window, ulong(QDateTime::currentMSecsSinceEpoch()), deviceLocal, deviceGlobal,
                                              QTabletEvent::Stylus, QTabletEvent::Pen, Qt::NoButton, 0, 0, 0, 0, 0, 0, uid, Qt::NoModifier);
    QCoreApplication::processEvents();
    QTRY_COMPARE(widget.moveEventCount, 3);
}

class CloseCountingWidget : public QWidget
{
public:
    int closeCount = 0;
    void closeEvent(QCloseEvent *ev) override;
};

void CloseCountingWidget::closeEvent(QCloseEvent *ev)
{
    ++closeCount;
    ev->accept();
}

void tst_QWidget::closeEvent()
{
    CloseCountingWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    // Yes we call the close() function twice. This mimics the behavior of QTBUG-43344 where
    // QApplication first closes all windows and then QCocoaApplication flushes window system
    // events, triggering more close events.
    widget.windowHandle()->close();
    widget.windowHandle()->close();
    QCOMPARE(widget.closeCount, 1);
}

void tst_QWidget::closeWithChildWindow()
{
    QWidget widget;
    auto childWidget = new QWidget(&widget);
    childWidget->setAttribute(Qt::WA_NativeWindow);
    childWidget->windowHandle()->create();
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.windowHandle()->close();
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    // Check that the child window inside the window is now visible
    QVERIFY(childWidget->isVisible());

    // Now explicitly hide the childWidget
    childWidget->hide();
    widget.windowHandle()->close();
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY(!childWidget->isVisible());
}

class WinIdChangeSpy : public QObject
{
    Q_OBJECT
public:
    QWidget *widget = nullptr;
    WId winId = 0;
    explicit WinIdChangeSpy(QWidget *w, QObject *parent = nullptr)
        : QObject(parent)
        , widget(w)
        , winId(widget->winId())
    {
    }

public slots:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == widget) {
            if (event->type() == QEvent::WinIdChange) {
                winId = widget->winId();
                return true;
            }
        }
        return false;
    }
};

void tst_QWidget::winIdAfterClose()
{
    auto widget = new QWidget;
    auto notifier = new QObject(widget);
    auto deleteWidget = new QWidget(new QWidget(widget));
    auto spy = new WinIdChangeSpy(deleteWidget);
    deleteWidget->installEventFilter(spy);
    connect(notifier, &QObject::destroyed, [&] { delete deleteWidget; });

    widget->setAttribute(Qt::WA_NativeWindow);
    widget->windowHandle()->create();
    widget->show();

    QVERIFY(QTest::qWaitForWindowExposed(widget));
    QVERIFY(spy->winId);

    widget->windowHandle()->close();
    delete widget;

    QCOMPARE(spy->winId, WId(0));
    delete spy;
}

class LanguageChangeEventWidget : public QWidget
{
public:
    LanguageChangeEventWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    int languageChangeCount = 0;
protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::LanguageChange)
            languageChangeCount++;
        return QWidget::event(e);
    }
};

class LanguageChangeEventWindow : public QWindow
{
public:
    LanguageChangeEventWindow(QWindow *parent = nullptr) : QWindow(parent) {}
    int languageChangeCount = 0;
protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::LanguageChange)
            languageChangeCount++;
        return QWindow::event(e);
    }
};

void tst_QWidget::receivesLanguageChangeEvent()
{
    // Confirm that any QWindow or QWidget only gets a single
    // LanguageChange event when a translator is installed
    LanguageChangeEventWidget topLevel;
    auto childWidget = new LanguageChangeEventWidget(&topLevel);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    LanguageChangeEventWindow ww;
    ww.show();
    QVERIFY(QTest::qWaitForWindowExposed(&ww));
    LanguageChangeEventWidget topLevelNotShown;
    QTranslator t;
    QVERIFY(t.load("hellotr_la.qm", ":/"));
    QVERIFY(qApp->installTranslator(&t));
    QCoreApplication::sendPostedEvents(0, QEvent::LanguageChange);
    QCOMPARE(topLevel.languageChangeCount, 1);
    QCOMPARE(topLevelNotShown.languageChangeCount, 1);
    QCOMPARE(childWidget->languageChangeCount, 1);
    QCOMPARE(ww.languageChangeCount, 1);
}

class DeleteOnCloseEventWidget : public QWidget
{
protected:
    virtual void closeEvent(QCloseEvent *e) override
    {
        e->accept();
        delete this;
    }
};

void tst_QWidget::deleteWindowInCloseEvent()
{
    // Just checking if closing this widget causes a crash
    auto widget = new DeleteOnCloseEventWidget;
    widget->close();
    QVERIFY(true);
}

QTEST_MAIN(tst_QWidget)
#include "tst_qwidget.moc"
