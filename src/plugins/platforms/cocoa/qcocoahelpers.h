/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAHELPERS_H
#define QCOCOAHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It provides helper functions
// for the Cocoa plugin. This header file may
// change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <AppKit/AppKit.h>

#include <private/qguiapplication_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qpalette.h>
#include <QtGui/qscreen.h>
#include <qpa/qplatformdialoghelper.h>

#include <objc/runtime.h>
#include <objc/message.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QNSView));

struct mach_header;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindow)
Q_DECLARE_LOGGING_CATEGORY(lcQpaDrawing)
Q_DECLARE_LOGGING_CATEGORY(lcQpaMouse)
Q_DECLARE_LOGGING_CATEGORY(lcQpaScreen)

class QPixmap;
class QString;

// Conversion functions
QStringList qt_mac_NSArrayToQStringList(NSArray<NSString *> *nsarray);
NSMutableArray<NSString *> *qt_mac_QStringListToNSMutableArray(const QStringList &list);

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action);
NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions);
Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions);
Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions);

template <typename T>
typename std::enable_if<std::is_pointer<T>::value, T>::type
qt_objc_cast(id object)
{
    if ([object isKindOfClass:[typename std::remove_pointer<T>::type class]])
        return static_cast<T>(object);

    return nil;
}

QT_MANGLE_NAMESPACE(QNSView) *qnsview_cast(NSView *view);

// Misc
void qt_mac_transformProccessToForegroundApplication();
QString qt_mac_applicationName();

QPointF qt_mac_flip(const QPointF &pos, const QRectF &reference);
QRectF qt_mac_flip(const QRectF &rect, const QRectF &reference);

Qt::MouseButton cocoaButton2QtButton(NSInteger buttonNum);
Qt::MouseButton cocoaButton2QtButton(NSEvent *event);

QEvent::Type cocoaEvent2QtMouseEvent(NSEvent *event);

Qt::MouseButtons cocoaMouseButtons2QtMouseButtons(NSInteger pressedMouseButtons);
Qt::MouseButtons currentlyPressedMouseButtons();

// strip out '&' characters, and convert "&&" to a single '&', in menu
// text - since menu text is sometimes decorated with these for Windows
// accelerators.
QString qt_mac_removeAmpersandEscapes(QString s);

enum {
    QtCocoaEventSubTypeWakeup       = SHRT_MAX,
    QtCocoaEventSubTypePostMessage  = SHRT_MAX-1
};

class QCocoaPostMessageArgs {
public:
    id target;
    SEL selector;
    int argCount;
    id arg1;
    id arg2;
    QCocoaPostMessageArgs(id target, SEL selector, int argCount=0, id arg1=0, id arg2=0)
        : target(target), selector(selector), argCount(argCount), arg1(arg1), arg2(arg2)
    {
        [target retain];
        [arg1 retain];
        [arg2 retain];
    }

    ~QCocoaPostMessageArgs()
    {
        [arg2 release];
        [arg1 release];
        [target release];
    }
};

template<typename T>
T qt_mac_resolveOption(const T &fallback, const QByteArray &environment)
{
    // check for environment variable
    if (!environment.isEmpty()) {
        QByteArray env = qgetenv(environment);
        if (!env.isEmpty())
            return T(env.toInt()); // works when T is bool, int.
    }

    return fallback;
}

template<typename T>
T qt_mac_resolveOption(const T &fallback, QWindow *window, const QByteArray &property, const QByteArray &environment)
{
    // check for environment variable
    if (!environment.isEmpty()) {
        QByteArray env = qgetenv(environment);
        if (!env.isEmpty())
            return T(env.toInt()); // works when T is bool, int.
    }

    // check for window property
    if (window && !property.isNull()) {
        QVariant windowProperty = window->property(property);
        if (windowProperty.isValid())
            return windowProperty.value<T>();
    }

    // return default value.
    return fallback;
}

// https://stackoverflow.com/a/52722575/2761869
template<class R>
struct backwards_t {
  R r;
  constexpr auto begin() const { using std::rbegin; return rbegin(r); }
  constexpr auto begin() { using std::rbegin; return rbegin(r); }
  constexpr auto end() const { using std::rend; return rend(r); }
  constexpr auto end() { using std::rend; return rend(r); }
};
template<class R>
constexpr backwards_t<R> backwards(R&& r) { return {std::forward<R>(r)}; }

QT_END_NAMESPACE

// @compatibility_alias doesn't work with protocols
#define QNSPanelDelegate QT_MANGLE_NAMESPACE(QNSPanelDelegate)

@protocol QNSPanelDelegate
@required
- (void)onOkClicked;
- (void)onCancelClicked;
@end

@interface QT_MANGLE_NAMESPACE(QNSPanelContentsWrapper) : NSView

@property (nonatomic, readonly) NSButton *okButton;
@property (nonatomic, readonly) NSButton *cancelButton;
@property (nonatomic, readonly) NSView *panelContents; // ARC: unretained, make it weak
@property (nonatomic, assign) NSEdgeInsets panelContentsMargins;

- (instancetype)initWithPanelDelegate:(id<QNSPanelDelegate>)panelDelegate;
- (void)dealloc;

- (NSButton *)createButtonWithTitle:(QPlatformDialogHelper::StandardButton)type;
- (void)layout;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSPanelContentsWrapper);

// -------------------------------------------------------------------------

// QAppleRefCounted expects the retain function to return the object
io_object_t q_IOObjectRetain(io_object_t obj);
// QAppleRefCounted expects the release function to return void
void q_IOObjectRelease(io_object_t obj);

