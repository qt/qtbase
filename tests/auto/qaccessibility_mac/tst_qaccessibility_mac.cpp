/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>

#if defined(Q_WS_MAC) && !defined (QT_MAC_USE_COCOA)

#include <private/qt_mac_p.h>
#undef verify // yes, lets reserve the word "verify"

#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QString>
#include <QFile>
#include <QVariant>
#include <QPushButton>
#include <QToolBar>
#include <QSlider>
#include <QListWidget>
#include <QTableWidget>
#include <QScrollArea>
#include <QLabel>
#include <QScrollBar>
#include <QTextEdit>
#include <QAccessibleInterface>
#include <QAccessible>
#include <QPluginLoader>
#include <private/qaccessible_mac_p.h>
#include <quiloader.h>

#include <sys/types.h> // for getpid()
#include <unistd.h>

Q_DECLARE_METATYPE(AXUIElementRef);

typedef QCFType<CFArrayRef> QCFArrayRef;

class tst_qaccessibility_mac : public QObject
{
Q_OBJECT
public slots:
    void printInfo();
    void testForm();
    void testButtons();
    void testLineEdit();
    void testLabel();
    void testGroups();
    void testTabWidget();
    void testTabBar();
    void testComboBox();
    void testDeleteWidget();
    void testDeleteWidgets();
    void testMultipleWindows();
    void testHiddenWidgets();
    void testActions();
    void testChangeState();
    void testSlider();
    void testScrollArea();
    void testListView();
    void testTableView();
    void testScrollBar();
    void testSplitter();
    void testTextEdit();
    void testItemViewsWithoutModel();
private slots:
    void testQAElement();
    void testQAInterface();

    // ui tests load an .ui file.
    void uitests_data();
    void uitests();
    
    void tests_data();
    void tests();
private:
    void runTest(const QString &testSlot);
};

/*
    VERIFYs that there is no error and prints an error message if there is.
*/
void testError(AXError error, const QString &text)
{
    if (error)
        qDebug() << "Error" << error << text;
    QVERIFY(error == 0);
}

/*
    Prints an CFArray holding CFStrings.
*/
void printCFStringArray(CFArrayRef array, const QString &title)
{
    const int numElements = CFArrayGetCount(array);
    qDebug() << "num" << title << " " <<  numElements;

    for (int i = 0; i < numElements; ++i) {
       CFStringRef str = (CFStringRef)CFArrayGetValueAtIndex(array, i);
       qDebug() << QCFString::toQString(str);
    }
}

QStringList toQStringList(const CFArrayRef array)
{
    const int numElements = CFArrayGetCount(array);
    QStringList qtStrings;
    
    for (int i = 0; i < numElements; ++i) {
        CFStringRef str = (CFStringRef)CFArrayGetValueAtIndex(array, i);
        qtStrings.append(QCFString::toQString(str));
    }
    
    return qtStrings;
}

QVariant AXValueToQVariant(AXValueRef value)
{
    QVariant var;
    const AXValueType type = AXValueGetType(value);
    switch (type) {
        case kAXValueCGPointType : {
            CGPoint point;
            if (AXValueGetValue(value, type, &point))
                var = QPointF(point.x, point.y);
        } break;
        case kAXValueCGSizeType : {
            CGSize size;
            if (AXValueGetValue(value, type, &size))
                var = QSizeF(size.width, size.height);
        } break;
        case kAXValueCGRectType :  {
            CGRect rect;
            if (AXValueGetValue(value, type, &rect))
                var = QRectF(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
        } break;
        case kAXValueCFRangeType :
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
        case kAXValueAXErrorType :
#endif
        case kAXValueIllegalType :
        default:
        qDebug() << "Illegal/Unsuported AXValue:" << type;
        break;
    };
    return var;
}

/*
    Converts a CFTypeRef to a QVariant, for certain selected types. Prints
    an error message and returns QVariant() if the type is not supported.
*/
QVariant CFTypeToQVariant(CFTypeRef value)
{
    QVariant var;
    if (value == 0)
        return var;
    const uint typeID = CFGetTypeID(value);
    if (typeID == CFStringGetTypeID()) {
        var.setValue(QCFString::toQString((CFStringRef)value));
    } else if (typeID == CFBooleanGetTypeID()) {
        var.setValue((bool)CFBooleanGetValue((CFBooleanRef)value));
    } else if (typeID == AXUIElementGetTypeID()) {
        var.setValue((AXUIElementRef)value);
    } else if (typeID == AXValueGetTypeID()) {
        var = AXValueToQVariant((AXValueRef)value);
    } else if (typeID == CFNumberGetTypeID()) {
        CFNumberRef number = (CFNumberRef)value;
        if (CFNumberGetType(number) != kCFNumberSInt32Type)
            qDebug() << "unsupported number type" << CFNumberGetType(number);
        int theNumber;
        CFNumberGetValue(number, kCFNumberSInt32Type, &theNumber);
        var.setValue(theNumber);
    } else if (typeID == CFArrayGetTypeID()) {
        CFArrayRef cfarray = static_cast<CFArrayRef>(value);
        QVariantList list;
        CFIndex size = CFArrayGetCount(cfarray);
        for (CFIndex i = 0; i < size; ++i)
            list << CFTypeToQVariant(CFArrayGetValueAtIndex(cfarray, i));
        var.setValue(list);
    } else {
        QCFString str = CFCopyTypeIDDescription(typeID);
        qDebug() << "Unknown CFType: " << typeID << (QString)str;
    }
    return var;
}

/*
    Tests if a given attribute is supported by an element. Expects either
    no error or error -25205 (Not supported). Causes a test failure
    on other error values.
*/
bool supportsAttribute(AXUIElementRef element, CFStringRef attribute)
{
    CFArrayRef array;
    AXError err = AXUIElementCopyAttributeNames(element, &array);
    if (err) {
        testError(err, QLatin1String("unexpected error when testing for supported attribute") + QCFString::toQString(attribute));
        return false;
    }
    CFRange range;
    range.location = 0;
    range.length = CFArrayGetCount(array);
    return CFArrayContainsValue(array, range, attribute);
}

/*
    Returns the accessibility attribute specified with attribute in a QVariant
*/
QVariant attribute(AXUIElementRef element, CFStringRef attribute)
{
    CFTypeRef value = 0;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);

    testError(err, QString("Error getting element attribute ") + QCFString::toQString(attribute));

    if (err)
        return QVariant();

    return CFTypeToQVariant(value);
}

/*
    Returns the title for an element.
*/
QString title(AXUIElementRef element)
{
    return attribute(element, kAXTitleAttribute).toString();
}

/*
    Returns the role for an element.
*/
QString role(AXUIElementRef element)
{
    return attribute(element, kAXRoleAttribute).toString();
}

/*
    Returns the subrole for an element.
*/
QString subrole(AXUIElementRef element)
{
    return attribute(element, kAXSubroleAttribute).toString();
}

/*
    Returns the role description for an element.
*/
QString roleDescription(AXUIElementRef element)
{
    return attribute(element, kAXRoleDescriptionAttribute).toString();
}

/*
    Returns the enabled attribute for an element.
*/
bool enabled(AXUIElementRef element)
{
    return attribute(element, kAXEnabledAttribute).toBool();
}

/*
    Returns the value attribute for an element as an QVariant.
*/
QVariant value(AXUIElementRef element)
{
    return attribute(element, kAXValueAttribute);
}

QVariant value(QAElement element)
{
    return value(element.element());
}

/*
    Returns the description attribute for an element as an QVariant.
*/
QVariant description(AXUIElementRef element)
{
    return attribute(element, kAXDescriptionAttribute);
}

/*
    Returns the value attribute for an element as an bool.
*/
bool boolValue(AXUIElementRef element)
{
    return attribute(element, kAXValueAttribute).toBool();
}

/*
    Returns the parent for an element
*/
AXUIElementRef parent(AXUIElementRef element)
{
    return attribute(element, kAXParentAttribute).value<AXUIElementRef>();
}

/*
    Returns the (top-level) window(not a sheet or a drawer) for an element
*/
AXUIElementRef window(AXUIElementRef element)
{
    return attribute(element, kAXWindowAttribute).value<AXUIElementRef>();
}

/*
    Returns the (top-level) UI element(can also be a sheet or drawer) for an element
*/
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
AXUIElementRef topLevelUIElement(AXUIElementRef element)
{
    return attribute(element, kAXTopLevelUIElementAttribute).value<AXUIElementRef>();
}
#endif

