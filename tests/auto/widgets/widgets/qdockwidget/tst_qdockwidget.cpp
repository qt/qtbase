// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <qaction.h>
#include <qdockwidget.h>
#include <qmainwindow.h>
#include "private/qdockwidget_p.h"
#include "private/qmainwindowlayout_p.h"
#include <QAbstractButton>
#include <qlineedit.h>
#include <QtGui/qpa/qplatformwindow.h>
#include <qtabbar.h>
#include <QScreen>
#include <QTimer>
#include <QtGui/QPainter>
#include <QLabel>

Q_LOGGING_CATEGORY(lcTestDockWidget, "qt.widgets.tests.qdockwidget")

#include <QtWidgets/private/qapplication_p.h>

bool hasFeature(QDockWidget *dockwidget, QDockWidget::DockWidgetFeature feature)
{ return (dockwidget->features() & feature) == feature; }

void setFeature(QDockWidget *dockwidget, QDockWidget::DockWidgetFeature feature, bool on = true)
{
    const QDockWidget::DockWidgetFeatures features = dockwidget->features();
    dockwidget->setFeatures(on ? features | feature : features & ~feature);
}

class tst_QDockWidget : public QObject
{
     Q_OBJECT

public:
    tst_QDockWidget();

private slots:
    void getSetCheck();
    void widget();
    void features();
    void setFloating();
    void setFloatingReparenting();
    void allowedAreas();
    void toggleViewAction();
    void visibilityChanged();
    void visibilityChangedOnDestruction_data();
    void visibilityChangedOnDestruction();
    void updateTabBarOnVisibilityChanged();
    void dockLocationChanged();
    void setTitleBarWidget();
    void titleBarDoubleClick();
    void restoreStateOfFloating();
    void restoreDockWidget();
    void restoreStateWhileStillFloating();
    void setWindowTitle();
    void windowIcon();

    // task specific tests:
    void task165177_deleteFocusWidget();
    void task169808_setFloating();
    void task237438_setFloatingCrash();
    void task248604_infiniteResize();
    void task258459_visibilityChanged();
    void taskQTBUG_1665_closableChanged();
    void taskQTBUG_9758_undockedGeometry();

    // Dock area permissions for DockWidgets and DockWidgetGroupWindows
    void dockPermissions();

    // test floating tabs, item_tree and window title consistency
    void floatingTabs();
    void hoverWithoutDrop();

    // floating tab gets removed, when last child goes away
    void deleteFloatingTabWithSingleDockWidget_data();
    void deleteFloatingTabWithSingleDockWidget();

    // test hide & show
    void hideAndShow();

    // test closing and deleting consistency
    void closeAndDelete();
    void closeUnclosable();

    // test save and restore consistency
    void saveAndRestore();

private:
    // helpers and consts for dockPermissions, hideAndShow, closeAndDelete
#ifdef QT_BUILD_INTERNAL
    void createTestWidgets(QMainWindow* &MainWindow, QPointer<QWidget> &cent,
                           QPointer<QDockWidget> &d1, QPointer<QDockWidget> &d2) const;

    void unplugAndResize(QMainWindow* MainWindow, QDockWidget* dw, QPoint home, QSize size) const;

    void createFloatingTabs(QMainWindow* &MainWindow, QPointer<QWidget> &cent,
                            QPointer<QDockWidget> &d1, QPointer<QDockWidget> &d2,
                            QList<int> &path1, QList<int> &path2) const;

#if defined(Q_OS_DARWIN) || defined(Q_OS_ANDROID) || defined(Q_OS_QNX)
#define qCreateFloatingTabs(mainWindow, centralWidget, d1, d2, path1, path2)\
    mainWindow = nullptr;\
    Q_UNUSED(path1);\
    Q_UNUSED(path2);\
    QSKIP("Platform not supported");
#else
#define qCreateFloatingTabs(mainWindow, centralWidget, d1, d2, path1, path2)\
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))\
        QSKIP("Test skipped on Wayland.");\
    createFloatingTabs(mainWindow, centralWidget, d1, d2, path1, path2);\
    std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);\
    if (!platformSupportingRaise)\
        QSKIP("Platform not supporting raise(). Floating tab based tests will fail.")
#endif

    static inline QPoint dragPoint(QDockWidget* dockWidget);
    static inline QPoint home1(QMainWindow* MainWindow)
    { return MainWindow->mapToGlobal(MainWindow->rect().topLeft() + QPoint(0.1 * MainWindow->width(), 0.1 * MainWindow->height())); }

    static inline QPoint home2(QMainWindow* MainWindow)
    { return MainWindow->mapToGlobal(MainWindow->rect().topLeft() + QPoint(0.6 * MainWindow->width(), 0.15 * MainWindow->height())); }

    static inline QSize size1(QMainWindow* MainWindow)
    { return QSize (0.2 * MainWindow->width(), 0.25 * MainWindow->height()); }

    static inline QSize size2(QMainWindow* MainWindow)
    { return QSize (0.1 * MainWindow->width(), 0.15 * MainWindow->height()); }

    static inline QPoint dockPoint(QMainWindow* mw, Qt::DockWidgetArea area)
    { return mw->mapToGlobal(qobject_cast<QMainWindowLayout*>(mw->layout())->dockWidgetAreaRect(area, QMainWindowLayout::Maximum).center()); }

    bool checkFloatingTabs(QMainWindow* MainWindow, QPointer<QDockWidgetGroupWindow> &ftabs, const QList<QDockWidget*> &dwList = {}) const;

    // move a dock widget
    enum class MoveDockWidgetRule {
        Drop,
        Abort
    };

    void moveDockWidget(QDockWidget* dw, QPoint to, QPoint from, MoveDockWidgetRule rule) const;

#ifdef QT_BUILD_INTERNAL
    // Message handling for xcb error QTBUG 82059
    static void xcbMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    enum class ChildRemovalReason {
        Destroyed,
        Closed,
        Reparented
    };

public:
    bool xcbError = false;
    bool platformSupportingRaise = true;
#endif
private:

#ifdef QT_DEBUG
    // Grace time between mouse events. Set to 400 for debugging.
    const int waitingTime = 400;

    // Waiting time before closing widgets successful test. Set to 20000 for debugging.
    const int waitBeforeClose = 0;

    // Enable logging
    const bool dockWidgetLog = true;
#else
    const int waitingTime = 15;
    const int waitBeforeClose = 0;
    const bool dockWidgetLog = false;
#endif // QT_DEBUG
#endif // QT_BUILD_INTERNAL

};

// Testing get/set functions
void tst_QDockWidget::getSetCheck()
{
    QDockWidget obj1;
    // QWidget * QDockWidget::widget()
    // void QDockWidget::setWidget(QWidget *)
    QWidget *var1 = new QWidget();
    obj1.setWidget(var1);
    QCOMPARE(var1, obj1.widget());
    obj1.setWidget((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1.widget());
    delete var1;

    // DockWidgetFeatures QDockWidget::features()
    // void QDockWidget::setFeatures(DockWidgetFeatures)
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetClosable));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetClosable), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetMovable));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetMovable), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetFloatable));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetFloatable), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::NoDockWidgetFeatures));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::NoDockWidgetFeatures), obj1.features());
}

tst_QDockWidget::tst_QDockWidget()
{
    qRegisterMetaType<QDockWidget::DockWidgetFeatures>("QDockWidget::DockWidgetFeatures");
    qRegisterMetaType<Qt::DockWidgetAreas>("Qt::DockWidgetAreas");
}

void tst_QDockWidget::widget()
{
    {
        QDockWidget dw;
        QVERIFY(!dw.widget());
    }

    {
        QDockWidget dw;
        QWidget *w1 = new QWidget;
        QWidget *w2 = new QWidget;

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        dw.setWidget(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        dw.setWidget(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        dw.setWidget(0);
        QVERIFY(!dw.widget());
    }

    {
        QDockWidget dw;
        QWidget *w1 = new QWidget;
        QWidget *w2 = new QWidget;

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        w1->setParent(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        w2->setParent(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        w1->setParent(0);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        w2->setParent(0);
        QVERIFY(!dw.widget());
        delete w1;
        delete w2;
    }
}

void tst_QDockWidget::features()
{
    QDockWidget dw;

    QSignalSpy spy(&dw, SIGNAL(featuresChanged(QDockWidget::DockWidgetFeatures)));

    // default features for dock widgets
    const auto allDockWidgetFeatures = QDockWidget::DockWidgetClosable |
                                       QDockWidget::DockWidgetMovable  |
                                       QDockWidget::DockWidgetFloatable;

    // defaults
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));

    // test individual setting
    setFeature(&dw, QDockWidget::DockWidgetClosable, false);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*(static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData())),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetClosable);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetMovable, false);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetMovable);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetFloatable, false);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetFloatable);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    // set all at once
    dw.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    dw.setFeatures(QDockWidget::DockWidgetClosable);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    dw.setFeatures(allDockWidgetFeatures);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.size(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.size(), 0);
    spy.clear();
}

