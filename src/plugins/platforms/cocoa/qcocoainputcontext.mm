// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qnsview.h"
#include "qcocoainputcontext.h"
#include "qcocoanativeinterface.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"

#include <Carbon/Carbon.h>

#include <QtCore/QRect>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

/*!
    \class QCocoaInputContext
    \brief Cocoa Input context implementation

    Handles input of languages that support character composition,
    for example East Asian languages.

    \section1 Testing

    \list
    \o Select input sources like 'Kotoeri' in Language & Text Preferences
    \o Compile the \a mainwindows/mdi example and open a text window.
    \o In the language bar, switch to 'Hiragana'.
    \o In a text editor control, type the syllable \a 'la'.
       Underlined characters show up, indicating that there is completion
       available. Press the Space key two times. A completion popup occurs
       which shows the options.
    \endlist

    \section1 Interaction

    Input method support in Cocoa is based on the NSTextInputClient protocol,
    therefore almost all functionality is in QNSView (qnsview_complextext.mm).
*/

QCocoaInputContext::QCocoaInputContext()
    : QPlatformInputContext()
    , m_focusWindow(QGuiApplication::focusWindow())
{
    m_inputSourceObserver = QMacNotificationObserver(nil,
        NSTextInputContextKeyboardSelectionDidChangeNotification, [&]() {
        qCDebug(lcQpaInputMethods) << "Text input source changed";
        updateLocale();
    });

    updateLocale();
}

QCocoaInputContext::~QCocoaInputContext()
{
}

/*!
    Commits the current composition if there is one,
    by "unmarking" the text in the edit buffer, and
    informing the system input context of this fact.
*/
void QCocoaInputContext::commit()
{
    qCDebug(lcQpaInputMethods) << "Committing composition";

    if (!m_focusWindow)
        return;

    auto *platformWindow = m_focusWindow->handle();
    if (!platformWindow)
        return;

    auto *cocoaWindow = static_cast<QCocoaWindow *>(platformWindow);
    QNSView *view = qnsview_cast(cocoaWindow->view());
    if (!view)
        return;

    [view unmarkText];

    [view.inputContext discardMarkedText];
    if (view.inputContext != NSTextInputContext.currentInputContext) {
        // discardMarkedText will activate the TSM document of the given input context,
        // which may not match the current input context. To ensure the current input
        // context is not left in an inconsistent state with a deactivated document
        // we need to manually activate it here.
        [NSTextInputContext.currentInputContext activate];
    }
}


/*!
    \brief Cancels a composition.
*/
void QCocoaInputContext::reset()
{
    qCDebug(lcQpaInputMethods) << "Resetting input method";

    if (!m_focusWindow)
        return;

    QCocoaWindow *window = static_cast<QCocoaWindow *>(m_focusWindow->handle());
    QNSView *view = qnsview_cast(window->view());
    if (!view)
        return;

    if (NSTextInputContext *ctxt = [NSTextInputContext currentInputContext]) {
        [ctxt discardMarkedText];
        [view unmarkText];
    }
}

void QCocoaInputContext::setFocusObject(QObject *focusObject)
{
    qCDebug(lcQpaInputMethods) << "Focus object changed to" << focusObject;

    if (m_focusWindow == QGuiApplication::focusWindow()) {
        if (!m_focusWindow)
            return;

        QCocoaWindow *window = static_cast<QCocoaWindow *>(m_focusWindow->handle());
        if (!window)
            return;
        QNSView *view = qnsview_cast(window->view());
        if (!view)
            return;

        if (NSTextInputContext *ctxt = [NSTextInputContext currentInputContext]) {
            [ctxt discardMarkedText];
            [view cancelComposingText];
        }
    } else {
        m_focusWindow = QGuiApplication::focusWindow();
    }
}

void QCocoaInputContext::updateLocale()
{
    QCFType<TISInputSourceRef> source = TISCopyCurrentKeyboardInputSource();
    NSArray *languages = static_cast<NSArray*>(TISGetInputSourceProperty(source,
                                               kTISPropertyInputSourceLanguages));

    qCDebug(lcQpaInputMethods) << "Input source supports" << languages;
    if (!languages.count)
        return;

    QString language = QString::fromNSString(languages.firstObject);
    QLocale locale(language);

    bool localeUpdated = m_locale != locale;
    static bool firstUpdate = true;

    m_locale = locale;

    if (localeUpdated && !firstUpdate) {
        qCDebug(lcQpaInputMethods) << "Reporting new locale" << locale;
        emitLocaleChanged();
    }

    firstUpdate = false;
}

QT_END_NAMESPACE