/*
    Returns thie size of the element.
*/
QSizeF size(AXUIElementRef element)
{
    return attribute(element, kAXSizeAttribute).value<QSizeF>();
}

/*
    Returns the position of the element.
*/
QPointF position(AXUIElementRef element)
{
    return attribute(element, kAXPositionAttribute).value<QPointF>();
}

/*
    Returns the rect of the element.
*/
QRectF rect(AXUIElementRef element)
{
    return QRectF(position(element), size(element));
}

bool above(AXUIElementRef a, AXUIElementRef b)
{
    return (position(a).y() + size(a).height() <= position(b).y());
}

bool contains(AXUIElementRef a, AXUIElementRef b)
{
    return rect(a).contains(rect(b));
}

QList<AXUIElementRef> tabs(AXUIElementRef element)
{
    CFTypeRef value;
    AXError err = AXUIElementCopyAttributeValue(element, kAXTabsAttribute, &value);
    if (err)
        return QList<AXUIElementRef>();
        
    CFArrayRef array = (CFArrayRef)value;
    QList<AXUIElementRef> elements;
    const int count = CFArrayGetCount(array);
    for (int i = 0; i < count; ++i)
        elements.append((AXUIElementRef)CFArrayGetValueAtIndex(array, i));
   
    return elements;
}

QList<AXUIElementRef> elementListAttribute(AXUIElementRef element, CFStringRef attributeName)
{
    QList<AXUIElementRef> elementList;
    QVariantList variants = attribute(element, attributeName).value<QVariantList>();
    foreach(QVariant variant, variants)
        elementList.append(variant.value<AXUIElementRef>());
    return elementList;
}

AXUIElementRef elementAttribute(AXUIElementRef element, CFStringRef attributeName)
{
    return attribute(element, attributeName).value<AXUIElementRef>();
}

QString stringAttribute(AXUIElementRef element, CFStringRef attributeName)
{
    return attribute(element, attributeName).value<QString>();
}


/*
    Returns the UIElement at the given position.
*/
AXUIElementRef childAtPoint(QPointF position)
{
    AXUIElementRef element = 0;
    const AXError err = AXUIElementCopyElementAtPosition(AXUIElementCreateApplication(getpid()), position.x(), position.y(), &element);
    if (err) {
        qDebug() << "Error getting element at " << position;
        return 0;
    }

    return element;
}

/*
    Returns a QStringList containing the names of the actions the ui element supports
*/
QStringList actionNames(AXUIElementRef element)
{
    CFArrayRef cfStrings;
    const AXError err = AXUIElementCopyActionNames(element, &cfStrings);
    testError(err, "Unable to get action names");
    return toQStringList(cfStrings);
}

bool supportsAction(const AXUIElementRef element, const QString &actionName)
{
    const QStringList actions = actionNames(element);
    return actions.contains(actionName);
}

bool performAction(const AXUIElementRef element, const QString &actionName)
{
    const AXError err = AXUIElementPerformAction(element, QCFString(actionName));
    return (err == 0);
}

/*
    Om 10.4 and up, verifyes the AXRoleDescription attribute for an element,
    on 10.3 and below this test always passes.

    The reason for this is that the HICopyAccessibilityRoleDescription call
    used to implement this functionality was introduced in 10.4.
*/
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    #define VERIFY_ROLE_DESCRIPTION(ELEMENT,  TEXT) \
            QCOMPARE(roleDescription(ELEMENT), QString(TEXT))
#else
    #define VERIFY_ROLE_DESCRIPTION(ELEMENT,  TEXT) QVERIFY(true)
#endif


CFArrayRef childrenArray(AXUIElementRef element)
{
    CFTypeRef value = 0;
    AXError err = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, &value);
    if (!err && CFGetTypeID(value) == CFArrayGetTypeID()) {
        return (CFArrayRef)value;
    }
   
    return CFArrayCreate(0,0,0,0);
}

/*
    Gest the child count from an element.
*/
int numChildren(AXUIElementRef element)
{
    return CFArrayGetCount(childrenArray(element));
}

/*
    Gets the child with index childIndex from element. Returns 0 if not found.
*/
AXUIElementRef child(AXUIElementRef element, int childIndex)
{
    CFArrayRef children = childrenArray(element);
    if (childIndex >= CFArrayGetCount(children))
        return 0;

    const void *data  = CFArrayGetValueAtIndex(children, childIndex);
    return (AXUIElementRef)data;
}

/*
    Gets the child titled childTitle from element. Returns 0 if not found.
*/
AXUIElementRef childByTitle(AXUIElementRef element, const QString &childTitle)
{
    CFArrayRef children  = childrenArray(element);
    const int numChildren = CFArrayGetCount(children);
    for (int i = 0; i < numChildren; ++i) {
        const AXUIElementRef childElement = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        // Test for support for title attribute before getting it to avoid test fail.
        if (supportsAttribute(childElement, kAXTitleAttribute) && title(childElement) == childTitle)
            return childElement;
    }
    return 0;
}

/*
    Gets the child with the given value from element. Returns 0 if not found.
*/
AXUIElementRef childByValue(AXUIElementRef element, const QVariant &testValue)
{
    CFArrayRef children  = childrenArray(element);
    const int numChildren = CFArrayGetCount(children);
    for (int i = 0; i < numChildren; ++i) {
        const AXUIElementRef childElement = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        // Test for support for value attribute before getting it to avoid test fail.
        if (supportsAttribute(childElement, kAXValueAttribute) && value(childElement) == testValue)
            return childElement;
    }
    return 0;
}

/*
    Gets the child by role from element. Returns 0 if not found.
*/
AXUIElementRef childByRole(AXUIElementRef element, const QString &macRole)
{
    CFArrayRef children  = childrenArray(element);
    const int numChildren = CFArrayGetCount(children);
    for (int i = 0; i < numChildren; ++i) {
        const AXUIElementRef childElement = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (role(childElement) == macRole)
            return childElement;
    }
    return 0;
}

void printTypeForAttribute(AXUIElementRef element, CFStringRef attribute)
{
    CFTypeRef value = 0;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);
    if (!err) {
        qDebug() << "type id" << CFGetTypeID(value);
        QCFString str = CFCopyTypeIDDescription(CFGetTypeID(value));
        qDebug() << (QString)str;
    } else {
        qDebug() << "Attribute Get error" << endl;
    }
}

int indent = 0;
QString space()
{
    QString space;
    for (int i = 0; i < indent; ++i) {
        space += " ";
    }
    return space;
}


/*
    Recursively prints acccesibility info for currentElement and all its children.
*/
void printElementInfo(AXUIElementRef currentElement)
{
    if (HIObjectIsAccessibilityIgnored(AXUIElementGetHIObject(currentElement))) {
        qDebug() << space() << "Ignoring element with role" << role(currentElement);
        return;
    }
    
    qDebug() << space() <<"Role" << role(currentElement);
    if (supportsAttribute(currentElement, kAXTitleAttribute))
        qDebug() << space() << "Title" << title(currentElement);
    else
        qDebug() << space() << "Title not supported";
    
    if (supportsAttribute(currentElement, kAXValueAttribute))
        qDebug() << space() << "Value" << attribute(currentElement, kAXValueAttribute);
    else
        qDebug() << space() << "Value not supported";
    
    qDebug() << space() << "Number of children" << numChildren(currentElement);
    for (int i = 0; i < numChildren(currentElement); ++i) {
        AXUIElementRef childElement = child(currentElement, i);
        // Skip the menu bar.
        if (role(childElement) != "AXMenuBar") {
            indent+= 4;
            printElementInfo(childElement);
            indent-= 4;
        }
    }
    qDebug() << " ";
}

/*
    Recursively prints the child interfaces belonging to interface.
*/

void printChildren(const QAInterface &interface)
{
    if (interface.isValid() == false)
        return;
        
    QList<QAInterface> children = interface.children();
    if (children.isEmpty())
        return;

    qDebug() << "## Children for" << interface;
    foreach (const QAInterface &child, children) {
        qDebug() << child << "index in parent" << interface.indexOfChild(child);
    }
    foreach (const QAInterface &child, children) {
        printChildren(child);
    }
}

bool isIgnored(AXUIElementRef currentElement)
{
    return HIObjectIsAccessibilityIgnored(AXUIElementGetHIObject(currentElement));
}

bool equal(CFTypeRef o1, CFTypeRef o2)
{
    if (o1 == 0 || o2 == 0)
        return false;
    return CFEqual(o1, o2);
}

/*
    Verifies basic element info.
*/
#define VERIFY_ELEMENT(element, _parent, _role) \
    QVERIFY(element != 0); \
    QVERIFY(role(element) == _role); \
    QVERIFY(equal(::parent(element), _parent));
