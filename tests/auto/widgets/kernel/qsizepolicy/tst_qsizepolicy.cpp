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
#include <qsizepolicy.h>

class tst_QSizePolicy : public QObject
{
Q_OBJECT

public:
    tst_QSizePolicy();
    virtual ~tst_QSizePolicy();

private slots:
    void getSetCheck();
    void dataStream();
    void horizontalStretch();
    void verticalStretch();
};

tst_QSizePolicy::tst_QSizePolicy()
{
}

tst_QSizePolicy::~tst_QSizePolicy()
{
}


// Testing get/set functions
void tst_QSizePolicy::getSetCheck()
{
    {
        // check values of a default constructed QSizePolicy
        QSizePolicy sp;
        QCOMPARE(sp.horizontalPolicy(), QSizePolicy::Fixed);
        QCOMPARE(sp.verticalPolicy(), QSizePolicy::Fixed);
        QCOMPARE(sp.horizontalStretch(), 0);
        QCOMPARE(sp.verticalStretch(), 0);
        QCOMPARE(sp.verticalStretch(), 0);
        QCOMPARE(sp.controlType(), QSizePolicy::DefaultType);
        QCOMPARE(sp.hasHeightForWidth(), false);
        QCOMPARE(sp.hasWidthForHeight(), false);
    }

    {
        static const QSizePolicy::Policy policies[3] = {
            QSizePolicy::Fixed,
            QSizePolicy::Minimum,
            QSizePolicy::Ignored
        };
        static const QSizePolicy::ControlType controlTypes[4] = {
            QSizePolicy::DefaultType,
            QSizePolicy::ButtonBox,
            QSizePolicy::CheckBox,
            QSizePolicy::ToolButton
        };

#define ITEMCOUNT(arr) int(sizeof(arr)/sizeof(arr[0]))
        QSizePolicy sp, oldsp;
#ifdef GENERATE_BASELINE
        QFile out(QString::fromAscii("qsizepolicy-Qt%1%2.txt").arg((QT_VERSION >> 16) & 0xff).arg((QT_VERSION) >> 8 & 0xff));
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QDataStream stream(&out);
#endif
            /* Loop for permutating over the values most likely to trigger a bug:
              - mininumum, maximum values
              - Some values with LSB set, others with MSB unset. (check if shifts are ok)

            */
            // Look specifically for
            for (int ihp = 0; ihp < ITEMCOUNT(policies); ++ihp) {
                QSizePolicy::Policy hp = policies[ihp];
                for (int ivp = 0; ivp < ITEMCOUNT(policies); ++ivp) {
                    QSizePolicy::Policy vp = policies[ivp];
                    for (int ict = 0; ict < ITEMCOUNT(controlTypes); ++ict) {
                        QSizePolicy::ControlType ct = controlTypes[ict];
                        for (int hst= 0; hst <= 255; hst+=85) {         //[0,85,170,255]
                            for (int vst = 0; vst <= 255; vst+=85) {
                                for (int j = 0; j < 3; ++j) {
                                    bool hfw = j & 1;
                                    bool wfh = j & 2;   // cannot set hfw and wfh at the same time
                                    oldsp = sp;
                                    for (int i = 0; i < 5; ++i) {
                                        switch (i) {
                                        case 0: sp.setHorizontalPolicy(hp); break;
                                        case 1: sp.setVerticalPolicy(vp); break;
                                        case 2: sp.setHorizontalStretch(hst); break;
                           case 3: sp.setVerticalStretch(vst); break;
                                        case 4: sp.setControlType(ct); break;
                                        case 5: sp.setHeightForWidth(hfw); sp.setWidthForHeight(wfh); break;
                                        default: break;
                                        }
                                        QCOMPARE(sp.horizontalPolicy(),  (i >= 0 ? hp  : oldsp.horizontalPolicy()));
                                        QCOMPARE(sp.verticalPolicy(),    (i >= 1 ? vp  : oldsp.verticalPolicy()));
                                        QCOMPARE(sp.horizontalStretch(), (i >= 2 ? hst : oldsp.horizontalStretch()));
                                        QCOMPARE(sp.verticalStretch(),   (i >= 3 ? vst : oldsp.verticalStretch()));
                                        QCOMPARE(sp.controlType(),       (i >= 4 ? ct  : oldsp.controlType()));
                                        QCOMPARE(sp.hasHeightForWidth(), (i >= 5 ? hfw : oldsp.hasHeightForWidth()));
                                        QCOMPARE(sp.hasWidthForHeight(), (i >= 5 ? wfh : oldsp.hasWidthForHeight()));

                                        Qt::Orientations orients;
                                        if (sp.horizontalPolicy() & QSizePolicy::ExpandFlag)
                                            orients |= Qt::Horizontal;
                                        if (sp.verticalPolicy() & QSizePolicy::ExpandFlag)
                                            orients |= Qt::Vertical;

                                        QCOMPARE(sp.expandingDirections(), orients);
#ifdef GENERATE_BASELINE
                                        stream << sp;
#endif
                                    }
                                }
                            }
                        }
                    }
                }
            }
#ifdef GENERATE_BASELINE
            out.close();
        }
#endif
    }
}

void tst_QSizePolicy::dataStream()
{
    QByteArray data;
    QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Expanding);
    {
        QDataStream stream(&data, QIODevice::ReadWrite);
        sp.setHorizontalStretch(42);
        sp.setVerticalStretch(10);
        sp.setControlType(QSizePolicy::CheckBox);
        sp.setHeightForWidth(true);

        stream << sp;   // big endian
/*
|                     BYTE 0                    |                    BYTE 1                     |
|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  | 10  | 11  | 12  | 13  | 14  | 15  |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|               Horizontal stretch              |               Vertical stretch                |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

|                     BYTE 2                    |                    BYTE 3                     |
| 16  | 17  | 18  | 19  | 20  | 21  | 22  | 23  | 24  | 25  | 26  | 27  | 28  | 29  | 30  | 31  |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
| pad | wfh |        Control Type         | hfw |    Vertical policy    |   Horizontal policy   |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
*/
        QCOMPARE((char)data[0], char(42));                  // h stretch
        QCOMPARE((char)data[1], char(10));                  // v stretch
        QCOMPARE((char)data[2], char(1 | (2 << 1)));    // (hfw + CheckBox)
        QCOMPARE((char)data[3], char(QSizePolicy::Minimum | (QSizePolicy::Expanding << 4)));
    }

    {
        QSizePolicy readSP;
        QDataStream stream(data);
        stream >> readSP;
        QCOMPARE(sp, readSP);
    }
}


void tst_QSizePolicy::horizontalStretch()
{
    QSizePolicy sp;
    sp.setHorizontalStretch(257);
    QCOMPARE(sp.horizontalStretch(), 255);
    sp.setHorizontalStretch(-2);
    QCOMPARE(sp.horizontalStretch(), 0);
}

void tst_QSizePolicy::verticalStretch()
{
    QSizePolicy sp;
    sp.setVerticalStretch(-2);
    QCOMPARE(sp.verticalStretch(), 0);
    sp.setVerticalStretch(257);
    QCOMPARE(sp.verticalStretch(), 255);
}
QTEST_MAIN(tst_QSizePolicy)
#include "tst_qsizepolicy.moc"
