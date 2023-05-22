// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QtWidgets>
#include <QTest>
#include <QtCore/qcoreapplication.h>

// some versions of CALayer.h use 'slots' as an identifier
#define QT_NO_KEYWORDS

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets>
#include <QTest>
#include <unistd.h>

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

QT_USE_NAMESPACE

struct AXErrorTag {
    AXError err;
    explicit AXErrorTag(AXError theErr) : err(theErr) {}
};

QDebug operator<<(QDebug dbg, AXErrorTag err)
{
    QDebugStateSaver saver(dbg);

    const char *errDesc = 0;
    const char *errName = 0;
    switch (err.err) {
#define HANDLE_ERR(error, desc) case kAXError##error: errName = "kAXError" #error; errDesc = desc; break
        HANDLE_ERR(Success, "Success");
        HANDLE_ERR(Failure, "A system error occurred, such as the failure to allocate an object.");
        HANDLE_ERR(IllegalArgument, "An illegal argument was passed to the function.");
        HANDLE_ERR(InvalidUIElement, "The AXUIElementRef passed to the function is invalid.");
        HANDLE_ERR(InvalidUIElementObserver, "The AXObserverRef passed to the function is not a valid observer.");
        HANDLE_ERR(CannotComplete, "The function cannot complete because messaging failed in some way or because the application with which the function is communicating is busy or unresponsive.");
        HANDLE_ERR(AttributeUnsupported, "The attribute is not supported by the AXUIElementRef.");
        HANDLE_ERR(ActionUnsupported, "The action is not supported by the AXUIElementRef.");
        HANDLE_ERR(NotificationUnsupported, "The notification is not supported by the AXUIElementRef.");
        HANDLE_ERR(NotImplemented, "Indicates that the function or method is not implemented (this can be returned if a process does not support the accessibility API).");
        HANDLE_ERR(NotificationAlreadyRegistered, "This notification has already been registered for.");
        HANDLE_ERR(NotificationNotRegistered, "Indicates that a notification is not registered yet.");
        HANDLE_ERR(APIDisabled, "The accessibility API is disabled (as when, for example, the user deselects \"Enable access for assistive devices\" in Universal Access Preferences).");
        HANDLE_ERR(NoValue, "The requested value or AXUIElementRef does not exist.");
        HANDLE_ERR(ParameterizedAttributeUnsupported, "The parameterized attribute is not supported by the AXUIElementRef.");
        HANDLE_ERR(NotEnoughPrecision, "Not enough precision.");
        default: errName = "<unknown error>"; errDesc = "UNKNOWN ERROR"; break;
    }
#undef HANDLE_ERR

    dbg.nospace() << "AXError(value=" << err.err << ", name=" << errName << ", description=\"" << errDesc << "\")";

    return dbg;
}

@interface TestAXObject : NSObject
{
    AXUIElementRef reference;
    bool axError;
}
    @property (readonly) NSString *role;
    @property (readonly) NSString *title;
    @property (readonly) NSString *description;
    @property (readonly) NSString *value;
    @property (readonly) CGRect rect;
    @property (readonly) NSArray *actions;
@end

@implementation TestAXObject

- (instancetype)initWithAXUIElementRef:(AXUIElementRef)ref {

    if ((self = [super init])) {
        reference = ref;
        axError = false;
    }
    return self;
}

- (AXUIElementRef) ref { return reference; }
- (bool)errorOccurred { return axError; }
- (void) print {
    NSLog(@"Accessible Object role: '%@', title: '%@', description: '%@', value: '%@', rect: '%@'", self.role, self.title, self.description, self.value, NSStringFromRect(NSRectFromCGRect(self.rect)));
    NSLog(@"    Children: %ld", [[self childList] count]);
}

- (NSArray*) windowList
{
    NSArray *list;
    AXUIElementCopyAttributeValues(
                reference,
                kAXWindowsAttribute,
                0, 100, /*min, max*/
                (CFArrayRef *) &list);
    return list;
}

- (NSArray*) childList
{
    NSArray *list;
    AXUIElementCopyAttributeValues(
                reference,
                kAXChildrenAttribute,
                0, 100, /*min, max*/
                (CFArrayRef *) &list);
    return list;
}