/*
    Verifies that the application and the main form is there has the right info.
*/
void testAppAndForm(AXUIElementRef application)
{
    QVERIFY(title(application) == "tst_qaccessibility_mac");
    QVERIFY(role(application) == "AXApplication");

    AXUIElementRef form = childByTitle(application, "Form");
    VERIFY_ELEMENT(form, application, "AXWindow");
}

void tst_qaccessibility_mac::printInfo()
{
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    printElementInfo(currentApplication);
}

/*
    Tests for form.ui
*/
void tst_qaccessibility_mac::testForm()
{
    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    testAppAndForm(currentApplication);
    childByTitle(currentApplication, "Form");
}

/*
    Tests for buttons.ui
*/
void tst_qaccessibility_mac::testButtons()
{
    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    testAppAndForm(currentApplication);
    AXUIElementRef form = childByTitle(currentApplication, "Form");

    AXUIElementRef ren = childByTitle(form, "Ren");
    VERIFY_ELEMENT(ren, form, "AXButton");
    QVERIFY(enabled(ren) == true);
    VERIFY_ROLE_DESCRIPTION(ren, "button");

    AXUIElementRef stimpy = childByTitle(form, "Stimpy");
    VERIFY_ELEMENT(stimpy, form, "AXRadioButton");
    QVERIFY(enabled(stimpy) == true);
    QVERIFY(value(stimpy).toInt() == 1); // checked;
    VERIFY_ROLE_DESCRIPTION(stimpy, "radio button");

    AXUIElementRef pinky = childByTitle(form, "Pinky");
    VERIFY_ELEMENT(pinky, form, "AXCheckBox");
    QVERIFY(enabled(pinky) == false);
    QVERIFY(value(pinky).toInt() == 0); // unchecked;
    VERIFY_ROLE_DESCRIPTION(pinky, "check box");

    AXUIElementRef brain = childByTitle(form, "Brain");
    VERIFY_ELEMENT(brain, form, "AXButton");
    VERIFY_ROLE_DESCRIPTION(brain, "button");
}

void tst_qaccessibility_mac::testLabel()
{
    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());

    testAppAndForm(currentApplication);
    AXUIElementRef form = childByTitle(currentApplication, "Form");
    AXUIElementRef label = childByValue(form, "This is a Text Label");
    QVERIFY(label);
    VERIFY_ELEMENT(label, form, "AXStaticText");
    VERIFY_ROLE_DESCRIPTION(label, "text");
    QCOMPARE(supportsAttribute(label, kAXDescriptionAttribute), false);
}

/*
    Tests for lineedit.ui
*/
void tst_qaccessibility_mac::testLineEdit()
{
    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());

    testAppAndForm(currentApplication);
    AXUIElementRef form = childByTitle(currentApplication, "Form");
    AXUIElementRef lineEdit = childByValue(form, "Line edit");
    VERIFY_ELEMENT(lineEdit, form, "AXTextField");
    VERIFY_ROLE_DESCRIPTION(lineEdit, "text field");
}

/*
    Tests for groups.ui
*/
void tst_qaccessibility_mac::testGroups()
{
    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());

    testAppAndForm(currentApplication);
    AXUIElementRef form = childByTitle(currentApplication, "Form");

    AXUIElementRef groupA = childByTitle(form, "Group A");
    VERIFY_ELEMENT(groupA, form, "AXGroup");
    AXUIElementRef button1 = childByTitle(groupA, "PushButton 1");
    VERIFY_ELEMENT(button1, groupA, "AXButton");
    VERIFY_ROLE_DESCRIPTION(groupA, "group");

    AXUIElementRef groupB = childByTitle(form, "Group B");
    VERIFY_ELEMENT(groupB, form, "AXGroup");
    AXUIElementRef button3 = childByTitle(groupB, "PushButton 3");
    VERIFY_ELEMENT(button3, groupB, "AXButton");
}

/*
    Tests for tabs.ui
*/
void tst_qaccessibility_mac::testTabWidget()
{
    {   // Test that the QTabWidget hierarchy is what we expect it to be.
        QTabWidget tabWidget;
        tabWidget.show();
        QAInterface interface = QAccessible::queryAccessibleInterface(&tabWidget);
        tabWidget.addTab(new QPushButton("Foo"), "FooTab");
        tabWidget.addTab(new QPushButton("Bar"), "BarTab");
        QCOMPARE(interface.childCount(), 2);
        const QList<QAInterface> children = interface.children();
        QVERIFY(children.at(0).object()->inherits("QStackedWidget"));
        QVERIFY(children.at(1).object()->inherits("QTabBar"));
        
        const QList<QAInterface> tabBarChildren = children.at(1).children();
        QCOMPARE(tabBarChildren.count(), 4);
        QCOMPARE(tabBarChildren.at(0).text(QAccessible::Name), QLatin1String("FooTab"));
        QCOMPARE(tabBarChildren.at(1).text(QAccessible::Name), QLatin1String("BarTab"));
        QCOMPARE(tabBarChildren.at(0).role(), QAccessible::PageTab);
        QCOMPARE(tabBarChildren.at(1).role(), QAccessible::PageTab);

        // Check that the hierarchy manager is able to register the tab bar children.
        QAccessibleHierarchyManager *manager = QAccessibleHierarchyManager::instance();
        QAInterface tabBarInterface = children.at(1);
        QAElement tabBarElement = manager->registerInterface(tabBarInterface);
        QCOMPARE(manager->lookup(tabBarElement).childCount(), 4);
        manager->registerChildren(tabBarInterface);
        QAElement tabButtonElement = manager->lookup(tabBarChildren.at(1));
        QAInterface tabButtonInterface = manager->lookup(tabButtonElement);
        QCOMPARE(tabButtonInterface.text(QAccessible::Name), QLatin1String("BarTab"));
        QVERIFY(isItInteresting(tabButtonInterface) == true);
    }

    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());

    testAppAndForm(currentApplication);
    const QString formTitle = "Form";
    AXUIElementRef form = childByTitle(currentApplication, formTitle);
    QVERIFY(form);

    const QString tabRole = "AXTabGroup";
    AXUIElementRef tabGroup = childByRole(form, tabRole);
    QVERIFY(tabGroup);

    // Test that we have three child buttons (the tab buttons + plus the contents of the first tab)
    const int numChildren = ::numChildren(tabGroup);
    QCOMPARE(numChildren, 3);
    
    const QString tab1Title = "Tab 1";
    AXUIElementRef tabButton1 = childByTitle(tabGroup, tab1Title);
    QVERIFY (tabButton1);
    VERIFY_ELEMENT(tabButton1, tabGroup, "AXRadioButton");
    QCOMPARE(title(tabButton1), tab1Title);

    const QString tab2Title = "Tab 2";
    const AXUIElementRef tabButton2 = childByTitle(tabGroup, tab2Title);
    QVERIFY(tabButton2);
    VERIFY_ELEMENT(tabButton2, tabGroup, "AXRadioButton");
    QCOMPARE(title(tabButton2), tab2Title);
    
    // Test that the window and top-level-ui-elment is the form.
    // Window is not reported properly on 10.5
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5) {
        QVERIFY(equal(window(tabGroup), form));

    //   ### hangs on 10.4
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    QVERIFY(equal(window(tabButton1), form));
#endif
    }
//   ### hangs on 10.4
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    QVERIFY(equal(topLevelUIElement(tabGroup), form));
    QVERIFY(equal(topLevelUIElement(tabButton1), form));    
#endif
    // Test the bounding rectangles for the tab group and buttons.
    const QRectF groupRect(position(tabGroup), size(tabGroup));
    const QRectF tabButton1Rect(position(tabButton1), size(tabButton1));
    const QRectF tabButton2Rect(position(tabButton2), size(tabButton2));
    
    QVERIFY(groupRect.isNull() == false);
    QVERIFY(tabButton1Rect.isNull() == false);
    QVERIFY(tabButton1Rect.isNull() == false);
        
    QVERIFY(groupRect.contains(tabButton1Rect));
    QVERIFY(groupRect.contains(tabButton2Rect));
    QVERIFY(tabButton2Rect.contains(tabButton1Rect) == false);
    
    // Test the childAtPoint event.
    const AXUIElementRef childAtTab1Position = childAtPoint(position(tabButton1) + QPointF(5,5));
    QVERIFY(equal(childAtTab1Position, tabButton1));
    const AXUIElementRef childAtOtherPosition = childAtPoint(position(tabButton1) - QPointF(5,5));
    QVERIFY(equal(childAtOtherPosition, tabButton1) == false);

    // Test AXTabs attribute
    QVERIFY(supportsAttribute(tabGroup, kAXTabsAttribute));
    QList<AXUIElementRef> tabElements = tabs(tabGroup);
    QCOMPARE(tabElements.count(), 2);
    QVERIFY(equal(tabElements.at(0), tabButton1));
    QVERIFY(equal(tabElements.at(1), tabButton2));
    
    // Perform the press action on each child.
    for (int i = 0; i < numChildren; ++i) {
        const AXUIElementRef child = ::child(tabGroup, i);
        QVERIFY(supportsAction(child, "AXPress"));
        QVERIFY(performAction(child, "AXPress"));
    }
}

