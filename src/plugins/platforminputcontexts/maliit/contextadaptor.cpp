/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "contextadaptor.h"
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include "qmaliitplatforminputcontext.h"

/*
 * Implementation of adaptor class Inputcontext1Adaptor
 */

Inputcontext1Adaptor::Inputcontext1Adaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

Inputcontext1Adaptor::~Inputcontext1Adaptor()
{
    // destructor
}

void Inputcontext1Adaptor::activationLostEvent()
{
    // handle method call com.meego.inputmethod.inputcontext1.activationLostEvent
    QMetaObject::invokeMethod(parent(), "activationLostEvent");
}

void Inputcontext1Adaptor::commitString(const QString &in0, int in1, int in2, int in3)
{
    // handle method call com.meego.inputmethod.inputcontext1.commitString
    QMetaObject::invokeMethod(parent(), "commitString", Q_ARG(QString, in0), Q_ARG(int, in1), Q_ARG(int, in2), Q_ARG(int, in3));
}

void Inputcontext1Adaptor::updatePreedit(const QDBusMessage &message)
{
    // handle method call com.meego.inputmethod.inputcontext1.updatePreedit
    QMetaObject::invokeMethod(parent(), "updatePreedit", Q_ARG(QDBusMessage, message));
}

void Inputcontext1Adaptor::copy()
{
    // handle method call com.meego.inputmethod.inputcontext1.copy
    QMetaObject::invokeMethod(parent(), "copy");
}

void Inputcontext1Adaptor::imInitiatedHide()
{
    // handle method call com.meego.inputmethod.inputcontext1.imInitiatedHide
    QMetaObject::invokeMethod(parent(), "imInitiatedHide");
}

void Inputcontext1Adaptor::keyEvent(int in0, int in1, int in2, const QString &in3, bool in4, int in5, uchar in6)
{
    // handle method call com.meego.inputmethod.inputcontext1.keyEvent
    QMetaObject::invokeMethod(parent(), "keyEvent", Q_ARG(int, in0), Q_ARG(int, in1), Q_ARG(int, in2), Q_ARG(QString, in3), Q_ARG(bool, in4), Q_ARG(int, in5), Q_ARG(uchar, in6));
}

void Inputcontext1Adaptor::paste()
{
    // handle method call com.meego.inputmethod.inputcontext1.paste
    QMetaObject::invokeMethod(parent(), "paste");
}

bool Inputcontext1Adaptor::preeditRectangle(int &out1, int &out2, int &out3, int &out4)
{
    // handle method call com.meego.inputmethod.inputcontext1.preeditRectangle
    return static_cast<QMaliitPlatformInputContext *>(parent())->preeditRectangle(out1, out2, out3, out4);
}

bool Inputcontext1Adaptor::selection(QString &out1)
{
    // handle method call com.meego.inputmethod.inputcontext1.selection
    return static_cast<QMaliitPlatformInputContext *>(parent())->selection(out1);
}

void Inputcontext1Adaptor::setDetectableAutoRepeat(bool in0)
{
    // handle method call com.meego.inputmethod.inputcontext1.setDetectableAutoRepeat
    QMetaObject::invokeMethod(parent(), "setDetectableAutoRepeat", Q_ARG(bool, in0));
}

void Inputcontext1Adaptor::setGlobalCorrectionEnabled(bool in0)
{
    // handle method call com.meego.inputmethod.inputcontext1.setGlobalCorrectionEnabled
    QMetaObject::invokeMethod(parent(), "setGlobalCorrectionEnabled", Q_ARG(bool, in0));
}

void Inputcontext1Adaptor::setLanguage(const QString &in0)
{
    // handle method call com.meego.inputmethod.inputcontext1.setLanguage
    QMetaObject::invokeMethod(parent(), "setLanguage", Q_ARG(QString, in0));
}

void Inputcontext1Adaptor::setRedirectKeys(bool in0)
{
    // handle method call com.meego.inputmethod.inputcontext1.setRedirectKeys
    QMetaObject::invokeMethod(parent(), "setRedirectKeys", Q_ARG(bool, in0));
}

void Inputcontext1Adaptor::setSelection(int in0, int in1)
{
    // handle method call com.meego.inputmethod.inputcontext1.setSelection
    QMetaObject::invokeMethod(parent(), "setSelection", Q_ARG(int, in0), Q_ARG(int, in1));
}

void Inputcontext1Adaptor::updateInputMethodArea(int in0, int in1, int in2, int in3)
{
    // handle method call com.meego.inputmethod.inputcontext1.updateInputMethodArea
    QMetaObject::invokeMethod(parent(), "updateInputMethodArea", Q_ARG(int, in0), Q_ARG(int, in1), Q_ARG(int, in2), Q_ARG(int, in3));
}