- (NSArray *)tableRows
{
        NSArray *arr;
        AXUIElementCopyAttributeValues(
                    reference,
                    kAXRowsAttribute,
                    0, 100, /*min, max*/
                    (CFArrayRef *) &arr);
        return arr;
}

- (NSArray *)tableColumns
{
        NSArray *arr;
        AXUIElementCopyAttributeValues(
                    reference,
                    kAXColumnsAttribute,
                    0, 100, /*min, max*/
                    (CFArrayRef *) &arr);
        return arr;
}

- (AXUIElementRef) findDirectChildByRole: (CFStringRef) role
{
    TestAXObject *result = nil;
    NSArray *childList = [self childList];
    for (id child in childList) {
        TestAXObject *childObject = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)child];
        if ([childObject.role isEqualToString:(NSString*)role]) {
            result = childObject;
            break;
        }
    }
    AXUIElementRef ret = [result ref];
    [result release];
    return ret;
}

+ (TestAXObject *) getApplicationAXObject
{
    pid_t pid = getpid();
    AXUIElementRef appRef = AXUIElementCreateApplication(pid);
    TestAXObject *appObject = [[TestAXObject alloc] initWithAXUIElementRef: appRef];
    return appObject;
}

+ (NSInteger)_numberFromValue:(CFTypeRef)value
{
    NSInteger number = -1;
    if (!CFNumberGetValue((CFNumberRef)value, kCFNumberNSIntegerType, &number))
    {
        qDebug() << "Could not get NSInteger value out of CFNumberRef";
    }
    return number;
}

+ (BOOL)_boolFromValue:(CFTypeRef)value
{
    return CFBooleanGetValue((CFBooleanRef)value);
}

+ (NSRange)_rangeFromValue:(CFTypeRef)value
{
    CFRange cfRange;
    NSRange range = NSMakeRange(0, 0);

    if (!AXValueGetValue(AXValueRef(value), AXValueType(kAXValueCFRangeType), &cfRange))
        qDebug() << "Could not get CFRange value out of AXValueRef";
    else if (cfRange.location < 0 || cfRange.length < 0)
        qDebug() << "Cannot convert CFRange with negative location or length to NSRange";
    else if (static_cast<uintmax_t>(cfRange.location) > NSUIntegerMax || static_cast<uintmax_t>(cfRange.length) > NSUIntegerMax)
        qDebug() << "Cannot convert CFRange with location or length out of bounds for NSUInteger";
    else
    {
        range.length = static_cast<NSUInteger>(cfRange.length);
        range.location = static_cast<NSUInteger>(cfRange.location);
    }

    return range;
}

+ (NSRect)_rectFromValue:(CFTypeRef)value
{
    NSRect rect = NSMakeRect(0, 0, 0, 0);
    if (!AXValueGetValue(AXValueRef(value), AXValueType(kAXValueCGRectType), reinterpret_cast<CGRect*>(&rect)))
    {
        qDebug() << "Could not get CGRect value out of AXValueRef";
    }
    return rect;
}

+ (NSPoint)_pointFromValue:(CFTypeRef)value
{
    NSPoint point = NSMakePoint(0, 0);
    if (!AXValueGetValue(AXValueRef(value), AXValueType(kAXValueCGPointType), reinterpret_cast<CGPoint*>(&point)))
    {
        qDebug() << "Could not get CGPoint value out of AXValueRef";
    }
    return point;
}

+ (NSSize)_sizeFromValue:(CFTypeRef)value
{
    NSSize size = NSMakeSize(0, 0);
    if (!AXValueGetValue(AXValueRef(value), AXValueType(kAXValueCGSizeType), reinterpret_cast<CGSize*>(&size)))
    {
        qDebug() << "Could not get CGSize value out of AXValueRef";
    }
    return size;
}

- (CFTypeRef)_attributeValue:(CFStringRef)attribute
{
    CFTypeRef value = NULL;
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyAttributeValue(reference, attribute, &value))) {
        axError = true;
        qDebug() << "AXUIElementCopyAttributeValue(" << QString::fromCFString(attribute) << ") returned error = " << AXErrorTag(err);
    }
    return value;
}

- (NSString*)_stringAttributeValue:(CFStringRef)attribute
{
    return (NSString*)[self _attributeValue:attribute];
}