void tst_qaccessibility_mac::testTabBar()
{
    QTabBar tabBar;
    tabBar.addTab("Tab A");
    tabBar.addTab("Tab B");
    tabBar.show();

    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    AXUIElementRef window = childByRole(currentApplication, "AXWindow");
    QVERIFY(window);

    const QString tabRole = "AXTabGroup";
    AXUIElementRef tabGroup = childByRole(window, tabRole);
    QVERIFY(tabGroup);

    const int numChildren = ::numChildren(tabGroup);
    QCOMPARE(numChildren, 2);
    
    const QString tab1Title = "Tab A";
    AXUIElementRef tabButton1 = childByTitle(tabGroup, tab1Title);
    QVERIFY (tabButton1);
    VERIFY_ELEMENT(tabButton1, tabGroup, "AXRadioButton");
    QCOMPARE(title(tabButton1), tab1Title);

    const QString tab2Title = "Tab B";
    const AXUIElementRef tabButton2 = childByTitle(tabGroup, tab2Title);
    QVERIFY(tabButton2);
    VERIFY_ELEMENT(tabButton2, tabGroup, "AXRadioButton");
    QCOMPARE(title(tabButton2), tab2Title);

    // Test the childAtPoint event.
    const AXUIElementRef childAtTab1Position = childAtPoint(position(tabButton1) + QPointF(5,5));
    QVERIFY(equal(childAtTab1Position, tabButton1));
    const AXUIElementRef childAtOtherPosition = childAtPoint(position(tabButton1) - QPointF(5,5));
    QVERIFY(equal(childAtOtherPosition, tabButton1) == false);

    // Test AXTabs attribute
    QVERIFY(supportsAttribute(tabGroup, kAXTabsAttribute));
    QList<AXUIElementRef> tabElements = tabs(tabGroup);
    QCOMPARE(tabElements.count(), 2);
    QVERIFY(equal(tabElements.at(0), tabButton1));
    QVERIFY(equal(tabElements.at(1), tabButton2));
    
    // Perform the press action on each child.
    for (int i = 0; i < numChildren; ++i) {
        const AXUIElementRef child = ::child(tabGroup, i);
        QVERIFY(supportsAction(child, "AXPress"));
        QVERIFY(performAction(child, "AXPress"));
    }
}

void tst_qaccessibility_mac::testComboBox()
{
    // Get reference to the current application.
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());

    testAppAndForm(currentApplication);
    const QString formTitle = "Form";
    AXUIElementRef form = childByTitle(currentApplication, formTitle);
    
    const QString comboBoxRole = "AXPopUpButton";
    AXUIElementRef comboBox = childByRole(form, comboBoxRole);
    QVERIFY(comboBox != 0);
    QVERIFY(supportsAction(comboBox, "AXPress"));
    QVERIFY(performAction(comboBox, "AXPress"));
}

void tst_qaccessibility_mac::testDeleteWidget()
{
    const QString buttonTitle = "Hi there";
    QWidget *form = new QWidget(0, Qt::Window);
    form->setWindowTitle("Form");
    form->show();
    QPushButton *button = new QPushButton(buttonTitle, form);
    button->show();

    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    testAppAndForm(currentApplication);
    AXUIElementRef formElement = childByTitle(currentApplication, "Form");

    AXUIElementRef buttonElement = childByTitle(formElement, buttonTitle);
    QVERIFY(buttonElement);

    button->hide();
    delete button;

    buttonElement = childByTitle(formElement, buttonTitle);
    QVERIFY(!buttonElement);
    delete form;
}

void tst_qaccessibility_mac::testDeleteWidgets()
{
    const QString buttonTitle = "Hi there";
    const int repeats = 10;

    for (int i = 0; i < repeats; ++i) {

        QWidget *form = new QWidget(0, Qt::Window);
        form->setWindowTitle("Form");
        form->show();
    
        QPushButton *button = new QPushButton(buttonTitle, form);
        button->show();
    
        AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
        testAppAndForm(currentApplication);
        AXUIElementRef formElement = childByTitle(currentApplication, "Form");
        AXUIElementRef buttonElement = childByTitle(formElement, buttonTitle);
        QVERIFY(buttonElement);
        delete form;

        {
            AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
            QVERIFY(currentApplication);
            AXUIElementRef formElement = childByTitle(currentApplication, "Form");
            QVERIFY(!formElement);
        }
    }

    for (int i = 0; i < repeats; ++i) {
        QWidget *form = new QWidget(0, Qt::Window);
        form->setWindowTitle("Form");

    
        new QScrollBar(form);
        form->show();
    
        AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
        testAppAndForm(currentApplication);
        AXUIElementRef formElement = childByTitle(currentApplication, "Form");
        
        const AXUIElementRef scrollBarElement = childByRole(formElement, "AXScrollBar");
        QVERIFY(scrollBarElement);
        delete form;

        {
            AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
            QVERIFY(currentApplication);
            AXUIElementRef formElement = childByTitle(currentApplication, "Form");
            QVERIFY(!formElement);
        }
    }

    for (int i = 0; i < repeats; ++i) {
        QWidget *form = new QWidget(0, Qt::Window);
        form->setWindowTitle("Form");
    
        QListWidget *listWidget = new QListWidget(form);
        listWidget->addItem("Foo");
        listWidget->addItem("Bar");
        form->show();
    
        AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
        testAppAndForm(currentApplication);
        AXUIElementRef formElement = childByTitle(currentApplication, "Form");

        const AXUIElementRef scrollAreaElement = childByRole(formElement, "AXScrollArea");
        QVERIFY(scrollAreaElement);

        const AXUIElementRef listElement = childByRole(scrollAreaElement, "AXList");
        QVERIFY(listElement);
        delete form;

        {
            AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
            QVERIFY(currentApplication);
            AXUIElementRef formElement = childByTitle(currentApplication, "Form");
            QVERIFY(!formElement);
        }
    }

}

void tst_qaccessibility_mac::testMultipleWindows()
{
    const QString formATitle("FormA");
    const QString formBTitle("FormB");

    // Create a window
    QWidget *formA = new QWidget(0, Qt::Window);
    formA->setWindowTitle(formATitle);
    formA->show();

    // Test if we can access the window
    AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    AXUIElementRef formAElement = childByTitle(currentApplication, formATitle);
    QVERIFY(formAElement);

    // Create another window
    QWidget *formB = new QWidget(0, Qt::Window);
    formB->setWindowTitle(formBTitle);
    formB->show();

    // Test if we can access both windows
    formAElement = childByTitle(currentApplication, formATitle);
    QVERIFY(formAElement);

    AXUIElementRef formBElement = childByTitle(currentApplication, formBTitle);
    QVERIFY(formBElement);
    
    delete formA;
}

void tst_qaccessibility_mac::testHiddenWidgets()
{
    const QString windowTitle ="a widget";
    QWidget * const window = new QWidget(0);
    window->setWindowTitle(windowTitle);
    window->show();

    const AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    const AXUIElementRef windowElement = childByTitle(currentApplication, windowTitle);
    QVERIFY(windowElement);
    QCOMPARE(isIgnored(windowElement), false);
    
    const QString buttonTitle = "a button";
    QPushButton * const button = new QPushButton(window);    
    button->setText(buttonTitle);
    button->show();
    
    const AXUIElementRef buttonElement = childByTitle(windowElement, buttonTitle);
    QVERIFY(buttonElement);
    QCOMPARE(isIgnored(buttonElement), false);

    const QString toolbarTitle = "a toolbar";
    QToolBar * const toolbar = new QToolBar(toolbarTitle, window);
    toolbar->show();
    
    const AXUIElementRef toolBarElement = childByTitle(windowElement, toolbarTitle);
    QVERIFY(toolBarElement == 0);

    delete window;
};

