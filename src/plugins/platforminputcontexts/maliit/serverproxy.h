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

#ifndef SERVER_H_1318935108
#define SERVER_H_1318935108

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface com.meego.inputmethod.uiserver1
 */
class ComMeegoInputmethodUiserver1Interface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.meego.inputmethod.uiserver1"; }

public:
    ComMeegoInputmethodUiserver1Interface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~ComMeegoInputmethodUiserver1Interface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> activateContext()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("activateContext"), argumentList);
    }

    inline QDBusPendingReply<> appOrientationAboutToChange(int in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QLatin1String("appOrientationAboutToChange"), argumentList);
    }

    inline QDBusPendingReply<> appOrientationChanged(int in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QLatin1String("appOrientationChanged"), argumentList);
    }

    inline QDBusPendingReply<> hideInputMethod()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("hideInputMethod"), argumentList);
    }

    inline QDBusPendingReply<> mouseClickedOnPreedit(int in0, int in1, int in2, int in3, int in4, int in5)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0) << QVariant::fromValue(in1) << QVariant::fromValue(in2) << QVariant::fromValue(in3) << QVariant::fromValue(in4) << QVariant::fromValue(in5);
        return asyncCallWithArgumentList(QLatin1String("mouseClickedOnPreedit"), argumentList);
    }

    inline QDBusPendingReply<> processKeyEvent(int in0, int in1, int in2, const QString &in3, bool in4, int in5, uint in6, uint in7, uint in8)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0) << QVariant::fromValue(in1) << QVariant::fromValue(in2) << QVariant::fromValue(in3) << QVariant::fromValue(in4) << QVariant::fromValue(in5) << QVariant::fromValue(in6) << QVariant::fromValue(in7) << QVariant::fromValue(in8);
        return asyncCallWithArgumentList(QLatin1String("processKeyEvent"), argumentList);
    }

    inline QDBusPendingReply<> setPreedit(const QString &in0, int in1)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0) << QVariant::fromValue(in1);
        return asyncCallWithArgumentList(QLatin1String("setPreedit"), argumentList);
    }

    inline QDBusPendingReply<> showInputMethod()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("showInputMethod"), argumentList);
    }

    inline QDBusPendingReply<> updateWidgetInformation(const QMap<QString, QVariant> &stateInformation, bool focusChanged)
    {
        QDBusMessage msg = QDBusMessage::createMethodCall(service(), path(), interface(), "updateWidgetInformation");

        QDBusArgument map;
        map.beginMap(QVariant::String, qMetaTypeId<QDBusVariant>());
        for (QMap<QString, QVariant>::ConstIterator it = stateInformation.constBegin(), end = stateInformation.constEnd();
             it != end; ++it) {
            map.beginMapEntry();
            map << it.key();
            map << QDBusVariant(it.value());
            map.endMapEntry();
        }
        map.endMap();

        QList<QVariant> args;
        args << QVariant::fromValue(map) << QVariant(focusChanged);
        msg.setArguments(args);
        return connection().asyncCall(msg);
    }

    inline QDBusPendingReply<> reset()
    {
        return asyncCall(QLatin1String("reset"));
    }

    inline QDBusPendingReply<> setCopyPasteState(bool in0, bool in1)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0) << QVariant::fromValue(in1);
        return asyncCallWithArgumentList(QLatin1String("setCopyPasteState"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace com {
  namespace meego {
    namespace inputmethod {
      typedef ::ComMeegoInputmethodUiserver1Interface uiserver1;
    }
  }
}
#endif