- (NSInteger)_numberAttributeValue:(CFStringRef)attribute
{
    return [[self class] _numberFromValue:[self _attributeValue:attribute]];
}

- (BOOL)_boolAttributeValue:(CFStringRef)attribute
{
    return [[self class] _boolFromValue:[self _attributeValue:attribute]];
}

- (NSRange)_rangeAttributeValue:(CFStringRef)attribute
{
    return [[self class] _rangeFromValue:[self _attributeValue:attribute]];
}

- (NSRect)_rectAttributeValue:(CFStringRef)attribute
{
    return [[self class] _rectFromValue:[self _attributeValue:attribute]];
}

- (NSPoint)_pointAttributeValue:(CFStringRef)attribute
{
    return [[self class] _pointFromValue:[self _attributeValue:attribute]];
}

- (NSSize)_sizeAttributeValue:(CFStringRef)attribute
{
    return [[self class] _sizeFromValue:[self _attributeValue:attribute]];
}

- (CFTypeRef)_attributeValue:(CFStringRef)attribute forParameter:(CFTypeRef)parameter
{
    CFTypeRef value = NULL;
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyParameterizedAttributeValue(reference, attribute, parameter, &value))) {
        axError = true;
        CFStringRef description = CFCopyDescription(parameter);
        qDebug() << "AXUIElementCopyParameterizedAttributeValue(" << QString::fromCFString(attribute) << ", parameter=" << QString::fromCFString(description) << ") returned error = " << AXErrorTag(err);
        CFRelease(description);
    }
    return value;
}

- (CFTypeRef)_attributeValue:(CFStringRef)attribute forRange:(NSRange)aRange
{
    CFRange cfRange = CFRangeMake(aRange.location, aRange.length);
    AXValueRef range = AXValueCreate(AXValueType(kAXValueCFRangeType), &cfRange);
    CFTypeRef value =  [self _attributeValue:attribute forParameter:range];
    CFRelease(range);
    return value;
}

- (CFTypeRef)_attributeValue:(CFStringRef)attribute forNumber:(NSInteger)aNumber
{
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberNSIntegerType, &aNumber);
    CFTypeRef value = [self _attributeValue:attribute forParameter:number];
    CFRelease(number);
    return value;
}

- (CFTypeRef)_attributeValue:(CFStringRef)attribute forPoint:(CGPoint)aPoint
{
    AXValueRef point = AXValueCreate(AXValueType(kAXValueCGPointType), &aPoint);
    CFTypeRef value = [self _attributeValue:attribute forParameter:point];
    CFRelease(point);
    return value;
}

- (NSArray*)actions
{
    AXError err;
    CFArrayRef actions;

    if (kAXErrorSuccess != (err = AXUIElementCopyActionNames(reference, &actions))) {
        axError = true;
        qDebug() << "AXUIElementCopyActionNames(...) returned error = " << AXErrorTag(err);
    }

    return (NSArray*)actions;
}

- (void)performAction:(CFStringRef)action
{
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementPerformAction(reference, action))) {
        axError = true;
        qDebug() << "AXUIElementPerformAction("  << QString::fromCFString(action) << ") returned error = " << AXErrorTag(err);
    }
}

- (NSString*)           role { return [self _stringAttributeValue:kAXRoleAttribute]; }
- (NSString*)           title { return [self _stringAttributeValue:kAXTitleAttribute]; }
- (NSString*)           description { return [self _stringAttributeValue:kAXDescriptionAttribute]; }
- (NSString*)           value { return [self _stringAttributeValue:kAXValueAttribute]; }
- (NSInteger)           valueNumber { return [self _numberAttributeValue:kAXValueAttribute]; }
- (NSRect)              rect
{
    NSRect rect;
    rect.origin = [self _pointAttributeValue:kAXPositionAttribute];
    rect.size = [self _sizeAttributeValue:kAXSizeAttribute];
    return rect;
}
- (AXUIElementRef)      parent { return (AXUIElementRef)[self _attributeValue:kAXParentAttribute]; }
- (BOOL)                focused { return [self _boolAttributeValue:kAXFocusedAttribute]; }
- (NSInteger)           numberOfCharacters { return [self _numberAttributeValue:kAXNumberOfCharactersAttribute]; }
- (NSString*)           selectedText { return [self _stringAttributeValue:kAXSelectedTextAttribute]; }
- (NSRange)             selectedTextRange { return [self _rangeAttributeValue:kAXSelectedTextRangeAttribute]; }
- (NSRange)             visibleCharacterRange  { return [self _rangeAttributeValue:kAXVisibleCharacterRangeAttribute]; }
- (NSString*)           help { return [self _stringAttributeValue:kAXHelpAttribute]; }
- (NSInteger)           insertionPointLineNumber { return [self _numberAttributeValue:kAXInsertionPointLineNumberAttribute]; }

