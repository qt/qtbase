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

    Handles input of foreign characters (particularly East Asian)
    languages.

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

    Input method support in Cocoa uses NSTextInput protorol. Therefore
    almost all functionality is implemented in QNSView.

    \ingroup qt-lighthouse-cocoa
*/



QCocoaInputContext::QCocoaInputContext()
    : QPlatformInputContext()
    , mWindow(QGuiApplication::focusWindow())
{
    QMetaObject::invokeMethod(this, "connectSignals", Qt::QueuedConnection);
    updateLocale();
}

QCocoaInputContext::~QCocoaInputContext()
{
}

/*!
    \brief Cancels a composition.
*/

void QCocoaInputContext::reset()
{
    QPlatformInputContext::reset();

    if (!mWindow)
        return;

    QCocoaWindow *window = static_cast<QCocoaWindow *>(mWindow->handle());
    QNSView *view = qnsview_cast(window->view());
    if (!view)
        return;

    QMacAutoReleasePool pool;
    if (NSTextInputContext *ctxt = [NSTextInputContext currentInputContext]) {
        [ctxt discardMarkedText];
        [view unmarkText];
    }
}

void QCocoaInputContext::connectSignals()
{
    connect(qApp, SIGNAL(focusObjectChanged(QObject*)), this, SLOT(focusObjectChanged(QObject*)));
    focusObjectChanged(qApp->focusObject());
}

void QCocoaInputContext::focusObjectChanged(QObject *focusObject)
{
    Q_UNUSED(focusObject);
    if (mWindow == QGuiApplication::focusWindow()) {
        if (!mWindow)
            return;

        QCocoaWindow *window = static_cast<QCocoaWindow *>(mWindow->handle());
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
        mWindow = QGuiApplication::focusWindow();
    }
}

void QCocoaInputContext::updateLocale()
{
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    CFArrayRef languages = (CFArrayRef) TISGetInputSourceProperty(source, kTISPropertyInputSourceLanguages);
    if (CFArrayGetCount(languages) > 0) {
        CFStringRef langRef = (CFStringRef)CFArrayGetValueAtIndex(languages, 0);
        QString name = QString::fromCFString(langRef);
        QLocale locale(name);
        if (m_locale != locale) {
            m_locale = locale;
            emitLocaleChanged();
        }
    }
    CFRelease(source);
}

QT_END_NAMESPACE
