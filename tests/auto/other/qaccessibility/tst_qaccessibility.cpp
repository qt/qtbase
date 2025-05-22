// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qglobal.h>
#ifdef Q_OS_WIN
# include <QtCore/qt_windows.h>
# include <oleacc.h>
# include <uiautomation.h>
# include <servprov.h>
# include <winuser.h>
#endif
#include <QTest>
#include <QSignalSpy>
#include <QtGui>
#include <QtWidgets>
#include <math.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformaccessibility.h>
#ifdef Q_OS_WIN
#include <QtCore/private/qfunctions_win_p.h>
#endif
#include <QtGui/private/qaccessiblebridgeutils_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <QtWidgets/private/qapplication_p.h>
#include <QtWidgets/private/qdialog_p.h>

#if defined(Q_OS_WIN) && defined(interface)
#   undef interface
#endif

#include "QtTest/qtestaccessible.h"

#include <algorithm>

#include "accessiblewidgets.h"

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;
using namespace Qt::StringLiterals;

static inline bool verifyChild(QWidget *child, QAccessibleInterface *interface,
                               int index, const QRect &domain)
{
    if (!child) {
        qWarning("tst_QAccessibility::verifyChild: null pointer to child.");
        return false;
    }

    if (!interface) {
        qWarning("tst_QAccessibility::verifyChild: null pointer to interface.");
        return false;
    }

    // Verify that we get a valid QAccessibleInterface for the child.
    QAccessibleInterface *childInterface(QAccessible::queryAccessibleInterface(child));
    if (!childInterface) {
        qWarning("tst_QAccessibility::verifyChild: Failed to retrieve interface for child.");
        return false;
    }

    // QAccessibleInterface::indexOfChild():
    // Verify that indexOfChild() returns an index equal to the index passed in
    int indexFromIndexOfChild = interface->indexOfChild(childInterface);
    if (indexFromIndexOfChild != index) {
        qWarning("tst_QAccessibility::verifyChild (indexOfChild()):");
        qWarning() << "Expected:" << index;
        qWarning() << "Actual:  " << indexFromIndexOfChild;
        return false;
    }

    // Navigate to child, compare its object and role with the interface from queryAccessibleInterface(child).
    QAccessibleInterface *navigatedChildInterface(interface->child(index));
    if (!navigatedChildInterface)
        return false;

    const QRect rectFromInterface = navigatedChildInterface->rect();

    // QAccessibleInterface::childAt():
    // Calculate global child position and check that the interface
    // returns the correct index for that position.
    QPoint globalChildPos = child->mapToGlobal(QPoint(0, 0));
    QAccessibleInterface *childAtInterface(interface->childAt(globalChildPos.x(), globalChildPos.y()));
    if (!childAtInterface) {
        qWarning("tst_QAccessibility::verifyChild (childAt()):");
        qWarning() << "Expected:" << childInterface;
        qWarning() << "Actual:  no child";
        return false;
    }
    if (childAtInterface->object() != childInterface->object()) {
        qWarning("tst_QAccessibility::verifyChild (childAt()):");
        qWarning() << "Expected:" << childInterface;
        qWarning() << "Actual:  " << childAtInterface;
        return false;
    }

    // QAccessibleInterface::rect():
    // Calculate global child geometry and check that the interface
    // returns a QRect which is equal to the calculated QRect.
    const QRect expectedGlobalRect = QRect(globalChildPos, child->size());
    if (expectedGlobalRect != rectFromInterface) {
        qWarning("tst_QAccessibility::verifyChild (rect()):");
        qWarning() << "Expected:" << expectedGlobalRect;
        qWarning() << "Actual:  " << rectFromInterface;
        return false;
    }

    // Verify that the child is within its domain.
    if (!domain.contains(rectFromInterface)) {
        qWarning("tst_QAccessibility::verifyChild: Child is not within its domain.");
        return false;
    }

    return true;
}

#define EXPECT(cond) \
    do { \
        if (!errorAt && !(cond)) { \
            errorAt = __LINE__; \
            qWarning("level: %d, role: %d (%s)", treelevel, iface->role(), #cond); \
            break; \
        } \
    } while (0)

static int verifyHierarchy(QAccessibleInterface *iface)
{
    int errorAt = 0;
    static int treelevel = 0;   // for error diagnostics
    QAccessibleInterface *if2 = 0;
    ++treelevel;
    for (int i = 0; i < iface->childCount() && !errorAt; ++i) {
        if2 = iface->child(i);
        EXPECT(if2 != 0);
        EXPECT(iface->indexOfChild(if2) == i);
        // navigate Ancestor
        QAccessibleInterface *parent = if2->parent();
        EXPECT(iface->object() == parent->object());
        EXPECT(iface == parent);

        // verify children
        if (!errorAt)
            errorAt = verifyHierarchy(if2);
    }

    --treelevel;
    return errorAt;
}

QRect childRect(QAccessibleInterface *iface, int index = 0)
{
    return iface->child(index)->rect();
}

class tst_QAccessibility : public QObject
{
    Q_OBJECT
public:
    tst_QAccessibility();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void eventTest();
    void eventWithChildTest();
    void customWidget();
    void deletedWidget();
    void subclassedWidget();

    void statesStructTest();
    void navigateHierarchy();
    void sliderTest();
    void textAttributes_data();
    void textAttributes();
    void hideShowTest();

    void actionTest();

    void applicationTest();
    void mainWindowTest();
    void subWindowTest();
#if QT_CONFIG(shortcut)
    void buttonTest();
#endif
    void scrollBarTest();
    void tabTest();
    void tabWidgetTest();
#if QT_CONFIG(shortcut)
    void menuTest();
#endif
    void spinBoxTest();
    void doubleSpinBoxTest();
    void textEditTest();
    void textBrowserTest();
    void mdiAreaTest();
    void mdiSubWindowTest();
    void lineEditTest();
    void lineEditTextFunctions_data();
    void lineEditTextFunctions();
    void textInterfaceTest_data();
    void textInterfaceTest();
    void groupBoxTest();
    void dialogButtonBoxTest();
    void dialTest();
    void rubberBandTest();
    void abstractScrollAreaTest();
    void scrollAreaTest();

    void listTest();
    void treeTest();
    void tableTest();
    void rootIndexView();

    void uniqueIdTest();
    void calendarWidgetTest();
    void dockWidgetTest();
    void comboBoxTest();
    void accessibleName();
    void accessibleIdentifier();
#if QT_CONFIG(shortcut)
    void labelTest();
    void relationTest();
    void accelerators();
#endif
    void bridgeTest();
    void focusChild();

    void messageBoxTest_data();
    void messageBoxTest();

    void widgetLocaleTest();

protected slots:
    void onClicked();
private:
    int click_count;
};

QAccessible::State state(QWidget * const widget)
{
    QAccessibleInterface *iface(QAccessible::queryAccessibleInterface(widget));
    if (!iface) {
        qWarning() << "Cannot get QAccessibleInterface for widget";
        return QAccessible::State();
    }
    return iface->state();
}

tst_QAccessibility::tst_QAccessibility()
{
    click_count = 0;
}

void tst_QAccessibility::onClicked()
{
    click_count++;
}

static bool initAccessibility()
{
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    if (pfIntegration->accessibility()) {
        pfIntegration->accessibility()->setActive(true);
        return true;
    }
    return false;
}

void tst_QAccessibility::initTestCase()
{
    QTestAccessibility::initialize();
    if (!initAccessibility())
        QSKIP("This platform does not support accessibility");
}

void tst_QAccessibility::cleanupTestCase()
{
    QTestAccessibility::cleanup();
}

void tst_QAccessibility::init()
{
    QTestAccessibility::clearEvents();
#ifdef Q_OS_ANDROID
    // On Android a11y state is not explicitly set by calling
    // QPlatformAccessibility::setActive(), there is another flag that is
    // controlled from the Java side. The state of this flag is queried
    // during event processing, so a11y state can be reset to false while
    // we do QTest::qWait().
    // To overcome the issue in unit-tests, re-enable a11y before each test.
    // A more precise fix will require re-enabling it after every qWait() or
    // processEvents() call, but the current tests pass with such condition.
    initAccessibility();
#endif
}

void tst_QAccessibility::cleanup()
{
    const EventList list = QTestAccessibility::events();
    if (!list.isEmpty()) {
        qWarning("%zd accessibility event(s) were not handled in testfunction '%s':", size_t(list.size()),
                 QString(QTest::currentTestFunction()).toLatin1().constData());
        for (int i = 0; i < list.size(); ++i)
            qWarning(" %d: Object: %p Event: '%s' Child: %d", i + 1, list.at(i)->object(),
                     qAccessibleEventString(list.at(i)->type()), list.at(i)->child());
    }
    QTestAccessibility::clearEvents();
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QAccessibility::eventTest()
{
    auto buttonHolder = std::make_unique<QPushButton>(nullptr);
    auto button = buttonHolder.get();
    QAccessible::queryAccessibleInterface(button);
    button->setObjectName(QString("Olaf"));
    setFrameless(button);

    button->show();
    QAccessibleEvent showEvent(button, QAccessible::ObjectShow);
    // some platforms might send other events first, (such as state change event active=1)
    QVERIFY(QTestAccessibility::containsEvent(&showEvent));
    button->setFocus(Qt::MouseFocusReason);
    QTestAccessibility::clearEvents();
    QTest::mouseClick(button, Qt::LeftButton, { });

    button->setAccessibleName("Olaf the second");
    QAccessibleEvent nameEvent(button, QAccessible::NameChanged);
    QVERIFY_EVENT(&nameEvent);
    button->setAccessibleDescription("This is a button labeled Olaf");
    QAccessibleEvent descEvent(button, QAccessible::DescriptionChanged);
    QVERIFY_EVENT(&descEvent);

    button->hide();
    QAccessibleEvent hideEvent(button, QAccessible::ObjectHide);
    // some platforms might send other events first, (such as state change event active=1)
    QVERIFY(QTestAccessibility::containsEvent(&hideEvent));

    buttonHolder.reset();

    // Make sure that invalid events don't bring down the system
    // these events can be in user code.
    auto widgetHolder = std::make_unique<QWidget>();
    auto widget = widgetHolder.get();
    QAccessibleEvent ev1(widget, QAccessible::Focus);
    QAccessible::updateAccessibility(&ev1);

    QAccessibleEvent ev2(widget, QAccessible::Focus);
    ev2.setChild(7);
    QAccessible::updateAccessibility(&ev2);
    widgetHolder.reset();

    auto objectHolder = std::make_unique<QObject>();
    auto object = objectHolder.get();
    QAccessibleEvent ev3(object, QAccessible::Focus);
    QAccessible::updateAccessibility(&ev3);
    objectHolder.reset();

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::eventWithChildTest()
{
    // make sure that QAccessibleEvent created using either of the two QAccessibleEvent
    // behaves the same when the same underlying QObject is used
    QWidget widget;
    QWidget childWidget(&widget);

    // QAccessibleEvent constructor called with the QObject*
    QAccessibleEvent event1(&widget, QAccessible::Focus);

    // QAccessibleEvent constructor called with the QAccessibleInterface* for the same QObject*
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&widget);
    QAccessibleEvent event2(iface, QAccessible::Focus);

    QVERIFY(event1.accessibleInterface() != nullptr);
    QVERIFY(event2.accessibleInterface() != nullptr);
    QCOMPARE(event1.accessibleInterface(), event2.accessibleInterface());

    // set same child for both
    event1.setChild(0);
    event2.setChild(0);

    QVERIFY(event1.accessibleInterface() != nullptr);
    QVERIFY(event2.accessibleInterface() != nullptr);
    QCOMPARE(event1.accessibleInterface(), event2.accessibleInterface());
}

void tst_QAccessibility::customWidget()
{
    {
    QtTestAccessibleWidget widget(nullptr, "Heinz");
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    // By default we create QAccessibleWidget
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)&widget);
    QCOMPARE(iface->object()->objectName(), QString("Heinz"));
    QCOMPARE(iface->rect().height(), widget.height());
    QCOMPARE(iface->text(QAccessible::Help), QString());
    QCOMPARE(iface->rect().width(), widget.width());
    }
    {
    QAccessible::installFactory(QtTestAccessibleWidgetIface::ifaceFactory);
    QtTestAccessibleWidget widget(nullptr, "Heinz");
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)&widget);
    QCOMPARE(iface->object()->objectName(), QString("Heinz"));
    QCOMPARE(iface->rect().height(), widget.height());
    // The help text is only set if our factory works
    QCOMPARE(iface->text(QAccessible::Help), QString("Help yourself"));
    }
    {
    // A subclass of any class should still get the right QAccessibleInterface
    QtTestAccessibleWidgetSubclass subclassedWidget(nullptr, "Hans");
    QAccessibleInterface *subIface = QAccessible::queryAccessibleInterface(&subclassedWidget);
    QVERIFY(subIface != 0);
    QVERIFY(subIface->isValid());
    QCOMPARE(subIface->object(), (QObject*)&subclassedWidget);
    QCOMPARE(subIface->text(QAccessible::Help), QString("Help yourself"));
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::deletedWidget()
{
    auto widgetHolder = std::make_unique<QtTestAccessibleWidget>(nullptr, "Ralf");
    auto widget = widgetHolder.get();
    QAccessible::installFactory(QtTestAccessibleWidgetIface::ifaceFactory);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)widget);

    widgetHolder.reset();
    // fixme: QVERIFY(!iface->isValid());
}

void tst_QAccessibility::subclassedWidget()
{
    KFooButton button("Ploink", 0);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&button);
    QVERIFY(iface);
    QCOMPARE(iface->object(), (QObject*)&button);
    QCOMPARE(iface->text(QAccessible::Name), button.text());
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::statesStructTest()
{
    QAccessible::State s1;
    QVERIFY(s1.disabled == 0);
    QVERIFY(s1.focusable == 0);
    QVERIFY(s1.modal == 0);

    QAccessible::State s2;
    QCOMPARE(s2, s1);
    s2.busy = true;
    QVERIFY(!(s2 == s1));
    s1.busy = true;
    QCOMPARE(s2, s1);
    s1 = QAccessible::State();
    QVERIFY(!(s2 == s1));
    s1 = s2;
    QCOMPARE(s2, s1);
    QVERIFY(s1.busy == 1);
}