- (NSInteger)           lineForIndex:(NSInteger)index { return [[self class] _numberFromValue:[self _attributeValue:kAXLineForIndexParameterizedAttribute forNumber:index]]; }
- (NSRange)             rangeForLine:(NSInteger)line { return [[self class] _rangeFromValue:[self _attributeValue:kAXRangeForLineParameterizedAttribute forNumber:line]]; }
- (NSString*)           stringForRange:(NSRange)range { return (NSString*)[self _attributeValue:kAXStringForRangeParameterizedAttribute forRange:range]; }
- (NSAttributedString*) attributedStringForRange:(NSRange)range { return (NSAttributedString*)[self _attributeValue:kAXAttributedStringForRangeParameterizedAttribute forRange:range]; }
- (NSRect)              boundsForRange:(NSRange)range { return [[self class] _rectFromValue:[self _attributeValue:kAXBoundsForRangeParameterizedAttribute forRange:range]]; }
- (NSRange)             styleRangeForIndex:(NSInteger)index { return [[self class] _rangeFromValue:[self _attributeValue:kAXStyleRangeForIndexParameterizedAttribute forNumber:index]]; }

@end

QVector<int> notificationList;

void observerCallback(AXObserverRef /*observer*/, AXUIElementRef /*element*/, CFStringRef notification, void *)
{
    if ([(NSString*)notification isEqualToString: NSAccessibilityFocusedUIElementChangedNotification])
        notificationList.append(QAccessible::Focus);
    else if ([(NSString*)notification isEqualToString: NSAccessibilityValueChangedNotification])
        notificationList.append(QAccessible::ValueChanged);
    else
        notificationList.append(-1);
}

class AccessibleTestWindow : public QWidget
{
    Q_OBJECT
public:
    AccessibleTestWindow()
    {
        new QHBoxLayout(this);
    }

    void addWidget(QWidget* widget)
    {
        layout()->addWidget(widget);
        widget->show();
        QVERIFY(QTest::qWaitForWindowExposed(widget));
    }

    void clearChildren()
    {
        qDeleteAll(children());
        new QHBoxLayout(this);
    }
};

class tst_QAccessibilityMac : public QObject
{
Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void singleWidgetTest();
    void lineEditTest();
    void hierarchyTest();
    void notificationsTest();
    void checkBoxTest();
    void tableViewTest();
    void treeViewTest();

private:
    AccessibleTestWindow *m_window;
};


void tst_QAccessibilityMac::init()
{
    m_window = new AccessibleTestWindow();
    m_window->setWindowTitle(QString("Test window - %1").arg(QTest::currentTestFunction()));
    m_window->show();
    m_window->resize(400, 400);

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
}

void tst_QAccessibilityMac::cleanup()
{
    delete m_window;
}

