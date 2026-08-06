// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QtWidgets>
#include <QTest>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qregularexpression.h>

// some versions of CALayer.h use 'slots' as an identifier
#define QT_NO_KEYWORDS

#include <QtGui/qwindow.h>
#include <QtGui/private/qaccessiblewindow_p.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgroupbox.h>
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
    @property (readonly) NSString *roleDescription;
    @property (readonly) NSString *title;
    @property (readonly) NSString *description;
    @property (readonly) NSString *value;
    @property (readonly) CGRect rect;
    @property (readonly) NSArray *actions;
    @property (readonly) bool valid;
@end

@implementation TestAXObject

- (instancetype)initWithAXUIElementRef:(AXUIElementRef)ref {

    if ((self = [super init])) {
        reference = ref;
        if (!reference) {
            qCritical() << "Null reference!";
            axError = true;
        } else {
            axError = false;
        }
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
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyAttributeValues(reference, kAXChildrenAttribute,
                                                                 0, 100, /*min, max*/
                                                                 (CFArrayRef *) &list))) {
        axError = true;
        qDebug() << "AXUIElementCopyAttributeValue(kAXChildrenAttribute) returned error = "
                 << AXErrorTag(err) << "with reference" << reference;
    }
    return list;
}

- (NSArray *)tableRows
{
    NSArray *arr;
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyAttributeValues(reference, kAXRowsAttribute,
                                                                    0, 100, /*min, max*/
                                                                    (CFArrayRef *) &arr))) {
        axError = true;
        qDebug() << "AXUIElementCopyAttributeValue(kAXRowsAttribute) returned error = "
                 << AXErrorTag(err) << "with reference" << reference;
    }
    return arr;
}

- (NSArray *)tableColumns
{
    NSArray *arr;
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyAttributeValues(reference, kAXColumnsAttribute,
                                                                    0, 100, /*min, max*/
                                                                    (CFArrayRef *) &arr))) {
        axError = true;
        qDebug() << "AXUIElementCopyAttributeValue(kAXColumnsAttribute) returned error = "
                 << AXErrorTag(err) << "with reference" << reference;
    }
    return arr;
}

- (NSArray *)tabs
{
    NSArray *arr;
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyAttributeValues(reference, kAXTabsAttribute,
                                                                    0, 100, /*min, max*/
                                                                    (CFArrayRef *) &arr))) {
        axError = true;
        qDebug() << "AXUIElementCopyAttributeValue(kAXTabsAttribute) returned error = "
                 << AXErrorTag(err) << "with reference" << reference;
    }
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

// Quiet attribute access, for walking a tree where not every element answers
// for every attribute, and where an unanswered attribute is not a failure.
- (CFTypeRef)_optionalAttributeValue:(CFStringRef)attribute
{
    CFTypeRef value = nullptr;
    if (AXUIElementCopyAttributeValue(reference, attribute, &value) != kAXErrorSuccess)
        return nullptr;
    return value;
}

- (NSArray *)optionalChildList
{
    NSArray *list = nil;
    if (AXUIElementCopyAttributeValues(reference, kAXChildrenAttribute,
                                       0, 100, /*min, max*/
                                       (CFArrayRef *)&list) != kAXErrorSuccess) {
        return @[];
    }
    return list;
}

- (void)collectDescendantsWithRole:(CFStringRef)role title:(NSString *)title
                             depth:(int)depth into:(NSMutableArray *)result
{
    // A bridge that redirects into an element that already contains it leaves
    // the system walking in circles, so bound the walk instead of hanging.
    if (depth <= 0)
        return;

    for (id child in [self optionalChildList]) {
        TestAXObject *childObject = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)child];
        NSString *childRole = (NSString *)[childObject _optionalAttributeValue:kAXRoleAttribute];
        NSString *childTitle = (NSString *)[childObject _optionalAttributeValue:kAXTitleAttribute];
        if ([childRole isEqualToString:(NSString *)role] && [childTitle isEqualToString:title])
            [result addObject:child];
        [childObject collectDescendantsWithRole:role title:title depth:depth - 1 into:result];
        [childObject release];
    }
}

// Walks the entire subtree, as content behind a native view boundary is vended
// by that view, and may end up deeper than a direct child.
- (NSArray *)descendantsWithRole:(CFStringRef)role title:(NSString *)title
{
    NSMutableArray *result = [NSMutableArray array];
    [self collectDescendantsWithRole:role title:title depth:32 into:result];
    return result;
}

- (AXUIElementRef)elementAtPosition:(CGPoint)point
{
    AXUIElementRef element = nil;
    AXError err;

    if (kAXErrorSuccess != (err = AXUIElementCopyElementAtPosition(reference, point.x, point.y,
                                                                  &element))) {
        axError = true;
        qDebug() << "AXUIElementCopyElementAtPosition(" << point.x << "," << point.y
                 << ") returned error = " << AXErrorTag(err);
    }
    return element;
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

- (bool)                valid { return reference != nil; }
- (NSString*)           role { return [self _stringAttributeValue:kAXRoleAttribute]; }
- (NSString*)           roleDescription { return [self _stringAttributeValue:kAXRoleDescriptionAttribute]; }
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
- (AXUIElementRef)      optionalParent { return (AXUIElementRef)[self _optionalAttributeValue:kAXParentAttribute]; }
- (AXUIElementRef)      window { return (AXUIElementRef)[self _attributeValue:kAXWindowAttribute]; }
- (AXUIElementRef)      focusedUIElement { return (AXUIElementRef)[self _attributeValue:kAXFocusedUIElementAttribute]; }
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

// Creates a child widget that lives in a native view of its own, so that the
// walk from the window's accessible root has a native view boundary to stop at.
// The caller adds it to the window once its content is in place.
static QWidget *createNativeChild(QWidget *parent)
{
    QWidget *nativeChild = new QWidget(parent);
    nativeChild->setAttribute(Qt::WA_DontCreateNativeAncestors);
    nativeChild->setAttribute(Qt::WA_NativeWindow);
    nativeChild->setLayout(new QVBoxLayout);
    return nativeChild;
}

// Adds a native child widget holding a button, and returns the button.
static QPushButton *addNativeChildWithButton(AccessibleTestWindow *window, const QString &text)
{
    QWidget *nativeChild = createNativeChild(window);
    QPushButton *button = new QPushButton(text, nativeChild);
    nativeChild->layout()->addWidget(button);
    window->addWidget(nativeChild);
    return button;
}

// Hosts a native NSButton in a foreign window, in a window container, so that
// the window has content in front of it that Qt's interface tree cannot see.
// Returns the container, and reports the foreign window through foreignWindow.
static QWidget *addForeignNativeButton(AccessibleTestWindow *window, NSString *title,
                                       QWindow **foreignWindow)
{
    NSButton *nativeButton = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 200, 32)];
    nativeButton.title = title;

    *foreignWindow = QWindow::fromWinId(reinterpret_cast<WId>(nativeButton));
    if (!*foreignWindow)
        return nullptr;

    QWidget *container = QWidget::createWindowContainer(*foreignWindow, window);
    window->addWidget(container);
    return container;
}

