/****************************************************************************
**
** Copyright (C) 2013 Canonical Limited
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qcoreapplication.h>

#include <QtTest/QtTest>
#include <QtDBus/QtDBus>
#include <QtXml/QDomDocument>

static const QString serviceName = "org.example.qdbus";
static const QString interfaceName = serviceName;

Q_DECLARE_METATYPE(QDBusConnection::RegisterOption);

class DBusXmlGenetarorObject : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.example.qdbus")
public:
    Q_INVOKABLE void nonScriptableInvokable() {}
    Q_SCRIPTABLE Q_INVOKABLE void scriptableInvokable() {}
};

class tst_QDBusXmlGenerator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void introspect_data();
    void introspect();
};

void tst_QDBusXmlGenerator::initTestCase()
{
    QDBusConnection::sessionBus().registerService(serviceName);
}

void tst_QDBusXmlGenerator::introspect_data()
{
    QTest::addColumn<QString>("methodName");
    QTest::addColumn<QDBusConnection::RegisterOption>("flags");

    QTest::newRow("scriptableInvokable") << "scriptableInvokable" << QDBusConnection::ExportScriptableInvokables;
    QTest::newRow("nonScriptableInvokable") << "nonScriptableInvokable" << QDBusConnection::ExportNonScriptableInvokables;
}

void tst_QDBusXmlGenerator::introspect()
{
    QFETCH(QString, methodName);
    QFETCH(QDBusConnection::RegisterOption, flags);
    DBusXmlGenetarorObject obj;

    QDBusConnection::sessionBus().registerObject("/" + methodName, &obj, flags );

    QDBusInterface dif(serviceName, "/" + methodName, "", QDBusConnection::sessionBus());
    QDBusReply<QString> reply = dif.call("Introspect");

    bool found = false;
    QDomDocument d;
    d.setContent(reply.value(), false);
    QDomNode n = d.documentElement().firstChild();
    while (!found && !n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if (!e.isNull()) {
            if (e.tagName() == "interface" && e.attribute("name") == interfaceName ) {
                QDomNode n2 = e.firstChild();
                while (!n2.isNull()) {
                    QDomElement e2 = n2.toElement(); // try to convert the node to an element.
                    if (!e2.isNull()) {
                        if (e2.tagName() == "method") {
                            found = e2.attribute("name") == methodName;
                        }
                    }
                    n2 = n2.nextSibling();
                }
            }
        }
        n = n.nextSibling();
    }

    QVERIFY(found);
}

QTEST_MAIN(tst_QDBusXmlGenerator)

#include "tst_qdbusxmlgenerator.moc"

