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
    QVERIFY(anim.easingCurve() == QEasingCurve::Linear);
    anim.setEasingCurve(QEasingCurve::InQuad);
    QVERIFY(anim.easingCurve() == QEasingCurve::InQuad);
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

QTEST_MAIN(tst_QVariantAnimation)

#include "tst_qvariantanimation.moc"
