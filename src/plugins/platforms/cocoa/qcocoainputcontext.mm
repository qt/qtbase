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

    QMacAutoReleasePool pool;
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

    QPlatformInputContext::reset();

    if (!m_focusWindow)
        return;

    QCocoaWindow *window = static_cast<QCocoaWindow *>(m_focusWindow->handle());
    QNSView *view = qnsview_cast(window->view());
    if (!view)
        return;

    QMacAutoReleasePool pool;
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
    if (m_locale != locale) {
        qCDebug(lcQpaInputMethods) << "Reporting new locale" << locale;
        m_locale = locale;
        emitLocaleChanged();
    }
}

QT_END_NAMESPACE
