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
#include <qlayout.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qproxystyle.h>
#include <qsizepolicy.h>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <private/qdialog_p.h>

#include <QStyleFactory>
#include <QSharedPointer>

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

#include <qformlayout.h>

// ItemRole has enumerators for numerical values 0..2, thus the only
// valid numerical values for storing into an ItemRole variable are 0..3:
Q_CONSTEXPR QFormLayout::ItemRole invalidRole = QFormLayout::ItemRole(3);

struct QFormLayoutTakeRowResultHolder {
    QFormLayoutTakeRowResultHolder(QFormLayout::TakeRowResult result) noexcept
        : labelItem(result.labelItem),
          fieldItem(result.fieldItem)
    {
    }
    ~QFormLayoutTakeRowResultHolder()
    {
        // re-use a QFormLayout to recursively reap the QLayoutItems:
        QFormLayout disposer;
        if (labelItem)
            disposer.setItem(0, QFormLayout::LabelRole, labelItem);
        if (fieldItem)
            disposer.setItem(0, QFormLayout::FieldRole, fieldItem);
    }
    QFormLayoutTakeRowResultHolder(QFormLayoutTakeRowResultHolder &&other) noexcept
        : labelItem(other.labelItem),
          fieldItem(other.fieldItem)
    {
        other.labelItem = nullptr;
        other.fieldItem = nullptr;
    }
    QFormLayoutTakeRowResultHolder &operator=(QFormLayoutTakeRowResultHolder &&other) noexcept
    {
        swap(other);
        return *this;
    }

    void swap(QFormLayoutTakeRowResultHolder &other) noexcept
    {
        qSwap(labelItem, other.labelItem);
        qSwap(fieldItem, other.fieldItem);
    }

    QLayoutItem *labelItem;
    QLayoutItem *fieldItem;
};

class tst_QFormLayout : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void rowCount();
    void buddies();
    void getItemPosition();
    void wrapping();
    void spacing();
    void contentsRect();

    void setFormStyle();
    void setFieldGrowthPolicy();
    void setRowWrapPolicy();
    void setLabelAlignment();
    void setFormAlignment();

/*
    void setHorizontalSpacing(int spacing);
    int horizontalSpacing() const;
    void setVerticalSpacing(int spacing);
    int verticalSpacing() const;
*/

    void addRow();
    void insertRow_QWidget_QWidget();
    void insertRow_QWidget_QLayout();
    void insertRow_QString_QWidget();
    void insertRow_QString_QLayout();
    void insertRow_QWidget();
    void insertRow_QLayout();
    void removeRow();
    void removeRow_QWidget();
    void removeRow_QLayout();
    void takeRow();
    void takeRow_QWidget();
    void takeRow_QLayout();
    void setWidget();
    void setLayout();

/*
    QLayoutItem *itemAt(int row, ItemRole role) const;
    void getItemPosition(int index, int *rowPtr, ItemRole *rolePtr) const;
    void getLayoutPosition(QWidget *widget, int *rowPtr, ItemRole *rolePtr) const;
    void getItemPosition(QLayoutItem *item, int *rowPtr, ItemRole *rolePtr) const;
    QWidget *labelForField(QWidget *widget) const;
    QWidget *labelForField(QLayoutItem *item) const;

    void addItem(QLayoutItem *item);
*/

    void itemAt();
    void takeAt();
    void layoutAlone();
    void replaceWidget();
/*
    void setGeometry(const QRect &rect);
    QSize minimumSize() const;
    QSize sizeHint() const;

    bool hasHeightForWidth() const;
    int heightForWidth(int width) const;
    Qt::Orientations expandingDirections() const;
*/

    void taskQTBUG_27420_takeAtShouldUnparentLayout();
    void taskQTBUG_40609_addingWidgetToItsOwnLayout();
    void taskQTBUG_40609_addingLayoutToItself();

};

void tst_QFormLayout::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QFormLayout::rowCount()
{
    QWidget w;
    QFormLayout *fl = new QFormLayout(&w);

    fl->addRow(tr("Label 1"), new QLineEdit);
    fl->addRow(tr("Label 2"), new QLineEdit);
    fl->addRow(tr("Label 3"), new QLineEdit);
    QCOMPARE(fl->rowCount(), 3);

    fl->addRow(new QWidget);
    fl->addRow(new QHBoxLayout);
    QCOMPARE(fl->rowCount(), 5);

    fl->insertRow(1, tr("Label 0.5"), new QLineEdit);
    QCOMPARE(fl->rowCount(), 6);

    //TODO: remove items
}

