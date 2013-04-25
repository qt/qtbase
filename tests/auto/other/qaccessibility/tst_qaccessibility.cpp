/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/qglobal.h>
#ifdef Q_OS_WIN
# include <QtCore/qt_windows.h>
# include <oleacc.h>
# include <servprov.h>
# include <winuser.h>
# ifdef QT_SUPPORTS_IACCESSIBLE2
#  include <Accessible2.h>
#  include <AccessibleAction.h>
#  include <AccessibleComponent.h>
#  include <AccessibleEditableText.h>
#  include <AccessibleText.h>
#  include <AccessibleTable2.h>
#  include <AccessibleTableCell.h>
# endif
#endif
#include <QtTest/QtTest>
#include <QtGui>
#include <QtGui/private/qaccessible2_p.h>
#include <QtWidgets>
#include <QtWidgets/private/qaccessiblewidget_p.h>
#include <math.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformaccessibility.h>
#include <QtGui/private/qguiapplication_p.h>

#if defined(Q_OS_WIN) && defined(interface)
#   undef interface
#endif

#include <QtGui/private/qaccessible2_p.h>
#include <QtWidgets/private/qaccessiblewidget_p.h>
#include "QtTest/qtestaccessible.h"

// Make a widget frameless to prevent size constraints of title bars
// from interfering (Windows).
static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

#if defined(Q_OS_WINCE)
extern "C" bool SystemParametersInfo(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
#define SPI_GETPLATFORMTYPE 257
inline bool IsValidCEPlatform() {
    wchar_t tszPlatform[64];
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(tszPlatform) / sizeof(*tszPlatform), tszPlatform, 0)) {
        QString platform = QString::fromWCharArray(tszPlatform);
        if ((platform == QLatin1String("PocketPC")) || (platform == QLatin1String("Smartphone")))
            return false;
    }
    return true;
}
#endif

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

