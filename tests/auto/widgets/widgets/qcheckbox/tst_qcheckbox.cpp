// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QPixmap>
#include <QDateTime>
#include <QCheckBox>

class tst_QCheckBox : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void setChecked();
    void setCheckedSignal();
    void setTriState();
    void setText_data();
    void setText();
    void isToggleButton();
    void setDown();
    void setAutoRepeat();
    void toggle();
    void pressed();
    void toggled();
    void stateChanged();
    void foregroundRole();
    void minimumSizeHint();
};

void tst_QCheckBox::initTestCase()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");
}

// ***************************************************

void tst_QCheckBox::setChecked()
{
    QCheckBox testWidget;
    testWidget.setChecked(true);
    QVERIFY(testWidget.isChecked());
    QVERIFY(testWidget.isChecked());
    QVERIFY(testWidget.checkState() == Qt::Checked);

    testWidget.setChecked(false);
    QVERIFY(!testWidget.isChecked());
    QVERIFY(!testWidget.isChecked());
    QVERIFY(testWidget.checkState() == Qt::Unchecked);

    testWidget.setChecked(false);
    QTest::keyClick(&testWidget, ' ');
    QVERIFY(testWidget.isChecked());

    QTest::keyClick(&testWidget, ' ');
    QVERIFY(!testWidget.isChecked());
}

void tst_QCheckBox::setCheckedSignal()
{
    QCheckBox testWidget;
    testWidget.setCheckState(Qt::Unchecked);
    QSignalSpy checkStateChangedSpy(&testWidget, &QCheckBox::stateChanged);
    testWidget.setCheckState(Qt::Checked);
    testWidget.setCheckState(Qt::Checked);
    QTRY_COMPARE(checkStateChangedSpy.size(), 1);   // get signal only once
    QCOMPARE(testWidget.checkState(), Qt::Checked);
    testWidget.setCheckState(Qt::Unchecked);
    testWidget.setCheckState(Qt::Unchecked);
    QTRY_COMPARE(checkStateChangedSpy.size(), 2);   // get signal only once
    QCOMPARE(testWidget.checkState(), Qt::Unchecked);
    testWidget.setCheckState(Qt::PartiallyChecked);
    testWidget.setCheckState(Qt::PartiallyChecked);
    QTRY_COMPARE(checkStateChangedSpy.size(), 3);   // get signal only once
    QCOMPARE(testWidget.checkState(), Qt::PartiallyChecked);
}

void tst_QCheckBox::setTriState()
{
    QCheckBox testWidget;
    testWidget.setTristate(true);
    QVERIFY(testWidget.isTristate());
    QVERIFY(testWidget.checkState() == Qt::Unchecked);

    testWidget.setCheckState(Qt::PartiallyChecked);
    QVERIFY(testWidget.checkState() == Qt::PartiallyChecked);

    testWidget.setChecked(true);
    QVERIFY(testWidget.isChecked());
    QVERIFY(testWidget.checkState() == Qt::Checked);

    testWidget.setChecked(false);
    QVERIFY(!testWidget.isChecked());
    QVERIFY(testWidget.checkState() == Qt::Unchecked);

    testWidget.setCheckState(Qt::PartiallyChecked);
    QVERIFY(testWidget.checkState() == Qt::PartiallyChecked);

    testWidget.setTristate(false);
    QVERIFY(!testWidget.isTristate());

    testWidget.setCheckState(Qt::PartiallyChecked);
    QVERIFY(testWidget.checkState() == Qt::PartiallyChecked);

    testWidget.setChecked(true);
    QVERIFY(testWidget.checkState() == Qt::Checked);

    testWidget.setChecked(false);
    QVERIFY(testWidget.checkState() == Qt::Unchecked);
}

void tst_QCheckBox::setText_data()
{
    QTest::addColumn<QString>("s1");

    QTest::newRow("data0" ) << QString("This is a text");
    QTest::newRow("data1" ) << QString("A");
    QTest::newRow("data2" ) << QString("ABCDEFG ");
    QTest::newRow("data3" ) << QString("Text\nwith a cr-lf");
    QTest::newRow("data4" ) << QString("");
}

void tst_QCheckBox::setText()
{
    QCheckBox testWidget;
    QFETCH(QString, s1);
    testWidget.setText(s1);
    QCOMPARE(testWidget.text(), s1);
}

void tst_QCheckBox::setDown()
{
    QCheckBox testWidget;
    testWidget.setDown(true);
    QVERIFY(testWidget.isDown());

    testWidget.setDown(false);
    QVERIFY(!testWidget.isDown());
}