void tst_QFormLayout::buddies()
{
    QWidget w;
    QFormLayout *fl = new QFormLayout(&w);

    //normal buddy case
    QLineEdit *le = new QLineEdit;
    fl->addRow(tr("Label 1"), le);
    QLabel *label = qobject_cast<QLabel *>(fl->labelForField(le));
    QVERIFY(label);
    QWidget *lew = le;
    QCOMPARE(label->buddy(), lew);

    //null label
    QLineEdit *le2 = new QLineEdit;
    fl->addRow(0, le2);
    QWidget *label2 = fl->labelForField(le2);
    QVERIFY(!label2);

    //no label
    QLineEdit *le3 = new QLineEdit;
    fl->addRow(le3);
    QWidget *label3 = fl->labelForField(le3);
    QVERIFY(!label3);

    //TODO: empty label?
}

void tst_QFormLayout::getItemPosition()
{
    QWidget w;
    QFormLayout *fl = new QFormLayout(&w);

    QList<QLabel*> labels;
    QList<QLineEdit*> fields;
    for (int i = 0; i < 5; ++i) {
        labels.append(new QLabel(QLatin1String("Label " ) + QString::number(i + 1)));
        fields.append(new QLineEdit);
        fl->addRow(labels[i], fields[i]);
    }

    //a field
    {
        int row;
        QFormLayout::ItemRole role;
        fl->getWidgetPosition(fields[3], &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(role, QFormLayout::FieldRole);
    }

    //a label
    {
        int row;
        QFormLayout::ItemRole role;
        fl->getWidgetPosition(labels[2], &row, &role);
        QCOMPARE(row, 2);
        QCOMPARE(role, QFormLayout::LabelRole);
    }

    //a layout that's been inserted
    {
        QVBoxLayout *vbl = new QVBoxLayout;
        fl->insertRow(2, "Label 1.5", vbl);
        int row;
        QFormLayout::ItemRole role;
        fl->getLayoutPosition(vbl, &row, &role);
        QCOMPARE(row, 2);
        QCOMPARE(role, QFormLayout::FieldRole);
    }
}

void tst_QFormLayout::wrapping()
{
    QWidget w;
    QFormLayout *fl = new QFormLayout(&w);
    fl->setRowWrapPolicy(QFormLayout::WrapLongRows);

    QLineEdit *le = new QLineEdit;
    QLabel *lbl = new QLabel("A long label");
    le->setMinimumWidth(200);
    fl->addRow(lbl, le);

    w.setFixedWidth(240);
    w.setWindowTitle(QTest::currentTestFunction());
    w.show();

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "setFixedWidth does not work on WinRT", Abort);
#endif
    QCOMPARE(le->geometry().y() > lbl->geometry().y(), true);

    //TODO: additional tests covering different wrapping cases
}

class CustomLayoutStyle : public QProxyStyle
{
    Q_OBJECT
public:
    CustomLayoutStyle() : QProxyStyle(QStyleFactory::create("windows"))
    {
        hspacing = 5;
        vspacing = 10;
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0,
                            const QWidget * widget = 0 ) const;

    int hspacing;
    int vspacing;
};