void tst_QDockWidget::setFloating()
{
    const QRect deskRect = QGuiApplication::primaryScreen()->availableGeometry();
    QMainWindow mw;
    mw.move(deskRect.left() + deskRect.width() * 2 / 3, deskRect.top() + deskRect.height() / 3);
    QDockWidget dw;
    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);

    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    QVERIFY(!dw.isFloating());
    const QPoint dockedPosition = dw.mapToGlobal(dw.pos());

    QSignalSpy spy(&dw, SIGNAL(topLevelChanged(bool)));

    dw.setFloating(true);
    const QPoint floatingPosition = dw.pos();

    // QTBUG-31044, show approximately at old position, give or take window frame.
    QVERIFY((dockedPosition - floatingPosition).manhattanLength() < 50);

    QVERIFY(dw.isFloating());
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).value(0).toBool(), dw.isFloating());
    spy.clear();
    dw.setFloating(dw.isFloating());
    QCOMPARE(spy.size(), 0);
    spy.clear();

    dw.setFloating(false);
    QVERIFY(!dw.isFloating());
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).value(0).toBool(), dw.isFloating());
    spy.clear();
    dw.setFloating(dw.isFloating());
    QCOMPARE(spy.size(), 0);
    spy.clear();
}

void tst_QDockWidget::setFloatingReparenting()
{
#ifdef Q_OS_WIN
    QSKIP("Test skipped on Windows platforms");
#endif // Q_OS_WIN
#ifdef QT_BUILD_INTERNAL
    // Check that setFloating() reparents the dock widget to the main window,
    // in case it has a QDockWidgetGroupWindow parent
    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    QList<int> path1;
    QList<int> path2;
    qCreateFloatingTabs(mainWindow, cent, d1, d2, path1, path2);
    QVERIFY(qobject_cast<QDockWidgetGroupWindow *>(d1->parentWidget()));
    QVERIFY(qobject_cast<QDockWidgetGroupWindow *>(d2->parentWidget()));
    d1->setFloating(true);
    QTRY_COMPARE(mainWindow, d1->parentWidget());
    QTRY_COMPARE(mainWindow, d2->parentWidget());
#else
    QSKIP("test requires -developer-build option");
#endif // defined(QT_BUILD_INTERNAL)
}

void tst_QDockWidget::allowedAreas()
{
    QDockWidget dw;

    QSignalSpy spy(&dw, SIGNAL(allowedAreasChanged(Qt::DockWidgetAreas)));

    // default
    QCOMPARE(dw.allowedAreas(), Qt::AllDockWidgetAreas);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));

    // a single dock window area
    dw.setAllowedAreas(Qt::LeftDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::LeftDockWidgetArea);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    dw.setAllowedAreas(Qt::RightDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::RightDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    dw.setAllowedAreas(Qt::TopDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::TopDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    dw.setAllowedAreas(Qt::BottomDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::BottomDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    // multiple dock window areas
    dw.setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    //QVERIFY(!dw.isAreaAllowed(Qt::FloatingDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    dw.setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    //QVERIFY(!dw.isAreaAllowed(Qt::FloatingDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    dw.setAllowedAreas(Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    //QVERIFY(!dw.isAreaAllowed(Qt::FloatingDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    //dw.setAllowedAreas(Qt::BottomDockWidgetArea | Qt::FloatingDockWidgetArea);
    dw.setAllowedAreas(Qt::BottomDockWidgetArea);
    //QCOMPARE(dw.allowedAreas(), Qt::BottomDockWidgetArea | Qt::FloatingDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    //QVERIFY(dw.isAreaAllowed(Qt::FloatingDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);

    dw.setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    //QVERIFY(!dw.isAreaAllowed(Qt::FloatingDockWidgetArea));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.size(), 0);
}

void tst_QDockWidget::toggleViewAction()
{
    QMainWindow mw;
    QDockWidget dw(&mw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    mw.show();
    QAction *toggleViewAction = dw.toggleViewAction();
    QVERIFY(!dw.isHidden());
    toggleViewAction->trigger();
    QVERIFY(dw.isHidden());
    toggleViewAction->trigger();
    QVERIFY(!dw.isHidden());
    toggleViewAction->trigger();
    QVERIFY(dw.isHidden());
}

void tst_QDockWidget::visibilityChanged()
{
    QMainWindow mw;
    QDockWidget dw;
    QSignalSpy spy(&dw, SIGNAL(visibilityChanged(bool)));

    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    mw.show();

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw.hide();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw.hide();
    QCOMPARE(spy.size(), 0);

    dw.show();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw.show();
    QCOMPARE(spy.size(), 0);

    QDockWidget dw2;
    mw.tabifyDockWidget(&dw, &dw2);
    dw2.show();
    dw2.raise();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw2.hide();
    qApp->processEvents();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw2.show();
    dw2.raise();
    qApp->processEvents();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw.raise();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw.raise();
    QCOMPARE(spy.size(), 0);

    dw2.raise();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw2.raise();
    QCOMPARE(spy.size(), 0);

    mw.addDockWidget(Qt::RightDockWidgetArea, &dw2);
    QTRY_COMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
}

// QTBUG-136485 - QDockWidget didn't emit visibilityChanged when getting
// destroyed until 6.9.0; it did in 6.9.0, causing regressions in applications.
// So make sure we don't emit that signal when a QDockWidget gets destroyed.
// When implicitly destroyed as a child of a QMainWindow, it gets hidden first,
// so it emits the signal.
void tst_QDockWidget::visibilityChangedOnDestruction_data()
{
    QTest::addColumn<bool>("explicitDestroy");
    QTest::addColumn<bool>("floating");
    QTest::addColumn<int>("signalCount");

    QTest::addRow("Explicit, docked") << true << false << 0;
    QTest::addRow("Explicit, floating") << true << true << 0;
    QTest::addRow("Implicit, docked") << false << false << 1;
    QTest::addRow("Implicit, floating") << false << true << 0;
}

void tst_QDockWidget::visibilityChangedOnDestruction()
{
    QFETCH(const bool, explicitDestroy);
    QFETCH(const bool, floating);
    QFETCH(const int, signalCount);

    std::unique_ptr<QMainWindow> mw(new QMainWindow);
    QDockWidget *dw = new QDockWidget;
    mw->addDockWidget(Qt::LeftDockWidgetArea, dw);
    if (floating)
        dw->setFloating(true);
    mw->show();
    QVERIFY(QTest::qWaitForWindowExposed(mw.get()));

    QSignalSpy spy(dw, &QDockWidget::visibilityChanged);
    if (explicitDestroy)
        delete dw;
    else
        mw.reset();
    QCOMPARE(spy.count(), signalCount);
}

void tst_QDockWidget::updateTabBarOnVisibilityChanged()
{
    // QTBUG49045: Populate tabified dock area with 4 widgets, set the tab
    // index to 2 (dw2), hide dw0, dw1 and check that the tab index is 0 (dw3).
    QMainWindow mw;
    mw.setMinimumSize(400, 400);
    mw.setWindowTitle(QTest::currentTestFunction());
    QDockWidget *dw0 = new QDockWidget("d1", &mw);
    dw0->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw0);
    QDockWidget *dw1 = new QDockWidget("d2", &mw);
    dw1->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw1);
    QDockWidget *dw2 = new QDockWidget("d3", &mw);
    dw2->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw2);
    QDockWidget *dw3 = new QDockWidget("d4", &mw);
    dw3->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw3);
    mw.tabifyDockWidget(dw0, dw1);
    mw.tabifyDockWidget(dw1, dw2);
    mw.tabifyDockWidget(dw2, dw3);

    const auto list1 = QList<QDockWidget *>{dw1, dw2, dw3};
    QCOMPARE(mw.tabifiedDockWidgets(dw0), list1);

    QTabBar *tabBar = mw.findChild<QTabBar *>();
    QVERIFY(tabBar);
    tabBar->setCurrentIndex(2);

    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    QCOMPARE(tabBar->currentIndex(), 2);

    dw0->hide();
    dw1->hide();
    QTRY_COMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->currentIndex(), 0);

    QCOMPARE(mw.tabifiedDockWidgets(dw2), {dw3});

    mw.removeDockWidget(dw3);
    QCOMPARE(mw.tabifiedDockWidgets(dw2).count(), 0);
}