void tst_QAccessibilityMac::singleWidgetTest()
{
    delete m_window;
    m_window = 0;

    QLineEdit *le = new QLineEdit();
    le->setText("button");
    le->show();
    QVERIFY(QTest::qWaitForWindowExposed(le));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    QTRY_VERIFY(appObject.windowList.count == 1);

    AXUIElementRef windowRef = (AXUIElementRef) [appObject.windowList objectAtIndex: 0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    AXUIElementRef lineEditRef = [window findDirectChildByRole: kAXTextFieldRole];
    QVERIFY(lineEditRef != nil);
    TestAXObject *lineEdit = [[TestAXObject alloc] initWithAXUIElementRef: lineEditRef];
    QVERIFY([[lineEdit value] isEqualToString:@"button"]);

    // Access invalid reference, should return empty value
    delete le;
    QCoreApplication::processEvents();
    TestAXObject *lineEditInvalid = [[TestAXObject alloc] initWithAXUIElementRef: lineEditRef];
    QVERIFY([[lineEditInvalid value] length] == 0);
}

void tst_QAccessibilityMac::lineEditTest()
{
    QLineEdit *lineEdit = new QLineEdit(m_window);
    lineEdit->setText("a11y test QLineEdit");
    m_window->addWidget(lineEdit);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    // one window
    QTRY_VERIFY(appObject.windowList.count == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [appObject.windowList objectAtIndex: 0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    QVERIFY([window rect].size.width == 400);
    // height of window includes title bar
    QVERIFY([window rect].size.height >= 400);

    QVERIFY([window.title isEqualToString:@"Test window - lineEditTest"]);

    // children of window:
    AXUIElementRef lineEditElement = [window findDirectChildByRole: kAXTextFieldRole];
    QVERIFY(lineEditElement != nil);

    TestAXObject *le = [[TestAXObject alloc] initWithAXUIElementRef: lineEditElement];
    NSString *value = @"a11y test QLineEdit";
    QVERIFY([le.value isEqualToString:value]);
    QVERIFY(value.length <= NSIntegerMax);
    QVERIFY(le.numberOfCharacters == static_cast<NSInteger>(value.length));
    const NSRange ranges[] = {
        { 0, 0},
        { 0, 1},
        { 0, 5},
        { 5, 0},
        { 5, 1},
        { 0, value.length},
        { value.length, 0},
    };
    for (size_t i = 0; i < sizeof(ranges)/sizeof(ranges[0]); ++i) {
        NSRange range = ranges[i];
        NSString *expectedSubstring = [value substringWithRange:range];
        NSString *actualSubstring = [le stringForRange:range];
        NSString *actualAttributedSubstring = [le attributedStringForRange:range].string;
        QVERIFY([actualSubstring isEqualTo:expectedSubstring]);
        QVERIFY([actualAttributedSubstring isEqualTo:expectedSubstring]);
    }
}

void tst_QAccessibilityMac::hierarchyTest()
{
    QWidget *w = new QWidget(m_window);
    m_window->addWidget(w);

    w->setLayout(new QVBoxLayout());
    QPushButton *b = new QPushButton(w);
    w->layout()->addWidget(b);
    b->setText("I am a button");

    QPushButton *b2 = new QPushButton(w);
    w->layout()->addWidget(b2);
    b2->setText("Button 2");

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    // one window
    QTRY_VERIFY(appObject.windowList.count == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [appObject.windowList objectAtIndex: 0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    // Because the plain widget is filtered out of the hierarchy, we expect the button
    // to be a direct child of the window
    AXUIElementRef buttonRef = [window findDirectChildByRole: kAXButtonRole];
    QVERIFY(buttonRef != nil);

    TestAXObject *buttonObject = [[TestAXObject alloc] initWithAXUIElementRef: buttonRef];
    TestAXObject *parentObject = [[TestAXObject alloc] initWithAXUIElementRef: [buttonObject parent]];

    // check that the parent is a window
    QVERIFY([[parentObject role] isEqualToString: NSAccessibilityWindowRole]);

    // test the focus
    // child 0 is the layout, then button1 and 2
    QPushButton *button1 = qobject_cast<QPushButton*>(w->children().at(1));
    QVERIFY(button1);
    QPushButton *button2 = qobject_cast<QPushButton*>(w->children().at(2));
    QVERIFY(button2);
    button2->setFocus();

    AXUIElementRef systemWideElement = AXUIElementCreateSystemWide();
    AXUIElementRef focussedElement = NULL;
    AXError error = AXUIElementCopyAttributeValue(systemWideElement,
        (CFStringRef)NSAccessibilityFocusedUIElementAttribute, (CFTypeRef*)&focussedElement);
    QVERIFY(!error);
    QVERIFY(focussedElement);
    TestAXObject *focusButton2 = [[TestAXObject alloc] initWithAXUIElementRef: focussedElement];

    QVERIFY([[focusButton2 role] isEqualToString: NSAccessibilityButtonRole]);
    QVERIFY([[focusButton2 title] isEqualToString: @"Button 2"]);


    button1->setFocus();
    error = AXUIElementCopyAttributeValue(systemWideElement,
        (CFStringRef)NSAccessibilityFocusedUIElementAttribute, (CFTypeRef*)&focussedElement);
    QVERIFY(!error);
    QVERIFY(focussedElement);
    TestAXObject *focusButton1 = [[TestAXObject alloc] initWithAXUIElementRef: focussedElement];
    QVERIFY([[focusButton1 role] isEqualToString: NSAccessibilityButtonRole]);
    QVERIFY([[focusButton1 title] isEqualToString: @"I am a button"]);
}

void tst_QAccessibilityMac::notificationsTest()
{
    auto *w = m_window;
    QLineEdit *le1 = new QLineEdit(w);
    QLineEdit *le2 = new QLineEdit(w);
    w->layout()->addWidget(le1);
    w->layout()->addWidget(le2);

    QCoreApplication::processEvents();
    QTest::qWait(100);

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    // one window
    QTRY_VERIFY(appObject.windowList.count == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [appObject.windowList objectAtIndex: 0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    AXUIElementRef lineEdit1 = [window findDirectChildByRole: kAXTextFieldRole];
    QVERIFY(lineEdit1 != nil);

    AXObserverRef observer = 0;
    AXError err = AXObserverCreate(getpid(), observerCallback, &observer);
    QVERIFY(!err);
    AXObserverAddNotification(observer, appObject.ref, kAXFocusedUIElementChangedNotification, 0);
    AXObserverAddNotification(observer, lineEdit1, kAXValueChangedNotification, 0);

    CFRunLoopAddSource( [[NSRunLoop currentRunLoop] getCFRunLoop], AXObserverGetRunLoopSource(observer), kCFRunLoopDefaultMode);

    QVERIFY(notificationList.length() == 0);
    le2->setFocus();
    QTRY_VERIFY(notificationList.length() == 1);
    QTRY_VERIFY(notificationList.at(0) == QAccessible::Focus);
    le1->setFocus();
    QTRY_VERIFY(notificationList.length() == 2);
    QTRY_VERIFY(notificationList.at(1) == QAccessible::Focus);
    le1->setText("hello");
    QTRY_VERIFY(notificationList.length() == 3);
    QTRY_VERIFY(notificationList.at(2) == QAccessible::ValueChanged);
    le1->setText("foo");
    QTRY_VERIFY(notificationList.length() == 4);
    QTRY_VERIFY(notificationList.at(3) == QAccessible::ValueChanged);
}

void tst_QAccessibilityMac::checkBoxTest()
{
    QCheckBox *ckBox = new QCheckBox(m_window);
    ckBox->setText("Great option");
    m_window->addWidget(ckBox);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    // one window
    QTRY_VERIFY(appObject.windowList.count == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [appObject.windowList objectAtIndex: 0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    // children of window:
    AXUIElementRef checkBox = [window findDirectChildByRole: kAXCheckBoxRole];
    QVERIFY(checkBox != nil);

    TestAXObject *cb = [[TestAXObject alloc] initWithAXUIElementRef: checkBox];

    // here start actual checkbox tests
    QVERIFY([cb valueNumber] == 0);
    QVERIFY([cb.title isEqualToString:@"Great option"]);
    // EXPECT(cb.description == nil); // currently returns "" instead of nil

    QVERIFY([cb.actions containsObject:(NSString*)kAXPressAction]);

    [cb performAction:kAXPressAction];
    QVERIFY([cb valueNumber] == 1);

    [cb performAction:kAXPressAction];
    QVERIFY([cb valueNumber] == 0);

    ckBox->setCheckState(Qt::PartiallyChecked);
    QVERIFY([cb valueNumber] == 2);
}

void tst_QAccessibilityMac::tableViewTest()
{
    QTableWidget *tw = new QTableWidget(3, 2, m_window);
    struct Person
    {
        const char *name;
        const char *address;
    };
    const Person contents[] = { { "Socrates", "Greece" },
                                { "Confucius", "China" },
                                { "Kant", "Preussia" }
                              };
    for (int i = 0; i < int(sizeof(contents) / sizeof(Person)); ++i) {
        Person p = contents[i];
        QTableWidgetItem *name = new QTableWidgetItem(QString::fromLatin1(p.name));
        tw->setItem(i, 0, name);
        QTableWidgetItem *address = new QTableWidgetItem(QString::fromLatin1(p.address));
        tw->setItem(i, 1, address);
    }
    m_window->addWidget(tw);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    NSArray *windowList = [appObject windowList];
    // one window
    QVERIFY([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef)[windowList objectAtIndex:0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef:windowRef];

    // children of window:
    AXUIElementRef tableView = [window findDirectChildByRole:kAXTableRole];
    QVERIFY(tableView != nil);

    TestAXObject *tv = [[TestAXObject alloc] initWithAXUIElementRef:tableView];

    // here start actual tableview tests
    // Should have 2 columns
    const unsigned int columnCount = 2;
    NSArray *columnArray = [tv tableColumns];
    QCOMPARE([columnArray count], columnCount);

    // should have 3 rows
    const unsigned int rowCount = 3;
    NSArray *rowArray = [tv tableRows];
    QCOMPARE([rowArray count], rowCount);

    // The individual cells are children of the rows
    TestAXObject *row = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)rowArray[0]];
    TestAXObject *cell = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)[row childList][0]];
    QVERIFY([cell.title isEqualToString:@"Socrates"]);
    row = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)rowArray[2]];
    cell = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)[row childList][1]];
    QVERIFY([cell.title isEqualToString:@"Preussia"]);

    // both rows and columns are direct children of the table
    NSArray *childList = [tv childList];
    QCOMPARE([childList count], columnCount + rowCount);
    for (id child in childList) {
        TestAXObject *childObject = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)child];
        QVERIFY([childObject.role isEqualToString:NSAccessibilityRowRole] ||
               [childObject.role isEqualToString:NSAccessibilityColumnRole]);
    }
}