int CustomLayoutStyle::pixelMetric(PixelMetric metric, const QStyleOption * option /*= 0*/,
                                   const QWidget * widget /*= 0*/ ) const
{
    switch (metric) {
        case PM_LayoutHorizontalSpacing:
            return hspacing;
        case PM_LayoutVerticalSpacing:
            return vspacing;
        break;
        default:
            break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void tst_QFormLayout::spacing()
{
    //TODO: confirm spacing behavior
    QWidget w;
    QScopedPointer<CustomLayoutStyle> style(new CustomLayoutStyle);
    style->hspacing = 5;
    style->vspacing = 10;
    w.setStyle(style.data());
    QFormLayout *fl = new QFormLayout(&w);
    QCOMPARE(style->hspacing, fl->horizontalSpacing());
    QCOMPARE(style->vspacing, fl->verticalSpacing());

    //QCOMPARE(fl->spacing(), -1);
    fl->setVerticalSpacing(5);
    QCOMPARE(5, fl->horizontalSpacing());
    QCOMPARE(5, fl->verticalSpacing());
    //QCOMPARE(fl->spacing(), 5);
    fl->setVerticalSpacing(-1);
    QCOMPARE(style->hspacing, fl->horizontalSpacing());
    QCOMPARE(style->vspacing, fl->verticalSpacing());

    style->hspacing = 5;
    style->vspacing = 5;
    //QCOMPARE(fl->spacing(), 5);

    fl->setHorizontalSpacing(20);
    //QCOMPARE(fl->spacing(), -1);
    style->vspacing = 20;
    QCOMPARE(fl->horizontalSpacing(), 20);
    QCOMPARE(fl->verticalSpacing(), 20);
    //QCOMPARE(fl->spacing(), 20);
    fl->setHorizontalSpacing(-1);
    //QCOMPARE(fl->spacing(), -1);
    style->hspacing = 20;
    //QCOMPARE(fl->spacing(), 20);



    // Do not assert if spacings are negative (QTBUG-34731)
    style->vspacing = -1;
    style->hspacing = -1;
    QLabel *label = new QLabel(tr("Asserts"));
    QCheckBox *checkBox = new QCheckBox(tr("Yes"));
    fl->setWidget(0, QFormLayout::LabelRole, label);
    fl->setWidget(1, QFormLayout::FieldRole, checkBox);
    w.resize(200, 100);
    w.setWindowTitle(QTest::currentTestFunction());
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
}

void tst_QFormLayout::contentsRect()
{
    QWidget w;
    setFrameless(&w);
    QFormLayout form;
    w.setLayout(&form);
    form.addRow("Label", new QPushButton(&w));
    w.setWindowTitle(QTest::currentTestFunction());
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    int l, t, r, b;
    form.getContentsMargins(&l, &t, &r, &b);
    QRect geom = form.geometry();

    QCOMPARE(geom.adjusted(+l, +t, -r, -b), form.contentsRect());
}


class DummyMacStyle : public QCommonStyle
{
public:
    virtual int styleHint ( StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0 ) const
    {
        switch(hint) {
            case SH_FormLayoutFormAlignment:
                return Qt::AlignHCenter | Qt::AlignTop;
            case SH_FormLayoutLabelAlignment:
                return Qt::AlignRight;
            case SH_FormLayoutWrapPolicy:
                return QFormLayout::DontWrapRows;
            case SH_FormLayoutFieldGrowthPolicy:
                return QFormLayout::FieldsStayAtSizeHint;
            default:
                return QCommonStyle::styleHint(hint, option, widget, returnData);
        }
    }
};

class DummyQtopiaStyle : public QCommonStyle
{
public:
    virtual int styleHint ( StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0 ) const
    {
        switch(hint) {
            case SH_FormLayoutFormAlignment:
                return Qt::AlignLeft | Qt::AlignTop;
            case SH_FormLayoutLabelAlignment:
                return Qt::AlignRight;
            case SH_FormLayoutWrapPolicy:
                return QFormLayout::WrapLongRows;
            case SH_FormLayoutFieldGrowthPolicy:
                return QFormLayout::AllNonFixedFieldsGrow;
            default:
                return QCommonStyle::styleHint(hint, option, widget, returnData);
        }
    }
};

void tst_QFormLayout::setFormStyle()
{
    QWidget widget;
    QFormLayout layout;
    widget.setLayout(&layout);

#if 0 // QT_NO_STYLE_PLASTIQUE
    widget.setStyle(new QPlastiqueStyle());

    QCOMPARE(layout.labelAlignment(), Qt::AlignRight);
    QVERIFY(layout.formAlignment() == (Qt::AlignLeft | Qt::AlignTop));
    QCOMPARE(layout.fieldGrowthPolicy(), QFormLayout::ExpandingFieldsGrow);
    QCOMPARE(layout.rowWrapPolicy(), QFormLayout::DontWrapRows);
#endif

    const QScopedPointer<QStyle> windowsStyle(QStyleFactory::create("windows"));
    widget.setStyle(windowsStyle.data());

    QCOMPARE(layout.labelAlignment(), Qt::AlignLeft);
    QVERIFY(layout.formAlignment() == (Qt::AlignLeft | Qt::AlignTop));
    QCOMPARE(layout.fieldGrowthPolicy(), QFormLayout::AllNonFixedFieldsGrow);
    QCOMPARE(layout.rowWrapPolicy(), QFormLayout::DontWrapRows);

    /* can't directly create mac style or qtopia style, since
       this test is cross platform.. so create dummy styles that
       return all the right stylehints.
     */
    DummyMacStyle macStyle;
    widget.setStyle(&macStyle);

    QCOMPARE(layout.labelAlignment(), Qt::AlignRight);
    QVERIFY(layout.formAlignment() == (Qt::AlignHCenter | Qt::AlignTop));
    QCOMPARE(layout.fieldGrowthPolicy(), QFormLayout::FieldsStayAtSizeHint);
    QCOMPARE(layout.rowWrapPolicy(), QFormLayout::DontWrapRows);

    DummyQtopiaStyle qtopiaStyle;
    widget.setStyle(&qtopiaStyle);

    QCOMPARE(layout.labelAlignment(), Qt::AlignRight);
    QVERIFY(layout.formAlignment() == (Qt::AlignLeft | Qt::AlignTop));
    QCOMPARE(layout.fieldGrowthPolicy(), QFormLayout::AllNonFixedFieldsGrow);
    QCOMPARE(layout.rowWrapPolicy(), QFormLayout::WrapLongRows);
}

void tst_QFormLayout::setFieldGrowthPolicy()
{
    QWidget window;
    QLineEdit fld1, fld2, fld3;
    fld1.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fld2.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    fld3.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QFormLayout layout;
    layout.addRow("One:", &fld1);
    layout.addRow("Two:", &fld2);
    layout.addRow("Three:", &fld3);
    window.setLayout(&layout);
    window.resize(1000, 200);

    for (int i = 0; i < 3; ++i) {
        layout.setFieldGrowthPolicy(i == 0 ? QFormLayout::FieldsStayAtSizeHint :
                                    i == 1 ? QFormLayout::ExpandingFieldsGrow :
                                             QFormLayout::AllNonFixedFieldsGrow);
        layout.activate();

        if (i == 0) {
            QCOMPARE(fld1.width(), fld2.width());
            QCOMPARE(fld2.width(), fld3.width());
        } else if (i == 1) {
            QCOMPARE(fld1.width(), fld2.width());
            QVERIFY(fld2.width() < fld3.width());
        } else {
            QVERIFY(fld1.width() < fld2.width());
            QCOMPARE(fld2.width(), fld3.width());
        }
    }
}

void tst_QFormLayout::setRowWrapPolicy()
{
}

void tst_QFormLayout::setLabelAlignment()
{
}

void tst_QFormLayout::setFormAlignment()
{
}

void tst_QFormLayout::addRow()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);
    QWidget *w1 = new QWidget(&topLevel);
    QWidget *w2 = new QWidget(&topLevel);
    QWidget *w3 = new QWidget(&topLevel);
    QHBoxLayout *l1 = new QHBoxLayout;
    QHBoxLayout *l2 = new QHBoxLayout;
    QHBoxLayout *l3 = new QHBoxLayout;
    QLabel *lbl1 = new QLabel(&topLevel);
    QLabel *lbl2 = new QLabel(&topLevel);

    QCOMPARE(layout->rowCount(), 0);

    layout->addRow(lbl1, w1);
    layout->addRow(lbl2, l1);
    layout->addRow("Foo:", w2);
    layout->addRow("Bar:", l2);
    layout->addRow(w3);
    layout->addRow(l3);

    QCOMPARE(layout->rowCount(), 6);

    QVERIFY(layout->itemAt(0, QFormLayout::LabelRole)->widget() == lbl1);
    QVERIFY(layout->itemAt(1, QFormLayout::LabelRole)->widget() == lbl2);
    QCOMPARE(layout->itemAt(2, QFormLayout::LabelRole)->widget()->property("text").toString(), QLatin1String("Foo:"));
    QCOMPARE(layout->itemAt(3, QFormLayout::LabelRole)->widget()->property("text").toString(), QLatin1String("Bar:"));
    QVERIFY(layout->itemAt(4, QFormLayout::LabelRole) == 0);
    QVERIFY(layout->itemAt(5, QFormLayout::LabelRole) == 0);

    QVERIFY(layout->itemAt(0, QFormLayout::FieldRole)->widget() == w1);
    QVERIFY(layout->itemAt(1, QFormLayout::FieldRole)->layout() == l1);
    QVERIFY(layout->itemAt(2, QFormLayout::FieldRole)->widget() == w2);
    QVERIFY(layout->itemAt(3, QFormLayout::FieldRole)->layout() == l2);
//  ### should have a third role, FullRowRole?
//    QVERIFY(layout.itemAt(4, QFormLayout::FieldRole) == 0);
//    QVERIFY(layout.itemAt(5, QFormLayout::FieldRole) == 0);
}

