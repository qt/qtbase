/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