// Presents the embedded window as a single button, the way the foreign window
// presents the native button it wraps, so that both cases can be found and
// checked the same way. Everything else, the parent above all, comes from
// QAccessibleWindow, which is what a real window subclass inherits too.
class EmbeddedWindowInterface : public QAccessibleWindow
{
public:
    using QAccessibleWindow::QAccessibleWindow;

    QAccessible::Role role() const override { return QAccessible::Button; }

    QString text(QAccessible::Text text) const override
    {
        return text == QAccessible::Name ? window()->title() : QString();
    }
};

// A window class of its own, so that the factory below can answer for it. A
// plain QWindow has no accessible root, and so no element to check, and no way
// to report the window container hosting it as its parent.
class EmbeddedWindow : public QWindow
{
    Q_OBJECT
public:
    QAccessibleInterface *accessibleRoot() const override
    {
        return QAccessible::queryAccessibleInterface(const_cast<EmbeddedWindow *>(this));
    }
};

// The content of a WindowRoleWindow, so that the element under test has the
// window's own node as its parent, rather than being that node.
class EmbeddedContent : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
};

class EmbeddedContentInterface : public QAccessibleObject
{
public:
    using QAccessibleObject::QAccessibleObject;

    QWindow *window() const override
    {
        return static_cast<QWindow *>(object()->parent());
    }

    QAccessibleInterface *parent() const override
    {
        return QAccessible::queryAccessibleInterface(object()->parent());
    }

    QAccessibleInterface *child(int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return -1; }

    QRect rect() const override
    {
        return QRect(window()->mapToGlobal(QPoint(0, 0)), window()->size());
    }

    QAccessible::Role role() const override { return QAccessible::Button; }

    QAccessible::State state() const override
    {
        QAccessible::State state;
        state.invisible = !window()->isVisible();
        return state;
    }

    QString text(QAccessible::Text text) const override
    {
        return text == QAccessible::Name ? window()->title() : QString();
    }
};

// Reports the Window role for a window that is hosted, and so is not a window
// on screen at all. Interfaces in the wild do this, so the bridge has to keep
// placing the content correctly when they do.
class WindowRoleWindowInterface : public QAccessibleWindow
{
public:
    using QAccessibleWindow::QAccessibleWindow;

    QAccessible::Role role() const override { return QAccessible::Window; }

    int childCount() const override { return 1; }

    QAccessibleInterface *child(int index) const override
    {
        return index == 0 ? QAccessible::queryAccessibleInterface(content()) : nullptr;
    }

    int indexOfChild(const QAccessibleInterface *child) const override
    {
        return child && child->object() == content() ? 0 : -1;
    }

private:
    QObject *content() const { return window()->findChild<EmbeddedContent *>(); }
};

class WindowRoleWindow : public QWindow
{
    Q_OBJECT
public:
    WindowRoleWindow() { new EmbeddedContent(this); }

    QAccessibleInterface *accessibleRoot() const override
    {
        return QAccessible::queryAccessibleInterface(const_cast<WindowRoleWindow *>(this));
    }
};

static QAccessibleInterface *embeddedWindowFactory(const QString &classname, QObject *object)
{
    if (classname == QStringLiteral("EmbeddedWindow"))
        return new EmbeddedWindowInterface(static_cast<QWindow *>(object));
    if (classname == QStringLiteral("WindowRoleWindow"))
        return new WindowRoleWindowInterface(static_cast<QWindow *>(object));
    if (classname == QStringLiteral("EmbeddedContent"))
        return new EmbeddedContentInterface(object);
    return nullptr;
}

// Creates a window to embed in a window container, presenting a single
// accessible button with the given title, whether the window is one of ours
// or a foreign window wrapping a native button.
static QWindow *createEmbeddedWindow(bool foreign, const QString &title)
{
    if (!foreign) {
        auto *window = new EmbeddedWindow;
        window->setTitle(title);
        window->resize(200, 32);
        return window;
    }

    NSButton *nativeButton = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 200, 32)];
    nativeButton.title = title.toNSString();
    return QWindow::fromWinId(reinterpret_cast<WId>(nativeButton));
}

// Adds a group box, to give a window container an unignored ancestor to resolve
// to, as the container itself is not part of the user-visible tree.
static QGroupBox *addGroup(QWidget *parent, const QString &title)
{
    QGroupBox *group = new QGroupBox(title, parent);
    group->setLayout(new QVBoxLayout);
    parent->layout()->addWidget(group);
    group->show();
    return group;
}

static QWidget *addContainer(QWidget *parent, QWindow *embedded)
{
    QWidget *container = QWidget::createWindowContainer(embedded, parent);
    parent->layout()->addWidget(container);
    container->show();
    return container;
}

// Returns the elements a foreign view vends to its parent, which is the view
// itself when the view is unignored, and its accessible children when it is
// not. An NSButton, for one, vends its cell. This is the set that carries the
// parent Qt pushes, so it is where a pushed parent can be read back.
static NSArray *vendedElements(QWindow *foreignWindow)
{
    NSView *view = reinterpret_cast<NSView *>(foreignWindow->winId());
    return NSAccessibilityUnignoredChildrenForOnlyChild(view);
}

static QList<id> accessibilityParents(NSArray *elements)
{
    QList<id> parents;
    for (id<NSAccessibility> element in elements)
        parents.append(element.accessibilityParent);
    return parents;
}

// Drops the parent Qt pushed onto a foreign view, so that the only thing that
// can put a parent back is an update from Qt. Reports whether the view now
// answers with no parent, so that a test can tell a parent that was restored
// from one that was never dropped.
static bool clearPushedParents(QWindow *foreignWindow)
{
    NSArray *elements = vendedElements(foreignWindow);
    for (id<NSAccessibility> element in elements)
        element.accessibilityParent = nil;

    for (id parent : accessibilityParents(elements)) {
        if (parent)
            return false;
    }
    return true;
}

// Returns the object for the test window, once the accessibility layer knows
// about it. The test window is the application's only window.
static TestAXObject *testWindowObject()
{
    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    NSArray *windowList = nil;
    if (!QTest::qWaitFor([&]{ windowList = appObject.windowList; return [windowList count] == 1; }))
        return nil;
    return [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)windowList[0]];
}

// Walks the subtree below the given element and discards what it finds. The
// parent Qt pushes onto a foreign view is resolved as the view is handed out,
// so a client that holds an element from before a change in the hierarchy sees
// the parent it was given then, until a walk hands the view out again.
static void walkHierarchy(TestAXObject *root)
{
    [root descendantsWithRole:kAXUnknownRole title:nil];
}