void tst_QAccessibility::sliderTest()
{
    {
    auto sliderHolder = std::make_unique<QSlider>(nullptr);
    auto slider = sliderHolder.get();
    setFrameless(slider);
    slider->setObjectName(QString("Slidy"));
    slider->show();
    QAccessibleInterface *iface(QAccessible::queryAccessibleInterface(slider));
    QVERIFY(iface);
    QVERIFY(iface->isValid());

    QCOMPARE(iface->childCount(), 0);
    QCOMPARE(iface->role(), QAccessible::Slider);

    QAccessibleValueInterface *valueIface = iface->valueInterface();
    QVERIFY(valueIface != 0);
    QCOMPARE(valueIface->minimumValue().toInt(), slider->minimum());
    QCOMPARE(valueIface->maximumValue().toInt(), slider->maximum());
    QCOMPARE(valueIface->minimumStepSize().toInt(), slider->singleStep());
    slider->setValue(50);
    QCOMPARE(valueIface->currentValue().toInt(), slider->value());
    slider->setValue(0);
    QCOMPARE(valueIface->currentValue().toInt(), slider->value());
    slider->setValue(100);
    QCOMPARE(valueIface->currentValue().toInt(), slider->value());
    valueIface->setCurrentValue(77);
    QCOMPARE(77, slider->value());
    slider->setSingleStep(2);
    QCOMPARE(valueIface->minimumStepSize().toInt(), 2);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::navigateHierarchy()
{
    {
    auto widgetHolder = std::make_unique<QWidget>(nullptr);
    auto w = widgetHolder.get();
    w->setObjectName(QString("Hans"));
    w->show();
    QWidget *w1 = new QWidget(w);
    w1->setObjectName(QString("1"));
    w1->show();
    QWidget *w2 = new QWidget(w);
    w2->setObjectName(QString("2"));
    w2->show();
    QWidget *w3 = new QWidget(w);
    w3->setObjectName(QString("3"));
    w3->show();
    QWidget *w31 = new QWidget(w3);
    w31->setObjectName(QString("31"));
    w31->show();

    QAccessibleInterface *ifaceW(QAccessible::queryAccessibleInterface(w));
    QVERIFY(ifaceW != 0);
    QVERIFY(ifaceW->isValid());

    QAccessibleInterface *target = ifaceW->child(14);
    QVERIFY(!target);
    target = ifaceW->child(-1);
    QVERIFY(!target);
    target = ifaceW->child(0);
    QAccessibleInterface *interfaceW1(ifaceW->child(0));
    QVERIFY(target);
    QVERIFY(target->isValid());
    QCOMPARE(target->object(), (QObject*)w1);
    QVERIFY(interfaceW1 != 0);
    QVERIFY(interfaceW1->isValid());
    QCOMPARE(interfaceW1->object(), (QObject*)w1);

    target = ifaceW->child(2);
    QVERIFY(target != 0);
    QVERIFY(target->isValid());
    QCOMPARE(target->object(), (QObject*)w3);

    QAccessibleInterface *child = target->child(1);
    QVERIFY(!child);
    child = target->child(0);
    QVERIFY(child != 0);
    QVERIFY(child->isValid());
    QCOMPARE(child->object(), (QObject*)w31);

    ifaceW = QAccessible::queryAccessibleInterface(w);
    QAccessibleInterface *acc3(ifaceW->child(2));
    target = acc3->child(0);
    QCOMPARE(target->object(), (QObject*)w31);

    QAccessibleInterface *parent = target->parent();
    QVERIFY(parent != 0);
    QVERIFY(parent->isValid());
    QCOMPARE(parent->object(), (QObject*)w3);
    }
    QTestAccessibility::clearEvents();
}

#define QSETCOMPARE(thetypename, elements, otherelements) \
    QCOMPARE((QSet<thetypename>() << elements), (QSet<thetypename>() << otherelements))

static QWidget *createWidgets()
{
    QWidget *w = new QWidget();

    QHBoxLayout *box = new QHBoxLayout(w);

    int i = 0;
    box->addWidget(new QComboBox(w));
    box->addWidget(new QPushButton(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QHeaderView(Qt::Vertical, w));
    box->addWidget(new QTreeView(w));
    box->addWidget(new QTreeWidget(w));
    box->addWidget(new QListView(w));
    box->addWidget(new QListWidget(w));
    box->addWidget(new QTableView(w));
    box->addWidget(new QTableWidget(w));
    box->addWidget(new QCalendarWidget(w));
    box->addWidget(new QDialogButtonBox(w));
    box->addWidget(new QGroupBox(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QFrame(w));
    box->addWidget(new QLineEdit(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QProgressBar(w));
    box->addWidget(new QTabWidget(w));
    box->addWidget(new QCheckBox(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QRadioButton(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QDial(w));
    box->addWidget(new QScrollBar(w));
    box->addWidget(new QSlider(w));
    box->addWidget(new QDateTimeEdit(w));
    box->addWidget(new QDoubleSpinBox(w));
    box->addWidget(new QSpinBox(w));
    box->addWidget(new QLabel(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QLCDNumber(w));
    box->addWidget(new QStackedWidget(w));
    box->addWidget(new QToolBox(w));
    box->addWidget(new QLabel(QLatin1String("widget text ") + QString::number(i++), w));
    box->addWidget(new QTextEdit(QLatin1String("widget text ") + QString::number(i++), w));

    /* Not in the list
     * QAbstractItemView, QGraphicsView, QScrollArea,
     * QToolButton, QDockWidget, QFocusFrame, QMainWindow, QMenu, QMenuBar, QSizeGrip, QSplashScreen, QSplitterHandle,
     * QStatusBar, QSvgWidget, QTabBar, QToolBar, QSplitter
     */
    return w;
}

void tst_QAccessibility::accessibleName()
{
    auto holder = std::unique_ptr<QWidget>(createWidgets());
    auto toplevel = holder.get();
    toplevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(toplevel));

    QLayout *lout = toplevel->layout();
    for (int i = 0; i < lout->count(); i++) {
        QLayoutItem *item = lout->itemAt(i);
        QWidget *child = item->widget();

        QString name = tr("Widget Name %1").arg(i);
        child->setAccessibleName(name);
        QAccessibleInterface *acc = QAccessible::queryAccessibleInterface(child);
        QVERIFY(acc);
        QCOMPARE(acc->text(QAccessible::Name), name);

        QString desc = tr("Widget Description %1").arg(i);
        child->setAccessibleDescription(desc);
        QCOMPARE(acc->text(QAccessible::Description), desc);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::accessibleIdentifier()
{
    const QString objectName("button_objectname");
    const QString id("mybutton");

    QMainWindow mainWindow;
    QPushButton button("a button", &mainWindow);
    button.setObjectName(objectName);
    mainWindow.show();

    // verify that default implementation for platform bridges generates
    // an accessible ID that's based on (i.e. somehow contains) the object name
    QAccessibleInterface* accessible = QAccessible::queryAccessibleInterface(&button);
    QVERIFY(QAccessibleBridgeUtils::accessibleId(accessible).contains(objectName));

    // explicitly set an accessible ID, verify event and that the ID is set to
    // that exact string (not only containing it) afterwards
    QTestAccessibility::clearEvents();
    button.setAccessibleIdentifier(id);
    QAccessibleEvent event(&button, QAccessible::IdentifierChanged);
    QVERIFY(QTestAccessibility::containsEvent(&event));
    QCOMPARE(button.accessibleIdentifier(), id);
    QCOMPARE(QAccessibleBridgeUtils::accessibleId(accessible), id);
    QTestAccessibility::clearEvents();

    // verify that no event gets triggered when setting the same ID again
    button.setAccessibleIdentifier(id);
    QVERIFY(QTestAccessibility::events().empty());
    QCOMPARE(button.accessibleIdentifier(), id);
    QCOMPARE(QAccessibleBridgeUtils::accessibleId(accessible), id);

    QTestAccessibility::clearEvents();
}

// note: color should probably always be part of the attributes
void tst_QAccessibility::textAttributes_data()
{
    QTest::addColumn<QFont>("defaultFont");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("startOffsetResult");
    QTest::addColumn<int>("endOffsetResult");
    QTest::addColumn<QStringList>("attributeResult");

    static QFont defaultFont;
    defaultFont.setFamily("");
    defaultFont.setPointSize(13);

    static QFont defaultComplexFont = defaultFont;
    defaultComplexFont.setFamily("Arial");
    defaultComplexFont.setPointSize(20);
    defaultComplexFont.setWeight(QFont::Bold);
    defaultComplexFont.setStyle(QFont::StyleItalic);
    defaultComplexFont.setUnderline(true);

    static QStringList defaults = QString("font-style:normal;font-weight:normal;text-align:left;text-position:baseline;font-size:13pt;text-line-through-type:none").split(';');
    static QStringList bold = defaults;
    bold[1] = QString::fromLatin1("font-weight:bold");

    static QStringList italic = defaults;
    italic[0] = QString::fromLatin1("font-style:italic");

    static QStringList boldItalic = defaults;
    boldItalic[0] = QString::fromLatin1("font-style:italic");
    boldItalic[1] = QString::fromLatin1("font-weight:bold");

    static QStringList monospace = defaults;
    monospace.append(QLatin1String("font-family:\"monospace\""));

    static QStringList font8pt = defaults;
    font8pt[4] = (QLatin1String("font-size:8pt"));

    static QStringList color = defaults;
    color << QLatin1String("color:rgb(240,241,242)") << QLatin1String("background-color:rgb(20,240,30)");

    static QStringList rightAlign = defaults;
    rightAlign[2] = QStringLiteral("text-align:right");

    static QStringList defaultFontDifferent = defaults;
    defaultFontDifferent[0] = QString::fromLatin1("font-style:italic");
    defaultFontDifferent[1] = QString::fromLatin1("font-weight:bold");
    defaultFontDifferent[4] = QString::fromLatin1("font-size:20pt");
    defaultFontDifferent.append("text-underline-style:solid");
    defaultFontDifferent.append("text-underline-type:single");
    defaultFontDifferent.append("font-family:\"Arial\"");

    static QStringList defaultFontDifferentBoldItalic = defaultFontDifferent;
    defaultFontDifferentBoldItalic[0] = QString::fromLatin1("font-style:italic");
    defaultFontDifferentBoldItalic[1] = QString::fromLatin1("font-weight:bold");

    static QStringList defaultFontDifferentMonospace = defaultFontDifferent;
    defaultFontDifferentMonospace[8] = (QLatin1String("font-family:\"monospace\""));

    static QStringList defaultFontDifferentFont8pt = defaultFontDifferent;
    defaultFontDifferentFont8pt[4] = (QLatin1String("font-size:8pt"));

    static QStringList defaultFontDifferentColor = defaultFontDifferent;
    defaultFontDifferentColor << QLatin1String("color:rgb(240,241,242)") << QLatin1String("background-color:rgb(20,240,30)");

    QTest::newRow("defaults 1") << defaultFont << "hello" << 0 << 0 << 5 << defaults;
    QTest::newRow("defaults 2") << defaultFont << "hello" << 1 << 0 << 5 << defaults;
    QTest::newRow("defaults 3") << defaultFont << "hello" << 4 << 0 << 5 << defaults;
    QTest::newRow("defaults 4") << defaultFont << "hello" << 5 << 0 << 5 << defaults;
    QTest::newRow("offset -1 length") << defaultFont << "hello" << -1 << 0 << 5 << defaults;
    QTest::newRow("offset -2 cursor pos") << defaultFont << "hello" << -2 << 0 << 5 << defaults;
    QTest::newRow("offset -3") << defaultFont << "hello" << -3 << -1 << -1 << QStringList();
    QTest::newRow("invalid offset 2") << defaultFont << "hello" << 6 << -1 << -1 << QStringList();
    QTest::newRow("invalid offset 3") << defaultFont << "" << 1 << -1 << -1 << QStringList();

    QString boldText = QLatin1String("<html><b>bold</b>text");
    QTest::newRow("bold 0") << defaultFont << boldText << 0 << 0 << 4 << bold;
    QTest::newRow("bold 2") << defaultFont << boldText << 2 << 0 << 4 << bold;
    QTest::newRow("bold 3") << defaultFont << boldText << 3 << 0 << 4 << bold;
    QTest::newRow("bold 4") << defaultFont << boldText << 4 << 4 << 8 << defaults;
    QTest::newRow("bold 6") << defaultFont << boldText << 6 << 4 << 8 << defaults;

    QString longText = QLatin1String("<html>"
                 "Hello, <b>this</b> is an <i><b>example</b> text</i>."
                 "<span style=\"font-family: monospace\">Multiple fonts are used.</span>"
                 "Multiple <span style=\"font-size: 8pt\">text sizes</span> are used."
                 "Let's give some color to <span style=\"color:#f0f1f2; background-color:#14f01e\">Qt</span>.");

    QTest::newRow("default 5") << defaultFont << longText << 6 << 0 << 7 << defaults;
    QTest::newRow("default 6") << defaultFont << longText << 7 << 7 << 11 << bold;
    QTest::newRow("bold 7") << defaultFont << longText << 10 << 7 << 11 << bold;
    QTest::newRow("bold 8") << defaultFont << longText << 10 << 7 << 11 << bold;
    QTest::newRow("bold italic") << defaultFont << longText << 18 << 18 << 25 << boldItalic;
    QTest::newRow("monospace") << defaultFont << longText << 34 << 31 << 55 << monospace;
    QTest::newRow("8pt") << defaultFont << longText << 65 << 64 << 74 << font8pt;
    QTest::newRow("color") << defaultFont << longText << 110 << 109 << 111 << color;

    // make sure unset font properties default to those of document's default font
    QTest::newRow("defaultFont default 5") << defaultComplexFont << longText << 6 << 0 << 7 << defaultFontDifferent;
    QTest::newRow("defaultFont default 6") << defaultComplexFont << longText << 7 << 7 << 11 << defaultFontDifferent;
    QTest::newRow("defaultFont bold 7") << defaultComplexFont << longText << 10 << 7 << 11 << defaultFontDifferent;
    QTest::newRow("defaultFont bold 8") << defaultComplexFont << longText << 10 << 7 << 11 << defaultFontDifferent;
    QTest::newRow("defaultFont bold italic") << defaultComplexFont << longText << 18 << 18 << 25 << defaultFontDifferentBoldItalic;
    QTest::newRow("defaultFont monospace") << defaultComplexFont << longText << 34 << 31 << 55 << defaultFontDifferentMonospace;
    QTest::newRow("defaultFont 8pt") << defaultComplexFont << longText << 65 << 64 << 74 << defaultFontDifferentFont8pt;
    QTest::newRow("defaultFont color") << defaultComplexFont << longText << 110 << 109 << 111 << defaultFontDifferentColor;

    QString rightAligned = QLatin1String("<html><p align=\"right\">right</p>");
    QTest::newRow("right aligned 1") << defaultFont << rightAligned << 0 << 0 << 5 << rightAlign;
    QTest::newRow("right aligned 2") << defaultFont << rightAligned << 1 << 0 << 5 << rightAlign;
    QTest::newRow("right aligned 3") << defaultFont << rightAligned << 5 << 0 << 5 << rightAlign;

    // left \n right \n left, make sure bold and alignment borders coincide
    QString leftRightLeftAligned = QLatin1String("<html><p><b>left</b></p><p align=\"right\">right</p><p><b>left</b></p>");
    QTest::newRow("left right left aligned 1") << defaultFont << leftRightLeftAligned << 1 << 0 << 4 << bold;
    QTest::newRow("left right left aligned 3") << defaultFont << leftRightLeftAligned << 3 << 0 << 4 << bold;
    QTest::newRow("left right left aligned 4") << defaultFont << leftRightLeftAligned << 4 << 4 << 5 << defaults;
    QTest::newRow("left right left aligned 5") << defaultFont << leftRightLeftAligned << 5 << 5 << 10 << rightAlign;
    QTest::newRow("left right left aligned 8") << defaultFont << leftRightLeftAligned << 8 << 5 << 10 << rightAlign;
    QTest::newRow("left right left aligned 9") << defaultFont << leftRightLeftAligned << 9 << 5 << 10 << rightAlign;
    QTest::newRow("left right left aligned 10") << defaultFont << leftRightLeftAligned << 10 << 10 << 11 << rightAlign;
    QTest::newRow("left right left aligned 11") << defaultFont << leftRightLeftAligned << 11 << 11 << 15 << bold;
    QTest::newRow("left right left aligned 15") << defaultFont << leftRightLeftAligned << 15 << 11 << 15 << bold;
    QTest::newRow("empty with no fragments") << defaultFont << QString::fromLatin1("\n\n\n\n") << 0 << 0 << 1 << defaults;
}

void tst_QAccessibility::textAttributes()
{
    {
    QFETCH(QFont, defaultFont);
    QFETCH(QString, text);
    QFETCH(int, offset);
    QFETCH(int, startOffsetResult);
    QFETCH(int, endOffsetResult);
    QFETCH(QStringList, attributeResult);

    QTextEdit textEdit;
    textEdit.document()->setDefaultFont(defaultFont);
    textEdit.setText(text);
    if (textEdit.document()->characterCount() > 1)
        textEdit.textCursor().setPosition(1);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&textEdit);
    QAccessibleTextInterface *textInterface=interface->textInterface();
    QVERIFY(textInterface);
    QCOMPARE(textInterface->characterCount(), textEdit.toPlainText().size());

    int startOffset = -1;
    int endOffset = -1;
    QString attributes = textInterface->attributes(offset, &startOffset, &endOffset);

    QCOMPARE(startOffset, startOffsetResult);
    QCOMPARE(endOffset, endOffsetResult);
    QStringList attrList = attributes.split(QChar(';'), Qt::SkipEmptyParts);
    attributeResult.sort();
    attrList.sort();
    QCOMPARE(attrList, attributeResult);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::hideShowTest()
{
    auto windowHolder = std::make_unique<QWidget>();
    QWidget * const window = windowHolder.get();
    window->resize(200, 200);
    QWidget * const child = new QWidget(window);

    QVERIFY(state(window).invisible);
    QVERIFY(state(child).invisible);

    QTestAccessibility::clearEvents();

    // show() and veryfy that both window and child are not invisible and get ObjectShow events.
    window->show();
    QVERIFY(!state(window).invisible);
    QVERIFY(!state(child).invisible);

    QAccessibleEvent show(window, QAccessible::ObjectShow);
    QVERIFY(QTestAccessibility::containsEvent(&show));
    QAccessibleEvent showChild(child, QAccessible::ObjectShow);
    QVERIFY(QTestAccessibility::containsEvent(&showChild));
    QTestAccessibility::clearEvents();

    // hide() and veryfy that both window and child are invisible and get ObjectHide events.
    window->hide();
    QVERIFY(state(window).invisible);
    QVERIFY(state(child).invisible);
    QAccessibleEvent hide(window, QAccessible::ObjectHide);
    QVERIFY(QTestAccessibility::containsEvent(&hide));
    QAccessibleEvent hideChild(child, QAccessible::ObjectHide);
    QVERIFY(QTestAccessibility::containsEvent(&hideChild));
    QTestAccessibility::clearEvents();

    windowHolder.reset();
    QTestAccessibility::clearEvents();
}


void tst_QAccessibility::actionTest()
{
    {
    QCOMPARE(QAccessibleActionInterface::pressAction(), QString(QStringLiteral("Press")));

    auto widgetHolder = std::make_unique<QWidget>();
    auto widget = widgetHolder.get();
    widget->setFocusPolicy(Qt::NoFocus);
    widget->show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(interface);
    QVERIFY(interface->isValid());
    QAccessibleActionInterface *actions = interface->actionInterface();
    QVERIFY(actions);

    // no actions by default, except when focusable
    QCOMPARE(actions->actionNames(), QStringList());
    widget->setFocusPolicy(Qt::StrongFocus);
    QCOMPARE(actions->actionNames(), QStringList(QAccessibleActionInterface::setFocusAction()));
    }
    QTestAccessibility::clearEvents();

    {
    auto buttonHolder = std::make_unique<QPushButton>();
    auto button = buttonHolder.get();
    setFrameless(button);
    button->show();
    QVERIFY(QTest::qWaitForWindowExposed(button));
    button->clearFocus();
    QCOMPARE(button->hasFocus(), false);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(button);
    QAccessibleActionInterface *actions = interface->actionInterface();
    QVERIFY(actions);

    // Make sure the "primary action" press comes first!
    QCOMPARE(actions->actionNames(), QStringList() << QAccessibleActionInterface::pressAction() << QAccessibleActionInterface::setFocusAction());

    actions->doAction(QAccessibleActionInterface::setFocusAction());
    QTest::qWait(500);
    QCOMPARE(button->hasFocus(), true);

    connect(button, SIGNAL(clicked()), this, SLOT(onClicked()));
    QCOMPARE(click_count, 0);
    actions->doAction(QAccessibleActionInterface::pressAction());
    QTest::qWait(500);
    QCOMPARE(click_count, 1);
    }
    QTestAccessibility::clearEvents();

    {
    QCOMPARE(QAccessibleActionInterface::showMenuAction(), QString(QStringLiteral("ShowMenu")));

    auto widgetHolder = std::make_unique<QWidget>();
    auto widget = widgetHolder.get();
    widget->addAction(new QAction("Foo"));
    widget->addAction(new QAction("Bar"));
    widget->show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(interface);
    QVERIFY(interface->isValid());
    QAccessibleActionInterface *actions = interface->actionInterface();
    QVERIFY(actions);

    QCOMPARE(actions->actionNames(), QStringList());
    widget->setContextMenuPolicy(Qt::ActionsContextMenu);
    QCOMPARE(actions->actionNames(), QStringList(QAccessibleActionInterface::showMenuAction()));
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::applicationTest()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Platform does not support window activation");

    {
    QLatin1String name = QLatin1String("My Name");
    qApp->setApplicationName(name);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(qApp);
    QCOMPARE(interface->text(QAccessible::Name), name);
    QCOMPARE(interface->text(QAccessible::Description), qApp->applicationFilePath());
    QCOMPARE(interface->text(QAccessible::Value), QString());
    QCOMPARE(interface->role(), QAccessible::Application);
    QCOMPARE(interface->window(), static_cast<QWindow*>(0));
    QCOMPARE(interface->parent(), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->focusChild(), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->indexOfChild(0), -1);
    QCOMPARE(interface->child(0), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->child(-1), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->child(1), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->childCount(), 0);

    // Check that asking for the application interface twice returns the same object
    QAccessibleInterface *app2 = QAccessible::queryAccessibleInterface(qApp);
    QCOMPARE(interface, app2);

    QWidget widget;
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    QAccessibleInterface *widgetIface = QAccessible::queryAccessibleInterface(&widget);
    QCOMPARE(interface->childCount(), 1);
    QAccessibleInterface *focus = interface->focusChild();
    QCOMPARE(focus->object(), &widget);
    QCOMPARE(interface->indexOfChild(0), -1);
    QCOMPARE(interface->indexOfChild(widgetIface), 0);
    QAccessibleInterface *child = interface->child(0);
    QCOMPARE(child->object(), &widget);
    QCOMPARE(interface->child(-1), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->child(1), static_cast<QAccessibleInterface*>(0));
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::mainWindowTest()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Platform does not support window activation");

    {
    auto mwHolder = std::make_unique<QMainWindow>();
    auto mw = mwHolder.get();
    mw->resize(300, 200);
    mw->show(); // triggers layout
    QApplicationPrivate::setActiveWindow(mw);

    QLatin1String name = QLatin1String("I am the main window");
    mw->setWindowTitle(name);
    QVERIFY(QTest::qWaitForWindowActive(mw));

    // The order of events is not really that important.
    QAccessibleEvent show(mw, QAccessible::ObjectShow);
    QVERIFY(QTestAccessibility::containsEvent(&show));
    QAccessible::State activeState;
    activeState.active = true;
    QAccessibleStateChangeEvent active(mw, activeState);
    QVERIFY(QTestAccessibility::containsEvent(&active));

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(mw);
    QCOMPARE(iface->text(QAccessible::Name), name);
    QCOMPARE(iface->role(), QAccessible::Window);
    QVERIFY(iface->state().active);

    QTestAccessibility::clearEvents();
    QLatin1String newName = QLatin1String("Main window with updated title");
    mw->setWindowTitle(newName);
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String(newName));
    QAccessibleEvent event(mw, QAccessible::NameChanged);
    QVERIFY(QTestAccessibility::containsEvent(&event));
    }
    QTestAccessibility::clearEvents();

    {
    QWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    // We currently don't have an accessible interface for QWindow
    // the active state is either in the QMainWindow or QQuickView
    QAccessibleInterface *windowIface(QAccessible::queryAccessibleInterface(&window));
    QVERIFY(!windowIface);

    QAccessible::State activeState;
    activeState.active = true;

    // We should still not crash if we somehow end up sending state change events
    // Note that we do not QVERIFY_EVENT, as that relies on the updateHandler being
    // called, which does not happen/make sense when there's no interface for the event.
    QAccessibleStateChangeEvent active(&window, activeState);
    QAccessibleStateChangeEvent deactivate(&window, activeState);
    }
}

// Dialogs and other sub-windows must appear in the
// accessibility hierarchy exactly once as top level objects
void tst_QAccessibility::subWindowTest()
{
    {
    QWidget mainWidget;
    mainWidget.setGeometry(100, 100, 100, 100);
    mainWidget.show();
    QLabel label(QStringLiteral("Window Contents"), &mainWidget);
    mainWidget.setLayout(new QHBoxLayout());
    mainWidget.layout()->addWidget(&label);

    QDialog d(&mainWidget);
    d.show();

    QAccessibleInterface *app = QAccessible::queryAccessibleInterface(qApp);
    QVERIFY(app);
    QCOMPARE(app->childCount(), 2);

    QAccessibleInterface *windowIface = QAccessible::queryAccessibleInterface(&mainWidget);
    QVERIFY(windowIface);
    QCOMPARE(windowIface->childCount(), 1);
    QCOMPARE(app->child(0), windowIface);
    QCOMPARE(windowIface->parent(), app);

    QAccessibleInterface *dialogIface = QAccessible::queryAccessibleInterface(&d);
    QVERIFY(dialogIface);
    QCOMPARE(app->child(1), dialogIface);
    QCOMPARE(dialogIface->parent(), app);
    QCOMPARE(dialogIface->parent(), app);
    }

    {
    QMainWindow mainWindow;
    mainWindow.setGeometry(100, 100, 100, 100);
    mainWindow.show();
    QLabel label(QStringLiteral("Window Contents"), &mainWindow);
    mainWindow.setCentralWidget(&label);

    QDialog d(&mainWindow);
    d.show();

    QAccessibleInterface *app = QAccessible::queryAccessibleInterface(qApp);
    QVERIFY(app);
    QCOMPARE(app->childCount(), 2);

    QAccessibleInterface *windowIface = QAccessible::queryAccessibleInterface(&mainWindow);
    QVERIFY(windowIface);
    QCOMPARE(windowIface->childCount(), 1);
    QCOMPARE(app->child(0), windowIface);

    QAccessibleInterface *dialogIface = QAccessible::queryAccessibleInterface(&d);
    QVERIFY(dialogIface);
    QCOMPARE(app->child(1), dialogIface);
    QCOMPARE(dialogIface->parent(), app);
    QCOMPARE(windowIface->parent(), app);
    }
    QTestAccessibility::clearEvents();
}

class CounterButton : public QPushButton {
    Q_OBJECT
public:
    CounterButton(const QString& name, QWidget* parent)
        : QPushButton(name, parent), clickCount(0)
    {
        connect(this, SIGNAL(clicked(bool)), SLOT(incClickCount()));
    }
    int clickCount;
public Q_SLOTS:
    void incClickCount() {
        ++clickCount;
    }
};

#if QT_CONFIG(shortcut)

void tst_QAccessibility::buttonTest()
{
    QWidget window;
    window.setLayout(new QVBoxLayout);

    // Standard push button
    CounterButton pushButton("Ok", &window);

    // toggle button
    QPushButton toggleButton("Toggle", &window);
    toggleButton.setCheckable(true);

    // standard checkbox
    QCheckBox checkBox("Check me!", &window);

    // tristate checkbox
    QCheckBox tristate("Tristate!", &window);
    tristate.setTristate(true);

    // radiobutton
    QRadioButton radio("Radio me!", &window);

    // standard toolbutton
    QToolButton toolbutton(&window);
    toolbutton.setText("Tool");
    toolbutton.setMinimumSize(20,20);

    // standard toolbutton
    QToolButton toggletool(&window);
    toggletool.setCheckable(true);
    toggletool.setText("Toggle");
    toggletool.setMinimumSize(20,20);

    // test push button
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(&pushButton);
    QAccessibleActionInterface* actionInterface = interface->actionInterface();
    QVERIFY(actionInterface != 0);
    QCOMPARE(interface->role(), QAccessible::PushButton);

    // buttons only have a click action
    QCOMPARE(actionInterface->actionNames().size(), 2);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::pressAction() << QAccessibleActionInterface::setFocusAction());
    QCOMPARE(pushButton.clickCount, 0);
    actionInterface->doAction(QAccessibleActionInterface::pressAction());
    QTest::qWait(500);
    QCOMPARE(pushButton.clickCount, 1);

    // test toggle button
    interface = QAccessible::queryAccessibleInterface(&toggleButton);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::CheckBox);
    QCOMPARE(actionInterface->actionNames(),
             QStringList() << QAccessibleActionInterface::toggleAction()
                           << QAccessibleActionInterface::pressAction()
                           << QAccessibleActionInterface::setFocusAction());
    QCOMPARE(actionInterface->localizedActionDescription(QAccessibleActionInterface::toggleAction()), QString("Toggles the state"));
    QVERIFY(!toggleButton.isChecked());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::toggleAction());
    QTest::qWait(500);
    QVERIFY(toggleButton.isChecked());
    QCOMPARE(actionInterface->actionNames().at(0), QAccessibleActionInterface::toggleAction());
    QVERIFY(interface->state().checked);

    {
    // test menu push button
    QAction *foo = new QAction("Foo", nullptr);
    foo->setShortcut(QKeySequence("Ctrl+F"));
    auto menu = std::make_unique<QMenu>();
    menu->addAction(foo);
    QPushButton menuButton;
    setFrameless(&menuButton);
    menuButton.setMenu(menu.get());
    menuButton.show();
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&menuButton);
    QCOMPARE(interface->role(), QAccessible::ButtonMenu);
    QVERIFY(interface->state().hasPopup);
    QCOMPARE(interface->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction() << QAccessibleActionInterface::setFocusAction());
    // showing the menu enters a new event loop...
//    interface->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
//    QTest::qWait(500);
    }


    QTestAccessibility::clearEvents();
    {
    // test check box
    interface = QAccessible::queryAccessibleInterface(&checkBox);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::CheckBox);
    QCOMPARE(actionInterface->actionNames(),
             QStringList() << QAccessibleActionInterface::toggleAction()
                           << QAccessibleActionInterface::pressAction()
                           << QAccessibleActionInterface::setFocusAction());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::toggleAction());

    QTest::qWait(500);
    QCOMPARE(actionInterface->actionNames(),
             QStringList() << QAccessibleActionInterface::toggleAction()
                           << QAccessibleActionInterface::pressAction()
                           << QAccessibleActionInterface::setFocusAction());
    QVERIFY(interface->state().checked);
    QVERIFY(checkBox.isChecked());
    QAccessible::State st;
    st.checked = true;
    QAccessibleStateChangeEvent ev(&checkBox, st);
    QVERIFY_EVENT(&ev);
    checkBox.setChecked(false);
    QVERIFY_EVENT(&ev);
    }

    {
    // test radiobutton
    interface = QAccessible::queryAccessibleInterface(&radio);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::RadioButton);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::toggleAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::toggleAction());
    QTest::qWait(500);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::toggleAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(interface->state().checked);
    QVERIFY(radio.isChecked());
    QAccessible::State st;
    st.checked = true;
    QAccessibleStateChangeEvent ev(&radio, st);
    QVERIFY_EVENT(&ev);
    }

//    // test standard toolbutton
//    QVERIFY(QAccessible::queryAccessibleInterface(&toolbutton, &test));
//    QCOMPARE(test->role(), QAccessible::PushButton);
//    QCOMPARE(test->defaultAction(0), QAccessible::Press);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Press"));
//    QCOMPARE(test->state(), (int)QAccessible::Normal);
//    test->release();

//    // toggle tool button
//    QVERIFY(QAccessible::queryAccessibleInterface(&toggletool, &test));
//    QCOMPARE(test->role(), QAccessible::CheckBox);
//    QCOMPARE(test->defaultAction(0), QAccessible::Press);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Check"));
//    QCOMPARE(test->state(), (int)QAccessible::Normal);
//    QVERIFY(test->doAction(QAccessible::Press, 0));
//    QTest::qWait(500);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Uncheck"));
//    QCOMPARE(test->state(), (int)QAccessible::Checked);
//    test->release();

//    // test menu toolbutton
//    QVERIFY(QAccessible::queryAccessibleInterface(&menuToolButton, &test));
//    QCOMPARE(test->role(), QAccessible::ButtonMenu);
//    QCOMPARE(test->defaultAction(0), 1);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Open"));
//    QCOMPARE(test->state(), (int)QAccessible::HasPopup);
//    QCOMPARE(test->actionCount(0), 1);
//    QCOMPARE(test->actionText(QAccessible::Press, QAccessible::Name, 0), QString("Press"));
//    test->release();

//    // test split menu toolbutton
//    QVERIFY(QAccessible::queryAccessibleInterface(&splitToolButton, &test));
//    QCOMPARE(test->childCount(), 2);
//    QCOMPARE(test->role(), QAccessible::ButtonDropDown);
//    QCOMPARE(test->role(1), QAccessible::PushButton);
//    QCOMPARE(test->role(2), QAccessible::ButtonMenu);
//    QCOMPARE(test->defaultAction(0), QAccessible::Press);
//    QCOMPARE(test->defaultAction(1), QAccessible::Press);
//    QCOMPARE(test->defaultAction(2), QAccessible::Press);
//    QCOMPARE(test->actionText(test->defaultAction(0), QAccessible::Name, 0), QString("Press"));
//    QCOMPARE(test->state(), (int)QAccessible::HasPopup);
//    QCOMPARE(test->actionCount(0), 1);
//    QCOMPARE(test->actionText(1, QAccessible::Name, 0), QString("Open"));
//    QCOMPARE(test->actionText(test->defaultAction(1), QAccessible::Name, 1), QString("Press"));
//    QCOMPARE(test->state(1), (int)QAccessible::Normal);
//    QCOMPARE(test->actionText(test->defaultAction(2), QAccessible::Name, 2), QString("Open"));
//    QCOMPARE(test->state(2), (int)QAccessible::HasPopup);
//    test->release();
}

#endif // QT_CONFIG(shortcut)

void tst_QAccessibility::scrollBarTest()
{
    auto scrollBarHolder = std::make_unique<QScrollBar>(Qt::Horizontal);
    auto scrollBar = scrollBarHolder.get();
    QAccessibleInterface * const scrollBarInterface =
                                    QAccessible::queryAccessibleInterface(scrollBar);
    QVERIFY(scrollBarInterface);
    QVERIFY(scrollBarInterface->state().invisible);
    scrollBar->resize(200, 50);
    scrollBar->show();
    QVERIFY(!scrollBarInterface->state().invisible);
    QAccessibleEvent show(scrollBar, QAccessible::ObjectShow);
    QVERIFY(QTestAccessibility::containsEvent(&show));
    QTestAccessibility::clearEvents();

    scrollBar->hide();
    QVERIFY(scrollBarInterface->state().invisible);
    QAccessibleEvent hide(scrollBar, QAccessible::ObjectHide);
    QVERIFY(QTestAccessibility::containsEvent(&hide));
    QTestAccessibility::clearEvents();

    // Test that the left/right subcontrols are set to unavailable when the scrollBar is at the minimum/maximum.
    scrollBar->show();
    scrollBar->setMinimum(11);
    scrollBar->setMaximum(111);

    QAccessibleValueInterface *valueIface = scrollBarInterface->valueInterface();
    QVERIFY(valueIface != 0);
    QCOMPARE(valueIface->minimumValue().toInt(), scrollBar->minimum());
    QCOMPARE(valueIface->maximumValue().toInt(), scrollBar->maximum());
    scrollBar->setValue(50);
    QCOMPARE(valueIface->currentValue().toInt(), scrollBar->value());
    scrollBar->setValue(0);
    QCOMPARE(valueIface->currentValue().toInt(), scrollBar->value());
    scrollBar->setValue(100);
    QCOMPARE(valueIface->currentValue().toInt(), scrollBar->value());
    valueIface->setCurrentValue(77);
    QCOMPARE(77, scrollBar->value());

    const QRect scrollBarRect = scrollBarInterface->rect();
    QVERIFY(scrollBarRect.isValid());

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tabTest()
{
    auto tabBarHolder = std::make_unique<QTabBar>();
    auto tabBar = tabBarHolder.get();
    setFrameless(tabBar);
    tabBar->show();

    QAccessibleInterface * const interface = QAccessible::queryAccessibleInterface(tabBar);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 2);

    // Test that the Invisible bit for the navigation buttons gets set
    // and cleared correctly.
    QAccessibleInterface *leftButton = interface->child(0);
    QCOMPARE(leftButton->role(), QAccessible::PushButton);
    QVERIFY(leftButton->state().invisible);

    const int lots = 5;
    for (int i = 0; i < lots; ++i) {
        tabBar->addTab("Foo");
        tabBar->setTabToolTip(i, QLatin1String("Cool tool tip"));
        tabBar->setTabWhatsThis(i, QLatin1String("I don't know"));
    }

    QAccessibleInterface *child1 = interface->child(0);
    QAccessibleInterface *child2 = interface->child(1);
    QVERIFY(child1);
    QCOMPARE(child1->role(), QAccessible::PageTab);
    QVERIFY(child2);
    QCOMPARE(child2->role(), QAccessible::PageTab);

    QCOMPARE(child1->text(QAccessible::Name), QLatin1String("Foo"));
    QCOMPARE(child1->text(QAccessible::Description), QLatin1String("Cool tool tip"));
    QCOMPARE(child1->text(QAccessible::Help), QLatin1String("I don't know"));

    QVERIFY(!(child1->state().invisible));
    tabBar->hide();

    QCoreApplication::processEvents();
    QTest::qWait(100);

    QVERIFY(child1->state().invisible);

    tabBar->show();
    tabBar->setCurrentIndex(0);

    // Test that sending a focus action to a tab does not select it.
//    child2->doAction(QAccessible::Focus, 2, QVariantList());
    QCOMPARE(tabBar->currentIndex(), 0);

    // Test that sending a press action to a tab selects it.
    QVERIFY(child2->actionInterface());
    QCOMPARE(child2->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());
    QCOMPARE(tabBar->currentIndex(), 0);
    child2->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
    QCOMPARE(tabBar->currentIndex(), 1);

    // Test that setAccessibleTabName changes a tab's accessible name
    tabBar->setAccessibleTabName(0, "AccFoo");
    tabBar->setAccessibleTabName(1, "AccBar");
    QCOMPARE(child1->text(QAccessible::Name), QLatin1String("AccFoo"));
    QCOMPARE(child2->text(QAccessible::Name), QLatin1String("AccBar"));
    tabBar->setCurrentIndex(0);
    QCOMPARE(interface->text(QAccessible::Name), QLatin1String("AccFoo"));
    tabBar->setCurrentIndex(1);
    QCOMPARE(interface->text(QAccessible::Name), QLatin1String("AccBar"));

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tabWidgetTest()
{
    auto tabWidgetHolder = std::make_unique<QTabWidget>();
    auto tabWidget = tabWidgetHolder.get();
    tabWidget->show();

    // the interface for the tab is just a container for tabbar and stacked widget
    QAccessibleInterface * const interface = QAccessible::queryAccessibleInterface(tabWidget);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 2);
    QCOMPARE(interface->role(), QAccessible::Client);

    // Create pages, check navigation
    QLabel *label1 = new QLabel("Page 1", tabWidget);
    tabWidget->addTab(label1, "Tab 1");
    QLabel *label2 = new QLabel("Page 2", tabWidget);
    tabWidget->addTab(label2, "Tab 2");

    QCOMPARE(interface->childCount(), 2);

    QAccessibleInterface* tabBarInterface = 0;
    // there is no special logic to sort the children, so the contents will be 1, the tab bar 2
    tabBarInterface = interface->child(1);
    QCOMPARE(verifyHierarchy(tabBarInterface), 0);
    QVERIFY(tabBarInterface);
    QCOMPARE(tabBarInterface->childCount(), 4);
    QCOMPARE(tabBarInterface->role(), QAccessible::PageTabList);

    QAccessibleInterface* tabButton1Interface = tabBarInterface->child(0);
    QVERIFY(tabButton1Interface);
    QCOMPARE(tabButton1Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton1Interface->text(QAccessible::Name), QLatin1String("Tab 1"));

    QAccessibleInterface* tabButton2Interface = tabBarInterface->child(1);
    QVERIFY(tabButton2Interface);
    QCOMPARE(tabButton2Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton2Interface->text(QAccessible::Name), QLatin1String("Tab 2"));

    // Test that setAccessibleTabName changes a tab's accessible name
    tabWidget->setCurrentIndex(0);
    tabWidget->tabBar()->setAccessibleTabName(0, "Acc Tab");
    QCOMPARE(tabButton1Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton1Interface->text(QAccessible::Name), QLatin1String("Acc Tab"));
    QCOMPARE(tabBarInterface->text(QAccessible::Name), QLatin1String("Acc Tab"));

    QAccessibleInterface* tabButtonLeft = tabBarInterface->child(2);
    QVERIFY(tabButtonLeft);
    QCOMPARE(tabButtonLeft->role(), QAccessible::PushButton);
    QCOMPARE(tabButtonLeft->text(QAccessible::Name), QLatin1String("Scroll Left"));

    QAccessibleInterface* tabButtonRight = tabBarInterface->child(3);
    QVERIFY(tabButtonRight);
    QCOMPARE(tabButtonRight->role(), QAccessible::PushButton);
    QCOMPARE(tabButtonRight->text(QAccessible::Name), QLatin1String("Scroll Right"));

    QAccessibleInterface* stackWidgetInterface = interface->child(0);
    QVERIFY(stackWidgetInterface);
    QCOMPARE(stackWidgetInterface->childCount(), 2);
    QCOMPARE(stackWidgetInterface->role(), QAccessible::LayeredPane);

    QAccessibleInterface* stackChild1Interface = stackWidgetInterface->child(0);
    QVERIFY(stackChild1Interface);
    QCOMPARE(stackChild1Interface->childCount(), 0);
    QCOMPARE(stackChild1Interface->role(), QAccessible::StaticText);
    QCOMPARE(stackChild1Interface->text(QAccessible::Name), QLatin1String("Page 1"));
    QCOMPARE(label1, stackChild1Interface->object());

    // Navigation in stack widgets should be consistent
    QAccessibleInterface* parent = stackChild1Interface->parent();
    QVERIFY(parent);
    QCOMPARE(parent->childCount(), 2);
    QCOMPARE(parent->role(), QAccessible::LayeredPane);

    QAccessibleInterface* stackChild2Interface = stackWidgetInterface->child(1);
    QVERIFY(stackChild2Interface);
    QCOMPARE(stackChild2Interface->childCount(), 0);
    QCOMPARE(stackChild2Interface->role(), QAccessible::StaticText);
    QCOMPARE(label2, stackChild2Interface->object());
    QCOMPARE(label2->text(), stackChild2Interface->text(QAccessible::Name));

    parent = stackChild2Interface->parent();
    QVERIFY(parent);
    QCOMPARE(parent->childCount(), 2);
    QCOMPARE(parent->role(), QAccessible::LayeredPane);

    QTestAccessibility::clearEvents();
}

#if QT_CONFIG(shortcut)

void tst_QAccessibility::menuTest()
{
    {
    QMainWindow mw;
    mw.resize(300, 200);
    mw.menuBar()->setNativeMenuBar(false);
    QMenu *file = mw.menuBar()->addMenu("&File");
    QMenu *fileNew = file->addMenu("&New...");
    fileNew->menuAction()->setShortcut(tr("Ctrl+N"));
    fileNew->addAction("Text file");
    fileNew->addAction("Image file");
    file->addAction("&Open")->setShortcut(tr("Ctrl+O"));
    file->addAction("&Save")->setShortcut(tr("Ctrl+S"));
    file->addSeparator();
    file->addAction("E&xit")->setShortcut(tr("Alt+F4"));

    QMenu *edit = mw.menuBar()->addMenu("&Edit");
    edit->addAction("&Undo")->setShortcut(tr("Ctrl+Z"));
    edit->addAction("&Redo")->setShortcut(tr("Ctrl+Y"));
    edit->addSeparator();
    edit->addAction("Cu&t")->setShortcut(tr("Ctrl+X"));
    edit->addAction("&Copy")->setShortcut(tr("Ctrl+C"));
    edit->addAction("&Paste")->setShortcut(tr("Ctrl+V"));
    edit->addAction("&Delete")->setShortcut(tr("Del"));
    edit->addSeparator();
    edit->addAction("Pr&operties");

    mw.menuBar()->addSeparator();

    QMenu *help = mw.menuBar()->addMenu("&Help");
    help->addAction("&Contents");
    help->addAction("&About");

    mw.menuBar()->addAction("Action!");

    QMenu *childOfMainWindow = new QMenu(QStringLiteral("&Tools"), &mw);
    childOfMainWindow->addAction("&Options");
    mw.menuBar()->addMenu(childOfMainWindow);

    mw.show(); // triggers layout
    QTest::qWait(100);

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&mw);
    QCOMPARE(verifyHierarchy(interface),  0);

    interface = QAccessible::queryAccessibleInterface(mw.menuBar());

    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 6);
    QCOMPARE(interface->role(), QAccessible::MenuBar);

    QAccessibleInterface *iFile = interface->child(0);
    QAccessibleInterface *iEdit = interface->child(1);
    QAccessibleInterface *iSeparator = interface->child(2);
    QAccessibleInterface *iHelp = interface->child(3);
    QAccessibleInterface *iAction = interface->child(4);

    QCOMPARE(iFile->role(), QAccessible::MenuItem);
    QCOMPARE(iEdit->role(), QAccessible::MenuItem);
    QCOMPARE(iSeparator->role(), QAccessible::Separator);
    QCOMPARE(iHelp->role(), QAccessible::MenuItem);
    QCOMPARE(iAction->role(), QAccessible::MenuItem);
#ifndef Q_OS_MAC
    QCOMPARE(mw.mapFromGlobal(interface->rect().topLeft()), mw.menuBar()->geometry().topLeft());
    QCOMPARE(interface->rect().size(), mw.menuBar()->size());

    QVERIFY(interface->rect().contains(iFile->rect()));
    QVERIFY(interface->rect().contains(iEdit->rect()));
    // QVERIFY(interface->rect().contains(childSeparator->rect())); //separator might be invisible
    QVERIFY(interface->rect().contains(iHelp->rect()));
    QVERIFY(interface->rect().contains(iAction->rect()));
#endif

    QCOMPARE(iFile->text(QAccessible::Name), QString("File"));
    QCOMPARE(iEdit->text(QAccessible::Name), QString("Edit"));
    QCOMPARE(iSeparator->text(QAccessible::Name), QString());
    QCOMPARE(iHelp->text(QAccessible::Name), QString("Help"));
    QCOMPARE(iAction->text(QAccessible::Name), QString("Action!"));

// TODO: Currently not working, task to fix is #100019.
#ifndef Q_OS_MAC
    QCOMPARE(iFile->text(QAccessible::Accelerator), tr("Alt+F"));
    QCOMPARE(iEdit->text(QAccessible::Accelerator), tr("Alt+E"));
    QCOMPARE(iSeparator->text(QAccessible::Accelerator), QString());
    QCOMPARE(iHelp->text(QAccessible::Accelerator), tr("Alt+H"));
    QCOMPARE(iAction->text(QAccessible::Accelerator), QString());
#endif

    QVERIFY(iFile->actionInterface());

    QCOMPARE(iFile->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    QCOMPARE(iSeparator->actionInterface()->actionNames(), QStringList());
    QCOMPARE(iHelp->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    QCOMPARE(iAction->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());

    bool menuFade = qApp->isEffectEnabled(Qt::UI_FadeMenu);
    int menuFadeDelay = 300;
    iFile->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QTRY_VERIFY(file->isVisible() && !edit->isVisible() && !help->isVisible());
    iEdit->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QTRY_VERIFY(!file->isVisible() && edit->isVisible() && !help->isVisible());
    iHelp->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QTRY_VERIFY(!file->isVisible() && !edit->isVisible() && help->isVisible());
    iAction->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QTRY_VERIFY(!file->isVisible() && !edit->isVisible() && !help->isVisible());

    QVERIFY(interface->actionInterface());
    QCOMPARE(interface->actionInterface()->actionNames(), QStringList());
    interface = QAccessible::queryAccessibleInterface(file);
    QCOMPARE(interface->childCount(), 5);
    QCOMPARE(interface->role(), QAccessible::PopupMenu);

    QAccessibleInterface *iFileNew = interface->child(0);
    QAccessibleInterface *iFileOpen = interface->child(1);
    QAccessibleInterface *iFileSave = interface->child(2);
    QAccessibleInterface *iFileSeparator = interface->child(3);
    QAccessibleInterface *iFileExit = interface->child(4);

    QCOMPARE(iFileNew->role(), QAccessible::MenuItem);
    QCOMPARE(iFileOpen->role(), QAccessible::MenuItem);
    QCOMPARE(iFileSave->role(), QAccessible::MenuItem);
    QCOMPARE(iFileSeparator->role(), QAccessible::Separator);
    QCOMPARE(iFileExit->role(), QAccessible::MenuItem);
    QCOMPARE(iFileNew->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
    QCOMPARE(iFileOpen->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());
    QCOMPARE(iFileSave->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());
    QCOMPARE(iFileSeparator->actionInterface()->actionNames(), QStringList());
    QCOMPARE(iFileExit->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::pressAction());

    QAccessibleInterface *iface = 0;
    QAccessibleInterface *iface2 = 0;

    // traverse siblings with navigate(Sibling, ...)
    iface = interface->child(0);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    QAccessible::Role fileRoles[5] = {
        QAccessible::MenuItem,
        QAccessible::MenuItem,
        QAccessible::MenuItem,
        QAccessible::Separator,
        QAccessible::MenuItem
    };
    for (int child = 0; child < 5; ++child) {
        iface2 = interface->child(child);
        QVERIFY(iface2);
        QCOMPARE(iface2->role(), fileRoles[child]);
    }

    // "New" item
    iface = interface->child(0);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    // "New" menu
    iface2 = iface->child(0);
    iface = iface2;
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::PopupMenu);

    // "Text file" menu item
    iface2 = iface->child(0);
    iface = iface2;
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::MenuItem);

    // move mouse pointer away, since that might influence the
    // subsequent tests
    QTest::mouseMove(&mw, QPoint(-1, -1));
    QTest::qWait(100);
    if (menuFade)
        QTest::qWait(menuFadeDelay);

    iFile->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    iFileNew->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());

    QTRY_VERIFY(file->isVisible());
    QTRY_VERIFY(fileNew->isVisible());
    QVERIFY(!edit->isVisible());
    QVERIFY(!help->isVisible());

    QTestAccessibility::clearEvents();
    mw.hide();

    // Do not crash if the menu don't have a parent
    auto menu = std::make_unique<QMenu>();
    menu->addAction(QLatin1String("one"));
    menu->addAction(QLatin1String("two"));
    menu->addAction(QLatin1String("three"));
    iface = QAccessible::queryAccessibleInterface(menu.get());
    iface2 = iface->parent();
    QVERIFY(iface2);
    QCOMPARE(iface2->role(), QAccessible::Application);
    // caused a *crash*
    iface2->state();
    menu.reset();

    }
    QTestAccessibility::clearEvents();
}

#endif // QT_CONFIG(shortcut)

void tst_QAccessibility::spinBoxTest()
{
    auto spinBoxHolder = std::make_unique<QSpinBox>();
    const auto spinBox = spinBoxHolder.get();
    setFrameless(spinBox);
    spinBox->setValue(3);
    spinBox->show();

    QAccessibleInterface * const interface = QAccessible::queryAccessibleInterface(spinBox);
    QVERIFY(interface);
    QCOMPARE(interface->role(), QAccessible::SpinBox);

    QVERIFY(QTest::qWaitForWindowExposed(spinBox));

    const QRect widgetRect = spinBox->geometry();
    const QRect accessibleRect = interface->rect();
    QCOMPARE(accessibleRect, widgetRect);
    QCOMPARE(interface->text(QAccessible::Value), QLatin1String("3"));

    // make sure that the line edit is not there
    const int numChildren = interface->childCount();
    QCOMPARE(numChildren, 0);
    QVERIFY(!interface->child(0));

    QVERIFY(interface->valueInterface());
    QCOMPARE(interface->valueInterface()->currentValue().toInt(), 3);
    interface->valueInterface()->setCurrentValue(23);
    QCOMPARE(interface->valueInterface()->currentValue().toInt(), 23);
    QCOMPARE(spinBox->value(), 23);

    spinBox->setFocus();
    QTestAccessibility::clearEvents();
    QTest::keyPress(spinBox, Qt::Key_Up);
    QTest::qWait(200);
    QAccessibleValueChangeEvent expectedEvent(spinBox, spinBox->value());
    QVERIFY(QTestAccessibility::containsEvent(&expectedEvent));

    QAccessibleTextInterface *textIface = interface->textInterface();
    QVERIFY(textIface);

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::doubleSpinBoxTest()
{
    auto holder = std::make_unique<QDoubleSpinBox>();
    auto doubleSpinBox = holder.get();
    setFrameless(doubleSpinBox);
    doubleSpinBox->show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(doubleSpinBox);
    QVERIFY(interface);

    QVERIFY(QTest::qWaitForWindowExposed(doubleSpinBox));

    const QRect widgetRect = doubleSpinBox->geometry();
    const QRect accessibleRect = interface->rect();
    QCOMPARE(accessibleRect, widgetRect);

    // Test that we get valid rects for all the spinbox child interfaces.
    const int numChildren = interface->childCount();
    for (int i = 0; i < numChildren; ++i) {
        QAccessibleInterface *childIface = interface->child(i);
        const QRect childRect = childIface->rect();
        QVERIFY(childRect.isValid());
    }

    QTestAccessibility::clearEvents();
}

static QRect characterRect(const QTextEdit &edit, int offset)
{
    QTextBlock block = edit.document()->findBlock(offset);
    QTextLayout *layout = block.layout();
    QPointF layoutPosition = layout->position();
    int relativeOffset = offset - block.position();
    QTextLine line = layout->lineForTextPosition(relativeOffset);
    QTextBlock::iterator it = block.begin();
    while (!it.fragment().contains(offset))
        ++it;
    QFontMetrics fm(it.fragment().charFormat().font());
    QChar ch = edit.document()->characterAt(offset);
    int w = fm.horizontalAdvance(ch);
    int h = fm.height();

    qreal x = line.cursorToX(relativeOffset);
    QRect r(layoutPosition.x() + x, layoutPosition.y() + line.y() + line.ascent() + fm.descent() - h, w, h);
    r.moveTo(edit.viewport()->mapToGlobal(r.topLeft()));

    return r;
}

/* The rects does not have to be exactly the same. They may be slightly different due to
   different ways of calculating them. By having an acceptable delta, this should also
   make the test a bit more resilient against any future changes in the behavior of
   characterRect().
*/
static bool fuzzyRectCompare(const QRect &a, const QRect &b)
{
    static const int MAX_ACCEPTABLE_DELTA = 1;
    const QMargins delta(a.left() - b.left(), a.top() - b.top(),
                         a.right() - b.right(), a.bottom() - b.bottom());

    return qAbs(delta.left()) <= MAX_ACCEPTABLE_DELTA && qAbs(delta.top()) <= MAX_ACCEPTABLE_DELTA
           && qAbs(delta.right()) <= MAX_ACCEPTABLE_DELTA && qAbs(delta.bottom()) <= MAX_ACCEPTABLE_DELTA;
}

static QByteArray msgRectMismatch(const QRect &a, const QRect &b)
{
    QString result;
    QDebug(&result) << a << "!=" << b;
    return result.toLocal8Bit();
}

void tst_QAccessibility::textEditTest()
{
    for (int pass = 0; pass < 2; ++pass) {
        {
        QTextEdit edit;
        edit.setMinimumSize(600, 400);
        setFrameless(&edit);
        int startOffset;
        int endOffset;
        // create two blocks of text. The first block has two lines.
        QString text = "<p>hello world.<br/>How are you today?</p><p>I'm fine, thanks</p>";
        edit.setHtml(text);
        if (pass == 1) {
            QFont font(QStringList{"Helvetica"});
            font.setPointSizeF(12.5);
            font.setWordSpacing(1.1);
            edit.document()->setDefaultFont(font);
        }

        edit.show();
        QVERIFY(QTest::qWaitForWindowExposed(&edit));
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&edit);
        QCOMPARE(iface->text(QAccessible::Value), edit.toPlainText());
        QVERIFY(iface->state().focusable);
        QVERIFY(!iface->state().selectable);
        QVERIFY(!iface->state().selected);
        QVERIFY(iface->state().selectableText);

        QAccessibleTextInterface *textIface = iface->textInterface();
        QVERIFY(textIface);

        QCOMPARE(textIface->textAtOffset(8, QAccessible::WordBoundary, &startOffset, &endOffset), QString("world"));
        QCOMPARE(startOffset, 6);
        QCOMPARE(endOffset, 11);
        QCOMPARE(textIface->textAtOffset(15, QAccessible::LineBoundary, &startOffset, &endOffset), QString("How are you today?"));
        QCOMPARE(startOffset, 13);
        QCOMPARE(endOffset, 31);
        QCOMPARE(textIface->characterCount(), 48);
        QFontMetrics fm(edit.document()->defaultFont());
        QCOMPARE(textIface->characterRect(0).size(), QSize(fm.horizontalAdvance("h"), fm.height()));
        QCOMPARE(textIface->characterRect(5).size(), QSize(fm.horizontalAdvance(" "), fm.height()));
        QCOMPARE(textIface->characterRect(6).size(), QSize(fm.horizontalAdvance("w"), fm.height()));

        int offset = 10;
        QCOMPARE(textIface->text(offset, offset + 1), QStringLiteral("d"));
        const QRect actual10 = textIface->characterRect(offset);
        const QRect expected10 = characterRect(edit, offset);
        QVERIFY2(fuzzyRectCompare(actual10, expected10), msgRectMismatch(actual10, expected10).constData());
        offset = 13;
        QCOMPARE(textIface->text(offset, offset + 1), QStringLiteral("H"));
        const QRect actual13 = textIface->characterRect(offset);
        const QRect expected13 = characterRect(edit, offset);
        QVERIFY2(fuzzyRectCompare(actual13, expected13), msgRectMismatch(actual13, expected13).constData());
        offset = 21;
        QCOMPARE(textIface->text(offset, offset + 1), QStringLiteral("y"));
        const QRect actual21 = textIface->characterRect(offset);
        const QRect expected21 = characterRect(edit, offset);
        QVERIFY2(fuzzyRectCompare(actual21, expected21), msgRectMismatch(actual21, expected21).constData());
        offset = 32;
        QCOMPARE(textIface->text(offset, offset + 1), QStringLiteral("I"));
        const QRect actual32 = textIface->characterRect(offset);
        const QRect expected32 = characterRect(edit, offset);
        QVERIFY2(fuzzyRectCompare(actual32, expected32), msgRectMismatch(actual32, expected32).constData());

        QTestAccessibility::clearEvents();

        // select text
        QTextCursor c = edit.textCursor();
        c.setPosition(2);
        c.setPosition(4, QTextCursor::KeepAnchor);
        edit.setTextCursor(c);
        QAccessibleTextSelectionEvent sel(&edit, 2, 4);
        QVERIFY_EVENT(&sel);
        QAccessibleTextCursorEvent cursor(&edit, 4);
        QVERIFY_EVENT(&cursor);

        edit.selectAll();
        int end = edit.textCursor().position();
        sel.setCursorPosition(end);
        sel.setSelection(0, end);
        QVERIFY_EVENT(&sel);

        // check that we have newlines handled
        QString poem = QStringLiteral("Once upon a midnight dreary,\nwhile I pondered, weak and weary,\nOver many a quaint and curious volume of forgotten lore\n");
        QAccessibleEditableTextInterface *editableTextIface = iface->editableTextInterface();
        QVERIFY(editableTextIface);
        editableTextIface->replaceText(0, end, poem);
        QCOMPARE(iface->text(QAccessible::Value), poem);
        QCOMPARE(textIface->text(0, poem.size()), poem);
        QCOMPARE(textIface->text(28, 29), QLatin1String("\n"));
        int start;
        QCOMPARE(textIface->textAtOffset(42, QAccessible::LineBoundary, &start, &end), QStringLiteral("while I pondered, weak and weary,"));
        QCOMPARE(start, 29);
        QCOMPARE(end, 62);
        QCOMPARE(textIface->textAtOffset(28, QAccessible::CharBoundary, &start, &end), QLatin1String("\n"));
        QCOMPARE(start, 28);
        QCOMPARE(end, 29);

        edit.clear();
        QTestAccessibility::clearEvents();

        // make sure we get notifications when typing text
        QTestEventList keys;
        keys.addKeyClick('A');
        keys.simulate(&edit);
        keys.clear();
        QAccessibleTextInsertEvent insertA(&edit, 0, "A");
        QVERIFY_EVENT(&insertA);
        QAccessibleTextCursorEvent move1(&edit, 1);
        QVERIFY_EVENT(&move1);


        keys.addKeyClick('c');
        keys.simulate(&edit);
        keys.clear();
        QTRY_COMPARE(edit.toPlainText(), "Ac");

        QAccessibleTextInsertEvent insertC(&edit, 1, "c");
        QVERIFY_EVENT(&insertC);
        QAccessibleTextCursorEvent move2(&edit, 2);
        QVERIFY_EVENT(&move2);

        keys.addKeyClick(Qt::Key_Backspace);
        keys.simulate(&edit);
        keys.clear();
        QTRY_COMPARE(edit.toPlainText(), "A");

        // FIXME this should get a proper string instead of space
        QAccessibleTextRemoveEvent del(&edit, 1, " ");
        QVERIFY_EVENT(&del);
        QVERIFY_EVENT(&move1);

        // it would be nicer to get a text update event, but the current implementation
        // instead does remove and insert which is also fine
        edit.setText(QStringLiteral("Accessibility rocks"));
        QAccessibleTextRemoveEvent remove(&edit, 0, "  ");
        QVERIFY_EVENT(&remove);

        QAccessibleTextInsertEvent insert(&edit, 0, "Accessibility rocks");
        QVERIFY_EVENT(&insert);
        }
        QTestAccessibility::clearEvents();
    }
}

void tst_QAccessibility::textBrowserTest()
{
    {
    QTextBrowser textBrowser;
    QString text = QLatin1String("Hello world\nhow are you today?\n");
    textBrowser.setText(text);
    textBrowser.show();

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&textBrowser);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::StaticText);
    QCOMPARE(iface->text(QAccessible::Value), text);
    int startOffset;
    int endOffset;
    QCOMPARE(iface->textInterface()->textAtOffset(8, QAccessible::WordBoundary, &startOffset, &endOffset), QString("world"));
    QCOMPARE(startOffset, 6);
    QCOMPARE(endOffset, 11);
    QCOMPARE(iface->textInterface()->textAtOffset(14, QAccessible::LineBoundary, &startOffset, &endOffset), QString("how are you today?"));
    QCOMPARE(startOffset, 12);
    QCOMPARE(endOffset, 30);
    QCOMPARE(iface->textInterface()->characterCount(), 31);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::mdiAreaTest()
{
    {
    QMdiArea mdiArea;
    mdiArea.resize(400,300);
    mdiArea.show();
    const int subWindowCount = 3;
    for (int i = 0; i < subWindowCount; ++i)
        mdiArea.addSubWindow(new QWidget, Qt::Dialog)->show();

    QList<QMdiSubWindow *> subWindows = mdiArea.subWindowList();
    QCOMPARE(subWindows.size(), subWindowCount);

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&mdiArea);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), subWindowCount);

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::mdiSubWindowTest()
{
    {
    QMdiArea mdiArea;
    mdiArea.show();
    QApplicationPrivate::setActiveWindow(&mdiArea);
    QVERIFY(QTest::qWaitForWindowActive(&mdiArea));


    const int subWindowCount =  5;
    for (int i = 0; i < subWindowCount; ++i) {
        QMdiSubWindow *window = mdiArea.addSubWindow(new QPushButton("QAccessibilityTest"));
        window->setAttribute(Qt::WA_LayoutUsesWidgetRect);
        window->show();
        // Parts of this test requires that the sub windows are placed next
        // to each other. In order to achieve that QMdiArea must have
        // a width which is larger than subWindow->width() * subWindowCount.
        if (i == 0) {
            int minimumWidth = window->width() * subWindowCount + 20;
            mdiArea.resize(mdiArea.size().expandedTo(QSize(minimumWidth, 0)));
#if defined(Q_OS_UNIX)
            QCoreApplication::processEvents();
            QTest::qWait(100);
#endif
        }
    }

    QList<QMdiSubWindow *> subWindows = mdiArea.subWindowList();
    QCOMPARE(subWindows.size(), subWindowCount);

    QMdiSubWindow *testWindow = subWindows.at(3);
    QVERIFY(testWindow);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(testWindow);

    // childCount
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 1);

    // setText / text
    QCOMPARE(interface->text(QAccessible::Name), QString());
    testWindow->setWindowTitle(QLatin1String("ReplaceMe"));
    QCOMPARE(interface->text(QAccessible::Name), QLatin1String("ReplaceMe"));
    interface->setText(QAccessible::Name, QLatin1String("TitleSetOnWindow"));
    QCOMPARE(interface->text(QAccessible::Name), QLatin1String("TitleSetOnWindow"));

    mdiArea.setActiveSubWindow(testWindow);

#ifdef Q_OS_ANDROID // on Android QMdiSubWindow is maximized by default
    testWindow->showNormal();
#endif

    // state
    QAccessible::State state;
    state.focusable = true;
    state.focused = true;
    state.movable = true;
    state.sizeable = true;

    QCOMPARE(interface->state(), state);
    const QRect originalGeometry = testWindow->geometry();
    testWindow->showMaximized();
    state.sizeable = false;
    state.movable = false;
    QCOMPARE(interface->state(), state);
    testWindow->showNormal();
    testWindow->move(-10, 0);
    QVERIFY(interface->state().offscreen);
    testWindow->setVisible(false);
    QVERIFY(interface->state().invisible);
    testWindow->setVisible(true);
    testWindow->setEnabled(false);
    QVERIFY(interface->state().disabled);
    testWindow->setEnabled(true);
    QApplicationPrivate::setActiveWindow(&mdiArea);
    mdiArea.setActiveSubWindow(testWindow);
    testWindow->setFocus();
    QVERIFY(testWindow->isAncestorOf(qApp->focusWidget()));
    QVERIFY(interface->state().focused);
    testWindow->setGeometry(originalGeometry);

    // rect
    const QPoint globalPos = testWindow->mapToGlobal(QPoint(0, 0));
    QCOMPARE(interface->rect(), QRect(globalPos, testWindow->size()));
    testWindow->hide();
    QCOMPARE(interface->rect(), QRect());
    QCOMPARE(childRect(interface), QRect());
    testWindow->showMinimized();
    QCOMPARE(childRect(interface), QRect());
    testWindow->showNormal();
    testWindow->widget()->hide();
    QCOMPARE(childRect(interface), QRect());
    testWindow->widget()->show();
    const QRect widgetGeometry = testWindow->contentsRect();
    const QPoint globalWidgetPos = QPoint(globalPos.x() + widgetGeometry.x(),
                                          globalPos.y() + widgetGeometry.y());
#ifdef Q_OS_MAC
    QSKIP("QTBUG-22812");
#endif
    QCOMPARE(childRect(interface), QRect(globalWidgetPos, widgetGeometry.size()));

    // childAt
    QCOMPARE(interface->childAt(-10, 0), static_cast<QAccessibleInterface*>(0));
    QCOMPARE(interface->childAt(globalPos.x(), globalPos.y()), static_cast<QAccessibleInterface*>(0));
    QAccessibleInterface *child = interface->childAt(globalWidgetPos.x(), globalWidgetPos.y());
    QCOMPARE(child->role(), QAccessible::PushButton);
    QCOMPARE(child->text(QAccessible::Name), QString("QAccessibilityTest"));
    testWindow->widget()->hide();
    QCOMPARE(interface->childAt(globalWidgetPos.x(), globalWidgetPos.y()), static_cast<QAccessibleInterface*>(0));

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::lineEditTest()
{
    auto topLevelHolder = std::make_unique<QWidget>();
    auto toplevel = topLevelHolder.get();
    {
    auto le = std::make_unique<QLineEdit>();
    QAccessibleInterface *iface(QAccessible::queryAccessibleInterface(le.get()));
    QVERIFY(iface);
    QAccessibleTextInterface *textIface = iface->textInterface();
    QVERIFY(textIface);
    le->show();

    QApplication::processEvents();
    QCOMPARE(iface->childCount(), 0);
    QVERIFY(iface->state().sizeable);
    QVERIFY(iface->state().movable);
    QVERIFY(iface->state().focusable);
    QVERIFY(!iface->state().selectable);
    QVERIFY(iface->state().selectableText);
    QVERIFY(!iface->state().hasPopup);
    QVERIFY(!iface->state().readOnly);
    QVERIFY(iface->state().editable);

    le->setReadOnly(true);
    QVERIFY(iface->state().editable);
    QVERIFY(iface->state().readOnly);
    le->setReadOnly(false);
    QVERIFY(!iface->state().readOnly);

    QCOMPARE(bool(iface->state().focused), le->hasFocus());

    QString secret(QLatin1String("secret"));
    le->setText(secret);
    le->setEchoMode(QLineEdit::Normal);
    QVERIFY(!(iface->state().passwordEdit));
    QCOMPARE(iface->text(QAccessible::Value), secret);
    le->setEchoMode(QLineEdit::NoEcho);
    QVERIFY(iface->state().passwordEdit);
    QCOMPARE(iface->text(QAccessible::Value), QString());
    le->setEchoMode(QLineEdit::Password);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value) != le->text());
    QCOMPARE(iface->text(QAccessible::Value), le->displayText());
    QCOMPARE(textIface->characterCount(), secret.size());
    QVERIFY(textIface->text(0, textIface->characterCount()) != le->text());
    QCOMPARE(textIface->text(0, textIface->characterCount()), le->displayText());
    le->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value) != le->text());
    QCOMPARE(iface->text(QAccessible::Value), le->displayText());
    QCOMPARE(textIface->characterCount(), secret.size());
    QVERIFY(textIface->text(0, textIface->characterCount()) != le->text());
    QCOMPARE(textIface->text(0, textIface->characterCount()), le->displayText());
    le->setEchoMode(QLineEdit::Normal);
    QVERIFY(!(iface->state().passwordEdit));
    QCOMPARE(iface->text(QAccessible::Value), secret);
    QCOMPARE(textIface->characterCount(), secret.size());
    QCOMPARE(textIface->text(0, textIface->characterCount()), secret);

    le->setParent(toplevel);
    toplevel->show();
    QApplication::processEvents();
    QVERIFY(!(iface->state().sizeable));
    QVERIFY(!(iface->state().movable));
    QVERIFY(iface->state().focusable);
    QVERIFY(!iface->state().selectable);
    QVERIFY(!iface->state().selected);
    QVERIFY(iface->state().selectableText);
    QVERIFY(!iface->state().hasPopup);
    QCOMPARE(bool(iface->state().focused), le->hasFocus());

    QLineEdit *le2 = new QLineEdit(toplevel);
    le2->show();
    QTest::qWait(100);
    le2->activateWindow();
    QTest::qWait(100);
    le->setFocus(Qt::TabFocusReason);
    QTestAccessibility::clearEvents();
    le2->setFocus(Qt::TabFocusReason);
    QAccessibleEvent ev(le2, QAccessible::Focus);
    QTRY_VERIFY(QTestAccessibility::containsEvent(&ev));

    le->setText(QLatin1String("500"));
    le->setValidator(new QIntValidator());
    iface->setText(QAccessible::Value, QLatin1String("This text is not a number"));
    QCOMPARE(le->text(), QLatin1String("500"));

    le.reset();
    delete le2;
    }

    {
    // Text interface to get the current text
    QString cite = "I always pass on good advice. It is the only thing to do with it. It is never of any use to oneself. --Oscar Wilde";
    QLineEdit *le3 = new QLineEdit(cite, toplevel);
    le3->show();
    QAccessibleInterface *iface(QAccessible::queryAccessibleInterface(le3));
    QAccessibleTextInterface* textIface = iface->textInterface();
    le3->deselect();
    QTestAccessibility::clearEvents();
    le3->setCursorPosition(3);
    QCOMPARE(textIface->cursorPosition(), 3);

    QAccessibleTextCursorEvent caretEvent(le3, 3);
    QTRY_VERIFY(QTestAccessibility::containsEvent(&caretEvent));
    QCOMPARE(textIface->selectionCount(), 0);
    QTestAccessibility::clearEvents();

    int start, end;
    QCOMPARE(textIface->text(0, 8), QString::fromLatin1("I always"));
    QCOMPARE(textIface->textAtOffset(0, QAccessible::CharBoundary,&start,&end), QString::fromLatin1("I"));
    QCOMPARE(start, 0);
    QCOMPARE(end, 1);
    QCOMPARE(textIface->textBeforeOffset(0, QAccessible::CharBoundary,&start,&end), QString());
    QCOMPARE(textIface->textAfterOffset(0, QAccessible::CharBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(start, 1);
    QCOMPARE(end, 2);

    QCOMPARE(textIface->textAtOffset(5, QAccessible::CharBoundary,&start,&end), QString::fromLatin1("a"));
    QCOMPARE(start, 5);
    QCOMPARE(end, 6);
    QCOMPARE(textIface->textBeforeOffset(5, QAccessible::CharBoundary,&start,&end), QString::fromLatin1("w"));
    QCOMPARE(textIface->textAfterOffset(5, QAccessible::CharBoundary,&start,&end), QString::fromLatin1("y"));

    QCOMPARE(textIface->textAtOffset(5, QAccessible::WordBoundary,&start,&end), QString::fromLatin1("always"));
    QCOMPARE(start, 2);
    QCOMPARE(end, 8);

    QCOMPARE(textIface->textAtOffset(2, QAccessible::WordBoundary,&start,&end), QString::fromLatin1("always"));
    QCOMPARE(textIface->textAtOffset(7, QAccessible::WordBoundary,&start,&end), QString::fromLatin1("always"));
    QCOMPARE(textIface->textAtOffset(8, QAccessible::WordBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(textIface->textAtOffset(25, QAccessible::WordBoundary,&start,&end), QString::fromLatin1("advice"));
    QCOMPARE(textIface->textAtOffset(92, QAccessible::WordBoundary,&start,&end), QString::fromLatin1("oneself"));
    QCOMPARE(textIface->textAtOffset(101, QAccessible::WordBoundary,&start,&end), QString::fromLatin1(". --"));

    QCOMPARE(textIface->textBeforeOffset(5, QAccessible::WordBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(textIface->textAfterOffset(5, QAccessible::WordBoundary,&start,&end), QString::fromLatin1(" "));
    QCOMPARE(textIface->textAtOffset(5, QAccessible::SentenceBoundary,&start,&end), QString::fromLatin1("I always pass on good advice. "));
    QCOMPARE(start, 0);
    QCOMPARE(end, 30);

    QCOMPARE(textIface->textBeforeOffset(40, QAccessible::SentenceBoundary,&start,&end), QString::fromLatin1("I always pass on good advice. "));
    QCOMPARE(textIface->textAfterOffset(5, QAccessible::SentenceBoundary,&start,&end), QString::fromLatin1("It is the only thing to do with it. "));

    QCOMPARE(textIface->textAtOffset(5, QAccessible::ParagraphBoundary,&start,&end), cite);
    QCOMPARE(start, 0);
    QCOMPARE(end, cite.size());
    QCOMPARE(textIface->textAtOffset(5, QAccessible::LineBoundary,&start,&end), cite);
    QCOMPARE(textIface->textAtOffset(5, QAccessible::NoBoundary,&start,&end), cite);

    QTestAccessibility::clearEvents();
    }

    {
        QLineEdit le(QStringLiteral("My characters have geometries."), toplevel);
        // characterRect()
        le.show();
        QVERIFY(QTest::qWaitForWindowExposed(&le));
        QAccessibleInterface *iface(QAccessible::queryAccessibleInterface(&le));
        QAccessibleTextInterface* textIface = iface->textInterface();
        QVERIFY(textIface);
        const QRect lineEditRect = iface->rect();
        // Only first 10 characters, check if they are within the bounds of line edit
        for (int i = 0; i < 10; ++i) {
            QVERIFY(lineEditRect.contains(textIface->characterRect(i)));
        }
        QTestAccessibility::clearEvents();
    }

    {
    // Test events: cursor movement, selection, text changes
    QString text = "Hello, world";
    QLineEdit *lineEdit = new QLineEdit(text, toplevel);
    lineEdit->show();
    QTestAccessibility::clearEvents();
    // cursor
    lineEdit->setCursorPosition(5);
    QAccessibleTextCursorEvent cursorEvent(lineEdit, 5);
    QVERIFY_EVENT(&cursorEvent);
    lineEdit->setCursorPosition(0);
    cursorEvent.setCursorPosition(0);
    QVERIFY_EVENT(&cursorEvent);

    // selection
    lineEdit->setSelection(2, 4);

    QAccessibleTextSelectionEvent sel(lineEdit, 2, 2+4);
    QVERIFY_EVENT(&sel);

    lineEdit->selectAll();
    sel.setSelection(0, lineEdit->text().size());
    sel.setCursorPosition(lineEdit->text().size());
    QVERIFY_EVENT(&sel);

    lineEdit->setSelection(10, -4);
    QCOMPARE(lineEdit->cursorPosition(), 6);
    QAccessibleTextSelectionEvent sel2(lineEdit, 6, 10);
    sel2.setCursorPosition(6);
    QVERIFY_EVENT(&sel2);

    lineEdit->deselect();
    QAccessibleTextSelectionEvent sel3(lineEdit, -1, -1);
    sel3.setCursorPosition(6);
    QVERIFY_EVENT(&sel3);

    // editing
    lineEdit->clear();
    // FIXME: improve redundant updates
    QAccessibleTextRemoveEvent remove(lineEdit, 0, text);
    QVERIFY_EVENT(&remove);

    QAccessibleTextSelectionEvent noSel(lineEdit, -1, -1);
    QVERIFY_EVENT(&noSel);
    QAccessibleTextCursorEvent cursor(lineEdit, 0);
    QVERIFY_EVENT(&cursor);

    lineEdit->setText("foo");
    cursorEvent.setCursorPosition(3);
    QVERIFY_EVENT(&cursorEvent);

    QAccessibleTextInsertEvent e(lineEdit, 0, "foo");
    QVERIFY(QTestAccessibility::containsEvent(&e));

    lineEdit->setText("bar");
    QAccessibleTextUpdateEvent update(lineEdit, 0, "foo", "bar");
    QVERIFY(QTestAccessibility::containsEvent(&update));

    // FIXME check what extra events are around and get rid of them
    QTestAccessibility::clearEvents();

    QTestEventList keys;
    keys.addKeyClick('D');
    keys.simulate(lineEdit);
    QTRY_COMPARE(lineEdit->text(), "barD");

    QAccessibleTextInsertEvent insertD(lineEdit, 3, "D");
    QVERIFY_EVENT(&insertD);
    keys.clear();
    keys.addKeyClick('E');
    keys.simulate(lineEdit);
    QTRY_COMPARE(lineEdit->text(), "barDE");

    QAccessibleTextInsertEvent insertE(lineEdit, 4, "E");
    QVERIFY(QTestAccessibility::containsEvent(&insertE));
    keys.clear();
    keys.addKeyClick(Qt::Key_Left);
    keys.addKeyClick(Qt::Key_Left);
    keys.simulate(lineEdit);
    QTRY_COMPARE(lineEdit->cursorPosition(), 3);

    cursorEvent.setCursorPosition(4);
    QVERIFY(QTestAccessibility::containsEvent(&cursorEvent));
    cursorEvent.setCursorPosition(3);
    QVERIFY(QTestAccessibility::containsEvent(&cursorEvent));

    keys.clear();
    keys.addKeyClick('C');
    keys.simulate(lineEdit);
    QTRY_COMPARE(lineEdit->text(), "barCDE");

    QAccessibleTextInsertEvent insertC(lineEdit, 3, "C");
    QVERIFY(QTestAccessibility::containsEvent(&insertC));

    keys.clear();
    keys.addKeyClick('O');
    keys.simulate(lineEdit);
    QTRY_COMPARE(lineEdit->text(), "barCODE");

    QAccessibleTextInsertEvent insertO(lineEdit, 4, "O");
    QVERIFY(QTestAccessibility::containsEvent(&insertO));
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::lineEditTextFunctions_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("textFunction"); // before = 0, at = 1, after = 2
    QTest::addColumn<int>("boundaryType");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("expectedStart");
    QTest::addColumn<int>("expectedEnd");
    QTest::addColumn<QString>("expectedText");

    // -2 gives cursor position, -1 is length
    // invalid positions will return empty strings and either -1 and -1 or both the cursor position, both is fine
    QTest::newRow("char before -2") << "hello" << 0 << (int) QAccessible::CharBoundary << 3 << -2 << 2 << 3 << "l";
    QTest::newRow("char at -2") << "hello" << 1 << (int) QAccessible::CharBoundary << 3 << -2 << 3 << 4 << "l";
    QTest::newRow("char after -2") << "hello" << 2 << (int) QAccessible::CharBoundary << 3 << -2 << 4 << 5 << "o";
    QTest::newRow("char before -1") << "hello" << 0 << (int) QAccessible::CharBoundary << 3 << -1 << 4 << 5 << "o";
    QTest::newRow("char at -1") << "hello" << 1 << (int) QAccessible::CharBoundary << 3 << -1 << -1 << -1 << "";
    QTest::newRow("char after -1") << "hello" << 2 << (int) QAccessible::CharBoundary << 3 << -1 << -1 << -1 << "";
    QTest::newRow("char before 0") << "hello" << 0 << (int) QAccessible::CharBoundary << 0 << 0 << -1 << -1 << "";
    QTest::newRow("char at 0") << "hello" << 1 << (int) QAccessible::CharBoundary << 0 << 0 << 0 << 1 << "h";
    QTest::newRow("char after 0") << "hello" << 2 << (int) QAccessible::CharBoundary << 0 << 0 << 1 << 2 << "e";
    QTest::newRow("char before 1") << "hello" << 0 << (int) QAccessible::CharBoundary << 3 << 1 << 0 << 1 << "h";
    QTest::newRow("char at 1") << "hello" << 1 << (int) QAccessible::CharBoundary << 3 << 1 << 1 << 2 << "e";
    QTest::newRow("char after 1") << "hello" << 2 << (int) QAccessible::CharBoundary << 3 << 1 << 2 << 3 << "l";
    QTest::newRow("char before 4") << "hello" << 0 << (int) QAccessible::CharBoundary << 3 << 4 << 3 << 4 << "l";
    QTest::newRow("char at 4") << "hello" << 1 << (int) QAccessible::CharBoundary << 3 << 4 << 4 << 5 << "o";
    QTest::newRow("char after 4") << "hello" << 2 << (int) QAccessible::CharBoundary << 3 << 4 << -1 << -1 << "";
    QTest::newRow("char before 5") << "hello" << 0 << (int) QAccessible::CharBoundary << 3 << 5 << 4 << 5 << "o";
    QTest::newRow("char at 5") << "hello" << 1 << (int) QAccessible::CharBoundary << 3 << 5 << -1 << -1 << "";
    QTest::newRow("char after 5") << "hello" << 2 << (int) QAccessible::CharBoundary << 3 << 5 << -1 << -1 << "";
    QTest::newRow("char before 6") << "hello" << 0 << (int) QAccessible::CharBoundary << 3 << 6 << -1 << -1 << "";
    QTest::newRow("char at 6") << "hello" << 1 << (int) QAccessible::CharBoundary << 3 << 6 << -1 << -1 << "";
    QTest::newRow("char after 6") << "hello" << 2 << (int) QAccessible::CharBoundary << 3 << 6 << -1 << -1 << "";

    for (int i = -2; i < 6; ++i) {
        const QByteArray iB = QByteArray::number(i);
        QTest::newRow(("line before " + iB).constData())
                      << "hello" << 0 << (int) QAccessible::LineBoundary << 3 << i << -1 << -1 << "";
        QTest::newRow(("line at " + iB).constData())
                      << "hello" << 1 << (int) QAccessible::LineBoundary << 3 << i << 0 << 5 << "hello";
        QTest::newRow(("line after " + iB).constData())
                      << "hello" << 2 << (int) QAccessible::LineBoundary << 3 << i << -1 << -1 << "";
    }
}

void tst_QAccessibility::lineEditTextFunctions()
{
    {
    QFETCH(QString, text);
    QFETCH(int, textFunction);
    QFETCH(int, boundaryType);
    QFETCH(int, cursorPosition);
    QFETCH(int, offset);
    QFETCH(int, expectedStart);
    QFETCH(int, expectedEnd);
    QFETCH(QString, expectedText);

    QLineEdit le;
    le.show();
    le.setText(text);
    le.setCursorPosition(cursorPosition);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&le);
    QVERIFY(iface);
    QAccessibleTextInterface *textIface = iface->textInterface();
    QVERIFY(textIface);

    int start = -33;
    int end = -33;
    QString result;
    switch (textFunction) {
    case 0:
        result = textIface->textBeforeOffset(offset, (QAccessible::TextBoundaryType) boundaryType, &start, &end);
        break;
    case 1:
        result = textIface->textAtOffset(offset, (QAccessible::TextBoundaryType) boundaryType, &start, &end);
        break;
    case 2:
        result = textIface->textAfterOffset(offset, (QAccessible::TextBoundaryType) boundaryType, &start, &end);
        break;
    }
    QCOMPARE(result, expectedText);
    QCOMPARE(start, expectedStart);
    QCOMPARE(end, expectedEnd);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::textInterfaceTest_data()
{
    lineEditTextFunctions_data();
    QString hello = QStringLiteral("hello\nworld\nend");
    QTest::newRow("multi line at 0") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 0 << 0 << 6 << "hello\n";
    QTest::newRow("multi line at 1") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 1 << 0 << 6 << "hello\n";
    QTest::newRow("multi line at 2") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 2 << 0 << 6 << "hello\n";
    QTest::newRow("multi line at 5") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 5 << 0 << 6 << "hello\n";
    QTest::newRow("multi line at 6") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 6 << 6 << 12 << "world\n";
    QTest::newRow("multi line at 7") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 7 << 6 << 12 << "world\n";
    QTest::newRow("multi line at 8") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 8 << 6 << 12 << "world\n";
    QTest::newRow("multi line at 10") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 10 << 6 << 12 << "world\n";
    QTest::newRow("multi line at 11") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 11 << 6 << 12 << "world\n";
    QTest::newRow("multi line at 12") << hello << 1 << (int) QAccessible::LineBoundary << 0 << 12 << 12 << 15 << "end";

    QTest::newRow("multi line before 0") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 0 << -1 << -1 << "";
    QTest::newRow("multi line before 1") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 1 << -1 << -1 << "";
    QTest::newRow("multi line before 2") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 2 << -1 << -1 << "";
    QTest::newRow("multi line before 5") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 5 << -1 << -1 << "";
    QTest::newRow("multi line before 6") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 6 << 0 << 6 << "hello\n";
    QTest::newRow("multi line before 7") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 7 << 0 << 6 << "hello\n";
    QTest::newRow("multi line before 8") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 8 << 0 << 6 << "hello\n";
    QTest::newRow("multi line before 10") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 10 << 0 << 6 << "hello\n";
    QTest::newRow("multi line before 11") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 11 << 0 << 6 << "hello\n";
    QTest::newRow("multi line before 12") << hello << 0 << (int) QAccessible::LineBoundary << 0 << 12 << 6 << 12 << "world\n";

    QTest::newRow("multi line after 0") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 0 << 6 << 12 << "world\n";
    QTest::newRow("multi line after 1") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 1 << 6 << 12 << "world\n";
    QTest::newRow("multi line after 2") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 2 << 6 << 12 << "world\n";
    QTest::newRow("multi line after 5") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 5 << 6 << 12 << "world\n";
    QTest::newRow("multi line after 6") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 6 << 12 << 15 << "end";
    QTest::newRow("multi line after 7") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 7 << 12 << 15 << "end";
    QTest::newRow("multi line after 8") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 8 << 12 << 15 << "end";
    QTest::newRow("multi line after 10") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 10 << 12 << 15 << "end";
    QTest::newRow("multi line after 11") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 11 << 12 << 15 << "end";
    QTest::newRow("multi line after 12") << hello << 2 << (int) QAccessible::LineBoundary << 0 << 12 << -1 << -1 << "";

    QTest::newRow("before 4 \\nFoo\\n") << QStringLiteral("\nFoo\n") << 0 << (int) QAccessible::LineBoundary << 0 << 4 << 0 << 1 << "\n";
    QTest::newRow("at 4 \\nFoo\\n") << QStringLiteral("\nFoo\n") << 1 << (int) QAccessible::LineBoundary << 0 << 4 << 1 << 5 << "Foo\n";
    QTest::newRow("after 4 \\nFoo\\n") << QStringLiteral("\nFoo\n") << 2 << (int) QAccessible::LineBoundary << 0 << 4 << 5 << 5 << "";
    QTest::newRow("before 4 Foo\\nBar\\n") << QStringLiteral("Foo\nBar\n") << 0 << (int) QAccessible::LineBoundary << 0 << 7 << 0 << 4 << "Foo\n";
    QTest::newRow("at 4 Foo\\nBar\\n") << QStringLiteral("Foo\nBar\n") << 1 << (int) QAccessible::LineBoundary << 0 << 7 << 4 << 8 << "Bar\n";
    QTest::newRow("after 4 Foo\\nBar\\n") << QStringLiteral("Foo\nBar\n") << 2 << (int) QAccessible::LineBoundary << 0 << 7 << 8 << 8 << "";
    QTest::newRow("at 0 Foo\\n") << QStringLiteral("Foo\n") << 1 << (int) QAccessible::LineBoundary << 0 << 0 << 0 << 4 << "Foo\n";
}

void tst_QAccessibility::textInterfaceTest()
{
    QFETCH(QString, text);
    QFETCH(int, textFunction);
    QFETCH(int, boundaryType);
    QFETCH(int, cursorPosition);
    QFETCH(int, offset);
    QFETCH(int, expectedStart);
    QFETCH(int, expectedEnd);
    QFETCH(QString, expectedText);

    QAccessible::installFactory(CustomTextWidgetIface::ifaceFactory);
    auto w = std::make_unique<CustomTextWidget>();
    w->text = text;
    w->cursorPosition = cursorPosition;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(w.get());
    QVERIFY(iface);
    QCOMPARE(iface->text(QAccessible::Value), text);
    QAccessibleTextInterface *textIface = iface->textInterface();
    QVERIFY(textIface);

    int start = -33;
    int end = -33;
    QString result;
    switch (textFunction) {
    case 0:
        result = textIface->textBeforeOffset(offset, (QAccessible::TextBoundaryType) boundaryType, &start, &end);
        break;
    case 1:
        result = textIface->textAtOffset(offset, (QAccessible::TextBoundaryType) boundaryType, &start, &end);
        break;
    case 2:
        result = textIface->textAfterOffset(offset, (QAccessible::TextBoundaryType) boundaryType, &start, &end);
        break;
    }

    QCOMPARE(result, expectedText);
    QCOMPARE(start, expectedStart);
    QCOMPARE(end, expectedEnd);

    QAccessible::removeFactory(CustomTextWidgetIface::ifaceFactory);
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::groupBoxTest()
{
    {
    auto gbHolder = std::make_unique<QGroupBox>();
    auto groupBox = gbHolder.get();
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(groupBox);

    groupBox->setTitle(QLatin1String("Test QGroupBox"));

    QAccessibleEvent ev(groupBox, QAccessible::NameChanged);
    QVERIFY_EVENT(&ev);

    groupBox->setToolTip(QLatin1String("This group box will be used to test accessibility"));
    QVBoxLayout *layout = new QVBoxLayout();
    QRadioButton *rbutton = new QRadioButton();
    layout->addWidget(rbutton);
    groupBox->setLayout(layout);
    QAccessibleInterface *rButtonIface = QAccessible::queryAccessibleInterface(rbutton);

    QCOMPARE(iface->childCount(), 1);
    QCOMPARE(iface->role(), QAccessible::Grouping);
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("Test QGroupBox"));
    QCOMPARE(iface->text(QAccessible::Description), QLatin1String("This group box will be used to test accessibility"));
    QList<QPair<QAccessibleInterface *, QAccessible::Relation>> relations =
            rButtonIface->relations();
    QCOMPARE(relations.size(), 1);
    QPair<QAccessibleInterface*, QAccessible::Relation> relation = relations.first();
    QCOMPARE(relation.first->object(), groupBox);
    QCOMPARE(relation.second, QAccessible::Label);
    }

    {
    auto gbHolder = std::make_unique<QGroupBox>();
    auto groupBox = gbHolder.get();
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(groupBox);
    QVERIFY(!iface->state().checkable);
    groupBox->setCheckable(true);

    groupBox->setChecked(false);
    QAccessible::State st;
    st.checked = true;
    QAccessibleStateChangeEvent ev(groupBox, st);
    QVERIFY_EVENT(&ev);

    QCOMPARE(iface->role(), QAccessible::CheckBox);
    QAccessibleActionInterface *actionIface = iface->actionInterface();
    QVERIFY(actionIface);
    QAccessible::State state = iface->state();
    QVERIFY(state.checkable);
    QVERIFY(!state.checked);
    QVERIFY(actionIface->actionNames().contains(QAccessibleActionInterface::toggleAction()));
    actionIface->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(groupBox->isChecked());
    state = iface->state();
    QVERIFY(state.checked);
    QAccessibleStateChangeEvent ev2(groupBox, st);
    QVERIFY_EVENT(&ev2);
    }
}

bool accessibleInterfaceLeftOf(const QAccessibleInterface *a1, const QAccessibleInterface *a2)
{
    return a1->rect().x() < a2->rect().x();
}

bool accessibleInterfaceAbove(const QAccessibleInterface *a1, const QAccessibleInterface *a2)
{
    return a1->rect().y() < a2->rect().y();
}

void tst_QAccessibility::dialogButtonBoxTest()
{
    {
    QDialogButtonBox box(QDialogButtonBox::Reset |
                         QDialogButtonBox::Help |
                         QDialogButtonBox::Ok, Qt::Horizontal);


    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&box);
    QVERIFY(iface);
    box.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    QApplication::processEvents();
    QCOMPARE(iface->childCount(), 3);
    QCOMPARE(iface->role(), QAccessible::Grouping);
    QStringList actualOrder;
    QAccessibleInterface *child;
    child = iface->child(0);
    QCOMPARE(child->role(), QAccessible::PushButton);

    QList<QAccessibleInterface *> buttons;
    for (int i = 0; i < iface->childCount(); ++i)
        buttons <<  iface->child(i);

    std::sort(buttons.begin(), buttons.end(), accessibleInterfaceLeftOf);

    for (int i = 0; i < buttons.size(); ++i)
        actualOrder << buttons.at(i)->text(QAccessible::Name);

    QStringList expectedOrder;
    QDialogButtonBox::ButtonLayout btnlout =
        QDialogButtonBox::ButtonLayout(QApplication::style()->styleHint(QStyle::SH_DialogButtonLayout));
    switch (btnlout) {
    case QDialogButtonBox::WinLayout:
        expectedOrder << QDialogButtonBox::tr("Reset")
                      << QDialogButtonBox::tr("OK")
                      << QDialogButtonBox::tr("Help");
        break;
    case QDialogButtonBox::GnomeLayout:
    case QDialogButtonBox::KdeLayout:
    case QDialogButtonBox::MacLayout:
    case QDialogButtonBox::AndroidLayout:
        expectedOrder << QDialogButtonBox::tr("Help")
                      << QDialogButtonBox::tr("Reset")
                      << QDialogButtonBox::tr("OK");
        break;
    }
    QCOMPARE(actualOrder, expectedOrder);
    QApplication::processEvents();
    QTestAccessibility::clearEvents();
    }

    {
    QDialogButtonBox box(QDialogButtonBox::Reset |
                         QDialogButtonBox::Help |
                         QDialogButtonBox::Ok, Qt::Horizontal);
    setFrameless(&box);


    // Test up and down navigation
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&box);
    QVERIFY(iface);
    box.setOrientation(Qt::Vertical);
    box.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    QApplication::processEvents();
    QStringList actualOrder;

    QList<QAccessibleInterface *> buttons;
    for (int i = 0; i < iface->childCount(); ++i)
        buttons <<  iface->child(i);

    std::sort(buttons.begin(), buttons.end(), accessibleInterfaceAbove);

    for (int i = 0; i < buttons.size(); ++i)
        actualOrder << buttons.at(i)->text(QAccessible::Name);

    QStringList expectedOrder;
    expectedOrder << QDialogButtonBox::tr("OK")
                  << QDialogButtonBox::tr("Reset")
                  << QDialogButtonBox::tr("Help");

    QCOMPARE(actualOrder, expectedOrder);
    QApplication::processEvents();

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::dialTest()
{
    {
    QDial dial;
    setFrameless(&dial);
    dial.setMinimum(23);
    dial.setMaximum(121);
    dial.setValue(42);
    QCOMPARE(dial.value(), 42);
    dial.show();

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&dial);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 0);

    QVERIFY(QTest::qWaitForWindowExposed(&dial));

    QCOMPARE(interface->text(QAccessible::Value), QString::number(dial.value()));
    QCOMPARE(interface->rect(), dial.geometry());

    QAccessibleValueInterface *valueIface = interface->valueInterface();
    QVERIFY(valueIface != 0);
    QCOMPARE(valueIface->minimumValue().toInt(), dial.minimum());
    QCOMPARE(valueIface->maximumValue().toInt(), dial.maximum());
    QCOMPARE(valueIface->currentValue().toInt(), 42);
    dial.setValue(50);
    QCOMPARE(valueIface->currentValue().toInt(), dial.value());
    dial.setValue(0);
    QCOMPARE(valueIface->currentValue().toInt(), dial.value());
    dial.setValue(100);
    QCOMPARE(valueIface->currentValue().toInt(), dial.value());
    valueIface->setCurrentValue(77);
    QCOMPARE(77, dial.value());
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::rubberBandTest()
{
    QRubberBand rubberBand(QRubberBand::Rectangle);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&rubberBand);
    QVERIFY(interface);
    QCOMPARE(interface->role(), QAccessible::Border);
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::abstractScrollAreaTest()
{
    {
    QAbstractScrollArea abstractScrollArea;

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&abstractScrollArea);
    QVERIFY(interface);
    QVERIFY(!interface->rect().isValid());
    QCOMPARE(interface->childAt(200, 200), static_cast<QAccessibleInterface*>(0));

    abstractScrollArea.resize(400, 400);
    abstractScrollArea.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif
    const QRect globalGeometry = QRect(abstractScrollArea.mapToGlobal(QPoint(0, 0)),
                                       abstractScrollArea.size());

    // Viewport.
    QCOMPARE(interface->childCount(), 1);
    QWidget *viewport = abstractScrollArea.viewport();
    QVERIFY(viewport);
    QVERIFY(verifyChild(viewport, interface, 0, globalGeometry));

    // Horizontal scrollBar.
    abstractScrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QWidget *horizontalScrollBar = abstractScrollArea.horizontalScrollBar();

    // On OS X >= 10.9 the scrollbar will be hidden unless explicitly enabled in the preferences
    bool scrollBarsVisible = !horizontalScrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, 0, horizontalScrollBar);
    int childCount = scrollBarsVisible ? 2 : 1;
    QCOMPARE(interface->childCount(), childCount);
    QWidget *horizontalScrollBarContainer = horizontalScrollBar->parentWidget();
    if (scrollBarsVisible)
        QVERIFY(verifyChild(horizontalScrollBarContainer, interface, 1, globalGeometry));

    // Horizontal scrollBar widgets.
    QLabel *secondLeftLabel = new QLabel(QLatin1String("L2"));
    abstractScrollArea.addScrollBarWidget(secondLeftLabel, Qt::AlignLeft);
    QCOMPARE(interface->childCount(), childCount);

    QLabel *firstLeftLabel = new QLabel(QLatin1String("L1"));
    abstractScrollArea.addScrollBarWidget(firstLeftLabel, Qt::AlignLeft);
    QCOMPARE(interface->childCount(), childCount);

    QLabel *secondRightLabel = new QLabel(QLatin1String("R2"));
    abstractScrollArea.addScrollBarWidget(secondRightLabel, Qt::AlignRight);
    QCOMPARE(interface->childCount(), childCount);

    QLabel *firstRightLabel = new QLabel(QLatin1String("R1"));
    abstractScrollArea.addScrollBarWidget(firstRightLabel, Qt::AlignRight);
    QCOMPARE(interface->childCount(), childCount);

    // Vertical scrollBar.
    abstractScrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    if (scrollBarsVisible)
        ++childCount;
    QCOMPARE(interface->childCount(), childCount);
    QWidget *verticalScrollBar = abstractScrollArea.verticalScrollBar();
    QWidget *verticalScrollBarContainer = verticalScrollBar->parentWidget();
    if (scrollBarsVisible)
        QVERIFY(verifyChild(verticalScrollBarContainer, interface, 2, globalGeometry));

    // Vertical scrollBar widgets.
    QLabel *secondTopLabel = new QLabel(QLatin1String("T2"));
    abstractScrollArea.addScrollBarWidget(secondTopLabel, Qt::AlignTop);
    QCOMPARE(interface->childCount(), childCount);

    QLabel *firstTopLabel = new QLabel(QLatin1String("T1"));
    abstractScrollArea.addScrollBarWidget(firstTopLabel, Qt::AlignTop);
    QCOMPARE(interface->childCount(), childCount);

    QLabel *secondBottomLabel = new QLabel(QLatin1String("B2"));
    abstractScrollArea.addScrollBarWidget(secondBottomLabel, Qt::AlignBottom);
    QCOMPARE(interface->childCount(), childCount);

    QLabel *firstBottomLabel = new QLabel(QLatin1String("B1"));
    abstractScrollArea.addScrollBarWidget(firstBottomLabel, Qt::AlignBottom);
    QCOMPARE(interface->childCount(), childCount);

    // CornerWidget.
    ++childCount;
    abstractScrollArea.setCornerWidget(new QLabel(QLatin1String("C")));
    QCOMPARE(interface->childCount(), childCount);
    QWidget *cornerWidget = abstractScrollArea.cornerWidget();
    if (scrollBarsVisible)
        QVERIFY(verifyChild(cornerWidget, interface, 3, globalGeometry));

    QCOMPARE(verifyHierarchy(interface), 0);

    }

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::scrollAreaTest()
{
    {
    QScrollArea scrollArea;
    scrollArea.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&scrollArea);
    QVERIFY(interface);
    QCOMPARE(interface->childCount(), 1); // The viewport.
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::listTest()
{
    {
    const auto modelHolder = std::make_unique<QStandardItemModel>();
    auto model = modelHolder.get();
    model->appendRow({new QStandardItem("Norway"), new QStandardItem("Oslo"), new QStandardItem("NOK")});
    model->appendRow({new QStandardItem("Germany"), new QStandardItem("Berlin"), new QStandardItem("EUR")});
    model->appendRow({new QStandardItem("Australia"), new QStandardItem("Brisbane"), new QStandardItem("AUD")});
    auto lvHolder = std::make_unique<QListView>();
    auto listView = lvHolder.get();
    listView->setModel(model);
    listView->setModelColumn(1);
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listView->resize(400,400);
    listView->show();
    QVERIFY(QTest::qWaitForWindowExposed(listView));

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(listView);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE((int)iface->role(), (int)QAccessible::List);
    QCOMPARE(iface->childCount(), 3);

    {
    QAccessibleInterface *child1 = iface->child(0);
    QVERIFY(child1);
    QCOMPARE(iface->indexOfChild(child1), 0);
    QCOMPARE(child1->text(QAccessible::Name), QString("Oslo"));
    QCOMPARE(child1->role(), QAccessible::ListItem);

    QAccessibleInterface *child2 = iface->child(1);
    QVERIFY(child2);
    QCOMPARE(iface->indexOfChild(child2), 1);
    QCOMPARE(child2->text(QAccessible::Name), QString("Berlin"));

    QAccessibleInterface *child3 = iface->child(2);
    QVERIFY(child3);
    QCOMPARE(iface->indexOfChild(child3), 2);
    QCOMPARE(child3->text(QAccessible::Name), QString("Brisbane"));
    }

    // Check that application is accessible parent, since it's a top-level widget
    QAccessibleInterface *parentIface = iface->parent();
    QVERIFY(parentIface);
    QVERIFY(parentIface->role() == QAccessible::Application);

    QTestAccessibility::clearEvents();

    // Check for events
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, { }, listView->visualRect(model->index(1, listView->modelColumn())).center());
    QAccessibleEvent selectionEvent(listView, QAccessible::SelectionAdd);
    selectionEvent.setChild(1);

    QVERIFY(QTestAccessibility::containsEvent(&selectionEvent));
    // skip focus event tests on platforms where window focus cannot be ensured
    const bool checkFocus = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation);
    if (checkFocus) {
        QAccessibleEvent focusEvent(listView, QAccessible::Focus);
        focusEvent.setChild(1);
        QVERIFY(QTestAccessibility::containsEvent(&focusEvent));
    }

    QTest::mouseClick(listView->viewport(), Qt::LeftButton, { }, listView->visualRect(model->index(2, listView->modelColumn())).center());

    QAccessibleEvent selectionEvent2(listView, QAccessible::SelectionAdd);
    selectionEvent2.setChild(2);
    QVERIFY(QTestAccessibility::containsEvent(&selectionEvent2));
    if (checkFocus) {
        QAccessibleEvent focusEvent2(listView, QAccessible::Focus);
        focusEvent2.setChild(2);
        QVERIFY(QTestAccessibility::containsEvent(&focusEvent2));
    }

    QAccessibleTableInterface *table = iface->tableInterface();
    QAccessibleInterface *cell3 = table->cellAt(2, 0);
    QVERIFY(cell3->tableCellInterface()->isSelected());
    QCOMPARE(table->selectedCellCount(), 1);
    QCOMPARE(table->selectedCells(), {cell3});

    QAccessibleSelectionInterface *selection = iface->selectionInterface();
    QCOMPARE(selection->selectedItemCount(), 1);
    QCOMPARE(selection->selectedItems(), {cell3});
    QVERIFY(selection->isSelected(cell3));

    model->appendRow({new QStandardItem("Germany"), new QStandardItem("Munich"), new QStandardItem("EUR")});
    QCOMPARE(iface->childCount(), 4);

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 1);
    QCOMPARE(table2->rowCount(), 4);
    QAccessibleInterface *cell1 = table2->cellAt(0,0);
    QVERIFY(cell1);
    QCOMPARE(cell1->text(QAccessible::Name), QString("Oslo"));

    QAccessibleInterface *cell4 = table2->cellAt(3,0);
    QVERIFY(cell4);
    QCOMPARE(cell4->text(QAccessible::Name), QString("Munich"));
    QCOMPARE(cell4->role(), QAccessible::ListItem);

    QAccessibleTableCellInterface *cellInterface = cell4->tableCellInterface();
    QVERIFY(cellInterface);
    QCOMPARE(cellInterface->rowIndex(), 3);
    QCOMPARE(cellInterface->columnIndex(), 0);
    QCOMPARE(cellInterface->rowExtent(), 1);
    QCOMPARE(cellInterface->columnExtent(), 1);
    QVERIFY(cellInterface->rowHeaderCells().isEmpty());
    QVERIFY(cellInterface->columnHeaderCells().isEmpty());

    QCOMPARE(cellInterface->table()->object(), listView);

    listView->clearSelection();
    QVERIFY(!(cell4->state().expandable));
    QVERIFY( (cell4->state().selectable));
    QVERIFY(!(cell4->state().selected));
    QAccessibleSelectionInterface *selection2 = iface->selectionInterface();
    selection2->select(cell4);
    QCOMPARE(listView->selectionModel()->selectedIndexes().size(), 1);
    QCOMPARE(model->itemFromIndex(listView->selectionModel()->selectedIndexes().at(0))->text(), QLatin1String("Munich"));
    QVERIFY(cell4->state().selected);
    QVERIFY(cellInterface->isSelected());

    selection2->clear();
    QVERIFY(!listView->selectionModel()->hasSelection());
    QVERIFY(!cell4->state().selected);
    QVERIFY(!cellInterface->isSelected());

    selection2->selectAll();
    QCOMPARE(listView->selectionModel()->selectedIndexes().size(), 12);
    QCOMPARE(table2->selectedCellCount(), 4);

    QVERIFY(table2->cellAt(-1, 0) == 0);
    QVERIFY(table2->cellAt(0, -1) == 0);
    QVERIFY(table2->cellAt(0, 1) == 0);
    QVERIFY(table2->cellAt(4, 0) == 0);

    // verify that unique id stays the same
    QAccessible::Id axidMunich = QAccessible::uniqueId(cell4);
    // insertion and deletion of items
    model->insertRow(1, {new QStandardItem("Finland"), new QStandardItem("Helsinki"), new QStandardItem("EUR")});
    // list: Oslo, Helsinki, Berlin, Brisbane, Munich

    QAccessibleInterface *cellMunich2 = table2->cellAt(4,0);
    QCOMPARE(cell4, cellMunich2);
    QCOMPARE(axidMunich, QAccessible::uniqueId(cellMunich2));

    for (auto item : model->takeRow(2))
        delete item;
    for (auto item : model->takeRow(2))
        delete item;
    // list: Oslo, Helsinki, Munich

    QAccessibleInterface *cellMunich3 = table2->cellAt(2,0);
    QCOMPARE(cell4, cellMunich3);
    QCOMPARE(axidMunich, QAccessible::uniqueId(cellMunich3));
    for (auto item : model->takeRow(2))
        delete item;
    // list: Oslo, Helsinki
    // verify that it doesn't return an invalid item from the cache
    QVERIFY(table2->cellAt(2,0) == 0);
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::treeTest()
{
    auto treeView = std::make_unique<QTreeWidget>();

    // Empty model (do not crash, etc)
    treeView->setColumnCount(0);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(treeView.get());
    QCOMPARE(iface->child(0), static_cast<QAccessibleInterface*>(0));

    treeView->setColumnCount(2);
    QTreeWidgetItem *header = new QTreeWidgetItem;
    header->setText(0, "Artist");
    header->setText(1, "Work");
    treeView->setHeaderItem(header);

    QTreeWidgetItem *root1 = new QTreeWidgetItem;
    root1->setText(0, "Spain");
    treeView->addTopLevelItem(root1);

    QTreeWidgetItem *item1 = new QTreeWidgetItem;
    item1->setText(0, "Picasso");
    item1->setText(1, "Guernica");
    root1->addChild(item1);

    QTreeWidgetItem *item2 = new QTreeWidgetItem;
    item2->setText(0, "Tapies");
    item2->setText(1, "Ambrosia");
    root1->addChild(item2);

    QTreeWidgetItem *root2 = new QTreeWidgetItem;
    root2->setText(0, "Austria");
    treeView->addTopLevelItem(root2);

    QTreeWidgetItem *item3 = new QTreeWidgetItem;
    item3->setText(0, "Klimt");
    item3->setText(1, "The Kiss");
    root2->addChild(item3);

    treeView->resize(400,400);
    treeView->show();

    QCoreApplication::processEvents();
    QTest::qWait(100);

    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE((int)iface->role(), (int)QAccessible::Tree);
    // header and 2 rows (the others are not expanded, thus not visible)
    QCOMPARE(iface->childCount(), 6);

    QAccessibleInterface *header1 = iface->child(0);
    QVERIFY(header1);
    QCOMPARE(iface->indexOfChild(header1), 0);
    QCOMPARE(header1->text(QAccessible::Name), QString("Artist"));
    QCOMPARE(header1->role(), QAccessible::ColumnHeader);

    QAccessibleInterface *child1 = iface->child(2);
    QVERIFY(child1);
    QCOMPARE(iface->indexOfChild(child1), 2);
    QCOMPARE(child1->text(QAccessible::Name), QString("Spain"));
    QCOMPARE(child1->role(), QAccessible::TreeItem);
    QVERIFY(!(child1->state().expanded));

    QAccessibleInterface *child2 = 0;
    child2 = iface->child(4);
    QVERIFY(child2);
    QCOMPARE(iface->indexOfChild(child2), 4);
    QCOMPARE(child2->text(QAccessible::Name), QString("Austria"));

    bool headerHidden = true;
    do {
        treeView->setHeaderHidden(headerHidden);
        header1 = iface->child(0);
        QCOMPARE(header1->role(), QAccessible::ColumnHeader);
        QCOMPARE(!!header1->state().invisible, headerHidden);
        QCOMPARE(header1->text(QAccessible::Name), QStringLiteral("Artist"));
        header1 = iface->child(1);
        QCOMPARE(header1->role(), QAccessible::ColumnHeader);
        QCOMPARE(!!header1->state().invisible, headerHidden);
        QCOMPARE(header1->text(QAccessible::Name), QStringLiteral("Work"));

        QAccessibleInterface *accSpain = iface->child(2);
        QCOMPARE(accSpain->role(), QAccessible::TreeItem);
        QCOMPARE(iface->indexOfChild(accSpain), 2);
        headerHidden = !headerHidden;
    } while (!headerHidden);

    QTestAccessibility::clearEvents();

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 2);
    QCOMPARE(table2->rowCount(), 2);
    QAccessibleInterface *cell1 = table2->cellAt(0,0);
    QVERIFY(cell1);
    QCOMPARE(cell1->text(QAccessible::Name), QString("Spain"));
    QAccessibleInterface *cell2 = table2->cellAt(1,0);
    QVERIFY(cell2);
    QCOMPARE(cell2->text(QAccessible::Name), QString("Austria"));
    QCOMPARE(cell2->role(), QAccessible::TreeItem);
    QCOMPARE(cell2->tableCellInterface()->rowIndex(), 1);
    QCOMPARE(cell2->tableCellInterface()->columnIndex(), 0);
    QVERIFY(cell2->state().expandable);
    QCOMPARE(iface->indexOfChild(cell2), 4);
    QVERIFY(!(cell2->state().expanded));
    QCOMPARE(table2->columnDescription(1), QString("Work"));

    treeView->expandAll();

    // Need this for indexOfchild to work.
    QCoreApplication::processEvents();
    QTest::qWait(100);

    QCOMPARE(table2->columnCount(), 2);
    QCOMPARE(table2->rowCount(), 5);
    cell1 = table2->cellAt(1,0);
    QCOMPARE(cell1->text(QAccessible::Name), QString("Picasso"));
    QCOMPARE(iface->indexOfChild(cell1), 4); // 2 header + 2 for root item

    cell2 = table2->cellAt(4,0);
    QCOMPARE(cell2->text(QAccessible::Name), QString("Klimt"));
    QCOMPARE(cell2->role(), QAccessible::TreeItem);
    QCOMPARE(cell2->tableCellInterface()->rowIndex(), 4);
    QCOMPARE(cell2->tableCellInterface()->columnIndex(), 0);
    QVERIFY(!(cell2->state().expandable));
    QCOMPARE(iface->indexOfChild(cell2), 10);

    QPoint pos = treeView->mapToGlobal(QPoint(0,0));
    QModelIndex index = treeView->model()->index(0, 0, treeView->model()->index(1, 0));
    pos += treeView->visualRect(index).center();
    pos += QPoint(0, treeView->header()->height());
    QAccessibleInterface *childAt2(iface->childAt(pos.x(), pos.y()));
    QVERIFY(childAt2);
    QCOMPARE(childAt2->text(QAccessible::Name), QString("Klimt"));

    QCOMPARE(table2->columnDescription(0), QString("Artist"));
    QCOMPARE(table2->columnDescription(1), QString("Work"));

    QTestAccessibility::clearEvents();
}

// The table used below is this:
// Button  (0) | h1   (1) | h2   (2) | h3   (3)
// v1      (4) | 0.0  (5) | 1.0  (6) | 2.0  (7)
// v2      (8) | 0.1  (9) | 1.1 (10) | 2.1 (11)
// v3     (12) | 0.2 (13) | 1.2 (14) | 2.2 (15)
void tst_QAccessibility::tableTest()
{
    auto tvHolder = std::make_unique<QTableWidget>(3, 3);
    auto tableView = tvHolder.get();
    tableView->setColumnCount(3);
    QStringList hHeader;
    hHeader << "h1" << "h2" << "h3";
    tableView->setHorizontalHeaderLabels(hHeader);

    QStringList vHeader;
    vHeader << "v1" << "v2" << "v3";
    tableView->setVerticalHeaderLabels(vHeader);

    for (int i = 0; i<9; ++i) {
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setText(QString::number(i/3) + QString(".") + QString::number(i%3));
        tableView->setItem(i/3, i%3, item);
    }

    tableView->resize(600,600);
    tableView->show();
    QVERIFY(QTest::qWaitForWindowExposed(tableView));

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(tableView);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE(iface->role(), QAccessible::Table);
    // header and 2 rows (the others are not expanded, thus not visible)
    QCOMPARE(iface->childCount(), 9+3+3+1); // cell+headers+topleft button

    QAccessibleInterface *cornerButton(iface->child(0));
    QVERIFY(cornerButton);
    QCOMPARE(iface->indexOfChild(cornerButton), 0);
    QCOMPARE(cornerButton->role(), QAccessible::Pane);

    QAccessibleInterface *h2(iface->child(2));
    QVERIFY(h2);
    QCOMPARE(iface->indexOfChild(h2), 2);
    QCOMPARE(h2->text(QAccessible::Name), QString("h2"));
    QCOMPARE(h2->role(), QAccessible::ColumnHeader);
    QVERIFY(!(h2->state().expanded));

    QAccessibleInterface *v3(iface->child(12));
    QVERIFY(v3);
    QCOMPARE(iface->indexOfChild(v3), 12);
    QCOMPARE(v3->text(QAccessible::Name), QString("v3"));
    QCOMPARE(v3->role(), QAccessible::RowHeader);
    QVERIFY(!(v3->state().expanded));


    QAccessibleInterface *child10(iface->child(10));
    QVERIFY(child10);
    QCOMPARE(iface->indexOfChild(child10), 10);
    QCOMPARE(child10->text(QAccessible::Name), QString("1.1"));
    QAccessibleTableCellInterface *cell10Iface = child10->tableCellInterface();
    QCOMPARE(cell10Iface->rowIndex(), 1);
    QCOMPARE(cell10Iface->columnIndex(), 1);
    QPoint pos = tableView->mapToGlobal(QPoint(0,0));
    pos += tableView->visualRect(tableView->model()->index(1, 1)).center();
    pos += QPoint(tableView->verticalHeader()->width(), tableView->horizontalHeader()->height());
    QAccessibleInterface *childAt10(iface->childAt(pos.x(), pos.y()));
    QCOMPARE(childAt10->text(QAccessible::Name), QString("1.1"));

    QAccessibleInterface *child11(iface->child(11));
    QCOMPARE(iface->indexOfChild(child11), 11);
    QCOMPARE(child11->text(QAccessible::Name), QString("1.2"));


    QTestAccessibility::clearEvents();

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 3);
    QCOMPARE(table2->rowCount(), 3);
    QAccessibleInterface *cell1 = table2->cellAt(0,0);
    QVERIFY(cell1);
    QCOMPARE(cell1->text(QAccessible::Name), QString("0.0"));
    QCOMPARE(iface->indexOfChild(cell1), 5);

    QAccessibleInterface *cell2(table2->cellAt(0,1));
    QVERIFY(cell2);
    QCOMPARE(cell2->text(QAccessible::Name), QString("0.1"));
    QCOMPARE(cell2->role(), QAccessible::Cell);
    QCOMPARE(cell2->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell2->tableCellInterface()->columnIndex(), 1);
    QCOMPARE(iface->indexOfChild(cell2), 6);

    QAccessibleInterface *cell3(table2->cellAt(1,2));
    QVERIFY(cell3);
    QCOMPARE(cell3->text(QAccessible::Name), QString("1.2"));
    QCOMPARE(cell3->role(), QAccessible::Cell);
    QCOMPARE(cell3->tableCellInterface()->rowIndex(), 1);
    QCOMPARE(cell3->tableCellInterface()->columnIndex(), 2);
    QCOMPARE(iface->indexOfChild(cell3), 11);

    QCOMPARE(table2->columnDescription(0), QString("h1"));
    QCOMPARE(table2->columnDescription(1), QString("h2"));
    QCOMPARE(table2->columnDescription(2), QString("h3"));
    QCOMPARE(table2->rowDescription(0), QString("v1"));
    QCOMPARE(table2->rowDescription(1), QString("v2"));
    QCOMPARE(table2->rowDescription(2), QString("v3"));

    tableView->clearSelection();
    tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    QVERIFY(!table2->selectRow(0));
    QVERIFY(!table2->isRowSelected(0));
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    QVERIFY(table2->selectRow(0));
    QVERIFY(table2->selectRow(1));
    QVERIFY(!table2->isRowSelected(0));
    tableView->setSelectionMode(QAbstractItemView::MultiSelection);
    QVERIFY(table2->selectRow(0));
    QVERIFY(table2->isRowSelected(1));
    QVERIFY(table2->unselectRow(0));
    QVERIFY(!table2->isRowSelected(0));
    tableView->setSelectionBehavior(QAbstractItemView::SelectColumns);
    QVERIFY(!table2->selectRow(0));
    QVERIFY(!table2->isRowSelected(0));
    tableView->clearSelection();
    QCOMPARE(table2->selectedColumnCount(), 0);
    QCOMPARE(table2->selectedRowCount(), 0);
    QVERIFY(table2->selectColumn(1));
    QVERIFY(table2->isColumnSelected(1));
    tableView->clearSelection();
    tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    table2->selectColumn(0);
    table2->selectColumn(2);
    QVERIFY(!(table2->isColumnSelected(2) && table2->isColumnSelected(0)));
    tableView->clearSelection();
    tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableView->setSelectionMode(QAbstractItemView::MultiSelection);
    table2->selectColumn(1);
    table2->selectRow(1);
    QVERIFY(table2->isColumnSelected(1));
    QVERIFY(table2->isRowSelected(1));

    QAccessibleInterface *cell4 = table2->cellAt(2,2);
    QVERIFY(cell1->actionInterface());
    QVERIFY(cell1->tableCellInterface());

    tableView->clearSelection();
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    QVERIFY(!cell1->tableCellInterface()->isSelected());
    QVERIFY(cell1->actionInterface()->actionNames().contains(QAccessibleActionInterface::toggleAction()));
    cell1->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(cell2->tableCellInterface()->isSelected());

    tableView->clearSelection();
    tableView->setSelectionBehavior(QAbstractItemView::SelectColumns);
    cell3->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(cell4->tableCellInterface()->isSelected());

    tableView->clearSelection();
    tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    cell1->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(cell1->tableCellInterface()->isSelected());
    cell2->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(!cell1->tableCellInterface()->isSelected());

    tableView->clearSelection();
    tableView->setSelectionMode(QAbstractItemView::MultiSelection);
    cell1->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    cell2->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(cell1->tableCellInterface()->isSelected());
    QVERIFY(cell2->tableCellInterface()->isSelected());
    cell2->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
    QVERIFY(cell1->tableCellInterface()->isSelected());
    QVERIFY(!cell2->tableCellInterface()->isSelected());

    QAccessibleInterface *cell00 = table2->cellAt(0, 0);
    QAccessible::Id id00 = QAccessible::uniqueId(cell00);
    QVERIFY(id00);
    QCOMPARE(cell00->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell00->tableCellInterface()->columnIndex(), 0);

    QAccessibleInterface *cell01 = table2->cellAt(0, 1);

    QAccessibleInterface *cell02 = table2->cellAt(0, 2);
    QAccessible::Id id02 = QAccessible::uniqueId(cell02);
    QVERIFY(id02);
    QCOMPARE(cell02->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell02->tableCellInterface()->columnIndex(), 2);

    QAccessibleInterface *cell20 = table2->cellAt(2, 0);
    QAccessible::Id id20 = QAccessible::uniqueId(cell20);
    QVERIFY(id20);
    QCOMPARE(cell20->tableCellInterface()->rowIndex(), 2);
    QCOMPARE(cell20->tableCellInterface()->columnIndex(), 0);

    QAccessibleInterface *cell22 = table2->cellAt(2, 2);
    QAccessible::Id id22 = QAccessible::uniqueId(cell22);
    QVERIFY(id22);
    QCOMPARE(cell22->tableCellInterface()->rowIndex(), 2);
    QCOMPARE(cell22->tableCellInterface()->columnIndex(), 2);

    // modification: inserting and removing rows/columns
    tableView->insertRow(2);
    // Button  (0) | h1   (1) | h2   (2) | h3   (3)
    // v1      (4) | 0.0  (5) | 1.0  (6) | 2.0  (7)
    // v2      (8) | 0.1  (9) | 1.1 (10) | 2.1 (11)
    // new    (12) |     (13) |     (14) |     (15)
    // v3     (16) | 0.2 (17) | 1.2 (18) | 2.2 (19)

    QAccessibleInterface *cell00_new = table2->cellAt(0, 0);
    QCOMPARE(cell00, cell00_new);
    QCOMPARE(cell00->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell00->tableCellInterface()->columnIndex(), 0);

    QAccessibleInterface *cell02_new = table2->cellAt(0, 2);
    QCOMPARE(cell02, cell02_new);
    QCOMPARE(cell02_new->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell02_new->tableCellInterface()->columnIndex(), 2);

    QAccessibleInterface *cell20_new = table2->cellAt(2, 0);
    QAccessibleInterface *cell30_new = table2->cellAt(3, 0);
    QAccessible::Id id20_new = QAccessible::uniqueId(cell20_new);
    QVERIFY(id20_new != id20);
    QAccessible::Id id30_new = QAccessible::uniqueId(cell30_new);
    QCOMPARE(id20, id30_new);
    QCOMPARE(cell20->tableCellInterface()->rowIndex(), 3);
    QCOMPARE(cell20->tableCellInterface()->columnIndex(), 0);

    QAccessibleInterface *cell22_new = table2->cellAt(2, 2);
    QAccessibleInterface *cell32_new = table2->cellAt(3, 2);
    QAccessible::Id id22_new = QAccessible::uniqueId(cell22_new);
    QVERIFY(id22_new != id22);
    QAccessible::Id id32_new = QAccessible::uniqueId(cell32_new);
    QCOMPARE(id22, id32_new);
    QCOMPARE(cell32_new->tableCellInterface()->rowIndex(), 3);
    QCOMPARE(cell32_new->tableCellInterface()->columnIndex(), 2);


    QVERIFY(table2->cellAt(0, 0) == cell1);

    tableView->insertColumn(2);
    // Button  (0) | h1   (1) | h2   (2) |  (3) | h3   (4)
    // v1      (5) | 0.0  (6) | 1.0  (7) |  (8) | 2.0  (9)
    // v2     (10) | 0.1 (11) | 1.1 (12) | (13) | 2.1 (14)
    // new    (15) |     (16) |     (17) | (18) |     (19)
    // v3     (20) | 0.2 (21) | 1.2 (22) | (23) | 2.2 (24)

    cell00_new = table2->cellAt(0, 0);
    QCOMPARE(cell00, cell00_new);
    QCOMPARE(cell00->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell00->tableCellInterface()->columnIndex(), 0);

    QAccessibleInterface *cell01_new = table2->cellAt(0, 1);
    QCOMPARE(cell01, cell01_new);
    QCOMPARE(cell01_new->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell01_new->tableCellInterface()->columnIndex(), 1);

    QAccessibleInterface *cell03_new = table2->cellAt(0, 3);
    QVERIFY(cell03_new);
    QCOMPARE(cell03_new->tableCellInterface()->rowIndex(), 0);
    QCOMPARE(cell03_new->tableCellInterface()->columnIndex(), 3);
    QCOMPARE(iface->indexOfChild(cell03_new), 9);
    QCOMPARE(cell03_new, cell02);

    cell30_new = table2->cellAt(3, 0);
    QCOMPARE(cell30_new, cell20);
    QCOMPARE(iface->indexOfChild(cell30_new), 21);


    {
        QTestAccessibility::clearEvents();
        QModelIndex index00 = tableView->model()->index(1, 1, tableView->rootIndex());
        tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
        tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        tableView->selectionModel()->select(index00, QItemSelectionModel::ClearAndSelect);
        QAccessibleEvent event(tableView, QAccessible::SelectionAdd);
        event.setChild(12);
        QCOMPARE(QTestAccessibility::containsEvent(&event), true);
        QTestAccessibility::clearEvents();
        tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
        tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        tableView->selectionModel()->select(index00, QItemSelectionModel::ClearAndSelect);
        tableView->horizontalHeader()->setVisible(false);

    }
    tvHolder.reset();
    QVERIFY(!QAccessible::accessibleInterface(id00));
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::rootIndexView()
{
    QStandardItemModel model;
    for (int i = 0; i < 2; ++i) {
        QStandardItem *item = new QStandardItem(u"root %1"_s.arg(i));
        for (int j = 0; j < 5 * (i + 1); ++j) {
            switch (i) {
            case 0:
                item->appendRow(new QStandardItem(u"child0/%1"_s.arg(j)));
                break;
            case 1:
                item->appendRow({new QStandardItem(u"column0 1/%1"_s.arg(j)),
                                 new QStandardItem(u"column1 1/%1"_s.arg(j))
                                });
                break;
            }
        }
        model.appendRow(item);
    }

    QListView view;
    view.setModel(&model);
    QTestAccessibility::clearEvents();

    QAccessibleInterface *accView = QAccessible::queryAccessibleInterface(&view);
    QVERIFY(accView);
    QAccessibleTableInterface *accTable = accView->tableInterface();
    QVERIFY(accTable);
    QCOMPARE(accTable->rowCount(), 2);
    QCOMPARE(accTable->columnCount(), 1);

    view.setRootIndex(model.index(0, 0));
    QAccessibleTableModelChangeEvent resetEvent(&view, QAccessibleTableModelChangeEvent::ModelReset);
    QVERIFY(QTestAccessibility::containsEvent(&resetEvent));

    QCOMPARE(accTable->rowCount(), 5);
    QCOMPARE(accTable->columnCount(), 1);

    view.setRootIndex(model.index(1, 0));
    QCOMPARE(accTable->rowCount(), 10);
    QCOMPARE(accTable->columnCount(), 1);

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::uniqueIdTest()
{
    // Test that an ID isn't reassigned to another interface right away when an accessible interface
    // that has just been created is removed from the cache and deleted before the next
    // accessible interface is registered.
    // For example for AT-SPI, that would result in the same object path being used, and thus
    // data from the old and new interface can get confused due to caching.
    QWidget widget1;
    QAccessibleInterface *iface1 = QAccessible::queryAccessibleInterface(&widget1);
    QAccessible::Id id1 = QAccessible::uniqueId(iface1);
    QAccessible::deleteAccessibleInterface(id1);

    QWidget widget2;
    QAccessibleInterface *iface2 = QAccessible::queryAccessibleInterface(&widget2);
    QAccessible::Id id2 = QAccessible::uniqueId(iface2);

    QVERIFY(id1 != id2);
}

void tst_QAccessibility::calendarWidgetTest()
{
#if QT_CONFIG(calendarwidget)
    {
    QCalendarWidget calendarWidget;

    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&calendarWidget);
    QVERIFY(interface);
    QCOMPARE(interface->role(), QAccessible::Table);
    QVERIFY(!interface->rect().isValid());
    QCOMPARE(interface->childAt(200, 200), static_cast<QAccessibleInterface*>(0));

    calendarWidget.resize(400, 300);
    calendarWidget.show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    // 1 = navigationBar, 2 = view.
    QCOMPARE(interface->childCount(), 2);

    const QRect globalGeometry = QRect(calendarWidget.mapToGlobal(QPoint(0, 0)),
                                       calendarWidget.size());
    QCOMPARE(interface->rect(), globalGeometry);

    QWidget *navigationBar = 0;
    for (QObject *child : calendarWidget.children()) {
        if (child->objectName() == QLatin1String("qt_calendar_navigationbar")) {
            navigationBar = static_cast<QWidget *>(child);
            break;
        }
    }
    QVERIFY(navigationBar);
    QVERIFY(verifyChild(navigationBar, interface, 0, globalGeometry));

    QAbstractItemView *calendarView = 0;
    for (QObject *child : calendarWidget.children()) {
        if (child->objectName() == QLatin1String("qt_calendar_calendarview")) {
            calendarView = static_cast<QAbstractItemView *>(child);
            break;
        }
    }
    QVERIFY(calendarView);
    QVERIFY(verifyChild(calendarView, interface, 1, globalGeometry));

    // Hide navigation bar.
    calendarWidget.setNavigationBarVisible(false);
    QCOMPARE(interface->childCount(), 1);
    QVERIFY(!navigationBar->isVisible());

    QVERIFY(verifyChild(calendarView, interface, 0, globalGeometry));

    // Show navigation bar.
    calendarWidget.setNavigationBarVisible(true);
    QCOMPARE(interface->childCount(), 2);
    QVERIFY(navigationBar->isVisible());

    // Navigate to the navigation bar via Child.
    QAccessibleInterface *navigationBarInterface = interface->child(0);
    QVERIFY(navigationBarInterface);
    QCOMPARE(navigationBarInterface->object(), (QObject*)navigationBar);

    // Navigate to the view via Child.
    QAccessibleInterface *calendarViewInterface = interface->child(1);
    QVERIFY(calendarViewInterface);
    QCOMPARE(calendarViewInterface->object(), (QObject*)calendarView);

    QVERIFY(!interface->child(-1));

    // In order for geometric navigation to work they must share the same parent
    QCOMPARE(navigationBarInterface->parent()->object(), calendarViewInterface->parent()->object());
    QVERIFY(navigationBarInterface->rect().bottom() < calendarViewInterface->rect().top());
    calendarViewInterface = 0;
    navigationBarInterface = 0;

    }
    QTestAccessibility::clearEvents();
#endif // QT_CONFIG(calendarwidget)
}

void tst_QAccessibility::dockWidgetTest()
{
#if QT_CONFIG(dockwidget)
    // Set up a proper main window with two dock widgets
    auto mwHolder = std::make_unique<QMainWindow>();
    auto mw = mwHolder.get();
    QFrame *central = new QFrame(mw);
    mw->setCentralWidget(central);
    QMenuBar *mb = new QMenuBar(mw);
    mb->addAction(tr("&File"));
    mw->setMenuBar(mb);

    QDockWidget *dock1 = new QDockWidget(mw);
    dock1->setWindowTitle("Dock 1");
    mw->addDockWidget(Qt::LeftDockWidgetArea, dock1);
    QPushButton *pb1 = new QPushButton(tr("Push me"), dock1);
    dock1->setWidget(pb1);

    QDockWidget *dock2 = new QDockWidget(mw);
    dock2->setWindowTitle("Dock 2");
    mw->addDockWidget(Qt::BottomDockWidgetArea, dock2);
    QPushButton *pb2 = new QPushButton(tr("Push me"), dock2);
    dock2->setWidget(pb2);
    dock2->setFeatures(QDockWidget::DockWidgetClosable);

    mw->resize(600,400);
    mw->show();
    QVERIFY(QTest::qWaitForWindowExposed(mw));

    QAccessibleInterface *accMainWindow = QAccessible::queryAccessibleInterface(mw);
    // 4 children: menu bar, dock1, dock2, and central widget
    QCOMPARE(accMainWindow->childCount(), 4);
    QAccessibleInterface *accDock1 = 0;
    QAccessibleInterface *accDock2 = 0;
    for (int i = 0; i < 4; ++i) {
        QAccessibleInterface *child = accMainWindow->child(i);
        if (child && child->object() == dock1)
            accDock1 = child;
        if (child && child->object() == dock2)
            accDock2 = child;
    }

    // Dock widgets consist of
    // 0 contents
    // 1 close button
    // 2 float button
    QVERIFY(accDock1);
    QCOMPARE(accDock1->role(), QAccessible::Pane);
    QCOMPARE(accDock1->text(QAccessible::Name), dock1->windowTitle());
    QCOMPARE(accDock1->childCount(), 3);

    QAccessibleInterface *dock1Widget = accDock1->child(0);
    QCOMPARE(dock1Widget->role(), QAccessible::Button);
    QCOMPARE(dock1Widget->text(QAccessible::Name), pb1->text());

#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "Dock Widget geometry on Mac seems broken.", Continue);
#endif
    QVERIFY(accDock1->rect().contains(dock1Widget->rect()));
    QCOMPARE(accDock1->indexOfChild(dock1Widget), 0);
    QCOMPARE(dock1Widget->parent()->object(), dock1);

    QAccessibleInterface *dock1Close = accDock1->child(1);
    QCOMPARE(dock1Close->role(), QAccessible::Button);
    QCOMPARE(dock1Close->text(QAccessible::Name), QDockWidget::tr("Close"));
    QVERIFY(accDock1->rect().contains(dock1Close->rect()));
    QCOMPARE(accDock1->indexOfChild(dock1Close), 1);
    QCOMPARE(dock1Close->parent()->object(), dock1);

    QAccessibleInterface *dock1Float = accDock1->child(2);
    QCOMPARE(dock1Float->role(), QAccessible::Button);
    QCOMPARE(dock1Float->text(QAccessible::Name), QDockWidget::tr("Float"));
    QVERIFY(accDock1->rect().contains(dock1Float->rect()));
    QCOMPARE(accDock1->indexOfChild(dock1Float), 2);
    QVERIFY(!dock1Float->state().invisible);

    QVERIFY(accDock2);
    QCOMPARE(accDock2->role(), QAccessible::Pane);
    QCOMPARE(accDock2->text(QAccessible::Name), dock2->windowTitle());
    QCOMPARE(accDock2->childCount(), 3);

    QAccessibleInterface *dock2Widget = accDock2->child(0);
    QCOMPARE(dock2Widget->role(), QAccessible::Button);
    QCOMPARE(dock2Widget->text(QAccessible::Name), pb1->text());
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "Dock Widget geometry on Mac seems broken.", Continue);
#endif
    QVERIFY(accDock2->rect().contains(dock2Widget->rect()));
    QCOMPARE(accDock2->indexOfChild(dock2Widget), 0);

    QAccessibleInterface *dock2Close = accDock2->child(1);
    QCOMPARE(dock2Close->role(), QAccessible::Button);
    QCOMPARE(dock2Close->text(QAccessible::Name), QDockWidget::tr("Close"));
    QVERIFY(accDock2->rect().contains(dock2Close->rect()));
    QCOMPARE(accDock2->indexOfChild(dock2Close), 1);
    QVERIFY(!dock2Close->state().invisible);

    QAccessibleInterface *dock2Float = accDock2->child(2);
    QCOMPARE(dock2Float->role(), QAccessible::Button);
    QCOMPARE(dock2Float->text(QAccessible::Name), QDockWidget::tr("Float"));
    QCOMPARE(accDock2->indexOfChild(dock2Float), 2);
    QVERIFY(dock2Float->state().invisible);

    QPoint buttonPoint = pb2->mapToGlobal(QPoint(pb2->width()/2, pb2->height()/2));
    QAccessibleInterface *childAt = accDock2->childAt(buttonPoint.x(), buttonPoint.y());
    QVERIFY(childAt);
    QCOMPARE(childAt->object(), pb2);

    QWidget *close1 = qobject_cast<QWidget*>(dock1Close->object());
    QPoint close1ButtonPoint = close1->mapToGlobal(QPoint(close1->width()/2, close1->height()/2));
    QAccessibleInterface *childAt2 = accDock1->childAt(close1ButtonPoint.x(), close1ButtonPoint.y());
    QVERIFY(childAt2);
    QCOMPARE(childAt2->object(), close1);

    // custom title bar widget
    QDockWidget *dock3 = new QDockWidget(mw);
    dock3->setWindowTitle("Dock 3");
    mw->addDockWidget(Qt::LeftDockWidgetArea, dock3);
    QPushButton *pb3 = new QPushButton(tr("Push me"), dock3);
    dock3->setWidget(pb3);
    QLabel *titleLabel = new QLabel("I am a title widget");
    dock3->setTitleBarWidget(titleLabel);

    QAccessibleInterface *accDock3 = accMainWindow->child(4);
    QVERIFY(accDock3);
    QCOMPARE(accDock3->role(), QAccessible::Pane);
    QCOMPARE(accDock3->text(QAccessible::Name), dock3->windowTitle());
    QCOMPARE(accDock3->childCount(), 2);
    QAccessibleInterface *titleWidget = accDock3->child(1);
    QVERIFY(titleWidget);
    QCOMPARE(titleWidget->text(QAccessible::Name), titleLabel->text());
    QAccessibleInterface *dock3Widget = accDock3->child(0);
    QCOMPARE(dock3Widget->text(QAccessible::Name), pb3->text());

    // check role is QAccessible::Window when dock window is floating/undocked
    dock3->setFloating(true);
    QCOMPARE(accDock3->role(), QAccessible::Window);

    QTestAccessibility::clearEvents();
#endif // QT_CONFIG(dockwidget)
}

void tst_QAccessibility::comboBoxTest()
{
    { // not editable combobox
    QComboBox combo;
    combo.addItems(QStringList() << "one" << "two" << "three");
    // Fully decorated windows have a minimum width of 160 on Windows.
    combo.setMinimumWidth(200);
    combo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&combo));

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&combo);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE(iface->role(), QAccessible::ComboBox);
    QCOMPARE(iface->childCount(), 1);

#ifdef Q_OS_UNIX
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("one"));
#endif
    QCOMPARE(iface->text(QAccessible::Value), QLatin1String("one"));
    QCOMPARE(combo.view()->selectionModel()->currentIndex().row(), 0);

    combo.setCurrentIndex(2);
#ifdef Q_OS_UNIX
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("three"));
#endif
    QCOMPARE(iface->text(QAccessible::Value), QLatin1String("three"));

    QAccessibleInterface *listIface = iface->child(0);
    QCOMPARE(listIface->role(), QAccessible::List);
    QCOMPARE(listIface->childCount(), 3);

    QAccessibleSelectionInterface *selectionIface = listIface->selectionInterface();
    QVERIFY(selectionIface);
    QCOMPARE(selectionIface->selectedItemCount(), 1);
    QCOMPARE(listIface->indexOfChild(selectionIface->selectedItem(0)), 2);

    QVERIFY(!combo.view()->isVisible());
    QCOMPARE(combo.view()->selectionModel()->currentIndex().row(), 2);
    QVERIFY(iface->actionInterface());
    QCOMPARE(iface->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction() << QAccessibleActionInterface::pressAction());
    iface->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    QTRY_VERIFY(combo.view()->isVisible());

    }

    { // editable combobox
    QComboBox editableCombo;
    editableCombo.setMinimumWidth(200);
    editableCombo.show();
    editableCombo.setEditable(true);
    editableCombo.addItems(QStringList() << "foo" << "bar" << "baz");

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&editableCombo);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE(iface->role(), QAccessible::ComboBox);
    QCOMPARE(iface->childCount(), 2);

    QAccessibleInterface *listIface = iface->child(0);
    QCOMPARE(listIface->role(), QAccessible::List);
    QAccessibleInterface *editIface = iface->child(1);
    QCOMPARE(editIface->role(), QAccessible::EditableText);
    }

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::relationTest()
{
    auto windowHolder = std::make_unique<QWidget>();
    auto window = windowHolder.get();
    QString text = "Hello World";
    QLabel *label = new QLabel(text, window);
    setFrameless(label);
    QSpinBox *spinBox = new QSpinBox(window);
    label->setBuddy(spinBox);
    QProgressBar *pb = new QProgressBar(window);
    pb->setRange(0, 99);
    connect(spinBox, SIGNAL(valueChanged(int)), pb, SLOT(setValue(int)));

    window->resize(320, 200);
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);

    QAccessibleInterface *acc_label = QAccessible::queryAccessibleInterface(label);
    QVERIFY(acc_label);
    QAccessibleInterface *acc_spinBox = QAccessible::queryAccessibleInterface(spinBox);
    QVERIFY(acc_spinBox);
    QAccessibleInterface *acc_pb = QAccessible::queryAccessibleInterface(pb);
    QVERIFY(acc_pb);

    typedef QPair<QAccessibleInterface*, QAccessible::Relation> RelationPair;
    {
        const QList<RelationPair> rels = acc_label->relations(QAccessible::Labelled);
        QCOMPARE(rels.size(), 1);
        const RelationPair relPair = rels.first();

        // spinBox is Labelled by acc_label
        QCOMPARE(relPair.first->object(), spinBox);
        QCOMPARE(relPair.second, QAccessible::Labelled);
    }

    {
        // Test multiple relations (spinBox have two)
        const QList<RelationPair> rels = acc_spinBox->relations();
        QCOMPARE(rels.size(), 2);
        int visitCount = 0;
        for (const auto &relPair : rels) {
            if (relPair.second & QAccessible::Label) {
                // label is the Label of spinBox
                QCOMPARE(relPair.first->object(), label);
                ++visitCount;
            } else if (relPair.second & QAccessible::Controlled) {
                // progressbar is Controlled by the spinBox
                QCOMPARE(relPair.first->object(), pb);
                ++visitCount;
            }
        }
        QCOMPARE(visitCount, rels.size());
    }

    windowHolder.reset();
    QTestAccessibility::clearEvents();
}

