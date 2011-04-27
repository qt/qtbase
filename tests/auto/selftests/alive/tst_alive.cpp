/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QWidget>

#include "qtestalive.cpp"

class tst_Alive: public QObject
{
    Q_OBJECT

private slots:
    void alive();
    void addMouseDClick() const;
    void compareQStringLists() const;
    void compareQStringLists_data() const;
};

void tst_Alive::alive()
{
    QTestAlive a;
    a.start();

    sleep(5);
    QCoreApplication::processEvents();
    qDebug("CUT");
    sleep(5);
}

void tst_Alive::addMouseDClick() const
{
    class DClickListener : public QWidget
    {
    public:
        DClickListener() : isTested(false)
        {
        }

        bool isTested;
    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent * event)
        {
            isTested = true;
            QCOMPARE(event->type(), QEvent::MouseButtonDblClick);
        }
    };

    DClickListener listener;

    QTestEventList list;
    list.addMouseDClick(Qt::LeftButton);

    list.simulate(&listener);
    /* Check that we have been called at all. */
    QVERIFY(listener.isTested);
}

void tst_Alive::compareQStringLists() const
{
    QFETCH(QStringList, opA);
    QFETCH(QStringList, opB);

    QCOMPARE(opA, opB);
}

void tst_Alive::compareQStringLists_data() const
{
    QTest::addColumn<QStringList>("opA");
    QTest::addColumn<QStringList>("opB");

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));

        QStringList opB(opA);
        opA.append(QLatin1String("string3"));
        opB.append(QLatin1String("DIFFERS"));

        QTest::newRow("") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));

        QStringList opB(opA);
        opA.append(QLatin1String("string3"));
        opA.append(QLatin1String("string4"));

        opB.append(QLatin1String("DIFFERS"));
        opB.append(QLatin1String("string4"));

        QTest::newRow("") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("string1"));
        opA.append(QLatin1String("string2"));

        QStringList opB;
        opB.append(QLatin1String("string1"));

        QTest::newRow("") << opA << opB;
    }

    {
        QStringList opA;
        opA.append(QLatin1String("openInNewWindow"));
        opA.append(QLatin1String("openInNewTab"));
        opA.append(QLatin1String("separator"));
        opA.append(QLatin1String("bookmark_add"));
        opA.append(QLatin1String("savelinkas"));
        opA.append(QLatin1String("copylinklocation"));
        opA.append(QLatin1String("separator"));
        opA.append(QLatin1String("openWith_submenu"));
        opA.append(QLatin1String("preview1"));
        opA.append(QLatin1String("actions_submenu"));
        opA.append(QLatin1String("separator"));
        opA.append(QLatin1String("viewDocumentSource"));

        QStringList opB;
        opB.append(QLatin1String("viewDocumentSource"));

        QTest::newRow("") << opA << opB;

        QTest::newRow("") << opB << opA;
    }
}

QTEST_MAIN(tst_Alive)
#include "tst_alive.moc"