Q_DECLARE_METATYPE(Qt::DockWidgetArea)

Qt::DockWidgetArea dockLocation(const QSignalSpy *spy)
{
    Q_ASSERT(spy);
    return qvariant_cast<Qt::DockWidgetArea>(spy->at(0).at(0));
}

void tst_QDockWidget::dockLocationChanged()
{
    qRegisterMetaType<Qt::DockWidgetArea>("Qt::DockWidgetArea");

    QMainWindow mw;
    QDockWidget dw;
    dw.setObjectName("dock1");
    QSignalSpy spy(&dw, &QDockWidget::dockLocationChanged);

    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(dockLocation(&spy), Qt::LeftDockWidgetArea);

    constexpr std::array<Qt::DockWidgetArea, 5> areas{Qt::LeftDockWidgetArea,
                                                      Qt::TopDockWidgetArea,
                                                      Qt::RightDockWidgetArea,
                                                      Qt::BottomDockWidgetArea,
                                                      Qt::NoDockWidgetArea};

    for (const auto area : areas) {
        spy.clear();
        const int expectedCount = dw.dockLocation() == area ? 0 : 1;
        dw.setDockLocation(area);

        // Ensure signal is only fired on changes
        QCOMPARE(spy.count(), expectedCount);
        if (expectedCount)
            QCOMPARE(dockLocation(&spy), area);

        // Ensure getter reports correctly
        QTRY_COMPARE(dw.dockLocation(), area);

        // Ensure setting NoDockWidgetArea floats the dock widget
        if (area == Qt::NoDockWidgetArea) {
            QCOMPARE(dw.isFloating(), true);
            dw.setFloating(false);
        }
    }

    spy.clear();
    QDockWidget dw2;
    dw2.setObjectName("dock2");
    mw.addDockWidget(Qt::TopDockWidgetArea, &dw2);
    mw.tabifyDockWidget(&dw2, &dw);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(dockLocation(&spy), Qt::TopDockWidgetArea);
    spy.clear();

    mw.splitDockWidget(&dw2, &dw, Qt::Horizontal);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(dockLocation(&spy), Qt::TopDockWidgetArea);
    spy.clear();

    dw.setFloating(true);
    QTRY_COMPARE(spy.size(), 1);
    QCOMPARE(dockLocation(&spy), Qt::NoDockWidgetArea);
    spy.clear();

    dw.setFloating(false);
    QTRY_COMPARE(spy.size(), 1);
    QCOMPARE(dockLocation(&spy), Qt::TopDockWidgetArea);
    spy.clear();

    QByteArray ba = mw.saveState();
    mw.restoreState(ba);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(dockLocation(&spy), Qt::TopDockWidgetArea);
}

void tst_QDockWidget::setTitleBarWidget()
{
    //test the successive usage of setTitleBarWidget

    QDockWidget dock;
    QWidget w, w2;

    dock.show();
    qApp->processEvents();

    dock.setTitleBarWidget(&w);
    qApp->processEvents();
    QCOMPARE(w.isVisible(), true);

    //set another widget as the titlebar widget
    dock.setTitleBarWidget(&w2); // this used to crash (task 184668)
    qApp->processEvents();
    QCOMPARE(w.isVisible(), false);
    QCOMPARE(w2.isVisible(), true);

    //tries to reset the titlebarwidget to none
    dock.setTitleBarWidget(0);
    qApp->processEvents();
    QCOMPARE(w.isVisible(), false);
    QCOMPARE(w2.isVisible(), false);
}

void tst_QDockWidget::titleBarDoubleClick()
{
    QMainWindow win;
    QDockWidget dock(&win);
    win.show();
    dock.setFloating(true);

    QMouseEvent e(QEvent::NonClientAreaMouseButtonDblClick, {}, {}, {}, {}, {}, {}, {},
                  QPointingDevice::primaryPointingDevice());
    QApplication::sendEvent(&dock, &e);
    QVERIFY(dock.isFloating());
    QCOMPARE(win.dockWidgetArea(&dock), Qt::NoDockWidgetArea);

    win.addDockWidget(Qt::TopDockWidgetArea, &dock);
    dock.setFloating(true);
    QApplication::sendEvent(&dock, &e);
    QVERIFY(!dock.isFloating());
    QCOMPARE(win.dockWidgetArea(&dock), Qt::TopDockWidgetArea);
}

static QDockWidget *createTestDock(QMainWindow &parent)
{
    const QString title = QStringLiteral("dock1");
    QDockWidget *dock = new QDockWidget(title, &parent);
    dock->setObjectName(title);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    return dock;
}

void tst_QDockWidget::restoreStateOfFloating()
{
    QMainWindow mw;
    QDockWidget *dock = createTestDock(mw);
    mw.addDockWidget(Qt::TopDockWidgetArea, dock);
    QVERIFY(!dock->isFloating());
    QByteArray ba = mw.saveState();
    dock->setFloating(true);
    QVERIFY(dock->isFloating());
    QVERIFY(mw.restoreState(ba));
    QVERIFY(!dock->isFloating());
}

void tst_QDockWidget::restoreStateWhileStillFloating()
{
    // When the dock widget is already floating then it takes a different code path
    // so this test covers the case where the restoreState() is effectively just
    // moving it back and resizing it
    const QRect availGeom = QGuiApplication::primaryScreen()->availableGeometry();
    const QPoint startingDockPos = availGeom.center();
    QMainWindow mw;
    QDockWidget *dock = createTestDock(mw);
    mw.addDockWidget(Qt::TopDockWidgetArea, dock);
    dock->setFloating(true);
    dock->move(startingDockPos);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    QVERIFY(dock->isFloating());
    QByteArray ba = mw.saveState();
    const QPoint dockPos = dock->pos();
    dock->move(availGeom.topLeft() + QPoint(10, 10));
    dock->resize(dock->size() + QSize(10, 10));
    QVERIFY(mw.restoreState(ba));
    QVERIFY(dock->isFloating());
    if (!QGuiApplication::platformName().compare("xcb", Qt::CaseInsensitive))
        QTRY_COMPARE(dock->pos(), dockPos);
}

void tst_QDockWidget::restoreDockWidget()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Test skipped on Wayland.");
    QByteArray geometry;
    QByteArray state;

    const QString name = QStringLiteral("main");
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 5;
    const QPoint mainWindowPos = availableGeometry.bottomRight() - QPoint(size.width(), size.height()) - QPoint(100, 100);
    const QPoint dockPos = availableGeometry.center();

    {
        QMainWindow saveWindow;
        saveWindow.setObjectName(name);
        saveWindow.setWindowTitle(QTest::currentTestFunction() + QStringLiteral(" save"));
        saveWindow.resize(size);
        saveWindow.move(mainWindowPos);
        saveWindow.restoreState(QByteArray());
        QDockWidget *dock = createTestDock(saveWindow);
        QVERIFY(!saveWindow.restoreDockWidget(dock)); // Not added, no placeholder
        saveWindow.addDockWidget(Qt::TopDockWidgetArea, dock);
        dock->setFloating(true);
        dock->resize(size);
        dock->move(dockPos);
        saveWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&saveWindow));
        QVERIFY(dock->isFloating());
        state = saveWindow.saveState();
        geometry = saveWindow.saveGeometry();

        // QTBUG-49832: Delete and recreate the dock; it should be restored to the same position.
        delete dock;
        dock = createTestDock(saveWindow);
        QVERIFY(saveWindow.restoreDockWidget(dock));
        dock->show();
        QVERIFY(QTest::qWaitForWindowExposed(dock));
        QTRY_VERIFY(dock->isFloating());
        QTRY_COMPARE(dock->pos(), dockPos);
    }

    QVERIFY(!geometry.isEmpty());
    QVERIFY(!state.isEmpty());

    // QTBUG-45780: Completely recreate the dock widget from the saved state.
    {
        QMainWindow restoreWindow;
        restoreWindow.setObjectName(name);
        restoreWindow.setWindowTitle(QTest::currentTestFunction() + QStringLiteral(" restore"));
        QVERIFY(restoreWindow.restoreState(state));
        QVERIFY(restoreWindow.restoreGeometry(geometry));

        // QMainWindow::restoreDockWidget() restores the state when adding the dock
        // after restoreState().
        QDockWidget *dock = createTestDock(restoreWindow);
        QVERIFY(restoreWindow.restoreDockWidget(dock));
        restoreWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&restoreWindow));
        QTRY_VERIFY(dock->isFloating());
        QTRY_COMPARE(dock->pos(), dockPos);
    }
}