void tst_QFormLayout::insertRow_QWidget_QWidget()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);
    QLabel *lbl1 = new QLabel(&topLevel);
    QLabel *lbl2 = new QLabel(&topLevel);
    QLabel *lbl3 = new QLabel(&topLevel);
    QLabel *lbl4 = new QLabel(&topLevel);
    QLineEdit *fld1 = new QLineEdit(&topLevel);
    QLineEdit *fld2 = new QLineEdit(&topLevel);
    QLineEdit *fld3 = new QLineEdit(&topLevel);
    QLineEdit *fld4 = new QLineEdit(&topLevel);

    layout->insertRow(0, lbl1, fld1);
    QCOMPARE(layout->rowCount(), 1);

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout->getWidgetPosition(lbl1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout->getWidgetPosition(fld1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    // check that negative values append
    layout->insertRow(-2, lbl2, fld2);
    QCOMPARE(layout->rowCount(), 2);

    QVERIFY(layout->itemAt(0, QFormLayout::LabelRole)->widget() == lbl1);
    QVERIFY(layout->itemAt(1, QFormLayout::LabelRole)->widget() == lbl2);

    // check that too large values append
    layout->insertRow(100, lbl3, fld3);
    QCOMPARE(layout->rowCount(), 3);
    QCOMPARE(layout->count(), 6);

    layout->insertRow(3, (QWidget *)0, (QWidget *)0);
    QCOMPARE(layout->rowCount(), 4);
    QCOMPARE(layout->count(), 6);

    layout->insertRow(4, (QWidget *)0, fld4);
    QCOMPARE(layout->rowCount(), 5);
    QCOMPARE(layout->count(), 7);

    layout->insertRow(5, lbl4, (QWidget *)0);
    QCOMPARE(layout->rowCount(), 6);
    QCOMPARE(layout->count(), 8);

    QVERIFY(layout->itemAt(0, QFormLayout::LabelRole)->widget() == lbl1);
    QVERIFY(layout->itemAt(1, QFormLayout::LabelRole)->widget() == lbl2);
    QVERIFY(layout->itemAt(2, QFormLayout::LabelRole)->widget() == lbl3);
    QVERIFY(layout->itemAt(3, QFormLayout::LabelRole) == 0);
    QVERIFY(layout->itemAt(4, QFormLayout::LabelRole) == 0);
    QVERIFY(layout->itemAt(5, QFormLayout::LabelRole)->widget() == lbl4);

    QVERIFY(layout->itemAt(0, QFormLayout::FieldRole)->widget() == fld1);
    QVERIFY(layout->itemAt(1, QFormLayout::FieldRole)->widget() == fld2);
    QVERIFY(layout->itemAt(2, QFormLayout::FieldRole)->widget() == fld3);
    QVERIFY(layout->itemAt(3, QFormLayout::FieldRole) == 0);
    QVERIFY(layout->itemAt(4, QFormLayout::FieldRole)->widget() == fld4);
    QVERIFY(layout->itemAt(5, QFormLayout::FieldRole) == 0);
}

void tst_QFormLayout::insertRow_QWidget_QLayout()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);
    QLabel *lbl1 = new QLabel(&topLevel);
    QLabel *lbl2 = new QLabel(&topLevel);
    QLabel *lbl3 = new QLabel(&topLevel);
    QHBoxLayout *fld1 = new QHBoxLayout;
    QHBoxLayout *fld2 = new QHBoxLayout;
    QHBoxLayout *fld3 = new QHBoxLayout;

    layout->insertRow(0, lbl1, fld1);
    QCOMPARE(layout->rowCount(), 1);

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout->getWidgetPosition(lbl1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout->getLayoutPosition(fld1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    // check that negative values append
    layout->insertRow(-2, lbl2, fld2);
    QCOMPARE(layout->rowCount(), 2);

    QVERIFY(layout->itemAt(0, QFormLayout::LabelRole)->widget() == lbl1);
    QVERIFY(layout->itemAt(1, QFormLayout::LabelRole)->widget() == lbl2);

    // check that too large values append
    layout->insertRow(100, lbl3, fld3);
    QCOMPARE(layout->rowCount(), 3);

    QVERIFY(layout->itemAt(0, QFormLayout::LabelRole)->widget() == lbl1);
    QVERIFY(layout->itemAt(1, QFormLayout::LabelRole)->widget() == lbl2);
    QVERIFY(layout->itemAt(2, QFormLayout::LabelRole)->widget() == lbl3);

    QVERIFY(layout->itemAt(0, QFormLayout::FieldRole)->layout() == fld1);
    QVERIFY(layout->itemAt(1, QFormLayout::FieldRole)->layout() == fld2);
    QVERIFY(layout->itemAt(2, QFormLayout::FieldRole)->layout() == fld3);
}

void tst_QFormLayout::insertRow_QString_QWidget()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);
    QLineEdit *fld1 = new QLineEdit(&topLevel);
    QLineEdit *fld2 = new QLineEdit(&topLevel);
    QLineEdit *fld3 = new QLineEdit(&topLevel);

    layout->insertRow(-5, "&Name:", fld1);
    QLabel *label1 = qobject_cast<QLabel *>(layout->itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label1 != 0);
    QCOMPARE(label1->buddy(), fld1);

    layout->insertRow(0, "&Email:", fld2);
    QLabel *label2 = qobject_cast<QLabel *>(layout->itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label2 != 0);
    QCOMPARE(label2->buddy(), fld2);

    layout->insertRow(5, "&Age:", fld3);
    QLabel *label3 = qobject_cast<QLabel *>(layout->itemAt(2, QFormLayout::LabelRole)->widget());
    QVERIFY(label3 != 0);
    QCOMPARE(label3->buddy(), fld3);
}