// Returns the one element in the subtree with the given role and title, or nil
// if there is no such element, or more than one.
static AXUIElementRef elementWithRoleAndTitle(TestAXObject *root, CFStringRef role, NSString *title)
{
    NSArray *elements = [root descendantsWithRole:role title:title];
    return [elements count] == 1 ? (AXUIElementRef)elements[0] : nullptr;
}

// Wraps an element so that QCOMPARE compares the elements referred to, rather
// than the references, and reports what they were when they differ.
struct AXElement
{
    AXElement(AXUIElementRef element = nullptr) : element(element) {}
    AXUIElementRef element;
};

static bool operator==(const AXElement &lhs, const AXElement &rhs)
{
    if (lhs.element == rhs.element)
        return true;
    return lhs.element && rhs.element && CFEqual(lhs.element, rhs.element);
}

// An element reference says nothing about what it refers to, so describe the
// element by what a reader of a test failure needs to tell it from another.
static QByteArray describeElement(AXUIElementRef element)
{
    if (!element)
        return "<none>";

    TestAXObject *object = [[TestAXObject alloc] initWithAXUIElementRef:element];
    NSString *role = (NSString *)[object _optionalAttributeValue:kAXRoleAttribute];
    NSString *title = (NSString *)[object _optionalAttributeValue:kAXTitleAttribute];
    [object release];

    QByteArray description = role ? QString::fromNSString(role).toUtf8() : QByteArray("<no role>");
    if (title)
        description += " '" + QString::fromNSString(title).toUtf8() + '\'';
    return description;
}

static QByteArray describeElements(NSArray *elements)
{
    QByteArray description = "[";
    for (id element in elements) {
        if (description.size() > 1)
            description += ", ";
        description += describeElement((AXUIElementRef)element);
    }
    return description + ']';
}

char *toString(const AXElement &element)
{
    return QTest::toString(describeElement(element.element));
}

static bool containsElement(NSArray *elements, AXUIElementRef element)
{
    for (id candidate in elements) {
        if (AXElement((AXUIElementRef)candidate) == AXElement(element))
            return true;
    }
    return false;
}

// The two ways a window container gets its content: a window of ours, whose
// view resolves the parent when asked, and a foreign window, whose view has
// the parent pushed onto it.
static void addWindowTypeRows()
{
    QTest::addColumn<bool>("foreign");
    QTest::newRow("qt window") << false;
    QTest::newRow("foreign window") << true;
}

class tst_QAccessibilityMac : public QObject
{
Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void singleWidgetTest();
    void lineEditTest();
    void hierarchyTest();
    void notificationsTest();
    void checkBoxTest();
    void tableViewTest();
    void treeViewTest();
    void tabBarTest();
    void windowTest();

    void nativeChildWidget();
    void nativeChildWidgetViewParent();
    void hitTest();
    void focusAcrossNativeBoundary();
    void foreignWindow();
    void foreignWindowHitTest();
    void foreignWindowMultipleChildren();

    void windowContainerParent_data();
    void windowContainerParent();
    void windowContainerParentThroughWindowRole();
    void windowContainerParentWithoutGroup_data();
    void windowContainerParentWithoutGroup();
    void windowContainerParentWhileInactive_data();
    void windowContainerParentWhileInactive();
    void windowContainerReparent_data();
    void windowContainerReparent();
    void windowContainerIndirectReparent_data();
    void windowContainerIndirectReparent();
    void windowContainerBelowNativeWidget_data();
    void windowContainerBelowNativeWidget();
    void windowContainerBelowNestedNativeWidgets_data();
    void windowContainerBelowNestedNativeWidgets();
    void windowContainerRemoved_data();
    void windowContainerRemoved();
    void foreignWindowReleasedFromContainer();
    void foreignWindowParentWhileInactive_data();
    void foreignWindowParentWhileInactive();
    void windowContainerSiblingOrder_data();
    void windowContainerSiblingOrder();
    void windowContainerHidden_data();
    void windowContainerHidden();
    void windowContainerUpdatedWhileHidden_data();
    void windowContainerUpdatedWhileHidden();
    void windowContainerParentsRestoredByWalk();

private:
    AccessibleTestWindow *m_window;
};


void tst_QAccessibilityMac::initTestCase()
{
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QAccessible::installFactory(embeddedWindowFactory);

    // Queries from other accessibility clients, such as VoiceOver or Karabiner
    // Elements, will affect our tests, making them flakey, so bail out early.
    AccessibleTestWindow probe;
    probe.setWindowTitle("Accessibility client probe");
    probe.resize(400, 400);
    probe.show();
    QVERIFY(QTest::qWaitForWindowExposed(&probe));
    QTest::qWait(500);

    if (QAccessible::isActive())
        QSKIP("An accessibility client is already querying this process");
}

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
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("kAXErrorInvalidUIElement"));
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
    QVERIFY(window.valid);

    // children of window

    auto accessibleTreeView = [window]() -> TestAXObject * {
        AXUIElementRef treeView = [window findDirectChildByRole:kAXOutlineRole];
        Q_ASSERT(treeView != nil);
        TestAXObject *tv = [[TestAXObject alloc] initWithAXUIElementRef:treeView];
        return tv;
    };

    TestAXObject *tv = accessibleTreeView();
    QVERIFY(tv.valid);

    // here start actual treeview tests. NSAccessibilityOutline is a specialization
    // of NSAccessibilityTable, and we represent trees as tables.
    const auto cellText = [&tv](int rowIndex, int columnIndex) -> QString {
        NSArray *rowArray = [tv tableRows];
        Q_ASSERT(rowArray.count > uint(rowIndex));
        TestAXObject *row = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)rowArray[rowIndex]];
        Q_ASSERT(row && row.valid);
        TestAXObject *cell = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)row.childList[columnIndex]];
        Q_ASSERT(cell && cell.valid);
        QString result = QString::fromNSString(cell.title);
        [rowArray release];
        return result;
    };

    // Should have 2 columns
    const unsigned int columnCount = 2;
    {
        NSArray *columnArray = tv.tableColumns;
        QCOMPARE(columnArray.count, columnCount);
        [columnArray release];
    }

    // Should have 1 row for now - as long as the root item is not expanded.
    // That row should have 2 columns.
    {
        NSArray *rowArray = tv.tableRows;
        QCOMPARE(rowArray.count, 1u);
        {
            TestAXObject *row = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)rowArray[0]];
            QCOMPARE(row.childList.count, columnCount);
            [row release];
        }
        [rowArray release];
    }
    QCOMPARE(cellText(0, 0), root->text(0));

    root->setExpanded(true);
    // Now that the root node is expanded we should now see all rows.
    // Each row should have the same number of columns as the tree view.
    {
        NSArray *rowArray = tv.tableRows;
        QCOMPARE(rowArray.count, root->childCount() + 1u);

        for (ulong r = 0; r < rowArray.count; ++r) {
            QVERIFY(rowArray[r] != nil);
            TestAXObject *row = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)rowArray[r]];
            const uint childCount = row.childList.count;
            QCOMPARE(childCount, columnCount);
            [row release];
        }
        [rowArray release];
    }

    // this should not trigger any assert
    tw->setCurrentItem(lastChild);

    // cache will have been cleared, reinitialize
    [tv release];
    tv = accessibleTreeView();
    QVERIFY(tv.valid);

    QCOMPARE(cellText(0, 0), root->text(0));
    QCOMPARE(cellText(1, 0), users->text(0));
    QCOMPARE(cellText(1, 1), users->text(1));

    [appObject release];
    [window release];
}

