/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtTest/QtTest>
#include <private/qmetaobjectbuilder_p.h>

/*
    This test makes a testlog containing lots of characters which have a special meaning in
    XML, with the purpose of exposing bugs in testlib's XML output code.
*/
class tst_BadXml : public QObject
{
    Q_OBJECT

private slots:
    void badDataTag() const;
    void badDataTag_data() const;

    void badMessage() const;
    void badMessage_data() const;

    void failWithNoFile() const;

    void encoding();

public:
    static QList<QByteArray> const& badStrings();
};

/*
    Custom metaobject to make it possible to change class name at runtime.
*/
class EmptyClass : public tst_BadXml
{ Q_OBJECT };

class tst_BadXmlSub : public tst_BadXml
{
public:
    tst_BadXmlSub()
        : className("tst_BadXml"), mo(0) {}
    ~tst_BadXmlSub() { free(mo); }

    const QMetaObject* metaObject() const;

    QByteArray className;
private:
    QMetaObject *mo;
};

const QMetaObject* tst_BadXmlSub::metaObject() const
{
    if (!mo || (mo->className() != className)) {
        free(mo);
        QMetaObjectBuilder builder(&EmptyClass::staticMetaObject);
        builder.setClassName(className);
        const_cast<tst_BadXmlSub *>(this)->mo = builder.toMetaObject();
    }
    return mo;
}

/*
    Outputs incidents and benchmark results with the current data tag set to a bad string.
*/
void tst_BadXml::badDataTag() const
{
    qDebug("a message");

    QBENCHMARK {
    }

    QFETCH(bool, shouldFail);
    if (shouldFail)
        QFAIL("a failure");
}

void tst_BadXml::badDataTag_data() const
{
    QTest::addColumn<bool>("shouldFail");

    foreach (char const* str, badStrings()) {
        QTest::newRow(qPrintable(QString("fail %1").arg(str))) << true;
        QTest::newRow(qPrintable(QString("pass %1").arg(str))) << false;
    }
}

void tst_BadXml::failWithNoFile() const
{
    QTest::qFail("failure message", 0, 0);
}

// QTBUG-35743, test whether XML is using correct UTF-8 encoding
// on platforms where the console encoding differs.
void tst_BadXml::encoding()
{
    QStringList arguments = QCoreApplication::arguments();
    arguments.pop_front(); // Prevent match on binary "badxml"
    if (arguments.filter(QStringLiteral("xml")).isEmpty())
        QSKIP("Skipped for text due to unpredictable console encoding.");
    QString string;
    string += QChar(ushort(0xDC)); // German umlaut Ue
    string += QStringLiteral("lrich ");
    string += QChar(ushort(0xDC)); // German umlaut Ue
    string += QStringLiteral("ml");
    string += QChar(ushort(0xE4)); // German umlaut ae
    string += QStringLiteral("ut");
    qDebug() << string;
}

/*
    Outputs a message containing a bad string.
*/
void tst_BadXml::badMessage() const
{
    QFETCH(QByteArray, message);
    qDebug("%s", message.constData());
}

void tst_BadXml::badMessage_data() const
{
    QTest::addColumn<QByteArray>("message");

    int i = 0;
    foreach (QByteArray const& str, badStrings()) {
        QTest::newRow(qPrintable(QString::fromLatin1("string %1").arg(i++))) << str;
    }
}

/*
    Returns a list of strings likely to expose bugs in XML output code.
*/
QList<QByteArray> const& tst_BadXml::badStrings()
{
    static QList<QByteArray> out;
    if (out.isEmpty()) {
        out << "end cdata ]]> text ]]> more text";
        out << "quotes \" text\" more text";
        out << "xml close > open < tags < text";
        out << "all > \" mixed ]]> up > \" in < the ]]> hopes < of triggering \"< ]]> bugs";
    }
    return out;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    /*
        tst_selftests can't handle multiple XML documents in a single testrun, so we'll
        decide before we begin which of our "bad strings" we want to use for our testcase
        name.
    */
    int badstring = -1;
    QVector<char const*> args;
    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "-badstring")) {
            bool ok = false;
            if (i < argc-1) {
                badstring = QByteArray(argv[i+1]).toInt(&ok);
                ++i;
            }
            if (!ok) {
                qFatal("Bad `-badstring' option");
            }
        }
        else {
            args << argv[i];
        }
    }
    /*
        We just want testlib to output a benchmark result, we don't actually care about the value,
        so just do one iteration to save time.
    */
    args << "-iterations" << "1";

    if (badstring == -1) {
        tst_BadXml test;
        return QTest::qExec(&test, args.count(), const_cast<char**>(args.data()));
    }

    QList<QByteArray> badstrings = tst_BadXml::badStrings();
    if (badstring >= badstrings.count())
        qFatal("`-badstring %d' is out of range", badstring);

    tst_BadXmlSub test;
    test.className = badstrings[badstring].constData();
    return QTest::qExec(&test, args.count(), const_cast<char**>(args.data()));
}

#include "tst_badxml.moc"