void tst_QCheckBox::setAutoRepeat()
{
    QCheckBox testWidget;
    // setAutoRepeat has no effect on toggle buttons
    testWidget.setAutoRepeat(true);
    QVERIFY(testWidget.isCheckable());
}

void tst_QCheckBox::toggle()
{
    QCheckBox testWidget;
    bool cur_state;
    cur_state = testWidget.isChecked();
    testWidget.toggle();
    QVERIFY(cur_state != testWidget.isChecked());

    cur_state = testWidget.isChecked();
    testWidget.toggle();
    QVERIFY(cur_state != testWidget.isChecked());

    cur_state = testWidget.isChecked();
    testWidget.toggle();
    QVERIFY(cur_state != testWidget.isChecked());
}

void tst_QCheckBox::pressed()
{
    QCheckBox testWidget;
    int press_count = 0;
    int release_count = 0;
    connect(&testWidget, &QCheckBox::pressed, this, [&]() { ++press_count; });
    connect(&testWidget, &QCheckBox::released, this, [&]() { ++release_count; });
    testWidget.setDown(false);
    QVERIFY(!testWidget.isChecked());

    QTest::keyPress(&testWidget, Qt::Key_Space);
    QVERIFY(press_count == 1);
    QVERIFY(release_count == 0);
    QVERIFY(!testWidget.isChecked());

    QTest::keyRelease(&testWidget, Qt::Key_Space);
    QVERIFY(press_count == 1);
    QVERIFY(release_count == 1);
    QVERIFY(testWidget.isChecked());
}

void tst_QCheckBox::toggled()
{
    QCheckBox testWidget;
    int toggle_count = 0;
    int click_count = 0;
    connect(&testWidget, &QCheckBox::toggled, this, [&]() { ++toggle_count; });
    connect(&testWidget, &QCheckBox::clicked, this, [&]() { ++click_count; });

    testWidget.toggle();
    QCOMPARE(toggle_count, 1);

    testWidget.toggle();
    QCOMPARE(toggle_count, 2);

    testWidget.toggle();
    QCOMPARE(toggle_count, 3);

    QCOMPARE(click_count, 0);
}

void tst_QCheckBox::stateChanged()
{
    QCheckBox testWidget;
    QCOMPARE(testWidget.checkState(), Qt::Unchecked);

    Qt::CheckState cur_state = Qt::Unchecked;
    QSignalSpy stateChangedSpy(&testWidget, &QCheckBox::stateChanged);
    connect(&testWidget, &QCheckBox::stateChanged, this, [&](auto state) { cur_state = Qt::CheckState(state); });
    testWidget.setChecked(true);
    QVERIFY(QTest::qWaitFor([&]() { return stateChangedSpy.size() == 1; }));
    QCOMPARE(stateChangedSpy.size(), 1);
    QCOMPARE(cur_state, Qt::Checked);
    QCOMPARE(testWidget.checkState(), Qt::Checked);

    testWidget.setChecked(false);
    QVERIFY(QTest::qWaitFor([&]() { return stateChangedSpy.size() == 2; }));
    QCOMPARE(stateChangedSpy.size(), 2);
    QCOMPARE(cur_state, Qt::Unchecked);
    QCOMPARE(testWidget.checkState(), Qt::Unchecked);

    testWidget.setCheckState(Qt::PartiallyChecked);
    QVERIFY(QTest::qWaitFor([&]() { return stateChangedSpy.size() == 3; }));
    QCOMPARE(stateChangedSpy.size(), 3);
    QCOMPARE(cur_state, Qt::PartiallyChecked);
    QCOMPARE(testWidget.checkState(), Qt::PartiallyChecked);

    testWidget.setCheckState(Qt::PartiallyChecked);
    QCoreApplication::processEvents();
    QCOMPARE(stateChangedSpy.size(), 3);
}

void tst_QCheckBox::isToggleButton()
{
    QCheckBox testWidget;
    QVERIFY(testWidget.isCheckable());
}

void tst_QCheckBox::foregroundRole()
{
    QCheckBox testWidget;
    QCOMPARE(testWidget.foregroundRole(), QPalette::WindowText);
}

void tst_QCheckBox::minimumSizeHint()
{
    QCheckBox testWidget;
    QCheckBox box(tr("CheckBox's sizeHint is the same as it's minimumSizeHint"));
    QCOMPARE(box.sizeHint(), box.minimumSizeHint());
}

QTEST_MAIN(tst_QCheckBox)
#include "tst_qcheckbox.moc"