void tst_QAccessibilityMac::tabBarTest()
{
    QTabBar *tbar = new QTabBar;
    static const unsigned int nTabs = 20;
    for (unsigned int i = 0; i < nTabs; ++i)
        tbar->addTab(QString::number(i));
    tbar->setUsesScrollButtons(true);

    m_window->addWidget(tbar);
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
    QVERIFY(window.valid);

    // children of window
    AXUIElementRef axTarBar = [window findDirectChildByRole:kAXTabGroupRole];
    QVERIFY(axTarBar != nil);

    TestAXObject *tb = [[TestAXObject alloc] initWithAXUIElementRef:axTarBar];
    QVERIFY(tb.valid);

    [appObject release];
    [window release];

    NSArray *tbChildList = [tb childList];
    // +2 because of the scroll buttons
    QCOMPARE([tbChildList count], nTabs + 2);

    NSArray *tbTabsList = [tb tabs];
    QCOMPARE([tbTabsList count], nTabs);

    for (unsigned int i = 0; i < nTabs; ++i) {
        AXUIElementRef axTab = (AXUIElementRef)[tbTabsList objectAtIndex:i];
        QVERIFY(axTab != nil);

        TestAXObject *tab = [[TestAXObject alloc] initWithAXUIElementRef:axTab];
        QVERIFY(tab.valid);
        QCOMPARE(QString::fromNSString(tab.role), QString::fromCFString(kAXRadioButtonRole));
        QCOMPARE(QString::fromNSString(tab.title), QString::number(i));
        QCOMPARE(QString::fromNSString(tab.roleDescription), "tab");
    }
}

void tst_QAccessibilityMac::windowTest()
{
    QTextEdit *textEdit = new QTextEdit;
    m_window->addWidget(textEdit);
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
    QVERIFY(window.valid);

    AXUIElementRef axTextEdit = [window findDirectChildByRole:kAXTextAreaRole];
    QVERIFY(axTextEdit != nil);

    [appObject release];
    [window release];

    TestAXObject *edit = [[TestAXObject alloc] initWithAXUIElementRef:axTextEdit];
    QVERIFY(edit.valid);
    AXUIElementRef axWindow = edit.window;
    QVERIFY(axWindow);
}

void tst_QAccessibilityMac::nativeChildWidget()
{
    QPushButton *button = addNativeChildWithButton(m_window, "Button in native child");
    QVERIFY(button->nativeParentWidget()->windowHandle() != m_window->windowHandle());
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    QTRY_VERIFY(appObject.windowList.count == 1);
    AXUIElementRef windowRef = (AXUIElementRef)[appObject.windowList objectAtIndex:0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef:windowRef];
    QVERIFY(window.valid);

    // The child widget lives in a native view of its own, so the view of the
    // child window vends the button now, rather than the view of the top level.
    // The button must still appear, exactly once, and in the same place.
    NSArray *buttons = [window descendantsWithRole:kAXButtonRole
                                             title:button->text().toNSString()];
    QCOMPARE([buttons count], 1u);

    TestAXObject *buttonObject = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)buttons[0]];
    QVERIFY(buttonObject.valid);

    // Both views are containers the user does not interact with, and are
    // ignored, so the window is still the button's parent
    TestAXObject *parentObject = [[TestAXObject alloc] initWithAXUIElementRef:[buttonObject parent]];
    QCOMPARE(QString::fromNSString(parentObject.role), QString::fromNSString(NSAccessibilityWindowRole));

    TestAXObject *windowObject = [[TestAXObject alloc] initWithAXUIElementRef:[buttonObject window]];
    QCOMPARE(QString::fromNSString(windowObject.role), QString::fromNSString(NSAccessibilityWindowRole));

    [appObject release];
    [window release];
}

// The view of a native child widget is ignored, so it never shows up as an
// element, and the elements inside it resolve their own parents through Qt's
// interface tree without ever consulting the view. AppKit still asks the view
// itself whenever it walks up out of one, and there is no way to provoke that
// through an accessibility client, so ask the view directly.
void tst_QAccessibilityMac::nativeChildWidgetViewParent()
{
    QGroupBox *group = addGroup(m_window, "Group");
    QWidget *nativeChild = createNativeChild(group);
    QPushButton *button = new QPushButton("Button in native child", nativeChild);
    nativeChild->layout()->addWidget(button);
    group->layout()->addWidget(nativeChild);
    nativeChild->show();

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QTRY_VERIFY(nativeChild->windowHandle());
    QCoreApplication::processEvents();

    // The premise: the child lives in a native view of its own, below the group box
    QVERIFY(nativeChild->windowHandle() != m_window->windowHandle());

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    NSView *view = reinterpret_cast<NSView *>(nativeChild->windowHandle()->winId());
    id<NSAccessibility> parent = [view accessibilityParent];
    QVERIFY(parent);

    // The group box is what vends the view to accessibility clients, so it is
    // what the view has to name in return. Left to AppKit the view would report
    // the enclosing NSWindow, the nearest ancestor of the view that AppKit
    // itself puts in the tree.
    QCOMPARE(QString::fromNSString(parent.accessibilityRole),
             QString::fromNSString(NSAccessibilityGroupRole));
    QCOMPARE(QString::fromNSString(parent.accessibilityTitle), group->title());

    [window release];
}