void tst_QDockWidget::task165177_deleteFocusWidget()
{
    QMainWindow mw;
    QDockWidget *dw = new QDockWidget(&mw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw);
    QLineEdit *ledit = new QLineEdit;
    dw->setWidget(ledit);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    qApp->processEvents();
    dw->setFloating(true);
    delete ledit;
    QCOMPARE(mw.focusWidget(), nullptr);
    QCOMPARE(dw->focusWidget(), nullptr);
}

void tst_QDockWidget::task169808_setFloating()
{
    //we try to test if the sizeHint of the dock widget widget is taken into account

    class MyWidget : public QWidget
    {
    public:
        QSize sizeHint() const override
        {
            const QRect& deskRect = QGuiApplication::primaryScreen()->availableGeometry();
            return QSize(qMin(300, deskRect.width() / 2), qMin(300, deskRect.height() / 2));
        }

        QSize minimumSizeHint() const override
        {
            return QSize(20,20);
        }

        void paintEvent(QPaintEvent *) override
        {
            QPainter p(this);
            p.fillRect(rect(), Qt::red);
        }
    };
    QMainWindow mw;
    mw.setCentralWidget(new MyWidget);
    QDockWidget *dw = new QDockWidget("my dock");
    dw->setWidget(new MyWidget);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw);
    dw->setFloating(true);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    QCOMPARE(dw->widget()->size(), dw->widget()->sizeHint());

    //and now we try to test if the contents margin is taken into account
    dw->widget()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dw->setFloating(false);
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    qApp->processEvents(); //leave time processing events


    const QSize oldSize = dw->size();
    const int margin = 20;

    dw->setContentsMargins(margin, margin, margin, margin);

    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    qApp->processEvents(); //leave time processing events

    //widget size shouldn't have changed
    QCOMPARE(dw->widget()->size(), dw->widget()->sizeHint());
    //dockwidget should be bigger
    QCOMPARE(dw->size(), oldSize + QSize(margin * 2, margin * 2));


}

void tst_QDockWidget::task237438_setFloatingCrash()
{
    //should not crash
    QDockWidget pqdwDock;
    pqdwDock.setFloating(false);
    pqdwDock.show();
}

void tst_QDockWidget::task248604_infiniteResize()
{
    QDockWidget d;
    QTabWidget *t = new QTabWidget;
    t->addTab(new QWidget, "Foo");
    d.setWidget(t);
    d.setContentsMargins(2, 2, 2, 2);
    d.setMinimumSize(320, 240);
    d.showNormal();
    QTRY_COMPARE(d.size(), QSize(320, 240));
}


void tst_QDockWidget::task258459_visibilityChanged()
{
    QMainWindow win;
    QDockWidget dock1, dock2;
    win.addDockWidget(Qt::RightDockWidgetArea, &dock1);
    win.tabifyDockWidget(&dock1, &dock2);
    QSignalSpy spy1(&dock1, SIGNAL(visibilityChanged(bool)));
    QSignalSpy spy2(&dock2, SIGNAL(visibilityChanged(bool)));
    win.show();
    QVERIFY(QTest::qWaitForWindowActive(&win));
    QCOMPARE(spy1.size(), 1);
    QCOMPARE(spy1.first().first().toBool(), false); //dock1 is invisible
    QCOMPARE(spy2.size(), 1);
    QCOMPARE(spy2.first().first().toBool(), true); //dock1 is visible
}

void tst_QDockWidget::taskQTBUG_1665_closableChanged()
{
    QDockWidget dock;
    dock.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock));

    QDockWidgetLayout *l = qobject_cast<QDockWidgetLayout*>(dock.layout());

    if (l && !l->nativeWindowDeco())
        QSKIP("this machine doesn't support native dock widget");

    QVERIFY(dock.windowFlags() & Qt::WindowCloseButtonHint);

    //now let's remove the closable attribute
    dock.setFeatures(dock.features() ^ QDockWidget::DockWidgetClosable);
    QVERIFY(!(dock.windowFlags() & Qt::WindowCloseButtonHint));
}

void tst_QDockWidget::taskQTBUG_9758_undockedGeometry()
{
    QMainWindow window;
    QDockWidget dock1(&window);
    QDockWidget dock2(&window);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock1);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock2);
    window.tabifyDockWidget(&dock1, &dock2);
    dock1.hide();
    dock2.hide();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    dock1.setFloating(true);
    dock1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock1));

    QVERIFY(dock1.x() >= 0);
    QVERIFY(dock1.y() >= 0);
}

void tst_QDockWidget::setWindowTitle()
{
    QMainWindow window;
    QDockWidget dock1(&window);
    QDockWidget dock2(&window);
    constexpr QLatin1StringView dock1Title("&Window");
    constexpr QLatin1StringView dock2Title("&Modifiable Window [*]");

    // Set title on docked dock widgets, before main window is shown
    dock1.setWindowTitle(dock1Title);
    dock2.setWindowTitle(dock2Title);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock1);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock2);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QCOMPARE(dock1.windowTitle(), dock1Title);
    QCOMPARE(dock2.windowTitle(), dock2Title);

    // Check if title remains unchanged when docking / undocking
    dock1.setFloating(true);
    dock1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock1));
    QCOMPARE(dock1.windowTitle(), dock1Title);
    dock1.setFloating(false);
    QCOMPARE(dock1.windowTitle(), dock1Title);
    dock1.setFloating(true);
    dock1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock1));

    // Change a floating dock widget's title and check remains unchanged when docking
    constexpr QLatin1StringView changed("Changed ");
    dock1.setWindowTitle(QString(changed + dock1Title));
    QCOMPARE(dock1.windowTitle(), QString(changed + dock1Title));
    dock1.setFloating(false);
    QVERIFY(QTest::qWaitFor([&dock1](){ return !dock1.windowHandle(); }));
    QCOMPARE(dock1.windowTitle(), QString(changed + dock1Title));

    // Test consistency after toggling modified and floating
    dock2.setWindowModified(true);
    QCOMPARE(dock2.windowTitle(), dock2Title);
    dock2.setFloating(true);
    dock2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock2));
    QCOMPARE(dock2.windowTitle(), dock2Title);
    dock2.setWindowModified(false);
    QCOMPARE(dock2.windowTitle(), dock2Title);
    dock2.setFloating(false);
    QCOMPARE(dock2.windowTitle(), dock2Title);
    dock2.setFloating(true);
    dock2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock2));
    QCOMPARE(dock2.windowTitle(), dock2Title);

    // Test title change of a closed dock widget
    static constexpr QLatin1StringView closedDock2("Closed D2");
    dock2.close();
    dock2.setWindowTitle(closedDock2);
    QCOMPARE(dock2.windowTitle(), closedDock2);
}

void tst_QDockWidget::windowIcon()
{
    QPixmap pm(5, 5);
    pm.fill(Qt::red);
    const QIcon icon(pm);
    pm.fill(Qt::green);
    const QIcon appIcon(pm);
    qApp->setWindowIcon(appIcon);
    QMainWindow mainWindow;
    mainWindow.setCentralWidget(new QWidget);
    auto *d1 = new QDockWidget;
    auto *d2 = new QDockWidget;
    d2->setWindowIcon(icon);
    mainWindow.addDockWidget(Qt::TopDockWidgetArea, d1);
    mainWindow.addDockWidget(Qt::TopDockWidgetArea, d2);
    mainWindow.tabifyDockWidget(d1, d2);
    auto *bar = mainWindow.findChild<QTabBar *>();
    Q_ASSERT(bar->count() == 2);
    QCOMPARE(bar->tabIcon(0), QIcon());
    QCOMPARE(bar->tabIcon(1), icon);
}

