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

// some versions of CALayer.h use 'slots' as an identifier
#define QT_NO_KEYWORDS

#include "tst_qaccessibilitymac_helpers.h"
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets>
#include <QtTest>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

QT_USE_NAMESPACE

bool macNativeAccessibilityEnabled()
{
    bool enabled = AXAPIEnabled();
    if (!enabled)
        qWarning() << "Accessibility is disabled (check System Preferences) skipping test.";
    return enabled;
}

bool trusted()
{
    return AXIsProcessTrusted();
}

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

#define EXPECT(cond) \
    if (!(cond)) { \
        qWarning("Failure in %s, line: %d", __FILE__ , __LINE__); \
        return false; \
    } \


@interface TestAXObject : NSObject
{
    AXUIElementRef reference;
}
    @property (readonly) NSString *role;
    @property (readonly) NSString *title;
    @property (readonly) NSString *description;
    @property (readonly) NSString *value;
    @property (readonly) CGRect rect;
    @property (readonly) NSArray *actions;
@end

@implementation TestAXObject

- (id) initWithAXUIElementRef: (AXUIElementRef) ref {

    if ( self = [super init] ) {
        reference = ref;
    }
    return self;
}

- (AXUIElementRef) ref { return reference; }
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

    if (kAXErrorSuccess != (err = AXUIElementCopyAttributeValue(reference, attribute, &value)))
    {
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

    if (kAXErrorSuccess != (err = AXUIElementCopyParameterizedAttributeValue(reference, attribute, parameter, &value)))
    {
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

    if (kAXErrorSuccess != (err = AXUIElementCopyActionNames(reference, &actions)))
    {
        qDebug() << "AXUIElementCopyActionNames(...) returned error = " << AXErrorTag(err);
    }

    return (NSArray*)actions;
}

- (void)performAction:(CFStringRef)action
{
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementPerformAction(reference, action)))
    {
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


bool singleWidget()
{
    QLineEdit *le = new QLineEdit();
    le->setText("button");
    le->show();
    EXPECT(QTest::qWaitForWindowExposed(le));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    EXPECT(appObject);

    NSArray *windows = [appObject windowList];
    EXPECT([windows count] == 1);

    AXUIElementRef windowRef = (AXUIElementRef) [windows objectAtIndex: 0];
    EXPECT(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    AXUIElementRef lineEditRef = [window findDirectChildByRole: kAXTextFieldRole];
    EXPECT(lineEditRef != nil);
    TestAXObject *lineEdit = [[TestAXObject alloc] initWithAXUIElementRef: lineEditRef];
    EXPECT([[lineEdit value] isEqualToString:@"button"]);

    // Access invalid reference, should return empty value
    delete le;
    QCoreApplication::processEvents();
    TestAXObject *lineEditInvalid = [[TestAXObject alloc] initWithAXUIElementRef: lineEditRef];
    EXPECT([[lineEditInvalid value] length] == 0);

    return true;
}

bool testLineEdit()
{
// not sure if this is needed. on my machine the calls succeed.
//    NSString *path = @"/Users/frederik/qt5/qtbase/tests/auto/other/qaccessibilitymac/tst_qaccessibilitymac.app/Contents/MacOS/tst_qaccessibilitymac";
//    NSString *path = @"/Users/frederik/qt5/qtbase/tests/auto/other/qaccessibilitymac/tst_qaccessibilitymac.app";
//    AXError e = AXMakeProcessTrusted((CFStringRef) path);
//    NSLog(@"error: %i", e);

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    EXPECT(appObject);

    NSArray *windowList = [appObject windowList];
    // one window
    EXPECT([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [windowList objectAtIndex: 0];
    EXPECT(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    EXPECT([window rect].size.width == 400);
    // height of window includes title bar
    EXPECT([window rect].size.height >= 400);

    EXPECT([window.title isEqualToString:@"Test window"]);

    // children of window:
    AXUIElementRef lineEdit = [window findDirectChildByRole: kAXTextFieldRole];
    EXPECT(lineEdit != nil);

    TestAXObject *le = [[TestAXObject alloc] initWithAXUIElementRef: lineEdit];
    NSString *value = @"a11y test QLineEdit";
    EXPECT([le.value isEqualToString:value]);
    EXPECT(value.length <= NSIntegerMax);
    EXPECT(le.numberOfCharacters == static_cast<NSInteger>(value.length));
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
        EXPECT([actualSubstring isEqualTo:expectedSubstring]);
        EXPECT([actualAttributedSubstring isEqualTo:expectedSubstring]);
    }
    return true;
}

bool testHierarchy(QWidget *w)
{
    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    EXPECT(appObject);

    NSArray *windowList = [appObject windowList];
    // one window
    EXPECT([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [windowList objectAtIndex: 0];
    EXPECT(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    // Because the plain widget is filtered out of the hierarchy, we expect the button
    // to be a direct child of the window
    AXUIElementRef buttonRef = [window findDirectChildByRole: kAXButtonRole];
    EXPECT(buttonRef != nil);

    TestAXObject *buttonObject = [[TestAXObject alloc] initWithAXUIElementRef: buttonRef];
    TestAXObject *parentObject = [[TestAXObject alloc] initWithAXUIElementRef: [buttonObject parent]];

    // check that the parent is a window
    EXPECT([[parentObject role] isEqualToString: NSAccessibilityWindowRole]);

    // test the focus
    // child 0 is the layout, then button1 and 2
    QPushButton *button1 = qobject_cast<QPushButton*>(w->children().at(1));
    EXPECT(button1);
    QPushButton *button2 = qobject_cast<QPushButton*>(w->children().at(2));
    EXPECT(button2);
    button2->setFocus();

    AXUIElementRef systemWideElement = AXUIElementCreateSystemWide();
    AXUIElementRef focussedElement = NULL;
    AXError error = AXUIElementCopyAttributeValue(systemWideElement,
        (CFStringRef)NSAccessibilityFocusedUIElementAttribute, (CFTypeRef*)&focussedElement);
    EXPECT(!error);
    EXPECT(focussedElement);
    TestAXObject *focusButton2 = [[TestAXObject alloc] initWithAXUIElementRef: focussedElement];

    EXPECT([[focusButton2 role] isEqualToString: NSAccessibilityButtonRole]);
    EXPECT([[focusButton2 title] isEqualToString: @"Button 2"]);


    button1->setFocus();
    error = AXUIElementCopyAttributeValue(systemWideElement,
        (CFStringRef)NSAccessibilityFocusedUIElementAttribute, (CFTypeRef*)&focussedElement);
    EXPECT(!error);
    EXPECT(focussedElement);
    TestAXObject *focusButton1 = [[TestAXObject alloc] initWithAXUIElementRef: focussedElement];
    EXPECT([[focusButton1 role] isEqualToString: NSAccessibilityButtonRole]);
    EXPECT([[focusButton1 title] isEqualToString: @"I am a button"]);

    return true;
}

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


bool notifications(QWidget *w)
{
    QLineEdit *le1 = new QLineEdit(w);
    QLineEdit *le2 = new QLineEdit(w);
    w->layout()->addWidget(le1);
    w->layout()->addWidget(le2);

    QCoreApplication::processEvents();
    QTest::qWait(100);

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    EXPECT(appObject);

    NSArray *windowList = [appObject windowList];
    // one window
    EXPECT([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [windowList objectAtIndex: 0];
    EXPECT(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    AXUIElementRef lineEdit1 = [window findDirectChildByRole: kAXTextFieldRole];
    EXPECT(lineEdit1 != nil);

    AXObserverRef observer = 0;
    AXError err = AXObserverCreate(getpid(), observerCallback, &observer);
    EXPECT(!err);
    AXObserverAddNotification(observer, appObject.ref, kAXFocusedUIElementChangedNotification, 0);
    AXObserverAddNotification(observer, lineEdit1, kAXValueChangedNotification, 0);

    CFRunLoopAddSource( [[NSRunLoop currentRunLoop] getCFRunLoop], AXObserverGetRunLoopSource(observer), kCFRunLoopDefaultMode);

    EXPECT(notificationList.length() == 0);
    le2->setFocus();
    QCoreApplication::processEvents();
    EXPECT(notificationList.length() == 1);
    EXPECT(notificationList.at(0) == QAccessible::Focus);
    le1->setFocus();
    QCoreApplication::processEvents();
    EXPECT(notificationList.length() == 2);
    EXPECT(notificationList.at(1) == QAccessible::Focus);
    le1->setText("hello");
    QCoreApplication::processEvents();
    EXPECT(notificationList.length() == 3);
    EXPECT(notificationList.at(2) == QAccessible::ValueChanged);
    le1->setText("foo");
    QCoreApplication::processEvents();
    EXPECT(notificationList.length() == 4);
    EXPECT(notificationList.at(3) == QAccessible::ValueChanged);

    return true;
}

bool testCheckBox()
{
    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    EXPECT(appObject);

    NSArray *windowList = [appObject windowList];
    // one window
    EXPECT([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [windowList objectAtIndex: 0];
    EXPECT(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    // children of window:
    AXUIElementRef checkBox = [window findDirectChildByRole: kAXCheckBoxRole];
    EXPECT(checkBox != nil);

    TestAXObject *cb = [[TestAXObject alloc] initWithAXUIElementRef: checkBox];

    // here start actual checkbox tests
    EXPECT([cb valueNumber] == 0);
    EXPECT([cb.title isEqualToString:@"Great option"]);
    // EXPECT(cb.description == nil); // currently returns "" instead of nil

    EXPECT([cb.actions containsObject:(NSString*)kAXPressAction]);

    [cb performAction:kAXPressAction];
    EXPECT([cb valueNumber] == 1);

    [cb performAction:kAXPressAction];
    EXPECT([cb valueNumber] == 0);

    return true;
}