void tst_QAccessibilityMac::hitTest()
{
    QPushButton *button = addNativeChildWithButton(m_window, "Hit me");
    QVERIFY(button->nativeParentWidget()->windowHandle() != m_window->windowHandle());
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    QTRY_VERIFY(appObject.windowList.count == 1);

    // A point over the button hits the button, even though the button is
    // vended by the view of the child window, and not by the one we start in
    const QPoint buttonCenter = button->mapToGlobal(button->rect().center());
    AXUIElementRef hitRef = [appObject elementAtPosition:CGPointMake(buttonCenter.x(),
                                                                    buttonCenter.y())];
    QVERIFY(hitRef != nil);
    TestAXObject *hitObject = [[TestAXObject alloc] initWithAXUIElementRef:hitRef];
    QCOMPARE(QString::fromNSString(hitObject.role), QString::fromNSString(NSAccessibilityButtonRole));
    QCOMPARE(QString::fromNSString(hitObject.title), button->text());

    // A point in the layout margin hits no child at all, and lands on the
    // window, rather than on whatever child was tried last. Check the title
    // too, so that landing on a window of an earlier test does not pass.
    const QPoint margin = m_window->mapToGlobal(QPoint(1, 1));
    AXUIElementRef missRef = [appObject elementAtPosition:CGPointMake(margin.x(), margin.y())];
    QVERIFY(missRef != nil);
    TestAXObject *missObject = [[TestAXObject alloc] initWithAXUIElementRef:missRef];
    QCOMPARE(QString::fromNSString(missObject.role), QString::fromNSString(NSAccessibilityWindowRole));
    QCOMPARE(QString::fromNSString(missObject.title), m_window->windowTitle());

    [appObject release];
    [hitObject release];
    [missObject release];
}

void tst_QAccessibilityMac::focusAcrossNativeBoundary()
{
    QPushButton *inner = addNativeChildWithButton(m_window, "Inner button");
    QVERIFY(inner->nativeParentWidget()->windowHandle() != m_window->windowHandle());

    // The outer button stays in the top level's own window, so the focus has
    // to cross the boundary in both directions
    QPushButton *outer = new QPushButton("Outer button", m_window);
    m_window->addWidget(outer);
    QCOMPARE(outer->nativeParentWidget()->windowHandle(), m_window->windowHandle());

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    m_window->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(m_window));
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    // The focus moving into the child window is the ordinary way for it to
    // leave ours, and the view of that window answers for its own content
    inner->setFocus();
    QTRY_VERIFY(inner->hasFocus());
    AXUIElementRef innerRef = [appObject focusedUIElement];
    QVERIFY(innerRef != nil);
    TestAXObject *focusInner = [[TestAXObject alloc] initWithAXUIElementRef:innerRef];
    QCOMPARE(QString::fromNSString(focusInner.role), QString::fromNSString(NSAccessibilityButtonRole));
    QCOMPARE(QString::fromNSString(focusInner.title), inner->text());

    // And back out again, so that the hand off is not one way
    outer->setFocus();
    QTRY_VERIFY(outer->hasFocus());
    AXUIElementRef outerRef = [appObject focusedUIElement];
    QVERIFY(outerRef != nil);
    TestAXObject *focusOuter = [[TestAXObject alloc] initWithAXUIElementRef:outerRef];
    QCOMPARE(QString::fromNSString(focusOuter.role), QString::fromNSString(NSAccessibilityButtonRole));
    QCOMPARE(QString::fromNSString(focusOuter.title), outer->text());

    [appObject release];
    [focusInner release];
    [focusOuter release];
}

void tst_QAccessibilityMac::foreignWindow()
{
    QWindow *foreignWindow = nullptr;
    QVERIFY(addForeignNativeButton(m_window, @"Native button", &foreignWindow));
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QTRY_VERIFY(foreignWindow->isVisible());
    QVERIFY(foreignWindow->handle());
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    QTRY_VERIFY(appObject.windowList.count == 1);
    AXUIElementRef windowRef = (AXUIElementRef)[appObject.windowList objectAtIndex:0];
    QVERIFY(windowRef != nil);
    TestAXObject *window = [[TestAXObject alloc] initWithAXUIElementRef:windowRef];
    QVERIFY(window.valid);

    // The foreign window's native subtree is content that Qt's interface tree
    // cannot see, so it can only appear if the walk stops at the foreign view
    // and leaves the system to walk into it
    NSArray *buttons = [window descendantsWithRole:kAXButtonRole title:@"Native button"];
    QCOMPARE([buttons count], 1u);

    [appObject release];
    [window release];
}

void tst_QAccessibilityMac::foreignWindowHitTest()
{
    QWindow *foreignWindow = nullptr;
    QWidget *container = addForeignNativeButton(m_window, @"Native button", &foreignWindow);
    QVERIFY(container);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QTRY_VERIFY(foreignWindow->isVisible());
    QVERIFY(foreignWindow->handle());
    QCoreApplication::processEvents();

    TestAXObject *appObject = [TestAXObject getApplicationAXObject];
    QVERIFY(appObject);

    QTRY_VERIFY(appObject.windowList.count == 1);

    const QPoint center = container->mapToGlobal(container->rect().center());
    AXUIElementRef hitRef = [appObject elementAtPosition:CGPointMake(center.x(), center.y())];
    QVERIFY(hitRef != nil);
    TestAXObject *hitObject = [[TestAXObject alloc] initWithAXUIElementRef:hitRef];
    QCOMPARE(QString::fromNSString(hitObject.role), QString::fromNSString(NSAccessibilityButtonRole));
    QCOMPARE(QString::fromNSString(hitObject.title), QString("Native button"));

    [appObject release];
    [hitObject release];
}

void tst_QAccessibilityMac::foreignWindowMultipleChildren()
{
    // A view whose own accessibility is more than one element, so that the
    // parent has to be reflected onto each of them, not just the first
    NSView *nativeView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 200, 64)];
    NSArray *titles = @[@"First native button", @"Second native button"];
    for (NSUInteger i = 0; i < [titles count]; ++i) {
        NSButton *nativeButton = [[NSButton alloc] initWithFrame:NSMakeRect(0, i * 32, 200, 32)];
        nativeButton.title = titles[i];
        [nativeView addSubview:nativeButton];
    }

    QWindow *embedded = QWindow::fromWinId(reinterpret_cast<WId>(nativeView));
    QVERIFY(embedded);
    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);
    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];

    for (NSString *title in titles) {
        AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, title);
        QVERIFY(buttonRef);
        TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
        QCOMPARE(AXElement([button parent]), AXElement(groupRef));
        NSArray *children = [groupObject childList];
        QVERIFY2(containsElement(children, buttonRef),
                 (describeElement(buttonRef) + " not among " + describeElements(children)).constData());
        [button release];
    }

    [window release];
    [groupObject release];
}

void tst_QAccessibilityMac::windowContainerParent_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerParent()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);

    // The window container is not part of the user-visible tree, so the group
    // box is what the embedded window points back to
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    // And the group box points to it in turn, so that walking down and walking
    // up agree on the hierarchy. The view of our own window is ignored and
    // never appears here, only the content it vends.
    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];
    NSArray *children = [groupObject childList];
    QVERIFY2(containsElement(children, buttonRef),
             (describeElement(buttonRef) + " not among " + describeElements(children)).constData());

    // Pushing a parent onto a foreign view puts a Qt element into the chain
    // AppKit derives the window attribute from
    TestAXObject *buttonWindow = [[TestAXObject alloc] initWithAXUIElementRef:[button window]];
    QCOMPARE(QString::fromNSString(buttonWindow.title), m_window->windowTitle());

    [window release];
    [button release];
    [groupObject release];
    [buttonWindow release];
}