void tst_qaccessibility_mac::testActions()
{
    // create a window with a push button
    const QString windowTitle ="a widget";
    QWidget * const window = new QWidget();
    window->setWindowTitle(windowTitle);
    window->show();

    const AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    const AXUIElementRef windowElement = childByTitle(currentApplication, windowTitle);
    QVERIFY(windowElement);

    const QString buttonTitle = "a button";
    QPushButton * const button = new QPushButton(window);    
    button->setText(buttonTitle);
    button->show();
    
    const AXUIElementRef buttonElement = childByTitle(windowElement, buttonTitle);
    QVERIFY(buttonElement);

    // Verify that the button has the Press action.
    const QStringList actions = actionNames(buttonElement);
    const QString pressActionName("AXPress");
    QVERIFY(actions.contains(pressActionName));
    
    // Press button and check the pressed signal
    QSignalSpy pressed(button, SIGNAL(pressed()));
    QVERIFY(performAction(buttonElement, pressActionName));
    QCOMPARE(pressed.count(), 1);
    
    pressed.clear();
    QVERIFY(performAction(buttonElement, QString("does not exist")));
    QCOMPARE(pressed.count(), 0);

    delete window;
};

void tst_qaccessibility_mac::testChangeState()
{
    const QString windowTitle ="a widget";
    QWidget * const window = new QWidget();
    window->setWindowTitle(windowTitle);
    window->show();
 
    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const int otherChildren = numChildren(windowElement);
    
    const QString buttonTitle = "Button";
    QPushButton * const button = new QPushButton(buttonTitle, window);
    button->setText(buttonTitle);  

    // Test that show/hide adds/removes the button from the hierachy.
    QVERIFY(childByTitle(windowElement, buttonTitle) == 0);
    QCOMPARE(numChildren(windowElement), otherChildren);
    button->show();
    QVERIFY(childByTitle(windowElement, buttonTitle) != 0);
    QCOMPARE(numChildren(windowElement), otherChildren + 1);
    button->hide();
    QVERIFY(childByTitle(windowElement, buttonTitle) == 0);
    QCOMPARE(numChildren(windowElement), otherChildren);
    button->show();
    QVERIFY(childByTitle(windowElement, buttonTitle) != 0);
    QCOMPARE(numChildren(windowElement), otherChildren + 1);

    // Test that hiding and showing a widget also removes and adds all its children.
    {
        QWidget * const parent = new QWidget(window);
        const int otherChildren = numChildren(windowElement);
        
        QPushButton * const child = new QPushButton(parent);
        const QString childButtonTitle = "child button";
        child->setText(childButtonTitle);
        
        parent->show();
        QVERIFY(childByTitle(windowElement, childButtonTitle) != 0);
        QCOMPARE(numChildren(windowElement), otherChildren + 1);

        parent->hide();
        QVERIFY(childByTitle(windowElement, childButtonTitle) == 0);
        QCOMPARE(numChildren(windowElement), otherChildren );

        parent->show();
        QVERIFY(childByTitle(windowElement, childButtonTitle) != 0);
        QCOMPARE(numChildren(windowElement), otherChildren + 1);
        
        delete parent;
    }

    // Test that the enabled attribute is updated after a call to setEnabled.
    const AXUIElementRef buttonElement = childByTitle(windowElement, buttonTitle);
    QVERIFY(enabled(buttonElement));
    button->setEnabled(false);
    QVERIFY(enabled(buttonElement) == false);
    button->setEnabled(true);
    QVERIFY(enabled(buttonElement));

    // Test that changing the title updates the accessibility information.
    const QString buttonTitle2 = "Button 2";    
    button->setText(buttonTitle2);
    QVERIFY(childByTitle(windowElement, buttonTitle2) != 0);
    QVERIFY(childByTitle(windowElement, buttonTitle) == 0);

    delete window;
}

void tst_qaccessibility_mac::testSlider()
{
    const QString windowTitle = "a widget";
    QWidget * const window = new QWidget();
    window->setWindowTitle(windowTitle);
    window->show();
 
    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    QVERIFY(applicationElement);
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const int windowChildren = numChildren(windowElement);
    
    QSlider * const slider = new QSlider(window);
    slider->show();
    const AXUIElementRef sliderElement = childByRole(windowElement, "AXSlider");
    QVERIFY(sliderElement);

    // Test that the slider and its children are removed from the hierachy when we call hide().
    QCOMPARE(numChildren(windowElement), windowChildren + 1);
    slider->hide();
    QCOMPARE(numChildren(windowElement), windowChildren);
    
    delete slider;
}

void tst_qaccessibility_mac::testScrollArea()
{
    QWidget window;
    const QString windowTitle = "window";
    window.setWindowTitle(windowTitle);
    window.resize(300, 300);

    QScrollArea scrollArea(&window);
    scrollArea.resize(300, 300);
    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    QLabel label;
    label.setText("Foo"); 
    scrollArea.setWidget(&label);

    window.show();

    // Verify that the QAinterface returns the correct children
    QAInterface interface = QAccessible::queryAccessibleInterface(&scrollArea);
    QCOMPARE(interface.childCount(), 3);
    
    QAInterface viewport = interface.navigate(QAccessible::Child, 1);
    QVERIFY(viewport.isValid());

    QAInterface scrollBarContainer1 = interface.navigate(QAccessible::Child, 2);
    QVERIFY(scrollBarContainer1.isValid());

    QAInterface scrollBar1 = scrollBarContainer1.navigate(QAccessible::Child, 1);

    QVERIFY(scrollBar1.isValid());
    QVERIFY(scrollBar1.role() == QAccessible::ScrollBar);

    QAInterface scrollBarContainer2 = interface.navigate(QAccessible::Child, 3);
    QVERIFY(scrollBarContainer1.isValid());

    QAInterface scrollBar2 = scrollBarContainer2.navigate(QAccessible::Child, 1);
    QVERIFY(scrollBar2.isValid());
    QVERIFY(scrollBar2.role() == QAccessible::ScrollBar);    

    // Navigate to the scroll area from the application
    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    QVERIFY(applicationElement);
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const AXUIElementRef scrollAreaElement = childByRole(windowElement, "AXScrollArea");
    QVERIFY(scrollAreaElement);

    // Get the scroll bars
    QVERIFY(supportsAttribute(scrollAreaElement, kAXHorizontalScrollBarAttribute));
    const AXUIElementRef horizontalScrollBar = elementAttribute(scrollAreaElement, kAXHorizontalScrollBarAttribute);
    QVERIFY(horizontalScrollBar);
    QVERIFY(role(horizontalScrollBar) == "AXScrollBar");
    QVERIFY(stringAttribute(horizontalScrollBar, kAXOrientationAttribute) == "AXHorizontalOrientation");

    QVERIFY(supportsAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute));
    const AXUIElementRef verticalScrollBar = elementAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute);
    QVERIFY(verticalScrollBar);
    QVERIFY(role(verticalScrollBar) == "AXScrollBar");
    QVERIFY(stringAttribute(verticalScrollBar, kAXOrientationAttribute) == "AXVerticalOrientation");

    // Get the contents and verify that we get the label.
    QVERIFY(supportsAttribute(scrollAreaElement, kAXContentsAttribute));
    const QList<AXUIElementRef> contents = elementListAttribute(scrollAreaElement, kAXContentsAttribute);
    QCOMPARE(contents.count(), 1);
    AXUIElementRef content = contents.at(0);
    QVERIFY(role(content) == "AXStaticText");
    QVERIFY(title(content) == "Foo");

    // Turn scroll bars off
    {
    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QVERIFY(supportsAttribute(scrollAreaElement, kAXHorizontalScrollBarAttribute) == false);
    QVERIFY(supportsAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute) == false);

    QVERIFY(supportsAttribute(scrollAreaElement, kAXContentsAttribute));
    const QList<AXUIElementRef> contents = elementListAttribute(scrollAreaElement, kAXContentsAttribute);
    QCOMPARE(contents.count(), 1);
    AXUIElementRef content = contents.at(0);
    
    QVERIFY(role(content) == "AXStaticText");
    QVERIFY(title(content) == "Foo");
    }

    // Turn the horizontal scrollbar on.
    {
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QVERIFY(supportsAttribute(scrollAreaElement, kAXHorizontalScrollBarAttribute) == true);
    QVERIFY(supportsAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute) == false);

    const AXUIElementRef horizontalScrollBar = elementAttribute(scrollAreaElement, kAXHorizontalScrollBarAttribute);
    QVERIFY(horizontalScrollBar);
    QVERIFY(role(horizontalScrollBar) == "AXScrollBar");
    QVERIFY(stringAttribute(horizontalScrollBar, kAXOrientationAttribute) == "AXHorizontalOrientation");

    QVERIFY(supportsAttribute(scrollAreaElement, kAXContentsAttribute));
    const QList<AXUIElementRef> contents = elementListAttribute(scrollAreaElement, kAXContentsAttribute);
    QCOMPARE(contents.count(), 1);
    AXUIElementRef content = contents.at(0);
    QVERIFY(role(content) == "AXStaticText");
    QVERIFY(title(content) == "Foo");
    }

    // Turn the vertical scrollbar on.
    {
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QVERIFY(supportsAttribute(scrollAreaElement, kAXHorizontalScrollBarAttribute) == false);
    QVERIFY(supportsAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute) == true);

    QVERIFY(supportsAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute));
    const AXUIElementRef verticalScrollBar = elementAttribute(scrollAreaElement, kAXVerticalScrollBarAttribute);
    QVERIFY(verticalScrollBar);
    QVERIFY(role(verticalScrollBar) == "AXScrollBar");
    QVERIFY(stringAttribute(verticalScrollBar, kAXOrientationAttribute) == "AXVerticalOrientation");

    QVERIFY(supportsAttribute(scrollAreaElement, kAXContentsAttribute));
    const QList<AXUIElementRef> contents = elementListAttribute(scrollAreaElement, kAXContentsAttribute);
    QCOMPARE(contents.count(), 1);
    AXUIElementRef content = contents.at(0);
    QVERIFY(role(content) == "AXStaticText");
    QVERIFY(title(content) == "Foo");
    }
}