void tst_QFormLayout::insertRow_QString_QLayout()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);
    QHBoxLayout *fld1 = new QHBoxLayout;
    QHBoxLayout *fld2 = new QHBoxLayout;
    QHBoxLayout *fld3 = new QHBoxLayout;

    layout->insertRow(-5, "&Name:", fld1);
    QLabel *label1 = qobject_cast<QLabel *>(layout->itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label1 != 0);
    QVERIFY(!label1->buddy());

    QCOMPARE(layout->rowCount(), 1);

    layout->insertRow(0, "&Email:", fld2);
    QLabel *label2 = qobject_cast<QLabel *>(layout->itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label2 != 0);
    QVERIFY(!label2->buddy());

    QCOMPARE(layout->rowCount(), 2);

    layout->insertRow(5, "&Age:", fld3);
    QLabel *label3 = qobject_cast<QLabel *>(layout->itemAt(2, QFormLayout::LabelRole)->widget());
    QVERIFY(label3 != 0);
    QVERIFY(!label3->buddy());

    QCOMPARE(layout->rowCount(), 3);
}

void tst_QFormLayout::insertRow_QWidget()
{
    // ### come back to this later
}

void tst_QFormLayout::insertRow_QLayout()
{
    // ### come back to this later
}

void tst_QFormLayout::removeRow()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QPointer<QWidget> w1 = new QWidget;
    QPointer<QWidget> w2 = new QWidget;

    layout->addRow("test1", w1);
    layout->addRow(w2);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->rowCount(), 2);

    layout->removeRow(1);

    QVERIFY(w1);
    QVERIFY(!w2);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->rowCount(), 1);

    layout->removeRow(0);

    QVERIFY(!w1);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);
}

