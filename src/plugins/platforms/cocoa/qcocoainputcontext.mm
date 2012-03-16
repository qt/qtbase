/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnsview.h"
#include "qcocoainputcontext.h"
#include "qcocoanativeinterface.h"
#include "qcocoaautoreleasepool.h"

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
    connect(qApp->inputMethod(), SIGNAL(inputItemChanged()), this, SLOT(inputItemChanged()));
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

    if (!mWindow) return;

    QCocoaNativeInterface *nativeInterface = qobject_cast<QCocoaNativeInterface *>(QGuiApplication::platformNativeInterface());
    if (!nativeInterface) return;

    QNSView *view = static_cast<QNSView *>(nativeInterface->nativeResourceForWindow("nsview", mWindow));
    if (!view) return;

    QCocoaAutoReleasePool pool;
    NSInputManager *currentIManager = [NSInputManager currentInputManager];
    if (currentIManager) {
        [currentIManager markedTextAbandoned:view];
        [view unmarkText];
    }
}

void QCocoaInputContext::inputItemChanged()
{
    mWindow = QGuiApplication::focusWindow();
}

QT_END_NAMESPACE
