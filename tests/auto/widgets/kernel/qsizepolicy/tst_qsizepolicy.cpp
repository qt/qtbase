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

#include <QtTest/QtTest>
#include <qsizepolicy.h>

Q_DECLARE_METATYPE(Qt::Orientations)
Q_DECLARE_METATYPE(QSizePolicy)
Q_DECLARE_METATYPE(QSizePolicy::Policy)
Q_DECLARE_METATYPE(QSizePolicy::ControlType)

class tst_QSizePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cleanup() { QVERIFY(QApplication::topLevelWidgets().isEmpty()); }
    void qtest();
    void defaultValues();
    void getSetCheck_data() { data(); }
    void getSetCheck();
    void dataStream();
    void horizontalStretch();
    void verticalStretch();
    void qhash_data() { data(); }
    void qhash();
private:
    void data() const;
};


struct PrettyPrint {
    const char *m_s;
    template <typename T>
    explicit PrettyPrint(const T &t) : m_s(Q_NULLPTR)
    {
        using QT_PREPEND_NAMESPACE(QTest)::toString;
        m_s = toString(t);
    }
    ~PrettyPrint() { delete[] m_s; }
    const char* s() const { return m_s ? m_s : "<null>" ; }
};

void tst_QSizePolicy::qtest()
{
#define CHECK(x) QCOMPARE(PrettyPrint(QSizePolicy::x).s(), #x)
    // Policy:
    CHECK(Fixed);
    CHECK(Minimum);
    CHECK(Ignored);
    CHECK(MinimumExpanding);
    CHECK(Expanding);
    CHECK(Maximum);
    CHECK(Preferred);
    // ControlType:
    CHECK(ButtonBox);
    CHECK(CheckBox);
    CHECK(ComboBox);
    CHECK(Frame);
    CHECK(GroupBox);
    CHECK(Label);
    CHECK(Line);
    CHECK(LineEdit);
    CHECK(PushButton);
    CHECK(RadioButton);
    CHECK(Slider);
    CHECK(SpinBox);
    CHECK(TabWidget);
    CHECK(ToolButton);
#undef CHECK
#define CHECK2(x, y) QCOMPARE(PrettyPrint(QSizePolicy::x|QSizePolicy::y).s(), \
                              QSizePolicy::x < QSizePolicy::y ? #x "|" #y : #y "|" #x)
    // ControlTypes (sample)
    CHECK2(ButtonBox, CheckBox);
    CHECK2(CheckBox, ButtonBox);
    CHECK2(ToolButton, Slider);
#undef CHECK2
}

void tst_QSizePolicy::defaultValues()
{
    {
        // check values of a default-constructed QSizePolicy
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
}

#define FETCH_TEST_DATA \
    QFETCH(QSizePolicy, sp); \
    QFETCH(QSizePolicy::Policy, hp); \
    QFETCH(QSizePolicy::Policy, vp); \
    QFETCH(int, hst); \
    QFETCH(int, vst); \
    QFETCH(QSizePolicy::ControlType, ct); \
    QFETCH(bool, hfw); \
    QFETCH(bool, wfh); \
    QFETCH(Qt::Orientations, ed)


// Testing get/set functions
void tst_QSizePolicy::getSetCheck()
{
    FETCH_TEST_DATA;

    QCOMPARE(QPixmap(), QPixmap());

    QCOMPARE(sp.horizontalPolicy(),    hp);
    QCOMPARE(sp.verticalPolicy(),      vp);
    QCOMPARE(sp.horizontalStretch(),   hst);
    QCOMPARE(sp.verticalStretch(),     vst);
    QCOMPARE(sp.controlType(),         ct);
    QCOMPARE(sp.hasHeightForWidth(),   hfw);
    QCOMPARE(sp.hasWidthForHeight(),   wfh);
    QCOMPARE(sp.expandingDirections(), ed);
}

static void makeRow(QSizePolicy sp, QSizePolicy::Policy hp, QSizePolicy::Policy vp,
                    int hst, int vst, QSizePolicy::ControlType ct, bool hfw, bool wfh,
                    Qt::Orientations orients)
{
    QTest::newRow(qPrintable(QString::asprintf("%s-%s-%d-%d-%s-%s-%s",
                                               PrettyPrint(hp).s(), PrettyPrint(vp).s(), hst, vst,
                                               PrettyPrint(ct).s(),
                                               hfw ? "true" : "false", wfh ? "true" : "false")))
            << sp << hp << vp << hst << vst << ct << hfw << wfh << orients;
}

void tst_QSizePolicy::data() const
{
    QTest::addColumn<QSizePolicy>("sp");
    QTest::addColumn<QSizePolicy::Policy>("hp");
    QTest::addColumn<QSizePolicy::Policy>("vp");
    QTest::addColumn<int>("hst");
    QTest::addColumn<int>("vst");
    QTest::addColumn<QSizePolicy::ControlType>("ct");
    QTest::addColumn<bool>("hfw");
    QTest::addColumn<bool>("wfh");
    QTest::addColumn<Qt::Orientations>("ed");

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

                                        Qt::Orientations orients;
                                        if (sp.horizontalPolicy() & QSizePolicy::ExpandFlag)
                                            orients |= Qt::Horizontal;
                                        if (sp.verticalPolicy() & QSizePolicy::ExpandFlag)
                                            orients |= Qt::Vertical;

                                        makeRow(sp,
                                                i >= 0 ? hp  : oldsp.horizontalPolicy(),
                                                i >= 1 ? vp  : oldsp.verticalPolicy(),
                                                i >= 2 ? hst : oldsp.horizontalStretch(),
                                                i >= 3 ? vst : oldsp.verticalStretch(),
                                                i >= 4 ? ct  : oldsp.controlType(),
                                                i >= 5 ? hfw : oldsp.hasHeightForWidth(),
                                                i >= 5 ? wfh : oldsp.hasWidthForHeight(),
                                                orients);
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
#undef ITEMCOUNT
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

void tst_QSizePolicy::qhash()
{
    FETCH_TEST_DATA;
    Q_UNUSED(ed);

    QSizePolicy sp2(hp, vp, ct);
    sp2.setVerticalStretch(vst);
    sp2.setHorizontalStretch(hst);
    if (hfw) sp2.setHeightForWidth(true);
    if (wfh) sp2.setWidthForHeight(true);
    QCOMPARE(sp, sp2);
    QCOMPARE(qHash(sp), qHash(sp2));
}

#undef FETCH_TEST_DATA

QTEST_MAIN(tst_QSizePolicy)
#include "tst_qsizepolicy.moc"
