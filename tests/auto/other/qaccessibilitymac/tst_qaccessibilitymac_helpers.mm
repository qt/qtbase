/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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


#define EXPECT(cond) \
    if (!(cond)) { \
        qWarning("Failure in %s, line: %d", __FILE__ , __LINE__); \
        return false; \
    } \


@interface TestAXObject : NSObject
{
    AXUIElementRef reference;
    NSString *_role;
    NSString *_description;
    NSString *_value;
    CGRect _rect;
}
    @property (readonly) NSString *role;
    @property (readonly) NSString *description;
    @property (readonly) NSString *value;
    @property (readonly) CGRect rect;
@end

@implementation TestAXObject

    @synthesize role = _role;
    @synthesize description = _description;
    @synthesize value = _value;
    @synthesize rect = _rect;

- (id) initWithAXUIElementRef: (AXUIElementRef) ref {

    if ( self = [super init] ) {
        reference = ref;
        AXUIElementCopyAttributeValue(ref, kAXRoleAttribute, (CFTypeRef*)&_role);
        AXUIElementCopyAttributeValue(ref, kAXDescriptionAttribute, (CFTypeRef*)&_description);
        AXUIElementCopyAttributeValue(ref, kAXValueAttribute, (CFTypeRef*)&_value);
        AXValueRef sizeValue;
        AXUIElementCopyAttributeValue(ref, kAXSizeAttribute, (CFTypeRef*)&sizeValue);
        AXValueGetValue(sizeValue, kAXValueCGSizeType, &_rect.size);
        AXValueRef positionValue;
        AXUIElementCopyAttributeValue(ref, kAXPositionAttribute, (CFTypeRef*)&positionValue);
        AXValueGetValue(positionValue, kAXValueCGPointType, &_rect.origin);
    }
    return self;
}

- (AXUIElementRef) ref { return reference; }
- (void) print {
    NSLog(@"Accessible Object role: '%@', description: '%@', value: '%@', rect: '%@'", self.role, self.description, self.value, NSStringFromRect(self.rect));
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
    AXUIElementRef result = nil;
    NSArray *childList = [self childList];
    for (id child in childList) {
        CFStringRef typeString;
        AXUIElementCopyAttributeValue((AXUIElementRef)child, kAXRoleAttribute, (CFTypeRef*)&typeString);
        if (CFStringCompare(typeString, role, 0) == 0) {
            result = (AXUIElementRef) child;
            break;
        }
    }
    return result;
}

- (AXUIElementRef) parent
{
    AXUIElementRef p = nil;
    AXUIElementCopyAttributeValue(reference, kAXParentAttribute, (CFTypeRef*)&p);
    return p;
}

+ (TestAXObject *) getApplicationAXObject
{
    pid_t pid = getpid();
    AXUIElementRef appRef = AXUIElementCreateApplication(pid);
    TestAXObject *appObject = [[TestAXObject alloc] initWithAXUIElementRef: appRef];
    return appObject;
}

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

    NSString *windowTitle;
    AXUIElementCopyAttributeValue(windowRef, kAXTitleAttribute, (CFTypeRef*)&windowTitle);
    EXPECT([windowTitle isEqualToString:@"Test window"]);

    // children of window:
    AXUIElementRef lineEdit = [window findDirectChildByRole: kAXTextFieldRole];
    EXPECT(lineEdit != nil);

    TestAXObject *le = [[TestAXObject alloc] initWithAXUIElementRef: lineEdit];
    EXPECT([[le value] isEqualToString:@"a11y test QLineEdit"]);
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
    EXPECT([[focusButton2 description] isEqualToString: @"Button 2"]);


    button1->setFocus();
    error = AXUIElementCopyAttributeValue(systemWideElement,
        (CFStringRef)NSAccessibilityFocusedUIElementAttribute, (CFTypeRef*)&focussedElement);
    EXPECT(!error);
    EXPECT(focussedElement);
    TestAXObject *focusButton1 = [[TestAXObject alloc] initWithAXUIElementRef: focussedElement];
    EXPECT([[focusButton1 role] isEqualToString: NSAccessibilityButtonRole]);
    EXPECT([[focusButton1 description] isEqualToString: @"I am a button"]);

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
