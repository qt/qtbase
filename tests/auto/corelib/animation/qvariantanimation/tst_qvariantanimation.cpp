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


#include <QtCore/qvariantanimation.h>
#include <QtTest>

class tst_QVariantAnimation : public QObject
{
    Q_OBJECT
private slots:
    void construction();
    void destruction();
    void currentValue();
    void easingCurve();
    void startValue();
    void endValue();
    void keyValueAt();
    void keyValues();
    void duration();
    void interpolation();
};

class TestableQVariantAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    void updateCurrentValue(const QVariant&) {}
};

void tst_QVariantAnimation::construction()
{
    TestableQVariantAnimation anim;
}

void tst_QVariantAnimation::destruction()
{
    TestableQVariantAnimation *anim = new TestableQVariantAnimation;
    delete anim;
}

void tst_QVariantAnimation::currentValue()
{
    TestableQVariantAnimation anim;
    QVERIFY(!anim.currentValue().isValid());
}

void tst_QVariantAnimation::easingCurve()
{
    TestableQVariantAnimation anim;
    QCOMPARE(anim.easingCurve().type(), QEasingCurve::Linear);
    anim.setEasingCurve(QEasingCurve::InQuad);
    QCOMPARE(anim.easingCurve().type(), QEasingCurve::InQuad);
}

void tst_QVariantAnimation::endValue()
{
    TestableQVariantAnimation anim;
    anim.setEndValue(QVariant(1));
    QCOMPARE(anim.endValue().toInt(), 1);
}

void tst_QVariantAnimation::startValue()
{
    TestableQVariantAnimation anim;
    anim.setStartValue(QVariant(1));
    QCOMPARE(anim.startValue().toInt(), 1);
    anim.setStartValue(QVariant(-1));
    QCOMPARE(anim.startValue().toInt(), -1);
}

void tst_QVariantAnimation::keyValueAt()
{
    TestableQVariantAnimation anim;

    int i=0;
    for (qreal r=0.0; r<1.0; r+=0.1) {
        anim.setKeyValueAt(0.1, ++i);
        QCOMPARE(anim.keyValueAt(0.1).toInt(), i);
    }
}

void tst_QVariantAnimation::keyValues()
{
    TestableQVariantAnimation anim;

    QVariantAnimation::KeyValues values;
    int i=0;
    for (qreal r=0.0; r<1.0; r+=0.1) {
        values.append(QVariantAnimation::KeyValue(r, i));
    }

    anim.setKeyValues(values);
    QCOMPARE(anim.keyValues(), values);
}

void tst_QVariantAnimation::duration()
{
    TestableQVariantAnimation anim;
    QCOMPARE(anim.duration(), 250);
    anim.setDuration(500);
    QCOMPARE(anim.duration(), 500);
    QTest::ignoreMessage(QtWarningMsg, "QVariantAnimation::setDuration: cannot set a negative duration");
    anim.setDuration(-1);
    QCOMPARE(anim.duration(), 500);
}

void tst_QVariantAnimation::interpolation()
{
    QVariantAnimation unsignedAnim;
    unsignedAnim.setStartValue(100u);
    unsignedAnim.setEndValue(0u);
    unsignedAnim.setDuration(100);
    unsignedAnim.setCurrentTime(50);
    QCOMPARE(unsignedAnim.currentValue().toUInt(), 50u);

    QVariantAnimation signedAnim;
    signedAnim.setStartValue(100);
    signedAnim.setEndValue(0);
    signedAnim.setDuration(100);
    signedAnim.setCurrentTime(50);
    QCOMPARE(signedAnim.currentValue().toInt(), 50);

    QVariantAnimation pointAnim;
    pointAnim.setStartValue(QPoint(100, 100));
    pointAnim.setEndValue(QPoint(0, 0));
    pointAnim.setDuration(100);
    pointAnim.setCurrentTime(50);
    QCOMPARE(pointAnim.currentValue().toPoint(), QPoint(50, 50));
}

QTEST_MAIN(tst_QVariantAnimation)

#include "tst_qvariantanimation.moc"