// A hosted window whose node reports the Window role puts the bridge on a
// different path: QMacAccessibilityElement's accessibilityParent declines to
// build an element for an Application or Window parent, and hands back the view
// of the interface's own window instead. So the view is what answers, and the
// window container has its say only through [QNSView accessibilityParent].
void tst_QAccessibilityMac::windowContainerParentThroughWindowRole()
{
    auto *embedded = new WindowRoleWindow;
    embedded->setTitle("Embedded button");
    embedded->resize(200, 32);

    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);

    // The window node is between the button and the container, and is ignored
    // in both directions, so the two ends still meet at the group box
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];
    NSArray *children = [groupObject childList];
    QVERIFY2(containsElement(children, buttonRef),
             (describeElement(buttonRef) + " not among " + describeElements(children)).constData());

    [window release];
    [button release];
    [groupObject release];
}

void tst_QAccessibilityMac::windowContainerParentWithoutGroup_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerParentWithoutGroup()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    addContainer(m_window, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);

    // With nothing but ignored widgets between the container and the top level,
    // the parent is the window itself
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement([window ref]));

    NSArray *children = [window childList];
    QVERIFY2(containsElement(children, buttonRef),
             (describeElement(buttonRef) + " not among " + describeElements(children)).constData());

    [window release];
    [button release];
}

void tst_QAccessibilityMac::windowContainerParentWhileInactive_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerParentWhileInactive()
{
    QFETCH(bool, foreign);

    // The normal order of events: a window container adopts its window long
    // before any accessibility client shows up, so the parent it sets is not
    // pushed onto a foreign view at that point, and the first query from a
    // client is what has to bring the two in sync.
    QAccessible::setActive(false);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    // Had something asked about accessibility while we were building the
    // hierarchy above, the parent would already be resolved, and the query
    // below would no longer be the first one
    QVERIFY(!QAccessible::isActive());

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);

    // Walking the hierarchy is what asks our views about accessibility, and
    // hence what turned the machinery on
    QVERIFY(QAccessible::isActive());

    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    [window release];
    [button release];
}

void tst_QAccessibilityMac::windowContainerReparent_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerReparent()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    NSArray *nativeElements = foreign ? vendedElements(embedded) : @[];
    QGroupBox *first = addGroup(m_window, "First group");
    QGroupBox *second = addGroup(m_window, "Second group");
    addContainer(first, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    // Walk the hierarchy before the move, so that the elements under test are
    // the ones the system already holds, rather than ones built afterwards
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];

    AXUIElementRef firstRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      first->title().toNSString());
    QVERIFY(firstRef);
    QCOMPARE(AXElement([button parent]), AXElement(firstRef));

    // What a foreign view answers with is a parent pushed onto the elements it
    // vends, so hold on to it, to tell a replaced parent from a replaced element
    const QList<id> pushedParents = accessibilityParents(nativeElements);

    addContainer(second, embedded);
    QCoreApplication::processEvents();

    // Finding the group box the window moved to is also the walk that hands the
    // foreign view out again, and hence what resolves its parent anew
    AXUIElementRef secondRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                       second->title().toNSString());
    QVERIFY(secondRef);

    // The push has to follow the move. Were it left as it was, the checks below
    // could still pass on an element AppKit built after the move.
    const QList<id> movedParents = accessibilityParents(nativeElements);
    for (int i = 0; i < pushedParents.count(); ++i) {
        QVERIFY(movedParents.at(i));
        QVERIFY(movedParents.at(i) != pushedParents.at(i));
    }

    // AppKit hands out a new element for a foreign view that has moved between
    // superviews, so the element we held before the move may not answer anymore.
    // Our own elements outlive the move, and are still the ones under test.
    if (![button optionalParent]) {
        buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
        QVERIFY(buttonRef);
        [button release];
        button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    }

    QCOMPARE(AXElement([button parent]), AXElement(secondRef));

    // The group box the window left has to let go of it, or the same window
    // is reported in two places
    TestAXObject *firstObject = [[TestAXObject alloc] initWithAXUIElementRef:firstRef];
    QCOMPARE([[firstObject descendantsWithRole:kAXButtonRole title:@"Embedded button"] count], 0u);

    [window release];
    [button release];
    [firstObject release];
}

void tst_QAccessibilityMac::windowContainerIndirectReparent_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerIndirectReparent()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QGroupBox *first = addGroup(m_window, "First group");
    QGroupBox *second = addGroup(m_window, "Second group");

    // The container is not a direct child of the group box, so the parent the
    // embedded window reports follows the widget in between
    QWidget *intermediate = new QWidget(first);
    intermediate->setLayout(new QVBoxLayout);
    addContainer(intermediate, embedded);
    first->layout()->addWidget(intermediate);
    intermediate->show();
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];

    AXUIElementRef firstRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      first->title().toNSString());
    QVERIFY(firstRef);
    QCOMPARE(AXElement([button parent]), AXElement(firstRef));

    // Find the group box to move to before moving, so that the element we
    // compare against is one the system already handed out, rather than one
    // built by the search afterwards
    AXUIElementRef secondRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                       second->title().toNSString());
    QVERIFY(secondRef);

    second->layout()->addWidget(intermediate);
    QCoreApplication::processEvents();

    walkHierarchy(window);
    QCOMPARE(AXElement([button parent]), AXElement(secondRef));

    [window release];
    [button release];
}

void tst_QAccessibilityMac::windowContainerBelowNativeWidget_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerBelowNativeWidget()
{
    QFETCH(bool, foreign);

    // A window container hands its window to the top level, not to the nearest
    // native ancestor, so with a native widget in between, the widget that
    // reparents and the window that hosts the embedded window sit in different
    // native views.
    QWidget *nativeChild = createNativeChild(m_window);
    QGroupBox *first = addGroup(nativeChild, "First group");
    QGroupBox *second = addGroup(nativeChild, "Second group");
    m_window->addWidget(nativeChild);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QWidget *container = addContainer(first, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    // The content of a foreign window has to be in the tree at all first. The
    // bridge substitutes a native view for the interface of the window it
    // represents only when that window is nested below the one responsible for
    // the ancestor, and the interfaces here answer with the native child's
    // window, which the embedded window is not below.
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];

    AXUIElementRef firstRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      first->title().toNSString());
    QVERIFY(firstRef);
    QCOMPARE(AXElement([button parent]), AXElement(firstRef));

    AXUIElementRef secondRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                       second->title().toNSString());
    QVERIFY(secondRef);

    // The container moves within the native child's widget tree, while the
    // embedded window stays a child of the top level window, so the walk that
    // resolves the parent anew starts in a different window than the one the
    // moved widget belongs to
    second->layout()->addWidget(container);
    QCoreApplication::processEvents();

    walkHierarchy(window);
    QCOMPARE(AXElement([button parent]), AXElement(secondRef));

    [window release];
    [button release];
}