#if QT_CONFIG(shortcut)

void tst_QAccessibility::labelTest()
{
    auto windowHolder = std::make_unique<QWidget>();
    auto window = windowHolder.get();
    QString text = "Hello World";
    QLabel *label = new QLabel(text, window);
    setFrameless(label);
    QLineEdit *buddy = new QLineEdit(window);
    label->setBuddy(buddy);
    window->resize(320, 200);
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);

    QAccessibleInterface *acc_label = QAccessible::queryAccessibleInterface(label);
    QVERIFY(acc_label);
    QAccessibleInterface *acc_lineEdit = QAccessible::queryAccessibleInterface(buddy);
    QVERIFY(acc_lineEdit);

    QCOMPARE(acc_label->text(QAccessible::Name), text);
    QCOMPARE(acc_label->state().editable, false);
    QCOMPARE(acc_label->state().passwordEdit, false);
    QCOMPARE(acc_label->state().disabled, false);
    QCOMPARE(acc_label->state().focused, false);
    QCOMPARE(acc_label->state().focusable, false);
    QCOMPARE(acc_label->state().readOnly, true);


    typedef QPair<QAccessibleInterface*, QAccessible::Relation> RelationPair;
    {
        const QList<RelationPair> rels = acc_label->relations(QAccessible::Labelled);
        QCOMPARE(rels.size(), 1);
        const RelationPair relPair = rels.first();
        QCOMPARE(relPair.first->object(), buddy);
        QCOMPARE(relPair.second, QAccessible::Labelled);
    }

    {
        const QList<RelationPair> rels = acc_lineEdit->relations(QAccessible::Label);
        QCOMPARE(rels.size(), 1);
        const RelationPair relPair = rels.first();
        QCOMPARE(relPair.first->object(), label);
        QCOMPARE(relPair.second, QAccessible::Label);
    }

    windowHolder.reset();
    QTestAccessibility::clearEvents();

    QPixmap testPixmap(50, 50);
    testPixmap.fill();

    QLabel imageLabel;
    imageLabel.setPixmap(testPixmap);
    imageLabel.setToolTip("Test Description");

    acc_label = QAccessible::queryAccessibleInterface(&imageLabel);
    QVERIFY(acc_label);

    QAccessibleImageInterface *imageInterface = acc_label->imageInterface();
    QVERIFY(imageInterface);

    QCOMPARE(imageInterface->imageSize(), testPixmap.size());
    QCOMPARE(imageInterface->imageDescription(), QString::fromLatin1("Test Description"));
    const QPoint labelPos = imageLabel.mapToGlobal(QPoint(0,0));
    QCOMPARE(imageInterface->imagePosition(), labelPos);

    QTestAccessibility::clearEvents();
}

