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

// some versions of CALayer.h use 'slots' as an identifier
#define QT_NO_KEYWORDS

#include "tst_qaccessibilitymac_helpers.h"
#include <QApplication>
#include <QDebug>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

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
}
    @property (readonly) NSString *role;
    @property (readonly) NSString *description;
    @property (readonly) NSString *value;
    @property (readonly) CGRect rect;
@end

@implementation TestAXObject
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
    NSLog(@"    Children: %ld", [self.childList count]);
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

@end


bool testLineEdit()
{
// not sure if this is needed. on my machine the calls succeed.
//    NSString *path = @"/Users/frederik/qt5/qtbase/tests/auto/other/qaccessibilitymac/tst_qaccessibilitymac.app/Contents/MacOS/tst_qaccessibilitymac";
//    NSString *path = @"/Users/frederik/qt5/qtbase/tests/auto/other/qaccessibilitymac/tst_qaccessibilitymac.app";
//    AXError e = AXMakeProcessTrusted((CFStringRef) path);
//    NSLog(@"error: %i", e);

    pid_t pid = getpid();
    AXUIElementRef app = AXUIElementCreateApplication(pid);
    EXPECT(app != nil);
    TestAXObject *appObject = [[TestAXObject alloc] initWithAXUIElementRef: app];

    NSArray *windowList = [appObject windowList];
    // one window
    EXPECT([windowList count] == 1);
    AXUIElementRef windowRef = (AXUIElementRef) [windowList objectAtIndex: 0];
    EXPECT(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef: windowRef];

    EXPECT([window rect].size.width == 400);
    // height of window includes title bar
    EXPECT([window rect].size.height >= 400);

    // children of window:
    AXUIElementRef lineEdit = [window findDirectChildByRole: kAXTextFieldRole];
    EXPECT(lineEdit != nil);

    TestAXObject *le = [[TestAXObject alloc] initWithAXUIElementRef: lineEdit];
    EXPECT([[le value] isEqualToString:@"a11y test QLineEdit"]);
    return true;
}

bool testHierarchy()
{
    pid_t pid = getpid();
    AXUIElementRef app = AXUIElementCreateApplication(pid);
    EXPECT(app != nil);
    TestAXObject *appObject = [[TestAXObject alloc] initWithAXUIElementRef: app];

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
    EXPECT([[parentObject role] isEqualToString: (NSString *)kAXWindowRole]);

    return true;
}