void tst_qaccessibility_mac::testListView()
{
    QWidget window;
    const QString windowTitle("window");
    window.setWindowTitle(windowTitle);
    window.resize(300, 300);

    QListWidget *listWidget = new QListWidget(&window);
    listWidget->setObjectName("listwidget");
    listWidget->addItem("A");
    listWidget->addItem("B");
    listWidget->addItem("C");
    
    window.show();
    QTest::qWait(1);
        
    {
        // Verify that QAInterface works as expected for list views
        QAInterface listWidgetInterface = QAccessible::queryAccessibleInterface(listWidget);
        QCOMPARE(listWidgetInterface.role(), QAccessible::Client);
        QCOMPARE(listWidgetInterface.childCount(), 1);
        QAInterface viewPort = listWidgetInterface.childAt(1);
        QCOMPARE(viewPort.role(), QAccessible::List);
        QVERIFY(viewPort.object() != 0);
        QCOMPARE(viewPort.childCount(), 3);
        const QList<QAInterface> rows = viewPort.children();
        QCOMPARE(rows.count(), 3);
        QVERIFY(rows.at(0).object() == 0);
        QCOMPARE(rows.at(0).parent().indexOfChild(rows.at(0)), 1);
        QCOMPARE(rows.at(1).parent().indexOfChild(rows.at(1)), 2);
        QCOMPARE(rows.at(2).parent().indexOfChild(rows.at(2)), 3);         
        
        // test the QAInterface comparison operator
        QVERIFY(rows.at(0) == rows.at(0));
        QVERIFY(rows.at(0) != rows.at(1));
        QVERIFY(rows.at(0) != viewPort);
        QVERIFY(viewPort == viewPort);
        QVERIFY(listWidgetInterface != viewPort);
        QVERIFY(listWidgetInterface == listWidgetInterface);
        
        // test QAInterface::isHIView()
        QVERIFY(viewPort.isHIView());
        QVERIFY(listWidgetInterface.isHIView());
        QVERIFY(rows.at(0).isHIView() == false);
    }

    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    QVERIFY(applicationElement);
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const AXUIElementRef scrollAreaElement = childByRole(windowElement, "AXScrollArea");
    QVERIFY(scrollAreaElement);
    const AXUIElementRef listElement = childByRole(scrollAreaElement, "AXList");
    QVERIFY(listElement);
    QVERIFY(equal(::parent(listElement), scrollAreaElement));
    // Window is not reported properly on 10.5
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5) 
        QVERIFY(equal(::window(listElement), windowElement));
    
    const AXUIElementRef A = childByTitle(listElement, "A");
    QVERIFY(A);
    const AXUIElementRef B = childByTitle(listElement, "B");
    QVERIFY(B);
    const AXUIElementRef C = childByTitle(listElement, "C");
    QVERIFY(C);

    QVERIFY(value(A) == "A");
    QVERIFY(equal(::parent(A), listElement));
    QVERIFY(enabled(A));

    // Window is not reported properly on 10.5, this test
    // hangs on 10.4. Disable it for now.
    //    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5) 
    //        QVERIFY(equal(::window(A), windowElement));
    
    QVERIFY(above(A, B));
    QVERIFY(!above(B, A));
    QVERIFY(above(B, C));
    QVERIFY(contains(listElement, A));
    QVERIFY(!contains(A, listElement));
    QVERIFY(contains(listElement, B));
    QVERIFY(contains(listElement, C));
}