#if defined(Q_OS_MACOS)
QT_BEGIN_NAMESPACE
    extern void qt_set_sequence_auto_mnemonic(bool);
QT_END_NAMESPACE
#endif

void tst_QAccessibility::accelerators()
{
#if defined(Q_OS_MACOS)
    qt_set_sequence_auto_mnemonic(true);
    const auto resetAutoMnemonic = qScopeGuard([] {
        qt_set_sequence_auto_mnemonic(false);
    });
#endif

    auto windowHolder = std::make_unique<QWidget>();
    auto window = windowHolder.get();
    QHBoxLayout *lay = new QHBoxLayout(window);
    QLabel *label = new QLabel(tr("&Line edit"), window);
    QLineEdit *le = new QLineEdit(window);
    lay->addWidget(label);
    lay->addWidget(le);
    label->setBuddy(le);

    window->show();

    QAccessibleInterface *accLineEdit = QAccessible::queryAccessibleInterface(le);
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT | Qt::Key_L).toString(QKeySequence::NativeText));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT | Qt::Key_L).toString(QKeySequence::NativeText));
    label->setText(tr("Q &"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q &&"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q && A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q &&&A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT | Qt::Key_A).toString(QKeySequence::NativeText));
    label->setText(tr("Q &&A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());

#if !defined(QT_NO_DEBUG)
    QTest::ignoreMessage(QtWarningMsg, "QKeySequence::mnemonic: \"Q &A&B\" contains multiple occurrences of '&'");
#endif
    label->setText(tr("Q &A&B"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT | Qt::Key_A).toString(QKeySequence::NativeText));

#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);
    windowHolder.reset();
    QTestAccessibility::clearEvents();
}

#endif // QT_CONFIG(shortcut)

#ifdef QT_SUPPORTS_IACCESSIBLE2
static IUnknown *queryIA2(IAccessible *acc, const IID &iid)
{
    IUnknown *resultInterface = 0;
    IServiceProvider *pService = 0;
    HRESULT hr = acc->QueryInterface(IID_IServiceProvider, (void **)&pService);
    if (SUCCEEDED(hr)) {
        IAccessible2 *pIA2 = 0;
        hr = pService->QueryService(IID_IAccessible, IID_IAccessible2, (void**)&pIA2);
        if (SUCCEEDED(hr) && pIA2) {
            // The control supports IAccessible2.
            // pIA2 is the reference to the accessible object's IAccessible2 interface.
            hr = pIA2->QueryInterface(iid, (void**)&resultInterface);
            pIA2->Release();
        }
        // The control supports IAccessible2.
        pService->Release();
    }
    return resultInterface;
}
#endif

void tst_QAccessibility::bridgeTest()
{
    // For now this is a simple test to see if the bridge is working at all.
    // Ideally it should be extended to test all aspects of the bridge.
#if defined(Q_OS_WIN)
    auto guard = qScopeGuard([]() { QTestAccessibility::clearEvents(); });

    QWidget window;
    QVBoxLayout *lay = new QVBoxLayout(&window);
    QPushButton *button = new QPushButton(tr("Push me"), &window);
    QTextEdit *te = new QTextEdit(&window);
    te->setText(QLatin1String("hello world\nhow are you today?\n"));

    // Add QTableWidget
    QTableWidget *tableWidget = new QTableWidget(3, 3, &window);
    tableWidget->setColumnCount(3);
    QStringList hHeader;
    hHeader << "h1" << "h2" << "h3";
    tableWidget->setHorizontalHeaderLabels(hHeader);

    QStringList vHeader;
    vHeader << "v1" << "v2" << "v3";
    tableWidget->setVerticalHeaderLabels(vHeader);

    for (int i = 0; i<9; ++i) {
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setText(QString::number(i/3) + QString(".") + QString::number(i%3));
        tableWidget->setItem(i/3, i%3, item);
    }

    tableWidget->setFixedSize(600, 600);

    QLabel *label = new QLabel(tr("Push my buddy"));
    label->setBuddy(button);

    lay->addWidget(button);
    lay->addWidget(te);
    lay->addWidget(tableWidget);
    lay->addWidget(label);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Validate button position through the accessible interface.
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(button);
    QPoint buttonPos = button->mapToGlobal(QPoint(0,0));
    QRect buttonRect = iface->rect();
    QCOMPARE(buttonRect.topLeft(), buttonPos);

    // All set, now test the bridge.
    const QPoint nativePos = QHighDpi::toNativePixels(buttonRect.center(), window.windowHandle());
    POINT pt{nativePos.x(), nativePos.y()};

    // Initialize COM stuff.
    QComHelper comHelper;
    QVERIFY(comHelper.isValid());

    // Get UI Automation interface.
    const GUID CLSID_CUIAutomation_test{0xff48dba4, 0x60ef, 0x4201,
                                        {0xaa,0x87, 0x54,0x10,0x3e,0xef,0x59,0x4e}};
    IUIAutomation *automation = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_CUIAutomation_test, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation));
    QVERIFY(SUCCEEDED(hr));

    // Get button element from UI Automation using point.
    IUIAutomationElement *buttonElement = nullptr;
    hr = automation->ElementFromPoint(pt, &buttonElement);
    QVERIFY(SUCCEEDED(hr));

    // Check that it has a button control type ID.
    CONTROLTYPEID controlTypeId;
    hr = buttonElement->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_ButtonControlTypeId);

    // Test the bounding rectangle.
    RECT rect;
    hr = buttonElement->get_CurrentBoundingRectangle(&rect);
    QVERIFY(SUCCEEDED(hr));
    const QRect boundingRect(rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1);
    const QRectF nativeRect = QHighDpi::toNativePixels(QRectF(buttonRect), window.windowHandle());
    const QRect truncRect(int(nativeRect.left()), int(nativeRect.top()),
                          int(nativeRect.right()) - int(nativeRect.left()) + 1,
                          int(nativeRect.bottom()) - int(nativeRect.top()) + 1);
    QCOMPARE(truncRect, boundingRect);

    buttonElement->Release();

    // Get native window handle.
    QWindow *windowHandle = window.windowHandle();
    QVERIFY(windowHandle != 0);
    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    QVERIFY(platform != 0);
    HWND hWnd = (HWND)platform->nativeResourceForWindow("handle", windowHandle);
    QVERIFY(hWnd != 0);

    // Get automation element for the window from handle.
    IUIAutomationElement *windowElement = nullptr;
    hr = automation->ElementFromHandle(hWnd, &windowElement);
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(windowElement != 0);

    // Validate that the top-level widget is reported as a window.
    hr = windowElement->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_WindowControlTypeId);

    // Get a tree walker to walk over elements.
    IUIAutomationTreeWalker *controlWalker = nullptr;
    IUIAutomationElement *node = nullptr;
    QList<IUIAutomationElement *> nodeList;

    hr = automation->get_ControlViewWalker(&controlWalker);
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(controlWalker != 0);

    hr = controlWalker->GetFirstChildElement(windowElement, &node);
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(node != 0);

    int numElements = 5; // Title bar + 4 widgets

    while (node) {
        nodeList.append(node);
        QVERIFY(nodeList.size() <= numElements);
        IUIAutomationElement *next = nullptr;
        hr = controlWalker->GetNextSiblingElement(node, &next);
        QVERIFY(SUCCEEDED(hr));
        node = next;
    }
    QCOMPARE(nodeList.size(), numElements);

    // Title bar
    hr = nodeList.at(0)->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_TitleBarControlTypeId);

    // Button
    hr = nodeList.at(1)->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_ButtonControlTypeId);

    // Edit
    IUIAutomationElement *uiaElement = nodeList.at(2);
    hr = uiaElement->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_EditControlTypeId);

    // "hello world\nhow are you today?\n"
    IUIAutomationTextPattern *textPattern = nullptr;
    hr = uiaElement->GetCurrentPattern(UIA_TextPattern2Id, reinterpret_cast<IUnknown**>(&textPattern));
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(textPattern);

    IUIAutomationTextRange *docRange = nullptr;
    hr = textPattern->get_DocumentRange(&docRange);
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(docRange);

    IUIAutomationTextRange *textRange = nullptr;
    hr = docRange->Clone(&textRange);
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(textRange);
    int moved;

    auto rangeText = [](IUIAutomationTextRange *textRange) {
        BSTR str;
        QString res = "IUIAutomationTextRange::GetText() failed";
        HRESULT hr = textRange->GetText(-1, &str);
        if (SUCCEEDED(hr)) {
            res = QString::fromWCharArray(str);
            ::SysFreeString(str);
        }
        return res;
    };

    // Move start endpoint past "hello " to "world"
    hr = textRange->Move(TextUnit_Character, 6, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(moved, 6);
    // If the range was not empty, it should be collapsed to contain a single text unit
    QCOMPARE(rangeText(textRange), QString("w"));

    // Move end endpoint to end of "world"
    hr = textRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 4, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(moved, 4);
    QCOMPARE(rangeText(textRange), QString("world"));

    // MSDN: "Zero has no effect". This behavior was also verified with native controls.
    hr = textRange->Move(TextUnit_Character, 0, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(moved, 0);
    QCOMPARE(rangeText(textRange), QString("world"));

    hr = textRange->Move(TextUnit_Character, 1, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(rangeText(textRange), QString("o"));

   // move as far towards the end as possible
    hr = textRange->Move(TextUnit_Character, 999, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(rangeText(textRange), QString(""));

    hr = textRange->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, -1, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(rangeText(textRange), QString("\n"));

    // move one forward (last possible position again)
    hr = textRange->Move(TextUnit_Character, 1, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(rangeText(textRange), QString(""));

    hr = textRange->Move(TextUnit_Character, -7, &moved);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(moved, -7);
    QCOMPARE(rangeText(textRange), QString(""));
    // simulate moving cursor (empty range) towards (and past) the end
    QString today(" today?\n");
    for (int i = 1; i < 9; ++i) {   // 9 is deliberately too much
        // peek one character back
        hr = textRange->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, -1, &moved);
        QVERIFY(SUCCEEDED(hr));
        QCOMPARE(rangeText(textRange), today.mid(i - 1, 1));

        hr = textRange->Move(TextUnit_Character, 1, &moved);
        QVERIFY(SUCCEEDED(hr));
        QCOMPARE(rangeText(textRange), today.mid(i, moved));       // when we cannot move further, moved will be 0

        // Make the range empty again
        hr = textRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, -moved, &moved);
        QVERIFY(SUCCEEDED(hr));

        // advance the empty range
        hr = textRange->Move(TextUnit_Character, 1, &moved);
        QVERIFY(SUCCEEDED(hr));
    }
    docRange->Release();
    textRange->Release();
    textPattern->Release();


    // Table
    hr = nodeList.at(3)->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_TableControlTypeId);

    // Label
    hr = nodeList.at(4)->get_CurrentControlType(&controlTypeId);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(controlTypeId, UIA_TextControlTypeId);

    for (auto nd : nodeList) {
        nd->Release();
    }

    controlWalker->Release();
    windowElement->Release();
    automation->Release();
#endif
}

class FocusChildTestAccessibleInterface : public QAccessibleInterface
{
public:
    FocusChildTestAccessibleInterface(int index, bool focus, QAccessibleInterface *parent)
        : m_parent(parent)
        , m_index(index)
        , m_focus(focus)
    {
        QAccessible::registerAccessibleInterface(this);
    }

    bool isValid() const override { return true; }
    QObject *object() const override { return nullptr; }
    QAccessibleInterface *childAt(int, int) const override { return nullptr; }
    QAccessibleInterface *parent() const override { return m_parent; }
    QAccessibleInterface *child(int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return -1; }
    QString text(QAccessible::Text) const override { return QStringLiteral("FocusChildTestAccessibleInterface %1").arg(m_index); }
    void setText(QAccessible::Text, const QString &) override { }
    QRect rect() const override { return QRect(); }
    QAccessible::Role role() const override { return QAccessible::StaticText; }

    QAccessible::State state() const override
    {
        QAccessible::State s;
        s.focused = m_focus;
        return s;
    }

private:
    QAccessibleInterface *m_parent;
    int m_index;
    bool m_focus;
};

class FocusChildTestAccessibleWidget : public QAccessibleWidgetV2
{
public:
    static QAccessibleInterface *ifaceFactory(const QString &key, QObject *o)
    {
        if (key == "QtTestAccessibleWidget")
            return new FocusChildTestAccessibleWidget(static_cast<QtTestAccessibleWidget *>(o));
        return 0;
    }

    FocusChildTestAccessibleWidget(QtTestAccessibleWidget *w)
        : QAccessibleWidgetV2(w)
    {
        m_children.push_back(new FocusChildTestAccessibleInterface(0, false, this));
        m_children.push_back(new FocusChildTestAccessibleInterface(1, true, this));
        m_children.push_back(new FocusChildTestAccessibleInterface(2, false, this));
    }

    QAccessible::State state() const override
    {
        QAccessible::State s = QAccessibleWidgetV2::state();
        s.focused = false;
        return s;
    }

    QAccessibleInterface *focusChild() const override
    {
        for (int i = 0; i < childCount(); ++i) {
            if (child(i)->state().focused)
                return child(i);
        }

        return nullptr;
    }

    QAccessibleInterface *child(int index) const override
    {
        return m_children[index];
    }

    int childCount() const override
    {
        return m_children.size();
    }

private:
    QList<QAccessibleInterface *> m_children;
};

void tst_QAccessibility::focusChild()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Platform does not support window activation");

    {
        QMainWindow mainWindow;
        QtTestAccessibleWidget *widget1 = new QtTestAccessibleWidget(0, "Widget1");
        QAccessibleInterface *iface1 = QAccessible::queryAccessibleInterface(widget1);
        QtTestAccessibleWidget *widget2 = new QtTestAccessibleWidget(0, "Widget2");
        QAccessibleInterface *iface2 = QAccessible::queryAccessibleInterface(widget2);

        QWidget *centralWidget = new QWidget;
        QHBoxLayout *centralLayout = new QHBoxLayout;
        centralWidget->setLayout(centralLayout);
        mainWindow.setCentralWidget(centralWidget);
        centralLayout->addWidget(widget1);
        centralLayout->addWidget(widget2);

        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

        // widget1 has not been focused yet -> it has no active focus nor focus widget.
        QVERIFY(!iface1->focusChild());

        // widget1 is focused -> it has active focus and focus widget.
        widget1->setFocus();
        QCOMPARE(iface1->focusChild(), iface1);
        QCOMPARE(QAccessible::queryAccessibleInterface(&mainWindow)->focusChild(), iface1);

        // widget1 lose focus -> it has no active focus but has focus widget what is itself.
        // In this case, the focus child of widget1's interface is itself and the focusChild() call
        // should not run into an infinite recursion.
        widget2->setFocus();
        QCOMPARE(iface1->focusChild(), iface1);
        QCOMPARE(iface2->focusChild(), iface2);
        QCOMPARE(QAccessible::queryAccessibleInterface(&mainWindow)->focusChild(), iface2);

        QTestAccessibility::clearEvents();
    }

    {
        QMainWindow mainWindow;
        QAccessible::installFactory(&FocusChildTestAccessibleWidget::ifaceFactory);
        QtTestAccessibleWidget *widget = new QtTestAccessibleWidget(0, "FocusChildTestWidget");
        mainWindow.setCentralWidget(widget);
        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&mainWindow);
        QVERIFY(!iface->focusChild());
        widget->setFocus();
        QCOMPARE(iface->focusChild(), QAccessible::queryAccessibleInterface(widget)->child(1));

        QAccessible::removeFactory(FocusChildTestAccessibleWidget::ifaceFactory);
        QTestAccessibility::clearEvents();
    }

    {
        QMainWindow mainWindow;
        QTabBar *tabBar = new QTabBar();
        tabBar->insertTab(0, "First tab");
        tabBar->insertTab(1, "Second tab");
        tabBar->insertTab(2, "Third tab");
        mainWindow.setCentralWidget(tabBar);
        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

        tabBar->setFocus();
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&mainWindow);
        QCOMPARE(iface->focusChild()->text(QAccessible::Name), QStringLiteral("First tab"));
        QCOMPARE(iface->focusChild()->role(), QAccessible::PageTab);
        tabBar->setCurrentIndex(1);
        QCOMPARE(iface->focusChild()->text(QAccessible::Name), QStringLiteral("Second tab"));
        QCOMPARE(iface->focusChild()->role(), QAccessible::PageTab);

        QTestAccessibility::clearEvents();
    }

    {
        auto tableView = std::make_unique<QTableWidget>(3, 3);

        QSignalSpy spy(tableView.get(), SIGNAL(currentCellChanged(int,int,int,int)));

        tableView->setColumnCount(3);
        QStringList hHeader;
        hHeader << "h1" << "h2" << "h3";
        tableView->setHorizontalHeaderLabels(hHeader);

        QStringList vHeader;
        vHeader << "v1" << "v2" << "v3";
        tableView->setVerticalHeaderLabels(vHeader);

        for (int i = 0; i < 9; ++i) {
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setText(QString::number(i/3) + QString(".") + QString::number(i%3));
            tableView->setItem(i/3, i%3, item);
        }

        tableView->resize(600, 600);
        tableView->show();
        QVERIFY(QTest::qWaitForWindowExposed(tableView.get()));

        tableView->setFocus();

        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(tableView.get());
        QVERIFY(iface);

        spy.clear();
        tableView->setCurrentCell(2, 1);
        QTRY_COMPARE(spy.size(), 1);

        QAccessibleInterface *child = iface->focusChild();
        QVERIFY(child);
        QCOMPARE(child->text(QAccessible::Name), QStringLiteral("2.1"));

        spy.clear();
        tableView->setCurrentCell(1, 2);
        QTRY_COMPARE(spy.size(), 1);

        child = iface->focusChild();
        QVERIFY(child);
        QCOMPARE(child->text(QAccessible::Name), QStringLiteral("1.2"));
    }

    {
        auto treeView = std::make_unique<QTreeWidget>();

        QSignalSpy spy(treeView.get(), SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

        treeView->setColumnCount(2);
        QTreeWidgetItem *header = new QTreeWidgetItem;
        header->setText(0, "Artist");
        header->setText(1, "Work");
        treeView->setHeaderItem(header);

        QTreeWidgetItem *root1 = new QTreeWidgetItem;
        root1->setText(0, "Spain");
        treeView->addTopLevelItem(root1);

        QTreeWidgetItem *item1 = new QTreeWidgetItem;
        item1->setText(0, "Picasso");
        item1->setText(1, "Guernica");
        root1->addChild(item1);

        QTreeWidgetItem *item2 = new QTreeWidgetItem;
        item2->setText(0, "Tapies");
        item2->setText(1, "Ambrosia");
        root1->addChild(item2);

        QTreeWidgetItem *root2 = new QTreeWidgetItem;
        root2->setText(0, "Austria");
        treeView->addTopLevelItem(root2);

        QTreeWidgetItem *item3 = new QTreeWidgetItem;
        item3->setText(0, "Klimt");
        item3->setText(1, "The Kiss");
        root2->addChild(item3);

        treeView->resize(400, 400);
        treeView->show();
        QVERIFY(QTest::qWaitForWindowExposed(treeView.get()));

        treeView->setFocus();

        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(treeView.get());
        QVERIFY(iface);

        spy.clear();
        treeView->setCurrentItem(item2);
        QTRY_COMPARE(spy.size(), 1);

        QAccessibleInterface *child = iface->focusChild();
        QVERIFY(child);
        QCOMPARE(child->text(QAccessible::Name), QStringLiteral("Tapies"));

        spy.clear();
        treeView->setCurrentItem(item3);
        QTRY_COMPARE(spy.size(), 1);

        child = iface->focusChild();
        QVERIFY(child);
        QCOMPARE(child->text(QAccessible::Name), QStringLiteral("Klimt"));
    }
    {
        QWidget window;
        // takes the initial focus
        QLineEdit lineEdit;
        QComboBox comboBox;
        comboBox.addItems({"One", "Two", "Three"});
        QComboBox editableComboBox;
        editableComboBox.setEditable(true);
        editableComboBox.addItems({"A", "B", "C"});
        QVBoxLayout vbox;
        vbox.addWidget(&lineEdit);
        vbox.addWidget(&comboBox);
        vbox.addWidget(&editableComboBox);
        window.setLayout(&vbox);

        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTestAccessibility::clearEvents();
        QAccessibleInterface *iface = nullptr;

        comboBox.setFocus();
        QTRY_VERIFY(comboBox.hasFocus());
        {
            QAccessibleEvent focusEvent(&comboBox, QAccessible::Focus);
            QVERIFY(QTestAccessibility::containsEvent(&focusEvent));
        }
        iface = QAccessible::queryAccessibleInterface(&comboBox);
        QVERIFY(iface);
        QCOMPARE(iface->focusChild(), nullptr);

        editableComboBox.setFocus();
        QTRY_VERIFY(editableComboBox.hasFocus());
        // Qt updates about the editable combobox, not the lineedit, as the
        // combobox is the lineedit's focus proxy.
        {
            QAccessibleEvent focusEvent(&editableComboBox, QAccessible::Focus);
            QVERIFY(QTestAccessibility::containsEvent(&focusEvent));
        }
        iface = QAccessible::queryAccessibleInterface(&editableComboBox);
        QVERIFY(iface);
        QVERIFY(iface->focusChild());
        QCOMPARE(iface->focusChild()->role(), QAccessible::EditableText);
    }
}