void tst_QFormLayout::removeRow_QWidget()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QPointer<QWidget> w1 = new QWidget;
    QPointer<QWidget> w2 = new QWidget;

    layout->addRow("test1", w1);
    layout->addRow(w2);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->rowCount(), 2);

    layout->removeRow(w1);

    QVERIFY(!w1);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->rowCount(), 1);

    layout->removeRow(w2);

    QVERIFY(!w2);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QWidget *w3 = new QWidget;
    layout->removeRow(w3);
    delete w3;
}

void tst_QFormLayout::removeRow_QLayout()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QPointer<QHBoxLayout> l1 = new QHBoxLayout;
    QPointer<QWidget> w1 = new QWidget;
    l1->addWidget(w1);
    QPointer<QHBoxLayout> l2 = new QHBoxLayout;
    QPointer<QWidget> w2 = new QWidget;
    l2->addWidget(w2);

    layout->addRow("test1", l1);
    layout->addRow(l2);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->rowCount(), 2);

    layout->removeRow(l1);

    QVERIFY(!l1);
    QVERIFY(!w1);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->rowCount(), 1);

    layout->removeRow(l2);

    QVERIFY(!l2);
    QVERIFY(!w2);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QHBoxLayout *l3 = new QHBoxLayout;
    layout->removeRow(l3);
    delete l3;
}

void tst_QFormLayout::takeRow()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QPointer<QWidget> w1 = new QWidget;
    QPointer<QWidget> w2 = new QWidget;

    layout->addRow("test1", w1);
    layout->addRow(w2);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->rowCount(), 2);

    QFormLayoutTakeRowResultHolder result = layout->takeRow(1);

    QVERIFY(w2);
    QVERIFY(result.fieldItem);
    QVERIFY(!result.labelItem);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->rowCount(), 1);
    QCOMPARE(result.fieldItem->widget(), w2.data());

    result = layout->takeRow(0);

    QVERIFY(w1);
    QVERIFY(result.fieldItem);
    QVERIFY(result.labelItem);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);
    QCOMPARE(result.fieldItem->widget(), w1.data());

    result = layout->takeRow(0);

    QVERIFY(!result.fieldItem);
    QVERIFY(!result.labelItem);
}

void tst_QFormLayout::takeRow_QWidget()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QPointer<QWidget> w1 = new QWidget;
    QPointer<QWidget> w2 = new QWidget;

    layout->addRow("test1", w1);
    layout->addRow(w2);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->rowCount(), 2);

    QFormLayoutTakeRowResultHolder result = layout->takeRow(w1);

    QVERIFY(w1);
    QVERIFY(result.fieldItem);
    QVERIFY(result.labelItem);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->rowCount(), 1);

    result = layout->takeRow(w2);

    QVERIFY(w2);
    QVERIFY(result.fieldItem);
    QVERIFY(!result.labelItem);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QWidget *w3 = new QWidget;
    result = layout->takeRow(w3);
    delete w3;

    QVERIFY(!result.fieldItem);
    QVERIFY(!result.labelItem);
}

void tst_QFormLayout::takeRow_QLayout()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QPointer<QHBoxLayout> l1 = new QHBoxLayout;
    QPointer<QWidget> w1 = new QWidget;
    l1->addWidget(w1);
    QPointer<QHBoxLayout> l2 = new QHBoxLayout;
    QPointer<QWidget> w2 = new QWidget;
    l2->addWidget(w2);

    layout->addRow("test1", l1);
    layout->addRow(l2);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->rowCount(), 2);

    QFormLayoutTakeRowResultHolder result = layout->takeRow(l1);

    QVERIFY(l1);
    QVERIFY(w1);
    QVERIFY(result.fieldItem);
    QVERIFY(result.labelItem);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->rowCount(), 1);

    result = layout->takeRow(l2);

    QVERIFY(l2);
    QVERIFY(w2);
    QVERIFY(result.fieldItem);
    QVERIFY(!result.labelItem);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->rowCount(), 0);

    QHBoxLayout *l3 = new QHBoxLayout;
    result = layout->takeRow(l3);
    delete l3;

    QVERIFY(!result.fieldItem);
    QVERIFY(!result.labelItem);
}