void tst_QAccessibilityMac::treeViewTest()
{
    QTreeWidget *tw = new QTreeWidget;
    tw->setColumnCount(2);
    QTreeWidgetItem *root = new QTreeWidgetItem(tw, {"/", "0"});
    root->setExpanded(false);
    QTreeWidgetItem *users = new QTreeWidgetItem(root,{ "Users", "1"});
    (void)new QTreeWidgetItem(root, {"Applications", "2"});
    QTreeWidgetItem *lastChild = new QTreeWidgetItem(root, {"Libraries", "3"});

    m_window->addWidget(tw);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    NSArray *windowList = [appObject windowList];
    // one window
    QVERIFY([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef)[windowList objectAtIndex:0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef:windowRef];

    // children of window
    AXUIElementRef treeView = [window findDirectChildByRole:kAXOutlineRole];
    QVERIFY(treeView != nil);

    TestAXObject *tv = [[TestAXObject alloc] initWithAXUIElementRef:treeView];

    // here start actual treeview tests. NSAccessibilityOutline is a specialization
    // of NSAccessibilityTable, and we represent trees as tables.
    // Should have 2 columns
    const unsigned int columnCount = 2;
    NSArray *columnArray = [tv tableColumns];
    QCOMPARE([columnArray count], columnCount);

    // should have 1 row for now - as long as the root item is not expanded
    NSArray *rowArray = [tv tableRows];
    QCOMPARE(int([rowArray count]), 1);

    root->setExpanded(true);
    rowArray = [tv tableRows];
    QCOMPARE(int([rowArray count]), root->childCount() + 1);

    // this should not trigger any assert
    tw->setCurrentItem(lastChild);

    bool errorOccurred = false;

    const auto cellText = [rowArray, &errorOccurred](int rowIndex, int columnIndex) -> QString {
        TestAXObject *row = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)rowArray[rowIndex]];
        Q_ASSERT(row);
        TestAXObject *cell = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)[row childList][columnIndex]];
        Q_ASSERT(cell);
        const QString result = QString::fromNSString(cell.title);
        errorOccurred = cell.errorOccurred;
        return result;
    };

    QString text = cellText(0, 0);
    if (errorOccurred)
        QSKIP("Cocoa Accessibility API error, aborting");
    QCOMPARE(text, root->text(0));
    QCOMPARE(cellText(1, 0), users->text(0));
    QCOMPARE(cellText(1, 1), users->text(1));
}

QTEST_MAIN(tst_QAccessibilityMac)
#include "tst_qaccessibilitymac.moc"