// helpers for dockPermissions, hideAndShow, closeAndDelete
#ifdef QT_BUILD_INTERNAL
void tst_QDockWidget::createTestWidgets(QMainWindow* &mainWindow, QPointer<QWidget> &cent, QPointer<QDockWidget> &d1, QPointer<QDockWidget> &d2) const
{
    // Enable logging if required
    if (dockWidgetLog)
        QLoggingCategory::setFilterRules("qt.widgets.dockwidgets=true");

    // Derive sizes and positions from primary screen
    const QRect screen = QApplication::primaryScreen()->availableGeometry();
    const QPoint m_topLeft = screen.topLeft();
    const QSize s_mwindow = QApplication::primaryScreen()->availableSize() * 0.7;

    mainWindow = new QMainWindow;
    mainWindow->setMouseTracking(true);
    mainWindow->setFixedSize(s_mwindow);
    cent = new QWidget;
    mainWindow->setCentralWidget(cent);
    cent->setLayout(new QGridLayout);
    cent->layout()->setContentsMargins(0, 0, 0, 0);
    cent->setMinimumSize(0, 0);
    mainWindow->setDockOptions(QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);
    mainWindow->move(m_topLeft);

    const int minWidth = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
    const QSize minSize(minWidth, 2 * minWidth);
    d1 = new QDockWidget(mainWindow);
    d1->setMinimumSize(minSize);
    d1->setWindowTitle("I am D1");
    d1->setObjectName("D1");
    d1->setFeatures(QDockWidget::DockWidgetFeatureMask);
    d1->setAllowedAreas(Qt::DockWidgetArea::AllDockWidgetAreas);

    d2 = new QDockWidget(mainWindow);
    d2->setMinimumSize(minSize);
    d2->setWindowTitle("I am D2");
    d2->setObjectName("D2");
    d2->setFeatures(QDockWidget::DockWidgetFeatureMask);
    d2->setAllowedAreas(Qt::DockWidgetArea::RightDockWidgetArea);

    mainWindow->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, d1);
    mainWindow->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, d2);
    d1->show();
    d2->show();
    mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(mainWindow));

}

QPoint tst_QDockWidget::dragPoint(QDockWidget* dockWidget)
{
    Q_ASSERT(dockWidget);
    QDockWidgetLayout *dwlayout = qobject_cast<QDockWidgetLayout *>(dockWidget->layout());
    Q_ASSERT(dwlayout);
    return dockWidget->mapToGlobal(dwlayout->titleArea().center());
}

void tst_QDockWidget::moveDockWidget(QDockWidget* dw, QPoint to, QPoint from, MoveDockWidgetRule rule) const
{
    Q_ASSERT(dw);

    // If no from point is given, use the drag point
    if (from.isNull())
        from = dragPoint(dw);

    // move and log
    const QPoint source = dw->mapFromGlobal(from);
    const QPoint target = dw->mapFromGlobal(to);
    qCDebug(lcTestDockWidget) << "Move" << dw->objectName() << "from" << source;
    qCDebug(lcTestDockWidget) << "Move" << dw->objectName() << "from" << from;
    QTest::mousePress(dw, Qt::LeftButton, Qt::KeyboardModifiers(), source);
    QTest::mouseMove(dw, target);
    qCDebug(lcTestDockWidget) << "Move" << dw->objectName() << "to" << target;
    qCDebug(lcTestDockWidget) << "Move" << dw->objectName() << "to" << to;
    if (rule == MoveDockWidgetRule::Drop) {
        QTest::mouseRelease(dw, Qt::LeftButton, Qt::KeyboardModifiers(), target);
        QTest::qWait(waitingTime);

        // Verify WindowActive only for floating dock widgets
        if (dw->isFloating())
            QTRY_VERIFY(QTest::qWaitForWindowActive(dw));
        return;
    }
    qCDebug(lcTestDockWidget) << "Aborting move and dropping at origin";

    // Give animations some time
    QTest::qWait(waitingTime);
    QTest::mouseMove(dw, from);
    QTest::mouseRelease(dw, Qt::LeftButton, Qt::KeyboardModifiers(), from);
    QTest::qWait(waitingTime);
}

void tst_QDockWidget::unplugAndResize(QMainWindow* mainWindow, QDockWidget* dw, QPoint home, QSize size) const
{
    Q_ASSERT(mainWindow);
    Q_ASSERT(dw);

    // Return if floating
    if (dw->isFloating())
        return;

    QMainWindowLayout* layout = qobject_cast<QMainWindowLayout*>(mainWindow->layout());
    Qt::DockWidgetArea area = layout->dockWidgetArea(dw);

    // calculate minimum lateral move to unplug a dock widget
    const int unplugMargin = 80;
    int my = 0;
    int mx = 0;

    switch (area) {
    case Qt::LeftDockWidgetArea:
        mx = unplugMargin;
        break;
    case Qt::TopDockWidgetArea:
        my = unplugMargin;
        break;
    case Qt::RightDockWidgetArea:
        mx = -unplugMargin;
        break;
    case Qt::BottomDockWidgetArea:
        my = -unplugMargin;
        break;
    default:
        return;
    }

    // Remember size for comparison with unplugged object
#ifdef Q_OS_LINUX
    const int pluggedWidth = dw->width();
    const int pluggedHeight = dw->height();
#endif

    // unplug and resize a dock Widget
    qCDebug(lcTestDockWidget) << "*** unplug and resize" << dw->objectName();
    QPoint pos1 = dw->mapToGlobal(dw->rect().center());
    pos1.rx() += mx;
    pos1.ry() += my;
    moveDockWidget(dw, pos1, dw->mapToGlobal(dw->rect().center()), MoveDockWidgetRule::Drop);
    QTRY_VERIFY(dw->isFloating());

    // Unplugged object's size may differ max. by 2x frame size
#ifdef Q_OS_LINUX
    const int xMargin = 2 * dw->frameSize().width();
    const int yMargin = 2 * dw->frameSize().height();
    QVERIFY(dw->height() - pluggedHeight <= xMargin);
    QVERIFY(dw->width() - pluggedWidth <= yMargin);
#endif

    qCDebug(lcTestDockWidget) << "Resizing" << dw->objectName() << "to" << size;
    dw->setFixedSize(size);
    QTest::qWait(waitingTime);
    qCDebug(lcTestDockWidget) << "Move" << dw->objectName() << "to its home" << dw->mapFromGlobal(home);
    dw->move(home);
}

bool tst_QDockWidget::checkFloatingTabs(QMainWindow* mainWindow, QPointer<QDockWidgetGroupWindow> &ftabs, const QList<QDockWidget*> &dwList) const
{
    Q_ASSERT(mainWindow);

    // Check if mainWindow has a floatingTab child
    ftabs = mainWindow->findChild<QDockWidgetGroupWindow*>();
    if (ftabs.isNull()) {
        qCDebug(lcTestDockWidget) << "MainWindow has no DockWidgetGroupWindow" << mainWindow;
        return false;
    }

    QTabBar* tab = ftabs->findChild<QTabBar*>();
    if (!tab) {
        qCDebug(lcTestDockWidget) << "DockWidgetGroupWindow has no tab bar" << ftabs;
        return false;
    }

    // both dock widgets must be direct children of the main window
    const QList<QDockWidget*> children = ftabs->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
    if (dwList.size() > 0)
    {
        if (dwList.size() != children.size()) {
            qCDebug(lcTestDockWidget) << "Expected DockWidgetGroupWindow children:" << dwList.size()
                                      << "Children found:" << children.size();

            qCDebug(lcTestDockWidget) << "Expected:" << dwList;
            qCDebug(lcTestDockWidget) << "Found in" << ftabs << ":" << children.size();
            return false;
        }

        for (const QDockWidget* child : dwList) {
            if (!children.contains(child)) {
                qCDebug(lcTestDockWidget) << "Expected child" << child << "not found in" << children;
                return false;
            }
        }
    }

    // Always select first tab position
    qCDebug(lcTestDockWidget) << "click on first tab";
    QTest::mouseClick(tab, Qt::LeftButton, Qt::KeyboardModifiers(), tab->tabRect(0).center());
    return true;
}

#ifdef QT_BUILD_INTERNAL
// Statics for xcb error, raise() suppert / msg handler
static tst_QDockWidget *qThis = nullptr;
static void (*oldMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
#define QXCBVERIFY(cond) do { if (xcbError) QSKIP("Test skipped due to XCB error"); QVERIFY(cond); } while (0)

// detect xcb error and missing raise() support
// qt.qpa.xcb: internal error:  void QXcbWindow::setNetWmStateOnUnmappedWindow() called on mapped window
void tst_QDockWidget::xcbMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_ASSERT(oldMessageHandler);

    if (type == QtWarningMsg) {
        Q_ASSERT(qThis);
        if (QString(context.category) == "qt.qpa.xcb" && msg.contains("internal error"))
            qThis->xcbError = true;
        if (msg.contains("does not support raise"))
            qThis->platformSupportingRaise = false;
    }

    return oldMessageHandler(type, context, msg);
}
#endif