void tst_QFormLayout::setWidget()
{
    QFormLayout layout;

    QWidget w1;
    QWidget w2;
    QWidget w3;
    QWidget w4;

    QCOMPARE(layout.count(), 0);
    QCOMPARE(layout.rowCount(), 0);

    layout.setWidget(5, QFormLayout::LabelRole, &w1);
    QCOMPARE(layout.count(), 1);
    QCOMPARE(layout.rowCount(), 6);

    layout.setWidget(3, QFormLayout::FieldRole, &w2);
    layout.setWidget(3, QFormLayout::LabelRole, &w3);
    QCOMPARE(layout.count(), 3);
    QCOMPARE(layout.rowCount(), 6);

    // should be ignored and generate warnings
    layout.setWidget(3, QFormLayout::FieldRole, &w4);
    layout.setWidget(-1, QFormLayout::FieldRole, &w4);

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getWidgetPosition(&w1, &row, &role);
        QCOMPARE(row, 5);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getWidgetPosition(&w2, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getWidgetPosition(&w3, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getWidgetPosition(&w4, &row, &role);
        // not found
        QCOMPARE(row, -1);
        QCOMPARE(int(role), int(invalidRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getWidgetPosition(0, &row, &role);
        // not found
        QCOMPARE(row, -1);
        QCOMPARE(int(role), int(invalidRole));
    }
}

void tst_QFormLayout::setLayout()
{
    QFormLayout layout;

    QHBoxLayout l1;
    QHBoxLayout l2;
    QHBoxLayout l3;
    QHBoxLayout l4;

    QCOMPARE(layout.count(), 0);
    QCOMPARE(layout.rowCount(), 0);

    layout.setLayout(5, QFormLayout::LabelRole, &l1);
    QCOMPARE(layout.count(), 1);
    QCOMPARE(layout.rowCount(), 6);

    layout.setLayout(3, QFormLayout::FieldRole, &l2);
    layout.setLayout(3, QFormLayout::LabelRole, &l3);
    QCOMPARE(layout.count(), 3);
    QCOMPARE(layout.rowCount(), 6);

    // should be ignored and generate warnings
    layout.setLayout(3, QFormLayout::FieldRole, &l4);
    layout.setLayout(-1, QFormLayout::FieldRole, &l4);
    QCOMPARE(layout.count(), 3);
    QCOMPARE(layout.rowCount(), 6);

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getLayoutPosition(&l1, &row, &role);
        QCOMPARE(row, 5);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getLayoutPosition(&l2, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getLayoutPosition(&l3, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getLayoutPosition(&l4, &row, &role);
        QCOMPARE(row, -1);
        QCOMPARE(int(role), int(invalidRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = invalidRole;
        layout.getLayoutPosition(0, &row, &role);
        QCOMPARE(row, -1);
        QCOMPARE(int(role), int(invalidRole));
    }
}

void tst_QFormLayout::itemAt()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QWidget *w1 = new QWidget(&topLevel);
    QWidget *w2 = new QWidget(&topLevel);
    QWidget *w3 = new QWidget(&topLevel);
    QWidget *w4 = new QWidget(&topLevel);
    QWidget *w5 = new QWidget(&topLevel);
    QHBoxLayout *l6 = new QHBoxLayout;

    layout->setWidget(5, QFormLayout::LabelRole, w1);
    layout->setWidget(3, QFormLayout::FieldRole, w2);
    layout->setWidget(3, QFormLayout::LabelRole, w3);
    layout->addRow(w4, w5);
    layout->addRow("Foo:", l6);

    QCOMPARE(layout->count(), 7);

    QBitArray scoreBoard(7);
    for (int i = 0; i < 7; ++i) {
        QLayoutItem *item = layout->itemAt(i);
        QVERIFY(item != 0);

        if (item->widget() == w1) {
            scoreBoard[0] = true;
        } else if (item->widget() == w2) {
            scoreBoard[1] = true;
        } else if (item->widget() == w3) {
            scoreBoard[2] = true;
        } else if (item->widget() == w4) {
            scoreBoard[3] = true;
        } else if (item->widget() == w5) {
            scoreBoard[4] = true;
        } else if (item->layout() == l6) {
            scoreBoard[5] = true;
        } else if (qobject_cast<QLabel *>(item->widget())) {
            scoreBoard[6] = true;
        }
    }
    QCOMPARE(scoreBoard.count(false), 0);
}

void tst_QFormLayout::takeAt()
{
    QWidget topLevel;
    QFormLayout *layout = new QFormLayout(&topLevel);

    QWidget *w1 = new QWidget(&topLevel);
    QWidget *w2 = new QWidget(&topLevel);
    QWidget *w3 = new QWidget(&topLevel);
    QWidget *w4 = new QWidget(&topLevel);
    QWidget *w5 = new QWidget(&topLevel);
    QHBoxLayout *l6 = new QHBoxLayout;

    layout->setWidget(5, QFormLayout::LabelRole, w1);
    layout->setWidget(3, QFormLayout::FieldRole, w2);
    layout->setWidget(3, QFormLayout::LabelRole, w3);
    layout->addRow(w4, w5);
    layout->addRow("Foo:", l6);

    QCOMPARE(layout->count(), 7);

    for (int i = 6; i >= 0; --i) {
        delete layout->takeAt(0);
        QCOMPARE(layout->count(), i);
    }
}

void tst_QFormLayout::layoutAlone()
{
    QWidget w;
    QFormLayout layout;
    layout.setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    w.setLayout(&layout);
    QLabel label("Here is a strange test case");
    layout.setWidget(0, QFormLayout::LabelRole, &label);
    QHBoxLayout hlay;
    layout.setLayout(1, QFormLayout::LabelRole, &hlay);
    QCOMPARE(layout.count(), 2);
    w.setWindowTitle(QTest::currentTestFunction());
    w.show();
    layout.activate();
}

void tst_QFormLayout::taskQTBUG_27420_takeAtShouldUnparentLayout()
{
    QSharedPointer<QFormLayout> outer(new QFormLayout);
    QAutoPointer<QFormLayout> holder{new QFormLayout};
    auto inner = holder.get();

    outer->addRow(inner);
    QCOMPARE(outer->count(), 1);
    QCOMPARE(inner->parent(), outer.data());

    QLayoutItem *item = outer->takeAt(0);
    QCOMPARE(item->layout(), inner);
    QVERIFY(!item->layout()->parent());

    outer.reset();

    QVERIFY(holder); // a taken item/layout should not be deleted when the old parent is deleted
}

void tst_QFormLayout::taskQTBUG_40609_addingWidgetToItsOwnLayout(){
    QWidget widget;
    widget.setObjectName("6435cbada60548b4522cbb6");
    QFormLayout layout(&widget);
    layout.setObjectName("c03c0e22c0b6d019a93a248");

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add parent widget QWidget/6435cbada60548b4522cbb6 to its child layout QFormLayout/c03c0e22c0b6d019a93a248");
    layout.addRow(QLatin1String("48c81f39b7320082f8"), &widget);
    QCOMPARE(layout.count(), 0);
}

void tst_QFormLayout::taskQTBUG_40609_addingLayoutToItself(){
    QWidget widget;
    widget.setObjectName("2bc425637d084c07ce65956");
    QFormLayout layout(&widget);
    layout.setObjectName("60e31de0c8800eaba713a4f2");

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add layout QFormLayout/60e31de0c8800eaba713a4f2 to itself");
    layout.addRow(QLatin1String("9a2cd4f40c06b489f889"), &layout);
    QCOMPARE(layout.count(), 0);
}

void tst_QFormLayout::replaceWidget()
{
    QWidget w;
    QFormLayout *layout = new QFormLayout();
    w.setLayout(layout);
    QLineEdit *edit1 = new QLineEdit();
    QLineEdit *edit2 = new QLineEdit();
    QLineEdit *edit3 = new QLineEdit();
    QLabel *label1 = new QLabel();
    QLabel *label2 = new QLabel();

    layout->addRow("Label", edit1);
    layout->addRow(label1, edit2);

    // Verify controls not in layout
    QCOMPARE(layout->indexOf(edit3), -1);
    QCOMPARE(layout->indexOf(label2), -1);

    // Verify controls in layout
    int editIndex = layout->indexOf(edit1);
    int labelIndex = layout->indexOf(label1);
    QVERIFY(editIndex > 0);
    QVERIFY(labelIndex > 0);
    int rownum;
    QFormLayout::ItemRole role;

    // replace editor
    delete layout->replaceWidget(edit1, edit3);
    edit1->hide(); // Not strictly needed for the test, but for normal usage it is.
    QCOMPARE(layout->indexOf(edit1), -1);
    QCOMPARE(layout->indexOf(edit3), editIndex);
    QCOMPARE(layout->indexOf(label1), labelIndex);
    rownum = -1;
    role = QFormLayout::SpanningRole;
    layout->getWidgetPosition(edit3, &rownum, &role);
    QCOMPARE(rownum, 0);
    QCOMPARE(role, QFormLayout::FieldRole);

    delete layout->replaceWidget(label1, label2);
    label1->hide();
    QCOMPARE(layout->indexOf(label1), -1);
    QCOMPARE(layout->indexOf(label2), labelIndex);
    layout->getWidgetPosition(label2, &rownum, &role);
    QCOMPARE(rownum, 1);
    QCOMPARE(role, QFormLayout::LabelRole);

}

QTEST_MAIN(tst_QFormLayout)

#include "tst_qformlayout.moc"