void tst_QAccessibility::messageBoxTest_data()
{
    QTest::addColumn<QMessageBox::Icon>("icon");
    QTest::addColumn<QMessageBox::StandardButtons>("buttons");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("infoText");
    QTest::addColumn<QString>("details");
    QTest::addColumn<bool>("checkbox");
    QTest::addColumn<bool>("textInteractive");

    QTest::addRow("Information") << QMessageBox::Information
                                 << QMessageBox::StandardButtons(QMessageBox::Ok)
                                 << "Information"
                                 << "Here, have some information."
                                 << QString()
                                 << QString()
                                 << false
                                 << false;

    QTest::addRow("Warning") << QMessageBox::Warning
                                 << QMessageBox::StandardButtons(QMessageBox::Ok | QMessageBox::Cancel)
                                 << "Warning"
                                 << "This is a dangerous operation!"
                                 << "Ok or Cancel?"
                                 << QString()
                                 << true
                                 << false;

    QTest::addRow("Error") << QMessageBox::Critical
                                 << QMessageBox::StandardButtons(QMessageBox::Abort | QMessageBox::Retry | QMessageBox::Ignore)
                                 << "Error"
                                 << "Operation failed for http://example.com"
                                 << "You have to decide what to do"
                                 << "Detailed log output"
                                 << false
                                 << true;
}

