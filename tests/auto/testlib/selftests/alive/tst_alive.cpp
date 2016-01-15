/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

QTEST_MAIN(tst_Alive)
#include "tst_alive.moc"