void tst_QDockWidget::createFloatingTabs(QMainWindow* &mainWindow, QPointer<QWidget> &cent,
                                         QPointer<QDockWidget> &d1, QPointer<QDockWidget> &d2,
                                         QList<int> &path1, QList<int> &path2) const
{
    createTestWidgets(mainWindow, cent, d1, d2);

#ifdef QT_BUILD_INTERNAL
    qThis = const_cast<tst_QDockWidget *>(this);
    oldMessageHandler = qInstallMessageHandler(xcbMessageHandler);
    auto resetMessageHandler = qScopeGuard([] { qInstallMessageHandler(oldMessageHandler); });
#endif

    // Test will fail if platform doesn't support raise.
    mainWindow->windowHandle()->handle()->raise();
    if (!platformSupportingRaise)
        return;

    // remember paths to d1 and d2
    QMainWindowLayout* layout = qobject_cast<QMainWindowLayout *>(mainWindow->layout());
    path1 = layout->layoutState.indexOf(d1);
    path2 = layout->layoutState.indexOf(d2);

    // unplug and resize both dock widgets
    unplugAndResize(mainWindow, d1, home1(mainWindow), size1(mainWindow));
    unplugAndResize(mainWindow, d2, home2(mainWindow), size2(mainWindow));

    // docks must be parented to the main window, no group window must exist
    QCOMPARE(d1->parentWidget(), mainWindow);
    QCOMPARE(d2->parentWidget(), mainWindow);
    QVERIFY(mainWindow->findChildren<QDockWidgetGroupWindow *>().isEmpty());

    // Test plugging
    qCDebug(lcTestDockWidget) << "*** move d1 dock over d2 dock ***";
    qCDebug(lcTestDockWidget) << "**********(test plugging)*************";
    qCDebug(lcTestDockWidget) << "Move d1 over d2";
    moveDockWidget(d1, d2->mapToGlobal(d2->rect().center()), QPoint(), MoveDockWidgetRule::Drop);

    // Now MainWindow has to have a floatingTab child
    QPointer<QDockWidgetGroupWindow> ftabs;
    QTRY_VERIFY(checkFloatingTabs(mainWindow, ftabs, QList<QDockWidget *>() << d1 << d2));
}
#endif // QT_BUILD_INTERNAL

// test floating tabs and item_tree consistency
void tst_QDockWidget::floatingTabs()
{
#ifdef Q_OS_WIN
    QSKIP("Test skipped on Windows platforms");
#endif // Q_OS_WIN
#ifdef QT_BUILD_INTERNAL
    // Create a mainwindow with a central widget and two dock widgets
    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    QList<int> path1;
    QList<int> path2;
    qCreateFloatingTabs(mainWindow, cent, d1, d2, path1, path2);

    QCOMPARE(mainWindow->tabifiedDockWidgets(d1), {d2});
    QCOMPARE(mainWindow->tabifiedDockWidgets(d2), {d1});

    /*
     * unplug both dockwidgets, resize them and plug them into a joint floating tab
     * expected behavior: QDOckWidgetGroupWindow with both widgets is created
     */

    // disabled due to flakiness on macOS and Windows
    if (d1->isFloating())
        qWarning("OS flakiness: D1 is docked and reports being floating");
    if (d2->isFloating())
        qWarning("OS flakiness: D2 is docked and reports being floating");

    // Now MainWindow has to have a floatingTab child
    QPointer<QDockWidgetGroupWindow> ftabs;
    QTRY_VERIFY(checkFloatingTabs(mainWindow, ftabs, QList<QDockWidget *>() << d1 << d2));

    // Hide both dock widgets. Verify that the group window is also hidden.
    qCDebug(lcTestDockWidget) << "*** Hide and show tabbed dock widgets ***";
    d1->hide();
    d2->hide();
    QTRY_VERIFY(ftabs->isHidden());

    // Show both dockwidgets again. Verify that the group window is visible.
    d1->show();
    d2->show();
    QTRY_VERIFY(ftabs->isVisible());

    /*
     * replug both dock widgets into their initial position
     * expected behavior:
       - both docks are plugged
       - both docks are no longer floating
       - title changes have been propagated
     */


    // limitation: QTest cannot handle drag to unplug.
    // reason: Object under mouse mutates from QTabBar::tab to QDockWidget. QTest cannot handle that.
    // => click float button to unplug
    qCDebug(lcTestDockWidget) << "*** test unplugging from floating dock ***";

    // QDockWidget must have a QAbstractButton with object name "qt_dockwidget_floatbutton"
    QAbstractButton* floatButton = d1->findChild<QAbstractButton*>("qt_dockwidget_floatbutton", Qt::FindDirectChildrenOnly);
    QTRY_VERIFY(floatButton != nullptr);
    QPoint pos1 = floatButton->rect().center();
    qCDebug(lcTestDockWidget) << "unplug d1" << pos1;
    QTest::mouseClick(floatButton, Qt::LeftButton, Qt::KeyboardModifiers(), pos1);
    QTest::qWait(waitingTime);

    // d1 and d2 must be floating again
    QTRY_VERIFY(d1->isFloating());
    QTRY_VERIFY(d2->isFloating());

    // d2 was the active tab, so d1 was not visible
    QTRY_VERIFY(d1->isVisible());
    QTRY_VERIFY(d2->isVisible());

    // Plug back into dock areas
    qCDebug(lcTestDockWidget) << "*** test plugging back to dock areas ***";
    qCDebug(lcTestDockWidget) << "Move d1 to left dock";
    moveDockWidget(d1, dockPoint(mainWindow, Qt::LeftDockWidgetArea), QPoint(), MoveDockWidgetRule::Drop);
    qCDebug(lcTestDockWidget) << "Move d2 to right dock";
    moveDockWidget(d2, dockPoint(mainWindow, Qt::RightDockWidgetArea), QPoint(), MoveDockWidgetRule::Drop);

    qCDebug(lcTestDockWidget) << "Waiting" << waitBeforeClose << "ms before plugging back.";
    QTest::qWait(waitBeforeClose);

    // Both dock widgets must no longer be floating
    QTRY_VERIFY(!d1->isFloating());
    QTRY_VERIFY(!d2->isFloating());

    // check if QDockWidgetGroupWindow has been removed from mainWindowLayout and properly deleted
    QTRY_VERIFY(!mainWindow->findChild<QDockWidgetGroupWindow*>());
    QTRY_VERIFY(ftabs.isNull());

    // Check if paths are consistent
    QMainWindowLayout* layout = qobject_cast<QMainWindowLayout *>(mainWindow->layout());
    qCDebug(lcTestDockWidget) << "Checking path consistency" << layout->layoutState.indexOf(d1) << layout->layoutState.indexOf(d2);

    // Paths must be identical
    QTRY_COMPARE(layout->layoutState.indexOf(d1), path1);
    QTRY_COMPARE(layout->layoutState.indexOf(d2), path2);

    QCOMPARE(mainWindow->tabifiedDockWidgets(d1), {});
    QCOMPARE(mainWindow->tabifiedDockWidgets(d2), {});
#else
    QSKIP("test requires -developer-build option");
#endif // QT_BUILD_INTERNAL
}

void tst_QDockWidget::deleteFloatingTabWithSingleDockWidget_data()
{
#ifdef QT_BUILD_INTERNAL
    QTest::addColumn<int>("reason");
    QTest::addRow("Delete child") << static_cast<int>(ChildRemovalReason::Destroyed);
    QTest::addRow("Close child") << static_cast<int>(ChildRemovalReason::Closed);
    QTest::addRow("Reparent child") << static_cast<int>(ChildRemovalReason::Reparented);
#endif
}

void tst_QDockWidget::deleteFloatingTabWithSingleDockWidget()
{
#ifdef Q_OS_WIN
    QSKIP("Test skipped on Windows platforms");
#endif // Q_OS_WIN
#ifdef QT_BUILD_INTERNAL

    QFETCH(int, reason);
    const ChildRemovalReason removalReason = static_cast<ChildRemovalReason>(reason);

    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    QList<int> path1;
    QList<int> path2;
    qCreateFloatingTabs(mainWindow, cent, d1, d2, path1, path2);

    switch (removalReason) {
    case ChildRemovalReason::Destroyed:
        delete d1;
        break;
    case ChildRemovalReason::Closed:
        d1->close();
        break;
    case ChildRemovalReason::Reparented:
        // This will create an invalid state, because setParent() doesn't fix the item_list.
        // Testing this case anyway, because setParent() includig item_list fixup is executed,
        // when the 2nd last dock widget is dragged out of a floating tab.
        // => despite of the broken state, the group window has to be gone.
        d1->setParent(mainWindow);
        break;
    }

    QTRY_VERIFY(!qobject_cast<QDockWidgetGroupWindow *>(d2->parentWidget()));
    QTRY_VERIFY(mainWindow->findChildren<QDockWidgetGroupWindow *>().isEmpty());
#endif // QT_BUILD_INTERNAL
}