template <typename T>
class QIOType : public QAppleRefCounted<T, io_object_t, q_IOObjectRetain, q_IOObjectRelease>
{
    using QAppleRefCounted<T, io_object_t, q_IOObjectRetain, q_IOObjectRelease>::QAppleRefCounted;
};

// -------------------------------------------------------------------------

// Depending on the ABI of the platform, we may need to use objc_msgSendSuper_stret:
// - http://www.sealiesoftware.com/blog/archive/2008/10/30/objc_explain_objc_msgSend_stret.html
// - https://lists.apple.com/archives/cocoa-dev/2008/Feb/msg02338.html
template <typename T>
struct objc_msgsend_requires_stret
{ static const bool value =
#if defined(Q_PROCESSOR_X86)
    #define PLATFORM_USES_SEND_SUPER_STRET 1
    // Any return value larger than two registers on i386/x86_64
    sizeof(T) > sizeof(void*) * 2;
#elif defined(Q_PROCESSOR_ARM_32)
    #define PLATFORM_USES_SEND_SUPER_STRET 1
    // Any return value larger than a single register on arm
    sizeof(T) > sizeof(void*);
#elif defined(Q_PROCESSOR_ARM_64)
    #define PLATFORM_USES_SEND_SUPER_STRET 0
    false; // Stret not used on arm64
#endif
};

template <>
struct objc_msgsend_requires_stret<void>
{ static const bool value = false; };

template <typename ReturnType, typename... Args>
ReturnType qt_msgSendSuper(id receiver, SEL selector, Args... args)
{
    static_assert(!objc_msgsend_requires_stret<ReturnType>::value,
        "The given return type requires stret on this platform");

    typedef ReturnType (*SuperFn)(objc_super *, SEL, Args...);
    SuperFn superFn = reinterpret_cast<SuperFn>(objc_msgSendSuper);
    objc_super sup = { receiver, [receiver superclass] };
    return superFn(&sup, selector, args...);
}

#if PLATFORM_USES_SEND_SUPER_STRET
template <typename ReturnType, typename... Args>
ReturnType qt_msgSendSuper_stret(id receiver, SEL selector, Args... args)
{
    static_assert(objc_msgsend_requires_stret<ReturnType>::value,
        "The given return type does not use stret on this platform");

    typedef void (*SuperStretFn)(ReturnType *, objc_super *, SEL, Args...);
    SuperStretFn superStretFn = reinterpret_cast<SuperStretFn>(objc_msgSendSuper_stret);

    objc_super sup = { receiver, [receiver superclass] };
    ReturnType ret;
    superStretFn(&ret, &sup, selector, args...);
    return ret;
}
#endif

template<typename... Args>
class QSendSuperHelper {
public:
    QSendSuperHelper(id receiver, SEL sel, Args... args)
        : m_receiver(receiver), m_selector(sel), m_args(std::make_tuple(args...)), m_sent(false)
    {
    }

    ~QSendSuperHelper()
    {
        if (!m_sent)
            msgSendSuper<void>(m_args);
    }

    template <typename ReturnType>
    operator ReturnType()
    {
#if defined(QT_DEBUG)
        Method method = class_getInstanceMethod(object_getClass(m_receiver), m_selector);
        char returnTypeEncoding[256];
        method_getReturnType(method, returnTypeEncoding, sizeof(returnTypeEncoding));
        NSUInteger alignedReturnTypeSize = 0;
        NSGetSizeAndAlignment(returnTypeEncoding, nullptr, &alignedReturnTypeSize);
        Q_ASSERT(alignedReturnTypeSize == sizeof(ReturnType));
#endif
        m_sent = true;
        return msgSendSuper<ReturnType>(m_args);
    }

private:
    template <typename ReturnType, bool V>
    using if_requires_stret = typename std::enable_if<objc_msgsend_requires_stret<ReturnType>::value == V, ReturnType>::type;

    template <typename ReturnType, int... Is>
    if_requires_stret<ReturnType, false> msgSendSuper(std::tuple<Args...>& args, QtPrivate::IndexesList<Is...>)
    {
        return qt_msgSendSuper<ReturnType>(m_receiver, m_selector, std::get<Is>(args)...);
    }

#if PLATFORM_USES_SEND_SUPER_STRET
    template <typename ReturnType, int... Is>
    if_requires_stret<ReturnType, true> msgSendSuper(std::tuple<Args...>& args, QtPrivate::IndexesList<Is...>)
    {
        return qt_msgSendSuper_stret<ReturnType>(m_receiver, m_selector, std::get<Is>(args)...);
    }
#endif

    template <typename ReturnType>
    ReturnType msgSendSuper(std::tuple<Args...>& args)
    {
        return msgSendSuper<ReturnType>(args, QtPrivate::makeIndexSequence<sizeof...(Args)>{});
    }

    id m_receiver;
    SEL m_selector;
    std::tuple<Args...> m_args;
    bool m_sent;
};

template<typename... Args>
QSendSuperHelper<Args...> qt_objcDynamicSuperHelper(id receiver, SEL selector, Args... args)
{
    return QSendSuperHelper<Args...>(receiver, selector, args...);
}

// Same as calling super, but the super_class field resolved at runtime instead of compile time
#define qt_objcDynamicSuper(...) qt_objcDynamicSuperHelper(self, _cmd, ##__VA_ARGS__)

#endif //QCOCOAHELPERS_H