void tst_qaccessibility_mac::testTableView()
{
    QWidget window;
    const QString windowTitle("window");
    window.setWindowTitle(windowTitle);
    window.resize(300, 300);

    QTableWidget *tableWidget = new QTableWidget(&window);
    tableWidget->setObjectName("tablewidget");
    tableWidget->setRowCount(3);
    tableWidget->setColumnCount(2);

    tableWidget->setItem(0, 0, new QTableWidgetItem("A1"));
    tableWidget->setItem(0, 1, new QTableWidgetItem("A2"));

    tableWidget->setItem(1, 0, new QTableWidgetItem("B1"));
    tableWidget->setItem(1, 1, new QTableWidgetItem("B2"));

    tableWidget->setItem(2, 0, new QTableWidgetItem("C1"));
    tableWidget->setItem(2, 1, new QTableWidgetItem("C2"));

    window.show();
    
    {
        // Verify that QAInterface works as expected for table view children.
        QAInterface tableWidgetInterface = QAccessible::queryAccessibleInterface(tableWidget);
        QCOMPARE(tableWidgetInterface.role(), QAccessible::Client);
        QCOMPARE(tableWidgetInterface.childCount(), 1);
        QAInterface viewPort = tableWidgetInterface.childAt(1);
        QCOMPARE(viewPort.childCount(), 4);
        QCOMPARE(viewPort.role(), QAccessible::Table);
        QVERIFY(viewPort.object() != 0);
        const QList<QAInterface> rows = viewPort.children();
        QCOMPARE(rows.count(), 4);
        QVERIFY(rows.at(0).object() == 0);
        QCOMPARE(rows.at(0).parent().indexOfChild(rows.at(0)), 1);
        QCOMPARE(rows.at(1).parent().indexOfChild(rows.at(1)), 2);
        QCOMPARE(rows.at(2).parent().indexOfChild(rows.at(2)), 3);         

        QAInterface Arow = rows.at(1);
        QCOMPARE(Arow.role(), QAccessible::Row);
        QAInterface Brow = rows.at(2);

        QVERIFY(Arow.name() == "1");
        QVERIFY(Brow.name() == "2");

        QVERIFY(Arow == Arow);
        QVERIFY(Brow != Arow);
        QVERIFY(Arow.isHIView() == false);
        QCOMPARE(Arow.childCount(), 3);
        QList<QAInterface> Achildren = Arow.children();
        QCOMPARE(Achildren.count(), 3);
        QAInterface A1 = Achildren.at(1);
        QAInterface A2 = Achildren.at(2);
        QCOMPARE(Arow.indexOfChild(A1), 2);
        QCOMPARE(Arow.indexOfChild(A2), 3);
        QCOMPARE(A1.role(), QAccessible::Cell);

        QList<QAInterface> Bchildren = Brow.children();
        QCOMPARE(Bchildren.count(), 3);
        QAInterface B1 = Bchildren.at(1);
        QAInterface B2 = Bchildren.at(2);
        QVERIFY(B1.parent() == Brow);
        QVERIFY(B1.parent() != Arow);
        QCOMPARE(Arow.indexOfChild(B1), -1);

        QVERIFY(A1 == A1);
        QVERIFY(A1 != A2);
        QVERIFY(B1 != A1);
        QVERIFY(B1 != A2);
        QVERIFY(A1 != Arow);
        QVERIFY(A1 != Brow);
        QVERIFY(A1 != viewPort);
        QVERIFY(A1.isHIView() == false);

        QVERIFY(B1.parent() == Brow);
        QVERIFY(A1.parent() == Arow);
        B1 = A1;
        QVERIFY(B1.parent() == Arow);
    }
    
    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    QVERIFY(applicationElement);
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const AXUIElementRef scrollAreaElement = childByRole(windowElement, "AXScrollArea");
    QVERIFY(scrollAreaElement);
    const AXUIElementRef tableElement = childByRole(scrollAreaElement, "AXTable");
    QVERIFY(tableElement);
   
    {
        // Verify that QAccessibleHierarchyManager can look up table view children correctly
        QAccessibleHierarchyManager *manager = QAccessibleHierarchyManager::instance();
        QAInterface tableInterface = manager->lookup(tableElement);
        QVERIFY(tableInterface.isValid());
        QVERIFY(tableInterface.role() == QAccessible::Table);
   
        QAInterface ArowInterface = tableInterface.childAt(2);

        QVERIFY(ArowInterface.name() == "1");
        QVERIFY(manager->lookup(manager->lookup(ArowInterface)).name() == "1");

        QCOMPARE(ArowInterface.childCount(), 3);
        QAInterface A1Interface = ArowInterface.childAt(2);

        QCOMPARE(A1Interface.value(), QString("A1"));
        QVERIFY(value(manager->lookup(A1Interface)).toString() == "A1");

        QAInterface A2Interface = ArowInterface.childAt(3);
        QAElement A2Element = manager->lookup(A2Interface);
        QVERIFY(manager->lookup(A2Element).value() == "A2");
        QVERIFY(value(A2Element).toString() == "A2");

        QAInterface BrowInterface = tableInterface.childAt(3);
        QVERIFY(BrowInterface.value() == "2");
        QVERIFY(manager->lookup(manager->lookup(BrowInterface)).value() == "2");

        QCOMPARE(BrowInterface.childCount(), 3);
        QAInterface B1Interface = BrowInterface.childAt(2);
        QVERIFY(value(manager->lookup(B1Interface)).toString() == "B1");

        QAInterface B2Interface = BrowInterface.childAt(3);
        QAElement B2Element = manager->lookup(B2Interface);
        QVERIFY(manager->lookup(B2Element).value() == "B2");
        QVERIFY(value(B2Element).toString() == "B2");
    }



    const AXUIElementRef Arow = childByTitle(tableElement, "1");
    QVERIFY(Arow);
    const AXUIElementRef Brow = childByTitle(tableElement, "2");
    QVERIFY(Brow);
    const AXUIElementRef Crow = childByTitle(tableElement, "3");
    QVERIFY(Crow);

    QCOMPARE(numChildren(Arow), 3);
    const AXUIElementRef A1cell = childByTitle(Arow, "A1");
    QVERIFY(A1cell);
    QVERIFY(role(A1cell) == "AXTextField");
    const AXUIElementRef A2cell = childByTitle(Arow, "A2");
    QVERIFY(A2cell);
    QVERIFY(equal(::parent(A2cell), Arow));

    const AXUIElementRef B2cell = childByTitle(Brow, "B2");
    QVERIFY(B2cell);
    QVERIFY(equal(::parent(B2cell), Brow));

    {
        QVERIFY(supportsAttribute(tableElement, kAXRowsAttribute));
        const QList<AXUIElementRef> rows = elementListAttribute(tableElement, kAXRowsAttribute);
        QCOMPARE(rows.count(), 3); // the header is not a row
        QVERIFY(value(rows.at(1)) == "2");
        QVERIFY(value(rows.at(2)) == "3");
    }

    {
        QVERIFY(supportsAttribute(tableElement, kAXVisibleRowsAttribute));
        const QList<AXUIElementRef> rows = elementListAttribute(tableElement, kAXVisibleRowsAttribute);
        QCOMPARE(rows.count(), 3);
        QVERIFY(value(rows.at(1)) == "2");
    }
    {
        QVERIFY(supportsAttribute(tableElement, kAXSelectedRowsAttribute));
        const QList<AXUIElementRef> rows = elementListAttribute(tableElement, kAXSelectedRowsAttribute);
        QCOMPARE(rows.count(), 0);
    }

    // test row visibility
    {
        QTableWidget tableWidget;
        tableWidget.setObjectName("tablewidget");
        tableWidget.setRowCount(1000);
        tableWidget.setColumnCount(1);

        for (int i =0; i < 1000; ++i) {
            tableWidget.setItem(i, 0, new QTableWidgetItem("item"));
        }
        tableWidget.show();
        
        QAInterface tableWidgetInterface = QAccessible::queryAccessibleInterface(&tableWidget);
        QAInterface viewPortInterface = tableWidgetInterface.childAt(1);
        QCOMPARE(viewPortInterface.childCount(), 1001);

        QVERIFY((viewPortInterface.childAt(2).state() & QAccessible::Invisible) == false);
        QVERIFY((viewPortInterface.childAt(2).state() & QAccessible::Offscreen) == false);
        
        QVERIFY(viewPortInterface.childAt(500).state() & QAccessible::Invisible);
//        QVERIFY(viewPortInterface.childAt(500).state() & QAccessible::Offscreen);
        tableWidget.hide();
    }

//    printElementInfo(tableElement);
//    QTest::qWait(1000000);
}

void tst_qaccessibility_mac::testScrollBar()
{
    {
        QScrollBar scrollBar;
        scrollBar.show();

        QAInterface scrollBarInterface = QAccessible::queryAccessibleInterface(&scrollBar);
        QVERIFY(scrollBarInterface.isValid());
        QCOMPARE(scrollBarInterface.childCount(), 5);
        QCOMPARE(scrollBarInterface.indexOfChild(scrollBarInterface.childAt(1)), 1);
        QCOMPARE(scrollBarInterface.indexOfChild(scrollBarInterface.childAt(2)), 2);
        QCOMPARE(scrollBarInterface.indexOfChild(scrollBarInterface.childAt(5)), 5);
        QCOMPARE(scrollBarInterface.indexOfChild(scrollBarInterface), -1);
    }

    const AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    testAppAndForm(currentApplication);
    const AXUIElementRef form = childByTitle(currentApplication, "Form");
    QVERIFY(form);
    const AXUIElementRef scrollBarElement = childByRole(form, "AXScrollBar");
    QVERIFY(scrollBarElement);
    QCOMPARE(attribute(scrollBarElement, kAXOrientationAttribute).toString(), QLatin1String("AXVerticalOrientation"));

    {
        const AXUIElementRef lineUpElement = childByTitle(scrollBarElement, "Line up");
        QVERIFY(lineUpElement);
        QCOMPARE (subrole(lineUpElement), QLatin1String("AXDecrementArrow"));
    }{
        const AXUIElementRef lineDownElement = childByTitle(scrollBarElement, "Line down");
        QVERIFY(lineDownElement);
        QCOMPARE (subrole(lineDownElement), QLatin1String("AXIncrementArrow"));
    }{
        const AXUIElementRef pageUpElement = childByTitle(scrollBarElement, "Page up");
        QVERIFY(pageUpElement);
        QCOMPARE (subrole(pageUpElement), QLatin1String("AXDecrementPage"));
    }{
        const AXUIElementRef pageDownElement = childByTitle(scrollBarElement, "Page down");
        QVERIFY(pageDownElement);
        QCOMPARE (subrole(pageDownElement), QLatin1String("AXIncrementPage"));
    }{
        const AXUIElementRef valueIndicatorElement = childByTitle(scrollBarElement, "Position");
        QVERIFY(valueIndicatorElement);
        QCOMPARE(value(valueIndicatorElement).toInt(), 50);
    }
}

void tst_qaccessibility_mac::testSplitter()
{
    const AXUIElementRef currentApplication = AXUIElementCreateApplication(getpid());
    testAppAndForm(currentApplication);
    const AXUIElementRef form = childByTitle(currentApplication, "Form");
    QVERIFY(form);
    
    const AXUIElementRef splitGroupElement = childByRole(form, "AXSplitGroup");
    QVERIFY(splitGroupElement);

    for (int i = 0; i < numChildren(splitGroupElement); ++i)
        QVERIFY(child(splitGroupElement, 3));

    // Visual Order: Foo splitter Bar splitter Baz
    QList<AXUIElementRef> splitterList = elementListAttribute(splitGroupElement, kAXSplittersAttribute);
    QCOMPARE(splitterList.count(), 2); 
    foreach (AXUIElementRef splitter, splitterList) {
        QCOMPARE(role(splitter), QLatin1String("AXSplitter"));
        QVERIFY(supportsAttribute(splitter, kAXPreviousContentsAttribute));
        QVERIFY(supportsAttribute(splitter, kAXNextContentsAttribute));
        QCOMPARE(attribute(splitter, kAXOrientationAttribute).toString(), QLatin1String("AXVerticalOrientation"));
        QList<AXUIElementRef> prevList = elementListAttribute(splitter, kAXPreviousContentsAttribute);  
        QCOMPARE(prevList.count(), 1); 
        QList<AXUIElementRef> nextList = elementListAttribute(splitter, kAXNextContentsAttribute);  
        QCOMPARE(nextList.count(), 1);
        
        // verify order
        if (title(prevList.at(0)) == QLatin1String("Foo"))
            QCOMPARE(title(nextList.at(0)), QLatin1String("Bar"));
        else if (title(prevList.at(0)) == QLatin1String("Bar"))
            QCOMPARE(title(nextList.at(0)), QLatin1String("Baz"));
        else {
            QFAIL("Splitter contents and handles are out of order"); 
        }
    }
}