void tst_QDockWidget::hoverWithoutDrop()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Test skipped on Wayland.");
#ifdef QT_BUILD_INTERNAL

    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    createTestWidgets(mainWindow, cent, d1, d2);
    std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

    // unplug and resize both dock widgets
    unplugAndResize(mainWindow, d1, home1(mainWindow), size1(mainWindow));
    unplugAndResize(mainWindow, d2, home2(mainWindow), size2(mainWindow));

    // Test plugging
    qCDebug(lcTestDockWidget) << "*** move d1 dock over d2 dock ***";
    qCDebug(lcTestDockWidget) << "*******(test hovering)***********";
    qCDebug(lcTestDockWidget) << "Move d1 over d2, wait and return to origin";
    const QPoint source = d1->mapToGlobal(d1->rect().center());
    const QPoint target = d2->mapToGlobal(d2->rect().center());
    moveDockWidget(d1, target, source, MoveDockWidgetRule::Abort);
    auto *groupWindow = mainWindow->findChild<QDockWidgetGroupWindow *>();
    QCOMPARE(groupWindow, nullptr);
#else
    QSKIP("test requires -developer-build option");
#endif // QT_BUILD_INTERNAL
}

// test hide & show
void tst_QDockWidget::hideAndShow()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Test skipped on Wayland.");
#ifdef QT_BUILD_INTERNAL
    // Skip test if xcb error is launched
    qThis = this;
    oldMessageHandler = qInstallMessageHandler(xcbMessageHandler);
    auto resetMessageHandler = qScopeGuard([] { qInstallMessageHandler(oldMessageHandler); });

    // Create a mainwindow with a central widget and two dock widgets
    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    createTestWidgets(mainWindow, cent, d1, d2);
    std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

    // Check hiding of docked widgets
    qCDebug(lcTestDockWidget) << "Hiding mainWindow with plugged dock widgets" << mainWindow;
    mainWindow->hide();
    QXCBVERIFY(!mainWindow->isVisible());
    QXCBVERIFY(!d1->isVisible());
    QXCBVERIFY(!d2->isVisible());

    // Check showing everything again
    qCDebug(lcTestDockWidget) << "Showing mainWindow with plugged dock widgets" << mainWindow;
    mainWindow->show();
    QXCBVERIFY(QTest::qWaitForWindowActive(mainWindow));
    QXCBVERIFY(QTest::qWaitForWindowExposed(mainWindow));
    QXCBVERIFY(mainWindow->isVisible());
    QXCBVERIFY(QTest::qWaitForWindowActive(d1));
    QXCBVERIFY(d1->isVisible());
    QXCBVERIFY(QTest::qWaitForWindowActive(d2));
    QXCBVERIFY(d2->isVisible());

    // in case of XCB errors, unplugAndResize will block and cause the test to time out.
    // => force skip
    QTest::qWait(waitingTime);
    if (xcbError)
        QSKIP("Test skipped due to XCB error");

    // unplug and resize both dock widgets
    unplugAndResize(mainWindow, d1, home1(mainWindow), size1(mainWindow));
    unplugAndResize(mainWindow, d2, home2(mainWindow), size2(mainWindow));

    // Check hiding of undocked widgets
    qCDebug(lcTestDockWidget) << "Hiding mainWindow with unplugged dock widgets" << mainWindow;
    mainWindow->hide();
    QTRY_VERIFY(!mainWindow->isVisible());
    QTRY_VERIFY(d1->isVisible());
    QTRY_VERIFY(d2->isVisible());
    d1->hide();
    d2->hide();
    QTRY_VERIFY(!d1->isVisible());
    QTRY_VERIFY(!d2->isVisible());


    // Check floating, hidden dock widgets remain hidden, when their state is restored
    qCDebug(lcTestDockWidget) << "Restoring state of unplugged, hidden dock widgets" << mainWindow;
    const QByteArray state = mainWindow->saveState();
    mainWindow->restoreState(state);
    mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(mainWindow));
    QTRY_VERIFY(!d1->isVisible());
    QTRY_VERIFY(!d2->isVisible());

    qCDebug(lcTestDockWidget) << "Waiting" << waitBeforeClose << "ms before closing.";
    QTest::qWait(waitBeforeClose);
#else
    QSKIP("test requires -developer-build option");
#endif // QT_BUILD_INTERNAL
}

// test closing and deleting consistency
void tst_QDockWidget::closeAndDelete()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Test skipped on Wayland.");
#ifdef QT_BUILD_INTERNAL
    if (QSysInfo::productType() == "rhel")
        QSKIP("Memory leak on RHEL 9.2 QTBUG-124559", TestFailMode::Abort);

    // Create a mainwindow with a central widget and two dock widgets
    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    createTestWidgets(mainWindow, cent, d1, d2);
    std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

    // unplug and resize both dock widgets
    unplugAndResize(mainWindow, d1, home1(mainWindow), size1(mainWindow));
    unplugAndResize(mainWindow, d2, home2(mainWindow), size2(mainWindow));

    // Create a floating tab and unplug it again
    qCDebug(lcTestDockWidget) << "Move d1 over d2";
    moveDockWidget(d1, d2->mapToGlobal(d2->rect().center()), QPoint(), MoveDockWidgetRule::Drop);

    // Both dock widgets must no longer be floating
    // disabled due to flakiness on macOS and Windows
    //QTRY_VERIFY(!d1->isFloating());
    //QTRY_VERIFY(!d2->isFloating());
    if (d1->isFloating())
        qWarning("OS flakiness: D1 is docked and reports being floating");
    if (d2->isFloating())
        qWarning("OS flakiness: D2 is docked and reports being floating");

    // Close everything with a single shot. Expected behavior: Event loop stops
    QSignalSpy closeSpy(qApp, &QApplication::lastWindowClosed);
    QObject localContext;

    QTimer::singleShot(0, &localContext, [&](){
        mainWindow->close();
        QTRY_VERIFY(!mainWindow->isVisible());
        QTRY_VERIFY(d1->isVisible());
        QTRY_VERIFY(d2->isVisible());
        d1->close();
        d2->close();
        QTRY_VERIFY(!d1->isVisible());
        QTRY_VERIFY(!d2->isVisible());
        QTRY_COMPARE(closeSpy.count(), 1);
        QApplication::quit();
    });

    QApplication::exec();

    // Check heap cleanup
    qCDebug(lcTestDockWidget) << "Deleting mainWindow";
    up_mainWindow.reset();
    QTRY_VERIFY(d1.isNull());
    QTRY_VERIFY(d2.isNull());
    QTRY_VERIFY(cent.isNull());
#else
    QSKIP("test requires -developer-build option");
#endif // QT_BUILD_INTERNAL
}

void tst_QDockWidget::closeUnclosable()
{
    QDockWidget *dockWidget = new QDockWidget("dock");
    dockWidget->setWidget(new QScrollArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetFloatable);

    QMainWindow mw;
    mw.addDockWidget(Qt::TopDockWidgetArea, dockWidget);
    mw.show();

    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    dockWidget->setFloating(true);

    QCOMPARE(dockWidget->close(), false);
    mw.close();
    QCOMPARE(dockWidget->close(), true);
}

// Test dock area permissions
void tst_QDockWidget::dockPermissions()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Test skipped on Wayland.");
#ifdef Q_OS_WIN
    QSKIP("Test skipped on Windows platforms");
