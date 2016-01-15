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
#include <QStyleOption>


class tst_QStyleOption: public QObject
{
    Q_OBJECT

private slots:
    void qstyleoptioncast_data();
    void qstyleoptioncast();
};

// Just a simple container for QStyleOption-pointer
struct StyleOptionPointerBase
{
    QStyleOption *pointer;

    StyleOptionPointerBase(QStyleOption *p = 0) : pointer(p) { }

    virtual ~StyleOptionPointerBase() { pointer = 0; }
};

template <typename T>
struct StyleOptionPointer: public StyleOptionPointerBase
{
    StyleOptionPointer(T *p = 0): StyleOptionPointerBase(p) {}
    ~StyleOptionPointer() { delete static_cast<T *>(pointer); pointer = 0; }
};

Q_DECLARE_METATYPE(StyleOptionPointerBase*)

template <typename T>
inline StyleOptionPointerBase *stylePtr(T *ptr) { return new StyleOptionPointer<T>(ptr); }

void tst_QStyleOption::qstyleoptioncast_data()
{
    QTest::addColumn<StyleOptionPointerBase *>("testOption");
    QTest::addColumn<bool>("canCastToComplex");
    QTest::addColumn<int>("type");

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
    QFETCH(StyleOptionPointerBase *, testOption);
    QFETCH(bool, canCastToComplex);
    QFETCH(int, type);

    QVERIFY(testOption->pointer != 0);

    QCOMPARE(testOption->pointer->type, type);

    // Cast to common base class
    QStyleOption *castOption = qstyleoption_cast<QStyleOption*>(testOption->pointer);
    QVERIFY(castOption != 0);

    // Cast to complex base class
    castOption = qstyleoption_cast<QStyleOptionComplex*>(testOption->pointer);
    QCOMPARE(canCastToComplex, (castOption != 0));

    // Cast to combo box
    castOption = qstyleoption_cast<QStyleOptionComboBox*>(testOption->pointer);
    QCOMPARE((castOption != 0),(testOption->pointer->type == QStyleOption::SO_ComboBox));

    // Cast to button
    castOption = qstyleoption_cast<QStyleOptionButton*>(testOption->pointer);
    QCOMPARE((castOption != 0),(testOption->pointer->type == QStyleOption::SO_Button));

    // Cast to lower version
    testOption->pointer->version += 1;
    castOption = qstyleoption_cast<QStyleOption*>(testOption->pointer);
    QVERIFY(castOption);

    // Cast a null pointer
    castOption = qstyleoption_cast<QStyleOption*>((QStyleOption*)0);
    QCOMPARE(castOption,(QStyleOption*)0);

    // Deallocate
    delete testOption;
}

QTEST_MAIN(tst_QStyleOption)
#include "tst_qstyleoption.moc"