void tst_qaccessibility_mac::testTextEdit()
{
    QWidget window;
    const QString windowTitle("window");
    window.setWindowTitle(windowTitle);
    window.resize(300, 300);

    QTextEdit *textEdit = new QTextEdit(&window);
    textEdit->resize(300, 300);
    const QString textLine("this is a line");
    textEdit->setText(textLine);
    
    window.show();

    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    QVERIFY(applicationElement);
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const AXUIElementRef scrollAreaElement = childByRole(windowElement, "AXScrollArea");
    QVERIFY(scrollAreaElement);
    const AXUIElementRef textElement = childByRole(scrollAreaElement, "AXTextField");
    QVERIFY(textElement);
    QVERIFY(value(textElement) == textLine);
}

void testModelLessItemView(QAbstractItemView *itemView, const QByteArray &role)
{
    const QString windowTitle("window");
    itemView->setWindowTitle(windowTitle);
    itemView->show();

    QTest::qWait(100);
#if defined(Q_WS_X11)
    qt_x11_wait_for_window_manager(w);
#endif

    QAccessibleInterface *acc = QAccessible::queryAccessibleInterface(itemView);
    QVERIFY(acc->isValid());
    QCOMPARE(acc->childCount(), 1);
    acc->role(0);
    acc->rect(0);

    QAccessibleInterface *accViewport = 0;
    int entry = acc->navigate(QAccessible::Child, 1, &accViewport);
    QVERIFY(accViewport);
    QCOMPARE(entry, 0);
    QVERIFY(accViewport->isValid());
    QCOMPARE(accViewport->childCount(), 0);
    accViewport->role(0);
    accViewport->rect(0);

    delete acc;
    delete accViewport;
        
    const AXUIElementRef applicationElement = AXUIElementCreateApplication(getpid());
    QVERIFY(applicationElement);
    const AXUIElementRef windowElement = childByTitle(applicationElement, windowTitle);
    QVERIFY(windowElement);
    const AXUIElementRef scrollAreaElement = childByRole(windowElement, "AXScrollArea");
    QVERIFY(scrollAreaElement);
    const AXUIElementRef tableElement = childByRole(scrollAreaElement, role);
    QVERIFY(tableElement);
    
    delete itemView;
}

void tst_qaccessibility_mac::testItemViewsWithoutModel()
{
    testModelLessItemView(new QListView(), "AXList");
    testModelLessItemView(new QTableView(), "AXTable");
}

void tst_qaccessibility_mac::testQAElement()
{
    {
        QAElement element;
        QVERIFY(element.isValid() == false);
    }
    
    {
        QAElement element(0, 0);
        QVERIFY(element.isValid() == false);
    }

    {
        int argc = 0;
        char **argv = 0;
        QApplication app(argc, argv);
        QWidget w;
        QAElement element(reinterpret_cast<HIObjectRef>(w.winId()), 0);
        QVERIFY(element.isValid() == true);
    }

}

void tst_qaccessibility_mac::testQAInterface()
{
    {
        QAInterface interface;
        QVERIFY(interface.isValid() == false);
    }
    
    {
        QAInterface interface(0, 0);
        QVERIFY(interface.isValid() == false);
    }

    {
        int argc = 0;
        char **argv = 0;
        QApplication app(argc, argv);

        {
            QWidget w;
            QAInterface element(QAccessible::queryAccessibleInterface(&w), 0);
            QVERIFY(element.isValid() == true);
        }
        {
            QWidget w;
            QAInterface element(QAccessible::queryAccessibleInterface(&w), 100);
            QVERIFY(element.isValid() == false);
        }
    }
}

void tst_qaccessibility_mac::uitests_data()
{
    QTest::addColumn<QString>("uiFilename");
    QTest::addColumn<QString>("testSlot");

    QTest::newRow("form") << "form.ui" << SLOT(testForm());
    QTest::newRow("buttons") << "buttons.ui" << SLOT(testButtons());
    QTest::newRow("label") << "label.ui" << SLOT(testLabel());
    QTest::newRow("line edit") << "lineedit.ui" << SLOT(testLineEdit());
    QTest::newRow("groups") << "groups.ui" << SLOT(testGroups());
    QTest::newRow("tabs") << "tabs.ui" << SLOT(testTabWidget());
    QTest::newRow("combobox") << "combobox.ui" << SLOT(testComboBox());
    QTest::newRow("scrollbar") << "scrollbar.ui" << SLOT(testScrollBar());
    QTest::newRow("splitters") << "splitters.ui" << SLOT(testSplitter());
}

void tst_qaccessibility_mac::uitests()
{
    QFETCH(QString, uiFilename);
    QFETCH(QString, testSlot);

    // The Accessibility interface must be enabled to run this test.
    if (!AXAPIEnabled())
        QSKIP("Accessibility not enabled. Check \"Enable access for assistive devices\" in the system preferences -> universal access to run this test.", SkipAll);

    int argc = 0;
    char **argv = 0;
    QApplication app(argc, argv);

    // Create and display form.
    QUiLoader loader;
    QFile file(":" + uiFilename);
    QVERIFY(file.exists());
    file.open(QFile::ReadOnly);
    QWidget *window = loader.load(&file, 0);
    QVERIFY(window);
    file.close();
    window->show();

    QTimer::singleShot(50, this, qPrintable(testSlot));
    // Quit when returning to the main event loop after running tests.
    QTimer::singleShot(200, &app, SLOT(quit()));
    app.exec();
    delete window;
}

void tst_qaccessibility_mac::tests_data()
{
    QTest::addColumn<QString>("testSlot");
    QTest::newRow("deleteWidget") << SLOT(testDeleteWidget());
    QTest::newRow("deleteWidgets") << SLOT(testDeleteWidgets());
    QTest::newRow("multipleWindows") << SLOT(testMultipleWindows());
    QTest::newRow("hiddenWidgets") << SLOT(testHiddenWidgets());
    QTest::newRow("actions") << SLOT(testActions());
    QTest::newRow("changeState") << SLOT(testChangeState());
    QTest::newRow("slider") << SLOT(testSlider());
    QTest::newRow("scrollArea") << SLOT(testScrollArea());
    QTest::newRow("listView") << SLOT(testListView());
    QTest::newRow("tableView") << SLOT(testTableView());
    QTest::newRow("textEdit") << SLOT(testTextEdit());
    QTest::newRow("ItemViews without model") << SLOT(testItemViewsWithoutModel());
    QTest::newRow("tabbar") << SLOT(testTabBar());
}

void tst_qaccessibility_mac::tests()
{
    QFETCH(QString, testSlot);
    runTest(testSlot);
}

/*
    Tests show that querying the accessibility interface directly does not work. (I get a
    kAXErrorAPIDisabled error, indicating that the accessible API is disabled, which it isn't.)
    To work around this, we run the tests in a callback slot called from the main event loop.
*/
void tst_qaccessibility_mac::runTest(const QString &testSlot)
{
    // The Accessibility interface must be enabled to run this test.
    if (!AXAPIEnabled())
        QSKIP("Accessibility not enabled. Check \"Enable access for assistive devices\" in the system preferences -> universal access to run this test.", SkipAll);

    int argc = 0;
    char **argv = 0;
    QApplication app(argc, argv);

    QTimer::singleShot(50, this, qPrintable(testSlot));
    // Quit when returning to the main event loop after running tests.
    QTimer::singleShot(200, &app, SLOT(quit()));
    app.exec();

}

QTEST_APPLESS_MAIN(tst_qaccessibility_mac)

#else // defined(Q_WS_MAC) && !defined (QT_MAC_USE_COCOA)

QTEST_NOOP_MAIN

#endif

#include "tst_qaccessibility_mac.moc"