void tst_QAccessibilityMac::windowContainerBelowNestedNativeWidgets_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerBelowNestedNativeWidgets()
{
    QFETCH(bool, foreign);

    // With one native widget between the container and the top level, the window
    // of the widget that vends the container's element is a sibling of the
    // embedded window. With two, it is neither a sibling nor an ancestor of it,
    // and the semantic hierarchy leaves a nested view for one that hangs off the
    // top level view directly.
    QWidget *outerNative = createNativeChild(m_window);
    QWidget *innerNative = createNativeChild(outerNative);
    outerNative->layout()->addWidget(innerNative);
    QGroupBox *group = addGroup(innerNative, "Group");
    m_window->addWidget(outerNative);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    // The premise of the test: a container hands its window to the top level,
    // not to the nearest native ancestor, so no amount of native widgets in
    // between changes where the embedded window ends up.
    QCOMPARE(embedded->parent(), m_window->windowHandle());

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);

    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];
    NSArray *children = [groupObject childList];
    QVERIFY2(containsElement(children, buttonRef),
             (describeElement(buttonRef) + " not among " + describeElements(children)).constData());

    // AppKit resolves the window attribute by walking the parent chain, so the
    // walk has to arrive at our window from an element that the view hierarchy
    // places two native views below it
    TestAXObject *buttonWindow = [[TestAXObject alloc] initWithAXUIElementRef:[button window]];
    QCOMPARE(QString::fromNSString(buttonWindow.title), m_window->windowTitle());

    [window release];
    [button release];
    [groupObject release];
    [buttonWindow release];
}

void tst_QAccessibilityMac::windowContainerRemoved_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerRemoved()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);

    // Remember what the native elements answer with on their own, before Qt
    // hosts them anywhere
    NSArray *nativeElements = foreign ? vendedElements(embedded) : @[];
    const QList<id> defaultParents = accessibilityParents(nativeElements);

    QGroupBox *group = addGroup(m_window, "Group");
    QWidget *container = addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);
    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    QVERIFY(elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button"));

    // And what Qt wrote onto them in its place, so that the two can be told
    // apart once the window is gone
    const QList<id> pushedParents = accessibilityParents(nativeElements);
    for (int i = 0; i < pushedParents.count(); ++i)
        QVERIFY(pushedParents.at(i) != defaultParents.at(i));

    // The container owns the window, so this takes the platform window with it
    delete container;
    QCoreApplication::processEvents();

    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];
    QCOMPARE([[groupObject descendantsWithRole:kAXButtonRole title:@"Embedded button"] count], 0u);

    // The native view outlives the window that hosted it, and must not be left
    // pointing at an element of a group box that no longer knows about it. The
    // override is set to nothing rather than removed, which for a view that has
    // left the view hierarchy is what it reported to begin with.
    for (int i = 0; i < defaultParents.count(); ++i) {
        id<NSAccessibility> element = nativeElements[i];
        QVERIFY(element.accessibilityParent != pushedParents.at(i));
        QVERIFY(element.accessibilityParent == defaultParents.at(i));
    }

    [window release];
    [groupObject release];
}

// A window that leaves its container is a different case from one that goes
// away with it, as the view lives on in a view hierarchy of its own.
void tst_QAccessibilityMac::foreignWindowReleasedFromContainer()
{
    QWindow *embedded = createEmbeddedWindow(true, "Embedded button");
    QVERIFY(embedded);
    NSArray *nativeElements = vendedElements(embedded);
    const QList<id> defaultParents = accessibilityParents(nativeElements);

    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);
    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    const QList<id> pushedParents = accessibilityParents(nativeElements);
    for (int i = 0; i < pushedParents.count(); ++i)
        QVERIFY(pushedParents.at(i) != defaultParents.at(i));

    // Hand the window to a window of our own instead, so that the view outlives
    // the container that hosted it
    QWindow host;
    host.setTitle("Host window");
    host.resize(200, 200);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    embedded->setParent(&host);
    QCoreApplication::processEvents();

    // An override can be set to nothing, but not removed, so the elements report
    // no parent rather than the one they would have resolved on their own, even
    // though the view is still in a view hierarchy here
    for (id parent : accessibilityParents(nativeElements))
        QVERIFY(!parent);

    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];
    QCOMPARE([[groupObject descendantsWithRole:kAXButtonRole title:@"Embedded button"] count], 0u);

    delete embedded;

    [window release];
    [button release];
    [groupObject release];
}

void tst_QAccessibilityMac::foreignWindowParentWhileInactive_data()
{
    // The window either stays where it is while nobody is listening, or is
    // handed over to someone else, which is what has to drop the push
    QTest::addColumn<bool>("released");
    QTest::newRow("kept") << false;
    QTest::newRow("released") << true;
}

void tst_QAccessibilityMac::foreignWindowParentWhileInactive()
{
    QFETCH(bool, released);

    QWindow *embedded = createEmbeddedWindow(true, "Embedded button");
    QVERIFY(embedded);
    NSArray *nativeElements = vendedElements(embedded);

    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);
    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    for (id parent : accessibilityParents(nativeElements))
        QVERIFY(parent);

    // Somewhere to hand the window over to, prepared before we go inactive, as
    // showing a window is enough for AppKit to ask about accessibility again
    QWindow host;
    if (released) {
        host.setTitle("Host window");
        host.resize(200, 200);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
    }

    // Only a query can observe the push, and a query is what refreshes it, so
    // going inactive leaves it as it is
    QAccessible::setActive(false);
    for (id parent : accessibilityParents(nativeElements))
        QVERIFY(parent);

    // Hand the window over with nobody listening, so that nothing but the move
    // itself can let go of the container as the accessible parent
    if (released)
        embedded->setParent(&host);

    QCoreApplication::processEvents();

    QVERIFY(!QAccessible::isActive());
    QAccessible::setActive(true);

    if (released) {
        // There is no parent to resolve anymore, and the view must not be left
        // pointing at the one it had before
        for (id parent : accessibilityParents(nativeElements))
            QVERIFY(!parent);
        delete embedded;
    } else {
        // The hierarchy is the one we left, so a client that walks it again
        // ends up where it started
        walkHierarchy(window);
        for (id parent : accessibilityParents(nativeElements))
            QVERIFY(parent);
        QCOMPARE(AXElement([button parent]), AXElement(groupRef));
    }

    [window release];
    [button release];
}