#endif // Q_OS_WIN
#ifdef QT_BUILD_INTERNAL
    // Create a mainwindow with a central widget and two dock widgets
    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    createTestWidgets(mainWindow, cent, d1, d2);
    std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

    /*
     * Unplug both dock widgets from their dock areas and hover them over each other
     * expected behavior:
     * - d2 hovering over d1 does nothing as d2 can only use right dock
     * - hovering d2 over top, left and bottom dock area will do nothing due to lacking permissions
     * - d1 hovering over d2 will create floating tabs as d1 has permission for DockWidgetArea::FloatingDockWidgetArea
     * - resizing and tab creation will add two gap items in the right dock (d2)
     */

    // unplug and resize both dock widgets
    unplugAndResize(mainWindow, d1, home1(mainWindow), size1(mainWindow));
    unplugAndResize(mainWindow, d2, home2(mainWindow), size2(mainWindow));

    // both dock widgets must be direct children of the main window
    {
        const QList<QDockWidget*> children = mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
        QTRY_VERIFY(children.size() == 2);
        for (const QDockWidget* child : children)
            QTRY_VERIFY(child == d1 || child == d2);
    }

    // The main window must not contain floating tabs
    QTRY_VERIFY(mainWindow->findChild<QDockWidgetGroupWindow*>() == nullptr);

    // Test unpermitted dock areas with d2
    qCDebug(lcTestDockWidget) << "*** move d2 to forbidden docks ***";

    // Move d2 to non allowed dock areas and verify it remains floating
    qCDebug(lcTestDockWidget) << "Move d2 to top dock";
    moveDockWidget(d2, dockPoint(mainWindow, Qt::TopDockWidgetArea), QPoint(), MoveDockWidgetRule::Drop);
    QTRY_VERIFY(d2->isFloating());

    qCDebug(lcTestDockWidget) << "Move d2 to left dock";
    //moveDockWidget(d2, d2->mapFrom(MainWindow, dockPoint(MainWindow, Qt::LeftDockWidgetArea)));
    moveDockWidget(d2, dockPoint(mainWindow, Qt::LeftDockWidgetArea), QPoint(), MoveDockWidgetRule::Drop);
    QTRY_VERIFY(d2->isFloating());

    qCDebug(lcTestDockWidget) << "Move d2 to bottom dock";
    moveDockWidget(d2, dockPoint(mainWindow, Qt::BottomDockWidgetArea), QPoint(), MoveDockWidgetRule::Drop);
    QTRY_VERIFY(d2->isFloating());

    qCDebug(lcTestDockWidget) << "Waiting" << waitBeforeClose << "ms before closing.";
    QTest::qWait(waitBeforeClose);
#else
    QSKIP("test requires -developer-build option");
#endif // QT_BUILD_INTERNAL
}

/*!
    \internal

    This test checks consistency of QMainWindow::saveState() / QMainWindow::restoreState().
    These methods (de)serialize dock widget properties via a QDataStream into a QByteArray.

    If the logic of (de)serializing Qt datatypes and classes changes, old settings can fail
    to restore properly without triggering warnings or assertions.

    The test consists of two parts:
    \list 1
    \li Read properties from a hard coded byte array and check if it is deserialized correctly.
    \li Serialize properties into a \a QByteArray and check if it is serialized correctly.
    \endlist
*/
void tst_QDockWidget::saveAndRestore()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Test skipped on Wayland.");
#ifdef Q_OS_WIN
    QSKIP("Test skipped on Windows platforms");
#endif // Q_OS_WIN
#ifndef QT_BUILD_INTERNAL
    QSKIP("test requires -developer-build option");
#else

    // Hard coded byte array for test initialization
    const QByteArray testArray = QByteArrayLiteral(
        "\x00\x00\x00\xFF\x00\x00\x00\x00\xFD\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x13\x00\x00\x05\xE8\xFC\x02\x00\x00\x00\x01\xFB\x00\x00\x00\x04\x00"
        "D\x00"
        "1\x03\x00\x00\x01\f\x00\x00\x00\x97\x00\x00\x02\x19\x00\x00\x01z\x00\x00\x00\x01\x00\x00\x00\x13\x00\x00\x05\xE8\xFC\x02\x00\x00\x00\x01\xFB\x00\x00\x00\x04\x00"
        "D\x00"
        "2\x03\x00\x00\x06L\x00\x00\x00\xFF\x00\x00\x01\f\x00\x00\x00\xE2\x00\x00\n\x80\x00\x00\x05\xE8\x00\x00\x00\x04\x00\x00\x00\x04\x00\x00\x00\b\x00\x00\x00\b\xFC\x00\x00\x00\x00"
    );

    QByteArray referenceArray;    // Copy of testArray, corrected for current screen limits
    QPoint topLeft1;      // Top left point of dock widget d1
    QPoint topLeft2;      // Top left point of dock widget d2
    QSize widgetSize1;    // Size of dock widget d1
    QSize widgetSize2;    // Size of dock widget d2
    bool isFloating1;     // Floating status of dock widget d1
    bool isFloating2;     // Floating status of dock widget d2

    // Create a mainwindow with a central widget and two dock widgets.
    // Import properties from hard coded byte array.
    // Use a scope to delete objects from screen after test.
    {
        QPointer<QDockWidget> d1;
        QPointer<QDockWidget> d2;
        QPointer<QWidget> cent;
        QMainWindow* mainWindow;
        createTestWidgets(mainWindow, cent, d1, d2);

        // Failure to restore properties might lead to inconsistencies and crash.
        // To leave a clean environment when the test inexpectedly goes out of scope,
        // => store main window pointer in a std::unique_ptr
        std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

        // Restore, wait for events to be processed
        mainWindow->restoreState(testArray);
        QVERIFY(QTest::qWaitForWindowExposed(d1));
        QVERIFY(QTest::qWaitForWindowExposed(d2));

        // Serialized dock widget positions and sizes might be overridden due
        // screen size limitations => do not check them here.
        // If the test fails between here and scope end, serialization format/sequence have changed
        QTRY_VERIFY(d1->isFloating());
        QTRY_VERIFY(d2->isFloating());

        // Hide main window and save their floating status.
        // Reason:
        // - KDE window managers do not take control over dock widgets.
        //   => They always close with the main window.
        // - Some non KDE window managers do take control over dock widgets.
        //   => They prevent them from closing with the main window (QTBUG-103474).
        // If properties are restored correctly, closing behavior must be consistent
        // throughout this test.
        mainWindow->hide();
        // FIXME: No method exists in 6.5 to wait for a window to be hidden.
        // => wait and hope the best, replace with qWaitForWindowHidden once implemented.
        QTest::qWait(200);
        isFloating1 = d1->isFloating();
        isFloating2 = d2->isFloating();
    }

    // Create a mainwindow with a central widget and two dock widgets.
    // Assign different properties to each dock widgets.
    // Write properties to a byte array.
    // Remember position and size properties for comparison.
    // Use a scope to delete objects from screen after test.
    {
        QPointer<QDockWidget> d1;
        QPointer<QDockWidget> d2;
        QPointer<QWidget> cent;
        QMainWindow* mainWindow;
        createTestWidgets(mainWindow, cent, d1, d2);
        std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

        // unplug, position and resize both dock widgets relative to screen size
        unplugAndResize(mainWindow, d1, home1(mainWindow), size1(mainWindow));
        unplugAndResize(mainWindow, d2, home2(mainWindow), size2(mainWindow));

        topLeft1 = d1->pos();
        topLeft2 = d2->pos();
        widgetSize1 = d1->size();
        widgetSize2 = d2->size();

        // save properties, potentially corrected for screen limits
        referenceArray = mainWindow->saveState();

        // Check closing behavior consistency
        mainWindow->hide();
        QTRY_VERIFY(d1->isFloating());
        QTRY_VERIFY(d2->isFloating());
        QCOMPARE(d1->isFloating(), isFloating1);
        QCOMPARE(d2->isFloating(), isFloating2);
    }

    // Create a new main window, central window and two dock widgets.
    QPointer<QDockWidget> d1;
    QPointer<QDockWidget> d2;
    QPointer<QWidget> cent;
    QMainWindow* mainWindow;
    createTestWidgets(mainWindow, cent, d1, d2);

    // Failure to restore properties might lead to inconsistencies and crash.
    // To leave a clean environment when the test inexpectedly goes out of scope,
    // - store main window pointer in a std::unique_ptr
    std::unique_ptr<QMainWindow> up_mainWindow(mainWindow);

    // Restore properties and wait for events to be processed
    mainWindow->restoreState(referenceArray);
    QVERIFY(QTest::qWaitForWindowExposed(d1));
    QVERIFY(QTest::qWaitForWindowExposed(d2));

    // Compare positions, sizes and floating status
    // If the test fails in the following 12 lines,
    // the de-serialization format/sequence have changed
    QTRY_COMPARE(topLeft1, d1->pos());
    QTRY_COMPARE(topLeft2, d2->pos());
    QCOMPARE(widgetSize1, d1->size());
    QCOMPARE(widgetSize2, d2->size());
    QVERIFY(d1->isFloating());
    QVERIFY(d2->isFloating());

    // Serialize again to compare all remaining properties
    const QByteArray comparisonArray = mainWindow->saveState();
    QCOMPARE(comparisonArray, referenceArray);

    // Check closing behavior consistency
    mainWindow->hide();
    QTRY_VERIFY(d1->isFloating());
    QTRY_VERIFY(d2->isFloating());
    QCOMPARE(d1->isFloating(), isFloating1);
    QCOMPARE(d2->isFloating(), isFloating2);

#undef qCreateFloatingTabs
#endif // QT_BUILD_INTERNAL
}

QTEST_MAIN(tst_QDockWidget)
#include "tst_qdockwidget.moc"