static inline int indexOfChild(QAccessibleInterface *parentInterface, QWidget *childWidget)
{
    if (!parentInterface || !childWidget)
        return -1;
    QAccessibleInterface *childInterface(QAccessible::queryAccessibleInterface(childWidget));
    if (!childInterface)
        return -1;
    return parentInterface->indexOfChild(childInterface);
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
    virtual ~tst_QAccessibility();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void eventTest();
    void customWidget();
    void deletedWidget();
    void subclassedWidget();

    void statesStructTest();
    void navigateHierarchy();
    void sliderTest();
    void textAttributes();
    void hideShowTest();

    void actionTest();

    void applicationTest();
    void mainWindowTest();
    void buttonTest();
    void scrollBarTest();
    void tabTest();
    void tabWidgetTest();
    void menuTest();
    void spinBoxTest();
    void doubleSpinBoxTest();
    void textEditTest();
    void textBrowserTest();
    void mdiAreaTest();
    void mdiSubWindowTest();
    void lineEditTest();
    void groupBoxTest();
    void dialogButtonBoxTest();
    void dialTest();
    void rubberBandTest();
    void abstractScrollAreaTest();
    void scrollAreaTest();

    void listTest();
    void treeTest();
    void tableTest();

    void calendarWidgetTest();
    void dockWidgetTest();
    void comboBoxTest();
    void accessibleName();
    void labelTest();
    void accelerators();
    void bridgeTest();

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

tst_QAccessibility::~tst_QAccessibility()
{
}

void tst_QAccessibility::onClicked()
{
    click_count++;
}

void tst_QAccessibility::initTestCase()
{
    QTestAccessibility::initialize();
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    pfIntegration->accessibility()->setActive(true);
}

void tst_QAccessibility::cleanupTestCase()
{
    QTestAccessibility::cleanup();
}

void tst_QAccessibility::init()
{
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::cleanup()
{
    const EventList list = QTestAccessibility::events();
    if (!list.isEmpty()) {
        qWarning("%d accessibility event(s) were not handled in testfunction '%s':", list.count(),
                 QString(QTest::currentTestFunction()).toLatin1().constData());
        for (int i = 0; i < list.count(); ++i)
            qWarning(" %d: Object: %p Event: '%s' Child: %d", i + 1, list.at(i)->object(),
                     qAccessibleEventString(list.at(i)->type()), list.at(i)->child());
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::eventTest()
{
    QPushButton* button = new QPushButton(0);
    QAccessible::queryAccessibleInterface(button);
    button->setObjectName(QString("Olaf"));
    setFrameless(button);

    button->show();
    QAccessibleEvent showEvent(button, QAccessible::ObjectShow);
    // some platforms might send other events first, (such as state change event active=1)
    QVERIFY(QTestAccessibility::containsEvent(&showEvent));
    button->setFocus(Qt::MouseFocusReason);
    QTestAccessibility::clearEvents();
    QTest::mouseClick(button, Qt::LeftButton, 0);

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

    delete button;

    // Make sure that invalid events don't bring down the system
    // these events can be in user code.
    QWidget *widget = new QWidget();
    QAccessibleEvent ev1(widget, QAccessible::Focus);
    QAccessible::updateAccessibility(&ev1);

    QAccessibleEvent ev2(widget, QAccessible::Focus);
    ev2.setChild(7);
    QAccessible::updateAccessibility(&ev2);
    delete widget;

    QObject *object = new QObject();
    QAccessibleEvent ev3(object, QAccessible::Focus);
    QAccessible::updateAccessibility(&ev3);
    delete object;

    QTestAccessibility::clearEvents();
}


class QtTestAccessibleWidget: public QWidget
{
    Q_OBJECT
public:
    QtTestAccessibleWidget(QWidget *parent, const char *name): QWidget(parent)
    {
        setObjectName(name);
    }
};

class QtTestAccessibleWidgetIface: public QAccessibleWidget
{
public:
    QtTestAccessibleWidgetIface(QtTestAccessibleWidget *w): QAccessibleWidget(w) {}
    QString text(QAccessible::Text t) const
    {
        if (t == QAccessible::Help)
            return QString::fromLatin1("Help yourself");
        return QAccessibleWidget::text(t);
    }
    static QAccessibleInterface *ifaceFactory(const QString &key, QObject *o)
    {
        if (key == "QtTestAccessibleWidget")
            return new QtTestAccessibleWidgetIface(static_cast<QtTestAccessibleWidget*>(o));
        return 0;
    }
};

class QtTestAccessibleWidgetSubclass: public QtTestAccessibleWidget
{
    Q_OBJECT
public:
    QtTestAccessibleWidgetSubclass(QWidget *parent, const char *name): QtTestAccessibleWidget(parent, name)
    {}
};

void tst_QAccessibility::customWidget()
{
    {
    QtTestAccessibleWidget* widget = new QtTestAccessibleWidget(0, "Heinz");
    widget->show();
    QTest::qWaitForWindowExposed(widget);
    // By default we create QAccessibleWidget
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)widget);
    QCOMPARE(iface->object()->objectName(), QString("Heinz"));
    QCOMPARE(iface->rect().height(), widget->height());
    QCOMPARE(iface->text(QAccessible::Help), QString());
    QCOMPARE(iface->rect().height(), widget->height());
    delete widget;
    }
    {
    QAccessible::installFactory(QtTestAccessibleWidgetIface::ifaceFactory);
    QtTestAccessibleWidget* widget = new QtTestAccessibleWidget(0, "Heinz");
    widget->show();
    QTest::qWaitForWindowExposed(widget);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)widget);
    QCOMPARE(iface->object()->objectName(), QString("Heinz"));
    QCOMPARE(iface->rect().height(), widget->height());
    // The help text is only set if our factory works
    QCOMPARE(iface->text(QAccessible::Help), QString("Help yourself"));
    delete widget;
    }
    {
    // A subclass of any class should still get the right QAccessibleInterface
    QtTestAccessibleWidgetSubclass* subclassedWidget = new QtTestAccessibleWidgetSubclass(0, "Hans");
    QAccessibleInterface *subIface = QAccessible::queryAccessibleInterface(subclassedWidget);
    QVERIFY(subIface != 0);
    QVERIFY(subIface->isValid());
    QCOMPARE(subIface->object(), (QObject*)subclassedWidget);
    QCOMPARE(subIface->text(QAccessible::Help), QString("Help yourself"));
    delete subclassedWidget;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::deletedWidget()
{
    QtTestAccessibleWidget *widget = new QtTestAccessibleWidget(0, "Ralf");
    QAccessible::installFactory(QtTestAccessibleWidgetIface::ifaceFactory);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(iface != 0);
    QVERIFY(iface->isValid());
    QCOMPARE(iface->object(), (QObject*)widget);

    delete widget;
    widget = 0;
    // fixme: QVERIFY(!iface->isValid());
}

class KFooButton: public QPushButton
{
    Q_OBJECT
public:
    KFooButton(const QString &text, QWidget* parent = 0) : QPushButton(text, parent)
    {}
};

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
    QVERIFY(s2 == s1);
    s2.busy = true;
    QVERIFY(!(s2 == s1));
    s1.busy = true;
    QVERIFY(s2 == s1);
    s1 = QAccessible::State();
    QVERIFY(!(s2 == s1));
    s1 = s2;
    QVERIFY(s2 == s1);
    QVERIFY(s1.busy == 1);
}

void tst_QAccessibility::sliderTest()
{
    {
    QSlider *slider = new QSlider(0);
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

    delete slider;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::navigateHierarchy()
{
    {
    QWidget *w = new QWidget(0);
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
    QVERIFY(target == 0);
    target = ifaceW->child(-1);
    QVERIFY(target == 0);
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
    QVERIFY(child == 0);
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

    delete w;
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
    box->addWidget(new QPushButton(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QHeaderView(Qt::Vertical, w));
    box->addWidget(new QTreeView(w));
    box->addWidget(new QTreeWidget(w));
    box->addWidget(new QListView(w));
    box->addWidget(new QListWidget(w));
    box->addWidget(new QTableView(w));
    box->addWidget(new QTableWidget(w));
    box->addWidget(new QCalendarWidget(w));
    box->addWidget(new QDialogButtonBox(w));
    box->addWidget(new QGroupBox(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QFrame(w));
    box->addWidget(new QLineEdit(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QProgressBar(w));
    box->addWidget(new QTabWidget(w));
    box->addWidget(new QCheckBox(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QRadioButton(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QDial(w));
    box->addWidget(new QScrollBar(w));
    box->addWidget(new QSlider(w));
    box->addWidget(new QDateTimeEdit(w));
    box->addWidget(new QDoubleSpinBox(w));
    box->addWidget(new QSpinBox(w));
    box->addWidget(new QLabel(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QLCDNumber(w));
    box->addWidget(new QStackedWidget(w));
    box->addWidget(new QToolBox(w));
    box->addWidget(new QLabel(QString::fromLatin1("widget text %1").arg(i++), w));
    box->addWidget(new QTextEdit(QString::fromLatin1("widget text %1").arg(i++), w));

    /* Not in the list
     * QAbstractItemView, QGraphicsView, QScrollArea,
     * QToolButton, QDockWidget, QFocusFrame, QMainWindow, QMenu, QMenuBar, QSizeGrip, QSplashScreen, QSplitterHandle,
     * QStatusBar, QSvgWidget, QTabBar, QToolBar, QSplitter
     */
    return w;
}

void tst_QAccessibility::accessibleName()
{
    QWidget *toplevel = createWidgets();
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

    delete toplevel;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::textAttributes()
{
    QTextEdit textEdit;
    int startOffset;
    int endOffset;
    QString attributes;
    QString text("<html><head></head><body>"
                 "Hello, <b>this</b> is an <i><b>example</b> text</i>."
                 "<span style=\"font-family: monospace\">Multiple fonts are used.</span>"
                 "Multiple <span style=\"font-size: 8pt\">text sizes</span> are used."
                 "Let's give some color to <span style=\"color:#f0f1f2; background-color:#14f01e\">Qt</span>."
                 "</body></html>");

    textEdit.setText(text);
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&textEdit);

    QAccessibleTextInterface *textInterface=interface->textInterface();

    QVERIFY(textInterface);
    QCOMPARE(textInterface->characterCount(), 112);

    attributes = textInterface->attributes(10, &startOffset, &endOffset);
    QCOMPARE(startOffset, 7);
    QCOMPARE(endOffset, 11);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-weight:bold;")));

    attributes = textInterface->attributes(18, &startOffset, &endOffset);
    QCOMPARE(startOffset, 18);
    QCOMPARE(endOffset, 25);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-weight:bold;")));
    QVERIFY(attributes.contains(QLatin1String(";font-style:italic;")));

    attributes = textInterface->attributes(34, &startOffset, &endOffset);
    QCOMPARE(startOffset, 31);
    QCOMPARE(endOffset, 55);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-family:\"monospace\";")));

    attributes = textInterface->attributes(65, &startOffset, &endOffset);
    QCOMPARE(startOffset, 64);
    QCOMPARE(endOffset, 74);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";font-size:8pt;")));

    attributes = textInterface->attributes(110, &startOffset, &endOffset);
    QCOMPARE(startOffset, 109);
    QCOMPARE(endOffset, 111);
    attributes.prepend(';');
    QVERIFY(attributes.contains(QLatin1String(";background-color:rgb(20,240,30);")));
    QVERIFY(attributes.contains(QLatin1String(";color:rgb(240,241,242);")));
}

void tst_QAccessibility::hideShowTest()
{
    QWidget * const window = new QWidget();
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

    delete window;
    QTestAccessibility::clearEvents();
}


void tst_QAccessibility::actionTest()
{
    {
    QCOMPARE(QAccessibleActionInterface::pressAction(), QString(QStringLiteral("Press")));

    QWidget *widget = new QWidget;
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

    delete widget;
    }
    QTestAccessibility::clearEvents();

    {
    QPushButton *button = new QPushButton;
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

    delete button;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::applicationTest()
{
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

    QWidget widget;
    widget.show();
    qApp->setActiveWindow(&widget);
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
    {
    QMainWindow *mw = new QMainWindow;
    mw->resize(300, 200);
    mw->show(); // triggers layout
    qApp->setActiveWindow(mw);

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


    delete mw;
    }
    QTestAccessibility::clearEvents();

    {
    QWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);

//    We currently don't have an accessible interface for QWindow
//    the active state is either in the QMainWindow or QQuickView
//    QAccessibleInterface *windowIface(QAccessible::queryAccessibleInterface(&window));
//    QVERIFY(windowIface->state().active);

    QAccessible::State activeState;
    activeState.active = true;
    QAccessibleStateChangeEvent active(&window, activeState);
    QVERIFY_EVENT(&active);

    QWindow child;
    child.setParent(&window);
    child.setGeometry(10, 10, 20, 20);
    child.show();

    child.requestActivate();
    QTRY_VERIFY(QGuiApplication::focusWindow() == &child);

    QAccessibleStateChangeEvent deactivate(&window, activeState);
    QVERIFY_EVENT(&deactivate); // deactivation of parent

    QAccessibleStateChangeEvent activeChild(&child, activeState);
    QVERIFY_EVENT(&activeChild);
    }
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
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::toggleAction() << QAccessibleActionInterface::setFocusAction());
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
    QAction *foo = new QAction("Foo", 0);
    foo->setShortcut(QKeySequence("Ctrl+F"));
    QMenu *menu = new QMenu();
    menu->addAction(foo);
    QPushButton menuButton;
    setFrameless(&menuButton);
    menuButton.setMenu(menu);
    menuButton.show();
    QAccessibleInterface *interface = QAccessible::queryAccessibleInterface(&menuButton);
    QCOMPARE(interface->role(), QAccessible::ButtonMenu);
    QVERIFY(interface->state().hasPopup);
    QCOMPARE(interface->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction() << QAccessibleActionInterface::setFocusAction());
    // showing the menu enters a new event loop...
//    interface->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
//    QTest::qWait(500);
    delete menu;
    }


    QTestAccessibility::clearEvents();
    {
    // test check box
    interface = QAccessible::queryAccessibleInterface(&checkBox);
    actionInterface = interface->actionInterface();
    QCOMPARE(interface->role(), QAccessible::CheckBox);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::toggleAction() << QAccessibleActionInterface::setFocusAction());
    QVERIFY(!interface->state().checked);
    actionInterface->doAction(QAccessibleActionInterface::toggleAction());

    QTest::qWait(500);
    QCOMPARE(actionInterface->actionNames(), QStringList() << QAccessibleActionInterface::toggleAction() << QAccessibleActionInterface::setFocusAction());
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

void tst_QAccessibility::scrollBarTest()
{
    QScrollBar *scrollBar  = new QScrollBar(Qt::Horizontal);
    QAccessibleInterface * const scrollBarInterface = QAccessible::queryAccessibleInterface(scrollBar);
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

    delete scrollBar;

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tabTest()
{
    QTabBar *tabBar = new QTabBar();
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
    for (int i = 0; i < lots; ++i)
        tabBar->addTab("Foo");

    QAccessibleInterface *child1 = interface->child(0);
    QAccessibleInterface *child2 = interface->child(1);
    QVERIFY(child1);
    QCOMPARE(child1->role(), QAccessible::PageTab);
    QVERIFY(child2);
    QCOMPARE(child2->role(), QAccessible::PageTab);

    QVERIFY((child1->state().invisible) == false);
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

    delete tabBar;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::tabWidgetTest()
{
    QTabWidget *tabWidget = new QTabWidget();
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
    QVERIFY(tabBarInterface);
    QCOMPARE(tabBarInterface->childCount(), 4);
    QCOMPARE(tabBarInterface->role(), QAccessible::PageTabList);

    QAccessibleInterface* tabButton1Interface = tabBarInterface->child(0);
    QVERIFY(tabButton1Interface);
    QCOMPARE(tabButton1Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton1Interface->text(QAccessible::Name), QLatin1String("Tab 1"));

    QAccessibleInterface* tabButton2Interface = tabBarInterface->child(1);
    QVERIFY(tabButton1Interface);
    QCOMPARE(tabButton2Interface->role(), QAccessible::PageTab);
    QCOMPARE(tabButton2Interface->text(QAccessible::Name), QLatin1String("Tab 2"));

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
#ifndef Q_CC_INTEL
    QCOMPARE(stackChild1Interface->childCount(), 0);
#endif
    QCOMPARE(stackChild1Interface->role(), QAccessible::StaticText);
    QCOMPARE(stackChild1Interface->text(QAccessible::Name), QLatin1String("Page 1"));
    QCOMPARE(label1, stackChild1Interface->object());

    // Navigation in stack widgets should be consistent
    QAccessibleInterface* parent = stackChild1Interface->parent();
    QVERIFY(parent);
#ifndef Q_CC_INTEL
    QCOMPARE(parent->childCount(), 2);
#endif
    QCOMPARE(parent->role(), QAccessible::LayeredPane);

    QAccessibleInterface* stackChild2Interface = stackWidgetInterface->child(1);
    QVERIFY(stackChild2Interface);
    QCOMPARE(stackChild2Interface->childCount(), 0);
    QCOMPARE(stackChild2Interface->role(), QAccessible::StaticText);
    QCOMPARE(label2, stackChild2Interface->object());
    QCOMPARE(label2->text(), stackChild2Interface->text(QAccessible::Name));

    parent = stackChild2Interface->parent();
    QVERIFY(parent);
#ifndef Q_CC_INTEL
    QCOMPARE(parent->childCount(), 2);
#endif
    QCOMPARE(parent->role(), QAccessible::LayeredPane);

    delete tabWidget;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::menuTest()
{
    {
    QMainWindow mw;
    mw.resize(300, 200);
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
#ifdef Q_OS_WINCE
    if (!IsValidCEPlatform())
        QSKIP("Tests do not work on Mobile platforms due to native menus");
#endif
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
    QVERIFY(file->isVisible() && !edit->isVisible() && !help->isVisible());
    iEdit->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(!file->isVisible() && edit->isVisible() && !help->isVisible());
    iHelp->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(!file->isVisible() && !edit->isVisible() && help->isVisible());
    iAction->actionInterface()->doAction(QAccessibleActionInterface::showMenuAction());
    if(menuFade)
        QTest::qWait(menuFadeDelay);
    QVERIFY(!file->isVisible() && !edit->isVisible() && !help->isVisible());

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

    QVERIFY(file->isVisible());
    QVERIFY(fileNew->isVisible());
    QVERIFY(!edit->isVisible());
    QVERIFY(!help->isVisible());

    QTestAccessibility::clearEvents();
    mw.hide();

    // Do not crash if the menu don't have a parent
    QMenu *menu = new QMenu;
    menu->addAction(QLatin1String("one"));
    menu->addAction(QLatin1String("two"));
    menu->addAction(QLatin1String("three"));
    iface = QAccessible::queryAccessibleInterface(menu);
    iface2 = iface->parent();
    QVERIFY(iface2);
    QCOMPARE(iface2->role(), QAccessible::Application);
    // caused a *crash*
    iface2->state();
    delete menu;

    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::spinBoxTest()
{
    QSpinBox * const spinBox = new QSpinBox();
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

    // one child, the line edit
    const int numChildren = interface->childCount();
    QCOMPARE(numChildren, 1);
    QAccessibleInterface *lineEdit = interface->child(0);

    QCOMPARE(lineEdit->role(), QAccessible::EditableText);
    QCOMPARE(lineEdit->text(QAccessible::Value), QLatin1String("3"));

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
    delete spinBox;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::doubleSpinBoxTest()
{
    QDoubleSpinBox *doubleSpinBox = new QDoubleSpinBox;
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

    delete doubleSpinBox;
    QTestAccessibility::clearEvents();
}

static QRect characterRect(const QTextEdit &edit, int offset)
{
    QTextBlock block = edit.document()->findBlock(offset);
    QTextLayout *layout = block.layout();
    QPointF layoutPosition = layout->position();
    int relativeOffset = offset - block.position();
    QTextLine line = layout->lineForTextPosition(relativeOffset);
    QFontMetrics fm(edit.currentFont());
    QChar ch = edit.document()->characterAt(offset);
    int w = fm.width(ch);
    int h = fm.height();

    qreal x = line.cursorToX(relativeOffset);
    QRect r(layoutPosition.x() + x, layoutPosition.y() + line.y(), w, h);
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

void tst_QAccessibility::textEditTest()
{
    for (int pass = 0; pass < 2; ++pass) {
        {
        QTextEdit edit;
        setFrameless(&edit);
        int startOffset;
        int endOffset;
        // create two blocks of text. The first block has two lines.
        QString text = "<p>hello world.<br/>How are you today?</p><p>I'm fine, thanks</p>";
        edit.setHtml(text);
        if (pass == 1) {
            QFont font("Helvetica");
            font.setPointSizeF(12.5);
            font.setWordSpacing(1.1);
            edit.setCurrentFont(font);
        }

        edit.show();
        QTest::qWaitForWindowShown(&edit);
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&edit);
        QCOMPARE(iface->text(QAccessible::Value), edit.toPlainText());
        QCOMPARE(iface->textInterface()->textAtOffset(8, QAccessible::WordBoundary, &startOffset, &endOffset), QString("world"));
        QCOMPARE(startOffset, 6);
        QCOMPARE(endOffset, 11);
        QCOMPARE(iface->textInterface()->textAtOffset(15, QAccessible::LineBoundary, &startOffset, &endOffset), QString("How are you today?"));
        QCOMPARE(startOffset, 13);
        QCOMPARE(endOffset, 31);
        QCOMPARE(iface->textInterface()->characterCount(), 48);
        QFontMetrics fm(edit.currentFont());
        QCOMPARE(iface->textInterface()->characterRect(0).size(), QSize(fm.width("h"), fm.height()));
        QCOMPARE(iface->textInterface()->characterRect(5).size(), QSize(fm.width(" "), fm.height()));
        QCOMPARE(iface->textInterface()->characterRect(6).size(), QSize(fm.width("w"), fm.height()));

        int offset = 10;
        QCOMPARE(iface->textInterface()->text(offset, offset + 1), QStringLiteral("d"));
        QVERIFY(fuzzyRectCompare(iface->textInterface()->characterRect(offset), characterRect(edit, offset)));
        offset = 13;
        QCOMPARE(iface->textInterface()->text(offset, offset + 1), QStringLiteral("H"));
        QVERIFY(fuzzyRectCompare(iface->textInterface()->characterRect(offset), characterRect(edit, offset)));
        offset = 21;
        QCOMPARE(iface->textInterface()->text(offset, offset + 1), QStringLiteral("y"));
        QVERIFY(fuzzyRectCompare(iface->textInterface()->characterRect(offset), characterRect(edit, offset)));
        offset = 32;
        QCOMPARE(iface->textInterface()->text(offset, offset + 1), QStringLiteral("I"));
        QVERIFY(fuzzyRectCompare(iface->textInterface()->characterRect(offset), characterRect(edit, offset)));

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
    QCOMPARE(subWindows.count(), subWindowCount);

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
    qApp->setActiveWindow(&mdiArea);
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
    QCOMPARE(subWindows.count(), subWindowCount);

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
    qApp->setActiveWindow(&mdiArea);
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
    QWidget *toplevel = new QWidget;
    {
    QLineEdit *le = new QLineEdit;
    QAccessibleInterface *iface(QAccessible::queryAccessibleInterface(le));
    QVERIFY(iface);
    le->show();

    QApplication::processEvents();
    QCOMPARE(iface->childCount(), 0);
    QVERIFY(iface->state().sizeable);
    QVERIFY(iface->state().movable);
    QVERIFY(iface->state().focusable);
    QVERIFY(iface->state().selectable);
    QVERIFY(iface->state().hasPopup);
    QCOMPARE(bool(iface->state().focused), le->hasFocus());

    QString secret(QLatin1String("secret"));
    le->setText(secret);
    le->setEchoMode(QLineEdit::Normal);
    QVERIFY(!(iface->state().passwordEdit));
    QCOMPARE(iface->text(QAccessible::Value), secret);
    le->setEchoMode(QLineEdit::NoEcho);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value).isEmpty());
    le->setEchoMode(QLineEdit::Password);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value).isEmpty());
    le->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    QVERIFY(iface->state().passwordEdit);
    QVERIFY(iface->text(QAccessible::Value).isEmpty());
    le->setEchoMode(QLineEdit::Normal);
    QVERIFY(!(iface->state().passwordEdit));
    QCOMPARE(iface->text(QAccessible::Value), secret);

    le->setParent(toplevel);
    toplevel->show();
    QApplication::processEvents();
    QVERIFY(!(iface->state().sizeable));
    QVERIFY(!(iface->state().movable));
    QVERIFY(iface->state().focusable);
    QVERIFY(iface->state().selectable);
    QVERIFY(iface->state().hasPopup);
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

    delete le;
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
    QCOMPARE(end, cite.length());
    QCOMPARE(textIface->textAtOffset(5, QAccessible::LineBoundary,&start,&end), cite);
    QCOMPARE(textIface->textAtOffset(5, QAccessible::NoBoundary,&start,&end), cite);

    QTestAccessibility::clearEvents();
    }

    {
        QLineEdit le(QStringLiteral("My characters have geometries."), toplevel);
        // characterRect()
        le.show();
        QTest::qWaitForWindowShown(&le);
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
    sel.setSelection(0, lineEdit->text().length());
    sel.setCursorPosition(lineEdit->text().length());
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

    QAccessibleTextInsertEvent insertD(lineEdit, 3, "D");
    QVERIFY_EVENT(&insertD);
    keys.clear();
    keys.addKeyClick('E');
    keys.simulate(lineEdit);

    QAccessibleTextInsertEvent insertE(lineEdit, 4, "E");
    QVERIFY(QTestAccessibility::containsEvent(&insertE));
    keys.clear();
    keys.addKeyClick(Qt::Key_Left);
    keys.addKeyClick(Qt::Key_Left);
    keys.simulate(lineEdit);
    cursorEvent.setCursorPosition(4);
    QVERIFY(QTestAccessibility::containsEvent(&cursorEvent));
    cursorEvent.setCursorPosition(3);
    QVERIFY(QTestAccessibility::containsEvent(&cursorEvent));

    keys.clear();
    keys.addKeyClick('C');
    keys.simulate(lineEdit);

    QAccessibleTextInsertEvent insertC(lineEdit, 3, "C");
    QVERIFY(QTestAccessibility::containsEvent(&insertC));

    keys.clear();
    keys.addKeyClick('O');
    keys.simulate(lineEdit);
    QAccessibleTextInsertEvent insertO(lineEdit, 4, "O");
    QVERIFY(QTestAccessibility::containsEvent(&insertO));
    }
    delete toplevel;
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::groupBoxTest()
{
    {
    QGroupBox *groupBox = new QGroupBox();
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
    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> > relations = rButtonIface->relations();
    QVERIFY(relations.size() == 1);
    QPair<QAccessibleInterface*, QAccessible::Relation> relation = relations.first();
    QCOMPARE(relation.first->object(), groupBox);
    QCOMPARE(relation.second, QAccessible::Label);

    delete groupBox;
    }

    {
    QGroupBox *groupBox = new QGroupBox();
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

    delete groupBox;
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

    QVector<QAccessibleInterface *> buttons;
    for (int i = 0; i < iface->childCount(); ++i)
        buttons <<  iface->child(i);

    qSort(buttons.begin(), buttons.end(), accessibleInterfaceLeftOf);

    for (int i = 0; i < buttons.count(); ++i)
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

    QVector<QAccessibleInterface *> buttons;
    for (int i = 0; i < iface->childCount(); ++i)
        buttons <<  iface->child(i);

    qSort(buttons.begin(), buttons.end(), accessibleInterfaceAbove);

    for (int i = 0; i < buttons.count(); ++i)
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
    QCOMPARE(interface->childCount(), 2);
    QWidget *horizontalScrollBar = abstractScrollArea.horizontalScrollBar();
    QWidget *horizontalScrollBarContainer = horizontalScrollBar->parentWidget();
    QVERIFY(verifyChild(horizontalScrollBarContainer, interface, 1, globalGeometry));

    // Horizontal scrollBar widgets.
    QLabel *secondLeftLabel = new QLabel(QLatin1String("L2"));
    abstractScrollArea.addScrollBarWidget(secondLeftLabel, Qt::AlignLeft);
    QCOMPARE(interface->childCount(), 2);

    QLabel *firstLeftLabel = new QLabel(QLatin1String("L1"));
    abstractScrollArea.addScrollBarWidget(firstLeftLabel, Qt::AlignLeft);
    QCOMPARE(interface->childCount(), 2);

    QLabel *secondRightLabel = new QLabel(QLatin1String("R2"));
    abstractScrollArea.addScrollBarWidget(secondRightLabel, Qt::AlignRight);
    QCOMPARE(interface->childCount(), 2);

    QLabel *firstRightLabel = new QLabel(QLatin1String("R1"));
    abstractScrollArea.addScrollBarWidget(firstRightLabel, Qt::AlignRight);
    QCOMPARE(interface->childCount(), 2);

    // Vertical scrollBar.
    abstractScrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QCOMPARE(interface->childCount(), 3);
    QWidget *verticalScrollBar = abstractScrollArea.verticalScrollBar();
    QWidget *verticalScrollBarContainer = verticalScrollBar->parentWidget();
    QVERIFY(verifyChild(verticalScrollBarContainer, interface, 2, globalGeometry));

    // Vertical scrollBar widgets.
    QLabel *secondTopLabel = new QLabel(QLatin1String("T2"));
    abstractScrollArea.addScrollBarWidget(secondTopLabel, Qt::AlignTop);
    QCOMPARE(interface->childCount(), 3);

    QLabel *firstTopLabel = new QLabel(QLatin1String("T1"));
    abstractScrollArea.addScrollBarWidget(firstTopLabel, Qt::AlignTop);
    QCOMPARE(interface->childCount(), 3);

    QLabel *secondBottomLabel = new QLabel(QLatin1String("B2"));
    abstractScrollArea.addScrollBarWidget(secondBottomLabel, Qt::AlignBottom);
    QCOMPARE(interface->childCount(), 3);

    QLabel *firstBottomLabel = new QLabel(QLatin1String("B1"));
    abstractScrollArea.addScrollBarWidget(firstBottomLabel, Qt::AlignBottom);
    QCOMPARE(interface->childCount(), 3);

    // CornerWidget.
    abstractScrollArea.setCornerWidget(new QLabel(QLatin1String("C")));
    QCOMPARE(interface->childCount(), 4);
    QWidget *cornerWidget = abstractScrollArea.cornerWidget();
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
    QListWidget *listView = new QListWidget;
    listView->addItem("Oslo");
    listView->addItem("Berlin");
    listView->addItem("Brisbane");
    listView->resize(400,400);
    listView->show();
    QTest::qWait(1); // Need this for indexOfchild to work.
    QCoreApplication::processEvents();
    QTest::qWait(100);

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
    QTestAccessibility::clearEvents();

    // Check for events
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, 0, listView->visualItemRect(listView->item(1)).center());
    QAccessibleEvent selectionEvent(listView, QAccessible::Selection);
    selectionEvent.setChild(1);
    QAccessibleEvent focusEvent(listView, QAccessible::Focus);
    focusEvent.setChild(1);
    QVERIFY(QTestAccessibility::containsEvent(&selectionEvent));
    QVERIFY(QTestAccessibility::containsEvent(&focusEvent));
    QTest::mouseClick(listView->viewport(), Qt::LeftButton, 0, listView->visualItemRect(listView->item(2)).center());

    QAccessibleEvent selectionEvent2(listView, QAccessible::Selection);
    selectionEvent2.setChild(2);
    QAccessibleEvent focusEvent2(listView, QAccessible::Focus);
    focusEvent2.setChild(2);
    QVERIFY(QTestAccessibility::containsEvent(&selectionEvent2));
    QVERIFY(QTestAccessibility::containsEvent(&focusEvent2));

    listView->addItem("Munich");
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
    QCOMPARE(cellInterface->rowHeaderCells(), QList<QAccessibleInterface*>());
    QCOMPARE(cellInterface->columnHeaderCells(), QList<QAccessibleInterface*>());

    QCOMPARE(cellInterface->table()->object(), listView);

    listView->clearSelection();
    QVERIFY(!(cell4->state().expandable));
    QVERIFY( (cell4->state().selectable));
    QVERIFY(!(cell4->state().selected));
    table2->selectRow(3);
    QCOMPARE(listView->selectedItems().size(), 1);
    QCOMPARE(listView->selectedItems().at(0)->text(), QLatin1String("Munich"));
    QVERIFY(cell4->state().selected);
    QVERIFY(cellInterface->isSelected());

    QVERIFY(table2->cellAt(-1, 0) == 0);
    QVERIFY(table2->cellAt(0, -1) == 0);
    QVERIFY(table2->cellAt(0, 1) == 0);
    QVERIFY(table2->cellAt(4, 0) == 0);

    // verify that unique id stays the same
    QAccessible::Id axidMunich = QAccessible::uniqueId(cell4);
    // insertion and deletion of items
    listView->insertItem(1, "Helsinki");
    // list: Oslo, Helsinki, Berlin, Brisbane, Munich

    QAccessibleInterface *cellMunich2 = table2->cellAt(4,0);
    QCOMPARE(cell4, cellMunich2);
    QCOMPARE(axidMunich, QAccessible::uniqueId(cellMunich2));

    delete listView->takeItem(2);
    delete listView->takeItem(2);
    // list: Oslo, Helsinki, Munich

    QAccessibleInterface *cellMunich3 = table2->cellAt(2,0);
    QCOMPARE(cell4, cellMunich3);
    QCOMPARE(axidMunich, QAccessible::uniqueId(cellMunich3));


    delete listView;
    }
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::treeTest()
{
    QTreeWidget *treeView = new QTreeWidget;

    // Empty model (do not crash, etc)
    treeView->setColumnCount(0);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(treeView);
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

    treeView->setHeaderHidden(true);
    QAccessibleInterface *accSpain = iface->child(0);
    QCOMPARE(accSpain->role(), QAccessible::TreeItem);
    QCOMPARE(iface->indexOfChild(accSpain), 0);
    treeView->setHeaderHidden(false);

    QTestAccessibility::clearEvents();

    // table 2
    QAccessibleTableInterface *table2 = iface->tableInterface();
    QVERIFY(table2);
    QCOMPARE(table2->columnCount(), 2);
    QCOMPARE(table2->rowCount(), 2);
    QAccessibleInterface *cell1;
    QVERIFY(cell1 = table2->cellAt(0,0));
    QCOMPARE(cell1->text(QAccessible::Name), QString("Spain"));
    QAccessibleInterface *cell2;
    QVERIFY(cell2 = table2->cellAt(1,0));
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

    delete treeView;
    QTestAccessibility::clearEvents();
}

// The table used below is this:
// Button  (0) | h1   (1) | h2   (2) | h3   (3)
// v1      (4) | 0.0  (5) | 1.0  (6) | 2.0  (7)
// v2      (8) | 0.1  (9) | 1.1 (10) | 2.1 (11)
// v3     (12) | 0.2 (13) | 1.2 (14) | 2.2 (15)
void tst_QAccessibility::tableTest()
{
    QTableWidget *tableView = new QTableWidget(3, 3);
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
    QTest::qWaitForWindowExposed(tableView);

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
    QAccessibleInterface *cell1;
    QVERIFY(cell1 = table2->cellAt(0,0));
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
    delete tableView;

    QVERIFY(!QAccessible::accessibleInterface(id00));
    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::calendarWidgetTest()
{
#ifndef QT_NO_CALENDARWIDGET
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
    foreach (QObject *child, calendarWidget.children()) {
        if (child->objectName() == QLatin1String("qt_calendar_navigationbar")) {
            navigationBar = static_cast<QWidget *>(child);
            break;
        }
    }
    QVERIFY(navigationBar);
    QVERIFY(verifyChild(navigationBar, interface, 0, globalGeometry));

    QAbstractItemView *calendarView = 0;
    foreach (QObject *child, calendarWidget.children()) {
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
#endif // QT_NO_CALENDARWIDGET
}

void tst_QAccessibility::dockWidgetTest()
{
#ifndef QT_NO_DOCKWIDGET
    // Set up a proper main window with two dock widgets
    QMainWindow *mw = new QMainWindow();
    QFrame *central = new QFrame(mw);
    mw->setCentralWidget(central);
    QMenuBar *mb = new QMenuBar(mw);
    mb->addAction(tr("&File"));
    mw->setMenuBar(mb);

    QDockWidget *dock1 = new QDockWidget(mw);
    mw->addDockWidget(Qt::LeftDockWidgetArea, dock1);
    QPushButton *pb1 = new QPushButton(tr("Push me"), dock1);
    dock1->setWidget(pb1);

    QDockWidget *dock2 = new QDockWidget(mw);
    mw->addDockWidget(Qt::BottomDockWidgetArea, dock2);
    QPushButton *pb2 = new QPushButton(tr("Push me"), dock2);
    dock2->setWidget(pb2);

    mw->resize(600,400);
    mw->show();
#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
    QTest::qWait(100);
#endif

    QAccessibleInterface *accMainWindow = QAccessible::queryAccessibleInterface(mw);
    // 4 children: menu bar, dock1, dock2, and central widget
    QCOMPARE(accMainWindow->childCount(), 4);
    QAccessibleInterface *accDock1 = 0;
    for (int i = 0; i < 4; ++i) {
        accDock1 = accMainWindow->child(i);
        if (accMainWindow->role() == QAccessible::Window) {
            if (accDock1 && qobject_cast<QDockWidget*>(accDock1->object()) == dock1) {
                break;
            }
        }
    }
    QVERIFY(accDock1);
    QCOMPARE(accDock1->role(), QAccessible::Window);

    QAccessibleInterface *dock1TitleBar = accDock1->child(0);
    QCOMPARE(dock1TitleBar->role(), QAccessible::TitleBar);
    QVERIFY(accDock1->rect().contains(dock1TitleBar->rect()));

    QPoint globalPos = dock1->mapToGlobal(QPoint(0,0));
    globalPos.rx()+=5;  //### query style
    globalPos.ry()+=5;
    QAccessibleInterface *childAt = accDock1->childAt(globalPos.x(), globalPos.y());    //###
    QCOMPARE(childAt->role(), QAccessible::TitleBar);
    int index = accDock1->indexOfChild(childAt);
    QAccessibleInterface *accTitleBar = accDock1->child(index);

    QCOMPARE(accTitleBar->role(), QAccessible::TitleBar);
    QCOMPARE(accDock1->indexOfChild(accTitleBar), 0);
    QAccessibleInterface *acc;
    acc = accTitleBar->parent();
    QVERIFY(acc);
    QCOMPARE(acc->role(), QAccessible::Window);

    delete pb1;
    delete pb2;
    delete dock1;
    delete dock2;
    delete mw;
    QTestAccessibility::clearEvents();
#endif // QT_NO_DOCKWIDGET
}

void tst_QAccessibility::comboBoxTest()
{
#if defined(Q_OS_WINCE)
    if (!IsValidCEPlatform())
        QSKIP("Test skipped on Windows Mobile test hardware");
#endif
    { // not editable combobox
    QComboBox combo;
    combo.addItems(QStringList() << "one" << "two" << "three");
    // Fully decorated windows have a minimum width of 160 on Windows.
    combo.setMinimumWidth(200);
    combo.show();
    QVERIFY(QTest::qWaitForWindowShown(&combo));

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(&combo);
    QCOMPARE(verifyHierarchy(iface), 0);

    QCOMPARE(iface->role(), QAccessible::ComboBox);
    QCOMPARE(iface->childCount(), 1);

#ifdef Q_OS_UNIX
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("one"));
#endif
    QCOMPARE(iface->text(QAccessible::Value), QLatin1String("one"));
    combo.setCurrentIndex(2);
#ifdef Q_OS_UNIX
    QCOMPARE(iface->text(QAccessible::Name), QLatin1String("three"));
#endif
    QCOMPARE(iface->text(QAccessible::Value), QLatin1String("three"));

    QAccessibleInterface *listIface = iface->child(0);
    QCOMPARE(listIface->role(), QAccessible::List);
    QCOMPARE(listIface->childCount(), 3);

    QVERIFY(!combo.view()->isVisible());
    QVERIFY(iface->actionInterface());
    QCOMPARE(iface->actionInterface()->actionNames(), QStringList() << QAccessibleActionInterface::showMenuAction());
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

void tst_QAccessibility::labelTest()
{
    QString text = "Hello World";
    QLabel *label = new QLabel(text);
    setFrameless(label);
    label->show();

#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);

    QAccessibleInterface *acc_label = QAccessible::queryAccessibleInterface(label);
    QVERIFY(acc_label);

    QCOMPARE(acc_label->text(QAccessible::Name), text);

    delete label;
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
    QCOMPARE(imageInterface->imagePosition().topLeft(), labelPos);

    QTestAccessibility::clearEvents();
}

void tst_QAccessibility::accelerators()
{
    QWidget *window = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(window);
    QLabel *label = new QLabel(tr("&Line edit"), window);
    QLineEdit *le = new QLineEdit(window);
    lay->addWidget(label);
    lay->addWidget(le);
    label->setBuddy(le);

    window->show();

    QAccessibleInterface *accLineEdit = QAccessible::queryAccessibleInterface(le);
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("L"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("L"));
    label->setText(tr("Q &"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q &&"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q && A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());
    label->setText(tr("Q &&&A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("A"));
    label->setText(tr("Q &&A"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QString());

#if !defined(QT_NO_DEBUG) && !defined(Q_OS_MAC)
    QTest::ignoreMessage(QtWarningMsg, "QKeySequence::mnemonic: \"Q &A&B\" contains multiple occurrences of '&'");
#endif
    label->setText(tr("Q &A&B"));
    QCOMPARE(accLineEdit->text(QAccessible::Accelerator), QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + QLatin1String("A"));

#if defined(Q_OS_UNIX)
    QCoreApplication::processEvents();
#endif
    QTest::qWait(100);
    delete window;
    QTestAccessibility::clearEvents();
}

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
#ifdef Q_OS_WIN
    // First, test MSAA part of bridge
    QWidget *window = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(window);
    QPushButton *button = new QPushButton(tr("Push me"), window);
    QTextEdit *te = new QTextEdit(window);
    te->setText(QLatin1String("hello world\nhow are you today?\n"));

    // Add QTableWidget
    QTableWidget *tableWidget = new QTableWidget(3, 3, window);
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

    lay->addWidget(button);
    lay->addWidget(te);
    lay->addWidget(tableWidget);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));


    /**************************************************
     *   QPushButton
     **************************************************/
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(button);
    QPoint buttonPos = button->mapToGlobal(QPoint(0,0));
    QRect buttonRect = iface->rect();
    QCOMPARE(buttonRect.topLeft(), buttonPos);

    // All set, now test the bridge.
    POINT pt;
    pt.x = buttonRect.center().x();
    pt.y = buttonRect.center().y();
    IAccessible *iaccButton;

    VARIANT varChild;
    HRESULT hr = ::AccessibleObjectFromPoint(pt, &iaccButton, &varChild);
    QVERIFY(SUCCEEDED(hr));
    VARIANT varSELF;
    varSELF.vt = VT_I4;
    varSELF.lVal = 0;

    // **** Test get_accRole ****
    VARIANT varRole;
    hr = iaccButton->get_accRole(varSELF, &varRole);
    QVERIFY(SUCCEEDED(hr));

    QCOMPARE(varRole.vt, (VARTYPE)VT_I4);
    QCOMPARE(varRole.lVal, (LONG)ROLE_SYSTEM_PUSHBUTTON);

    // **** Test accLocation ****
    long x, y, w, h;
    // Do not crash on insane arguments.
    varChild.lVal = 999;
    hr = iaccButton->accLocation(&x, &y, &w, &h, varChild);
    QCOMPARE(SUCCEEDED(hr), false);

    hr = iaccButton->accLocation(&x, &y, &w, &h, varSELF);
    QCOMPARE(buttonRect, QRect(x, y, w, h));

#ifdef QT_SUPPORTS_IACCESSIBLE2
    // Test IAccessible2 part of bridge
    if (IAccessible2 *ia2Button = (IAccessible2*)queryIA2(iaccButton, IID_IAccessible2)) {
        // The control supports IAccessible2.
        // ia2Button is the reference to the accessible object's IAccessible2 interface.

        /***** Test IAccessibleComponent *****/
        IAccessibleComponent *ia2Component = 0;
        hr = ia2Button->QueryInterface(IID_IAccessibleComponent, (void**)&ia2Component);
        QVERIFY(SUCCEEDED(hr));
        long x, y;
        hr = ia2Component->get_locationInParent(&x, &y);
        QVERIFY(SUCCEEDED(hr));
        QCOMPARE(button->pos(), QPoint(x, y));
        ia2Component->Release();

        /***** Test IAccessibleAction *****/
        IAccessibleAction *ia2Action = 0;
        hr = ia2Button->QueryInterface(IID_IAccessibleAction, (void**)&ia2Action);
        QVERIFY(SUCCEEDED(hr));
        QVERIFY(ia2Action);
        long nActions;
        ia2Action->nActions(&nActions);
        QVERIFY(nActions >= 1); // "Press" and "SetFocus"
        BSTR actionName;
        ia2Action->get_name(0, &actionName);
        QString name((QChar*)actionName);
        QCOMPARE(name, QAccessibleActionInterface::pressAction());
        ia2Action->Release();


        // Done testing
        ia2Button->Release();
    }
#endif
    iaccButton->Release();

    /**************************************************
     *   QWidget
     **************************************************/
    QWindow *windowHandle = window->windowHandle();
    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    HWND hWnd = (HWND)platform->nativeResourceForWindow("handle", windowHandle);

    IAccessible *iaccWindow;
    hr = ::AccessibleObjectFromWindow(hWnd, OBJID_CLIENT, IID_IAccessible, (void**)&iaccWindow);
    QVERIFY(SUCCEEDED(hr));
    hr = iaccWindow->get_accRole(varSELF, &varRole);
    QVERIFY(SUCCEEDED(hr));

    QCOMPARE(varRole.vt, (VARTYPE)VT_I4);
    QCOMPARE(varRole.lVal, (LONG)ROLE_SYSTEM_CLIENT);

    long nChildren;
    hr = iaccWindow->get_accChildCount(&nChildren);
    QVERIFY(SUCCEEDED(hr));
    QCOMPARE(nChildren, (long)3);

    /**************************************************
     *   QTextEdit
     **************************************************/
    // Get the second child (the accessible interface for the text edit)
    varChild.vt = VT_I4;
    varChild.lVal = 2;
    QVERIFY(iaccWindow);
    IAccessible *iaccTextEdit;
    hr = iaccWindow->get_accChild(varChild, (IDispatch**)&iaccTextEdit);
    QVERIFY(SUCCEEDED(hr));
    QVERIFY(iaccTextEdit);
    hr = iaccTextEdit->get_accRole(varSELF, &varRole);
    QVERIFY(SUCCEEDED(hr));

    QCOMPARE(varRole.vt, (VARTYPE)VT_I4);
    QCOMPARE(varRole.lVal, (LONG)ROLE_SYSTEM_TEXT);



#ifdef QT_SUPPORTS_IACCESSIBLE2
    if (IAccessibleEditableText *ia2TextEdit = (IAccessibleEditableText*)queryIA2(iaccTextEdit, IID_IAccessibleEditableText)) {
        ia2TextEdit->copyText(6, 11);
        QCOMPARE(QApplication::clipboard()->text(), QLatin1String("world"));
        ia2TextEdit->cutText(12, 16);
        QCOMPARE(QApplication::clipboard()->text(), QLatin1String("how "));
        ia2TextEdit->Release();
    }
    if (IAccessibleText *ia2Text = (IAccessibleText*)queryIA2(iaccTextEdit, IID_IAccessibleText)) {
        BSTR txt;
        hr = ia2Text->get_text(12, 15, &txt);
        QVERIFY(SUCCEEDED(hr));
        QCOMPARE(QString((QChar*)txt), QLatin1String("are"));
        ia2Text->Release();
    }
#endif
    iaccTextEdit->Release();


    /**************************************************
     *   QTableWidget
     **************************************************/
    {
        // Get the second child (the accessible interface for the table widget)
        varChild.vt = VT_I4;
        varChild.lVal = 3;
        QVERIFY(iaccWindow);
        IAccessible *iaccTable = 0;
        hr = iaccWindow->get_accChild(varChild, (IDispatch**)&iaccTable);
        QVERIFY(SUCCEEDED(hr));
        QVERIFY(iaccTable);
        hr = iaccTable->get_accRole(varSELF, &varRole);
        QVERIFY(SUCCEEDED(hr));

        QCOMPARE(varRole.vt, (VARTYPE)VT_I4);
        QCOMPARE(varRole.lVal, (LONG)ROLE_SYSTEM_TABLE);


#ifdef QT_SUPPORTS_IACCESSIBLE2
        IAccessibleTable2 *ia2Table = (IAccessibleTable2*)queryIA2(iaccTable, IID_IAccessibleTable2);
        QVERIFY(ia2Table);
        BSTR bstrDescription;
        hr = ia2Table->get_columnDescription(0, &bstrDescription);
        QVERIFY(SUCCEEDED(hr));
        const QString description((QChar*)bstrDescription);
        QCOMPARE(description, QLatin1String("h1"));

        IAccessible *accTableCell = 0;
        hr = ia2Table->get_cellAt(1, 2, (IUnknown**)&accTableCell);
        QVERIFY(SUCCEEDED(hr));
        QVERIFY(accTableCell);
        IAccessibleTableCell *ia2TableCell = (IAccessibleTableCell *)queryIA2(accTableCell, IID_IAccessibleTableCell);
        QVERIFY(ia2TableCell);
        LONG index;
        ia2TableCell->get_rowIndex(&index);
        QCOMPARE(index, (LONG)1);
        ia2TableCell->get_columnIndex(&index);
        QCOMPARE(index, (LONG)2);

        IAccessible *iaccTableCell = 0;
        hr = ia2TableCell->QueryInterface(IID_IAccessible, (void**)&iaccTableCell);
        QVERIFY(SUCCEEDED(hr));
        QVERIFY(iaccTableCell);
        BSTR bstrCellName;
        hr = iaccTableCell->get_accName(varSELF, &bstrCellName);
        QVERIFY(SUCCEEDED(hr));
        QString cellName((QChar*)bstrCellName);
        QCOMPARE(cellName, QLatin1String("1.2"));

        accTableCell->Release();
        iaccTableCell->Release();
        ia2TableCell->Release();
        ia2Table->Release();
#endif
        iaccTable->Release();
    }

    iaccWindow->Release();
    QTestAccessibility::clearEvents();
#endif
}

QTEST_MAIN(tst_QAccessibility)
#include "tst_qaccessibility.moc"