void tst_QAccessibilityMac::windowContainerSiblingOrder_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerSiblingOrder()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QGroupBox *group = addGroup(m_window, "Group");

    QPushButton *first = new QPushButton("First button", group);
    group->layout()->addWidget(first);
    first->show();
    addContainer(group, embedded);
    QPushButton *last = new QPushButton("Last button", group);
    group->layout()->addWidget(last);
    last->show();

    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);
    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];

    // The embedded window takes the container's place among the group box's
    // children, rather than being appended to them
    NSMutableArray *titles = [NSMutableArray array];
    for (id child in [groupObject childList]) {
        TestAXObject *childObject = [[TestAXObject alloc] initWithAXUIElementRef:(AXUIElementRef)child];
        [titles addObject:childObject.title ?: @"<untitled>"];
        [childObject release];
    }
    NSArray *expected = @[@"First button", @"Embedded button", @"Last button"];
    QCOMPARE(QString::fromNSString([titles componentsJoinedByString:@", "]),
             QString::fromNSString([expected componentsJoinedByString:@", "]));

    [window release];
    [groupObject release];
}

void tst_QAccessibilityMac::windowContainerHidden_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerHidden()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    // Take hold of the elements before hiding anything, so that the ones
    // compared after the cycle are the ones the system held before it
    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);

    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    // A hidden widget reports itself as invisible, which takes it and
    // everything below it out of the tree, the embedded window included
    group->hide();
    QCoreApplication::processEvents();
    QCOMPARE([[window descendantsWithRole:kAXButtonRole title:@"Embedded button"] count], 0u);

    group->show();
    QTRY_VERIFY(embedded->isVisible());
    QCoreApplication::processEvents();

    // And on the way back in the embedded window names the group box again
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    [window release];
    [button release];
}

void tst_QAccessibilityMac::windowContainerUpdatedWhileHidden_data()
{
    addWindowTypeRows();
}

void tst_QAccessibilityMac::windowContainerUpdatedWhileHidden()
{
    QFETCH(bool, foreign);

    QWindow *embedded = createEmbeddedWindow(foreign, "Embedded button");
    QVERIFY(embedded);
    QGroupBox *group = addGroup(m_window, "Group");
    addContainer(group, embedded);
    QTRY_VERIFY(embedded->handle());
    QCoreApplication::processEvents();

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef groupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                      group->title().toNSString());
    QVERIFY(groupRef);
    AXUIElementRef buttonRef = elementWithRoleAndTitle(window, kAXButtonRole, @"Embedded button");
    QVERIFY(buttonRef);
    TestAXObject *button = [[TestAXObject alloc] initWithAXUIElementRef:buttonRef];
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    group->hide();
    QCoreApplication::processEvents();

    // Something elsewhere in the window changes parent while the group box is
    // hidden. A hidden widget is out of the user-visible tree, so a parent
    // resolved for the embedded window now would resolve past the group box.
    // Nothing hands the view out while it is hidden, so nothing resolves one.
    QPushButton *other = new QPushButton("Other button");
    m_window->addWidget(other);
    QCoreApplication::processEvents();

    group->show();
    QTRY_VERIFY(embedded->isVisible());
    QCoreApplication::processEvents();

    // The group box is back in the tree, and is the parent again
    QCOMPARE(AXElement([button parent]), AXElement(groupRef));

    TestAXObject *groupObject = [[TestAXObject alloc] initWithAXUIElementRef:groupRef];
    NSArray *children = [groupObject childList];
    QVERIFY2(containsElement(children, buttonRef),
             (describeElement(buttonRef) + " not among " + describeElements(children)).constData());

    [window release];
    [button release];
    [groupObject release];
}

// A walk puts a pushed parent back, wherever in the window tree the foreign
// window sits. The nested branch below is the awkward one, with its group box in
// the native child's window while the container hands the window it hosts to the
// top level, so the element that vends the foreign view and the window holding
// that view belong to different native views.
void tst_QAccessibilityMac::windowContainerParentsRestoredByWalk()
{
    QWidget *nativeChild = createNativeChild(m_window);
    QGroupBox *nestedGroup = addGroup(nativeChild, "Nested group");
    m_window->addWidget(nativeChild);
    QGroupBox *siblingGroup = addGroup(m_window, "Sibling group");

    QWindow *nestedEmbedded = createEmbeddedWindow(true, "Nested embedded button");
    QVERIFY(nestedEmbedded);
    addContainer(nestedGroup, nestedEmbedded);
    QWindow *siblingEmbedded = createEmbeddedWindow(true, "Sibling embedded button");
    QVERIFY(siblingEmbedded);
    addContainer(siblingGroup, siblingEmbedded);
    QTRY_VERIFY(nestedEmbedded->handle());
    QTRY_VERIFY(siblingEmbedded->handle());
    QCoreApplication::processEvents();

    // The premise of the nested branch: the group box lives in the native
    // child's window, while the window hosting its embedded window is the
    // top level
    QVERIFY(nativeChild->windowHandle());
    QVERIFY(nativeChild->windowHandle() != m_window->windowHandle());
    QCOMPARE(nestedEmbedded->parent(), m_window->windowHandle());

    TestAXObject *window = testWindowObject();
    QVERIFY(window);

    AXUIElementRef nestedGroupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                            nestedGroup->title().toNSString());
    QVERIFY(nestedGroupRef);
    AXUIElementRef nestedButtonRef = elementWithRoleAndTitle(window, kAXButtonRole,
                                                             @"Nested embedded button");
    QVERIFY(nestedButtonRef);
    TestAXObject *nestedButton = [[TestAXObject alloc] initWithAXUIElementRef:nestedButtonRef];
    QCOMPARE(AXElement([nestedButton parent]), AXElement(nestedGroupRef));

    AXUIElementRef siblingGroupRef = elementWithRoleAndTitle(window, kAXGroupRole,
                                                             siblingGroup->title().toNSString());
    QVERIFY(siblingGroupRef);
    AXUIElementRef siblingButtonRef = elementWithRoleAndTitle(window, kAXButtonRole,
                                                              @"Sibling embedded button");
    QVERIFY(siblingButtonRef);
    TestAXObject *siblingButton = [[TestAXObject alloc] initWithAXUIElementRef:siblingButtonRef];
    QCOMPARE(AXElement([siblingButton parent]), AXElement(siblingGroupRef));

    // Drop both, so that the walk below is the only thing that can put a parent
    // back on either of them
    QVERIFY(clearPushedParents(nestedEmbedded));
    QVERIFY(clearPushedParents(siblingEmbedded));

    walkHierarchy(window);

    for (id parent : accessibilityParents(vendedElements(nestedEmbedded)))
        QVERIFY(parent);
    for (id parent : accessibilityParents(vendedElements(siblingEmbedded)))
        QVERIFY(parent);

    QCOMPARE(AXElement([nestedButton parent]), AXElement(nestedGroupRef));
    QCOMPARE(AXElement([siblingButton parent]), AXElement(siblingGroupRef));

    [window release];
    [nestedButton release];
    [siblingButton release];
}

QTEST_MAIN(tst_QAccessibilityMac)
#include "tst_qaccessibilitymac.moc"
