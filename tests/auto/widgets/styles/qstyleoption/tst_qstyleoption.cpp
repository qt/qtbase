// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QStyleOption>

#include <memory>

class tst_QStyleOption: public QObject
{
    Q_OBJECT

private slots:
    void qstyleoptioncast_data();
    void qstyleoptioncast();
};

void tst_QStyleOption::qstyleoptioncast_data()
{
    QTest::addColumn<std::shared_ptr<QStyleOption>>("testOption");
    QTest::addColumn<bool>("canCastToComplex");
    QTest::addColumn<int>("type");

    // The shared_ptr ctor is templated; will always call the correct QStyleOption dtor
    using stylePtr = std::shared_ptr<QStyleOption>;

    QTest::newRow("optionDefault") << stylePtr(new QStyleOption) << false << int(QStyleOption::SO_Default);
    QTest::newRow("optionButton") << stylePtr(new QStyleOptionButton) << false << int(QStyleOption::SO_Button);
    QTest::newRow("optionComboBox") << stylePtr(new QStyleOptionComboBox) << true << int(QStyleOption::SO_ComboBox);
    QTest::newRow("optionComplex") << stylePtr(new QStyleOptionComplex) << true << int(QStyleOption::SO_Complex);
    QTest::newRow("optionDockWidget") << stylePtr(new QStyleOptionDockWidget) << false << int(QStyleOption::SO_DockWidget);
    QTest::newRow("optionFocusRect") << stylePtr(new QStyleOptionFocusRect) << false << int(QStyleOption::SO_FocusRect);
    QTest::newRow("optionFrame") << stylePtr(new QStyleOptionFrame) << false << int(QStyleOption::SO_Frame);
    QTest::newRow("optionHeader") << stylePtr(new QStyleOptionHeader) << false << int(QStyleOption::SO_Header);
    QTest::newRow("optionMenuItem") << stylePtr(new QStyleOptionMenuItem) << false << int(QStyleOption::SO_MenuItem);
    QTest::newRow("optionProgressBar") << stylePtr(new QStyleOptionProgressBar) << false << int(QStyleOption::SO_ProgressBar);
    QTest::newRow("optionSlider") << stylePtr(new QStyleOptionSlider) << true << int(QStyleOption::SO_Slider);
    QTest::newRow("optionSpinBox") << stylePtr(new QStyleOptionSpinBox) << true << int(QStyleOption::SO_SpinBox);
    QTest::newRow("optionTab") << stylePtr(new QStyleOptionTab) << false << int(QStyleOption::SO_Tab);
    QTest::newRow("optionTitleBar") << stylePtr(new QStyleOptionTitleBar) << true << int(QStyleOption::SO_TitleBar);
    QTest::newRow("optionToolBox") << stylePtr(new QStyleOptionToolBox) << false << int(QStyleOption::SO_ToolBox);
    QTest::newRow("optionToolButton") << stylePtr(new QStyleOptionToolButton) << true << int(QStyleOption::SO_ToolButton);
    QTest::newRow("optionViewItem") << stylePtr(new QStyleOptionViewItem) << false << int(QStyleOption::SO_ViewItem);
    QTest::newRow("optionGraphicsItem") << stylePtr(new QStyleOptionGraphicsItem) << false << int(QStyleOption::SO_GraphicsItem);
}

void tst_QStyleOption::qstyleoptioncast()
{
    QFETCH(const std::shared_ptr<QStyleOption>, testOption);
    QFETCH(bool, canCastToComplex);
    QFETCH(int, type);

    QCOMPARE_NE(testOption, nullptr);

    QCOMPARE_EQ(testOption->type, type);

    // Cast to common base class
    QStyleOption *castOption = qstyleoption_cast<QStyleOption*>(testOption.get());
    QVERIFY(castOption != nullptr);

    // Cast to complex base class
    castOption = qstyleoption_cast<QStyleOptionComplex*>(testOption.get());
    QCOMPARE(canCastToComplex, (castOption != nullptr));

    // Cast to combo box
    castOption = qstyleoption_cast<QStyleOptionComboBox*>(testOption.get());
    if (castOption)
        QCOMPARE_EQ(testOption->type, QStyleOption::SO_ComboBox);
    else
        QCOMPARE_NE(testOption->type, QStyleOption::SO_ComboBox);


    // Cast to button
    castOption = qstyleoption_cast<QStyleOptionButton*>(testOption.get());
    if (castOption)
        QCOMPARE_EQ(testOption->type, QStyleOption::SO_Button);
    else
        QCOMPARE_NE(testOption->type, QStyleOption::SO_Button);

    // Cast to lower version
    testOption->version += 1;
    castOption = qstyleoption_cast<QStyleOption*>(testOption.get());
    QVERIFY(castOption);

    // Cast a null pointer
    castOption = qstyleoption_cast<QStyleOption*>((QStyleOption*)0);
    QCOMPARE(castOption, nullptr);
}

QTEST_MAIN(tst_QStyleOption)
#include "tst_qstyleoption.moc"

