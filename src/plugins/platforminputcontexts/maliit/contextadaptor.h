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

#ifndef CONTEXT_H_1318935171
#define CONTEXT_H_1318935171

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Adaptor class for interface com.meego.inputmethod.inputcontext1
 */
class Inputcontext1Adaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.meego.inputmethod.inputcontext1")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.meego.inputmethod.inputcontext1\">\n"
"    <method name=\"activationLostEvent\"/>\n"
"    <method name=\"imInitiatedHide\"/>\n"
"    <method name=\"commitString\">\n"
"      <arg type=\"s\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"    </method>\n"
"    <method name=\"updatePreedit\">\n"
"      <arg type=\"s\"/>\n"
"      <arg type=\"a(iii)\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"    </method>\n"
"    <method name=\"keyEvent\">\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"s\"/>\n"
"      <arg type=\"b\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"y\"/>\n"
"    </method>\n"
"    <method name=\"updateInputMethodArea\">\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"    </method>\n"
"    <method name=\"setGlobalCorrectionEnabled\">\n"
"      <arg type=\"b\"/>\n"
"    </method>\n"
"    <method name=\"preeditRectangle\">\n"
"      <arg direction=\"out\" type=\"b\"/>\n"
"      <arg direction=\"out\" type=\"i\"/>\n"
"      <arg direction=\"out\" type=\"i\"/>\n"
"      <arg direction=\"out\" type=\"i\"/>\n"
"      <arg direction=\"out\" type=\"i\"/>\n"
"    </method>\n"
"    <method name=\"copy\"/>\n"
"    <method name=\"paste\"/>\n"
"    <method name=\"setRedirectKeys\">\n"
"      <arg type=\"b\"/>\n"
"    </method>\n"
"    <method name=\"setDetectableAutoRepeat\">\n"
"      <arg type=\"b\"/>\n"
"    </method>\n"
"    <method name=\"setSelection\">\n"
"      <arg type=\"i\"/>\n"
"      <arg type=\"i\"/>\n"
"    </method>\n"
"    <method name=\"selection\">\n"
"      <arg direction=\"out\" type=\"b\"/>\n"
"      <arg direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"    <method name=\"setLanguage\">\n"
"      <arg type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    Inputcontext1Adaptor(QObject *parent);
    virtual ~Inputcontext1Adaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    void activationLostEvent();
    void commitString(const QString &in0, int in1, int in2, int in3);
    void updatePreedit(const QDBusMessage &message);
    void copy();
    void imInitiatedHide();
    void keyEvent(int in0, int in1, int in2, const QString &in3, bool in4, int in5, uchar in6);
    void paste();
    bool preeditRectangle(int &out1, int &out2, int &out3, int &out4);
    bool selection(QString &out1);
    void setDetectableAutoRepeat(bool in0);
    void setGlobalCorrectionEnabled(bool in0);
    void setLanguage(const QString &in0);
    void setRedirectKeys(bool in0);
    void setSelection(int in0, int in1);
    void updateInputMethodArea(int in0, int in1, int in2, int in3);
Q_SIGNALS: // SIGNALS
};

#endif
