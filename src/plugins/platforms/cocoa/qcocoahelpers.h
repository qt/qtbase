// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <private/qguiapplication_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/qpalette.h>
#include <QtGui/qscreen.h>
#include <qpa/qplatformdialoghelper.h>

#include <objc/runtime.h>
#include <objc/message.h>

#if defined(__OBJC__)

#import <AppKit/NSDragging.h>
#import <AppKit/NSEvent.h>
#import <AppKit/NSButton.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QNSView));

struct mach_header;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindow)
Q_DECLARE_LOGGING_CATEGORY(lcQpaDrawing)
Q_DECLARE_LOGGING_CATEGORY(lcQpaMouse)
Q_DECLARE_LOGGING_CATEGORY(lcQpaKeys)
Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods)
Q_DECLARE_LOGGING_CATEGORY(lcQpaScreen)
Q_DECLARE_LOGGING_CATEGORY(lcQpaApplication)
Q_DECLARE_LOGGING_CATEGORY(lcQpaClipboard)
Q_DECLARE_LOGGING_CATEGORY(lcQpaDialogs)
Q_DECLARE_LOGGING_CATEGORY(lcQpaMenus)

class QPixmap;
class QString;

// Conversion functions
QStringList qt_mac_NSArrayToQStringList(NSArray<NSString *> *nsarray);
NSMutableArray<NSString *> *qt_mac_QStringListToNSMutableArray(const QStringList &list);

NSDragOperation qt_mac_mapDropAction(Qt::DropAction action);
NSDragOperation qt_mac_mapDropActions(Qt::DropActions actions);
Qt::DropAction qt_mac_mapNSDragOperation(NSDragOperation nsActions);
Qt::DropActions qt_mac_mapNSDragOperations(NSDragOperation nsActions);

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

// Similar to __NXKitString for localized AppKit strings
NSString *qt_mac_AppKitString(NSString *table, NSString *key);

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

QT_BEGIN_NAMESPACE

// -------------------------------------------------------------------------

struct InputMethodQueryResult : public QHash<int, QVariant>
{
    operator bool() { return !isEmpty(); }
};

InputMethodQueryResult queryInputMethod(QObject *object, Qt::InputMethodQueries queries = Qt::ImEnabled);

// -------------------------------------------------------------------------

struct KeyEvent
{
    ulong timestamp = 0;
    QEvent::Type type = QEvent::None;

    Qt::Key key = Qt::Key_unknown;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    QString text;
    bool isRepeat = false;

    // Scan codes are hardware dependent codes for each key. There is no way to get these
    // from Carbon or Cocoa, so leave it 0, as documented in QKeyEvent::nativeScanCode().
    static const quint32 nativeScanCode = 0;

    quint32 nativeVirtualKey = 0;
    NSEventModifierFlags nativeModifiers = 0;

    KeyEvent(NSEvent *nsevent);
    bool sendWindowSystemEvent(QWindow *window) const;
};

QDebug operator<<(QDebug debug, const KeyEvent &e);

// -------------------------------------------------------------------------

QDebug operator<<(QDebug, const NSRange &);
QDebug operator<<(QDebug, SEL);

#endif // __OBJC__

QT_END_NAMESPACE

#endif //QCOCOAHELPERS_H