void tst_QAccessibility::messageBoxTest()
{
    QFETCH(QMessageBox::Icon, icon);
    QFETCH(QMessageBox::StandardButtons, buttons);
    QFETCH(QString, title);
    QFETCH(QString, text);
    QFETCH(QString, infoText);
    QFETCH(QString, details);
    QFETCH(bool, checkbox);
    QFETCH(bool, textInteractive);

    QMessageBox box(icon, title, text, buttons);
    // avoid native dialogs, they don't emit accessibility events on start/end
    box.setOption(QMessageBox::Option::DontUseNativeDialog);

    box.show();
    QAccessibleEvent showEvent(&box, QAccessible::DialogStart);
    QVERIFY(QTestAccessibility::containsEvent(&showEvent));

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&box);
    QVERIFY(iface);
    QCOMPARE(iface->role(), QAccessible::AlertMessage);
#ifndef Q_OS_DARWIN // macOS message boxes show no title
    QCOMPARE(iface->text(QAccessible::Name), title);
#endif
    QCOMPARE(iface->text(QAccessible::Value), text);

    int expectedChildCount = 3;
    QCOMPARE(iface->childCount(), expectedChildCount);

    if (textInteractive)
        box.setTextInteractionFlags(Qt::TextBrowserInteraction);

    if (!infoText.isEmpty()) {
        box.setInformativeText(infoText);
        QCOMPARE(iface->childCount(), ++expectedChildCount);
    }
    QCOMPARE(iface->text(QAccessible::Help), infoText);

    if (!details.isEmpty()) {
        box.setDetailedText(details);
        QCOMPARE(iface->childCount(), ++expectedChildCount);
    }
    if (checkbox) {
        box.setCheckBox(new QCheckBox("Don't show again"));
        QCOMPARE(iface->childCount(), ++expectedChildCount);
    }

    QTestAccessibility::clearEvents();

    box.hide();
    QAccessibleEvent hideEvent(&box, QAccessible::DialogEnd);
    QVERIFY(QTestAccessibility::containsEvent(&hideEvent));

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::widgetLocaleTest()
{
    QMainWindow mainWindow;
    QWidget w(&mainWindow);
    QHBoxLayout *box = new QHBoxLayout(&w);

    QLabel *label = new QLabel("Hello world");
    box->addWidget(label);

    // "你好世界" is "Hello world" in Chinese
    QLabel *chineseLabel = new QLabel(QString::fromUtf16(u"你好世界"));
    const QLocale chinese(QLocale::Chinese, QLocale::China);
    chineseLabel->setLocale(chinese);
    box->addWidget(chineseLabel);

    mainWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

    // verify that locale is the default locale if none was set explicitly
    QAccessibleInterface *labelAcc = QAccessible::queryAccessibleInterface(label);
    QVERIFY(labelAcc);
    QVERIFY(labelAcc->attributesInterface());
    QVERIFY(labelAcc->attributesInterface()->attributeKeys().contains(
            QAccessible::Attribute::Locale));
    const QVariant localeVariant =
            labelAcc->attributesInterface()->attributeValue(QAccessible::Attribute::Locale);
    QVERIFY(localeVariant.isValid() && localeVariant.canConvert<QLocale>());
    QCOMPARE(localeVariant.toLocale(), QLocale());

    // verify that locale matches the one explicitly set for the widget
    QAccessibleInterface *chineseLabelAcc = QAccessible::queryAccessibleInterface(chineseLabel);
    QVERIFY(chineseLabelAcc);
    QVERIFY(chineseLabelAcc->attributesInterface());
    QVERIFY(chineseLabelAcc->attributesInterface()->attributeKeys().contains(
            QAccessible::Attribute::Locale));
    const QVariant chineseLocaleVariant =
            chineseLabelAcc->attributesInterface()->attributeValue(QAccessible::Attribute::Locale);
    QVERIFY(chineseLocaleVariant.isValid() && chineseLocaleVariant.canConvert<QLocale>());
    QCOMPARE(chineseLocaleVariant.toLocale(), chinese);

    QTestAccessibility::clearEvents();
}

QTEST_MAIN(tst_QAccessibility)
#include "tst_qaccessibility.moc"
