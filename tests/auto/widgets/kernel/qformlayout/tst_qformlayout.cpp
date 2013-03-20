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
#include <qlayout.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qproxystyle.h>
#include <qsizepolicy.h>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QStyleFactory>
#include <QSharedPointer>

#include <qformlayout.h>

static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class tst_QFormLayout : public QObject
{
    Q_OBJECT

public:
    tst_QFormLayout();
    ~tst_QFormLayout();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
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
/*
    void setGeometry(const QRect &rect);
    QSize minimumSize() const;
    QSize sizeHint() const;

    bool hasHeightForWidth() const;
    int heightForWidth(int width) const;
    Qt::Orientations expandingDirections() const;
*/

    void taskQTBUG_27420_takeAtShouldUnparentLayout();

};

tst_QFormLayout::tst_QFormLayout()
{
}

tst_QFormLayout::~tst_QFormLayout()
{
}

void tst_QFormLayout::initTestCase()
{
}

void tst_QFormLayout::cleanupTestCase()
{
}

void tst_QFormLayout::init()
{
}

void tst_QFormLayout::cleanup()
{
}

void tst_QFormLayout::rowCount()
{
    QWidget *w = new QWidget;
    QFormLayout *fl = new QFormLayout(w);

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

    delete w;
}

void tst_QFormLayout::buddies()
{
    QWidget *w = new QWidget;
    QFormLayout *fl = new QFormLayout(w);

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
    QVERIFY(label2 == 0);

    //no label
    QLineEdit *le3 = new QLineEdit;
    fl->addRow(le3);
    QWidget *label3 = fl->labelForField(le3);
    QVERIFY(label3 == 0);

    //TODO: empty label?

    delete w;
}

void tst_QFormLayout::getItemPosition()
{
    QWidget *w = new QWidget;
    QFormLayout *fl = new QFormLayout(w);

    QList<QLabel*> labels;
    QList<QLineEdit*> fields;
    for (int i = 0; i < 5; ++i) {
        labels.append(new QLabel(QString("Label %1").arg(i+1)));
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

    delete w;
}

void tst_QFormLayout::wrapping()
{
    QWidget *w = new QWidget;
    QFormLayout *fl = new QFormLayout(w);
	fl->setRowWrapPolicy(QFormLayout::WrapLongRows);

    QLineEdit *le = new QLineEdit;
    QLabel *lbl = new QLabel("A long label");
    le->setMinimumWidth(200);
    fl->addRow(lbl, le);

    w->setFixedWidth(240);
    w->show();

    QCOMPARE(le->geometry().y() > lbl->geometry().y(), true);

    //TODO: additional tests covering different wrapping cases

    delete w;
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
    QWidget *w = new QWidget;
    CustomLayoutStyle *style = new CustomLayoutStyle;
    style->hspacing = 5;
    style->vspacing = 10;
    w->setStyle(style);
    QFormLayout *fl = new QFormLayout(w);
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

    delete w;
    delete style;
}

void tst_QFormLayout::contentsRect()
{
    QWidget w;
    setFrameless(&w);
    QFormLayout form;
    w.setLayout(&form);
    form.addRow("Label", new QPushButton(&w));
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

    QVERIFY(layout.labelAlignment() == Qt::AlignRight);
    QVERIFY(layout.formAlignment() == (Qt::AlignLeft | Qt::AlignTop));
    QVERIFY(layout.fieldGrowthPolicy() == QFormLayout::ExpandingFieldsGrow);
    QVERIFY(layout.rowWrapPolicy() == QFormLayout::DontWrapRows);
#endif

    widget.setStyle(QStyleFactory::create("windows"));

    QVERIFY(layout.labelAlignment() == Qt::AlignLeft);
    QVERIFY(layout.formAlignment() == (Qt::AlignLeft | Qt::AlignTop));
    QVERIFY(layout.fieldGrowthPolicy() == QFormLayout::AllNonFixedFieldsGrow);
    QVERIFY(layout.rowWrapPolicy() == QFormLayout::DontWrapRows);

    /* can't directly create mac style or qtopia style, since
       this test is cross platform.. so create dummy styles that
       return all the right stylehints.
     */
    widget.setStyle(new DummyMacStyle());

    QVERIFY(layout.labelAlignment() == Qt::AlignRight);
    QVERIFY(layout.formAlignment() == (Qt::AlignHCenter | Qt::AlignTop));
    QVERIFY(layout.fieldGrowthPolicy() == QFormLayout::FieldsStayAtSizeHint);
    QVERIFY(layout.rowWrapPolicy() == QFormLayout::DontWrapRows);

    widget.setStyle(new DummyQtopiaStyle());

    QVERIFY(layout.labelAlignment() == Qt::AlignRight);
    QVERIFY(layout.formAlignment() == (Qt::AlignLeft | Qt::AlignTop));
    QVERIFY(layout.fieldGrowthPolicy() == QFormLayout::AllNonFixedFieldsGrow);
    QVERIFY(layout.rowWrapPolicy() == QFormLayout::WrapLongRows);
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
            QVERIFY(fld1.width() == fld2.width());
            QVERIFY(fld2.width() == fld3.width());
        } else if (i == 1) {
            QVERIFY(fld1.width() == fld2.width());
            QVERIFY(fld2.width() < fld3.width());
        } else {
            QVERIFY(fld1.width() < fld2.width());
            QVERIFY(fld2.width() == fld3.width());
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
    QFormLayout layout;
    QWidget w1, w2, w3;
    QHBoxLayout l1, l2, l3;
    QLabel lbl1, lbl2;

    QCOMPARE(layout.rowCount(), 0);

    layout.addRow(&lbl1, &w1);
    layout.addRow(&lbl2, &l1);
    layout.addRow("Foo:", &w2);
    layout.addRow("Bar:", &l2);
    layout.addRow(&w3);
    layout.addRow(&l3);

    QCOMPARE(layout.rowCount(), 6);

    QVERIFY(layout.itemAt(0, QFormLayout::LabelRole)->widget() == &lbl1);
    QVERIFY(layout.itemAt(1, QFormLayout::LabelRole)->widget() == &lbl2);
    QVERIFY(layout.itemAt(2, QFormLayout::LabelRole)->widget()->property("text") == "Foo:");
    QVERIFY(layout.itemAt(3, QFormLayout::LabelRole)->widget()->property("text") == "Bar:");
    QVERIFY(layout.itemAt(4, QFormLayout::LabelRole) == 0);
    QVERIFY(layout.itemAt(5, QFormLayout::LabelRole) == 0);

    QVERIFY(layout.itemAt(0, QFormLayout::FieldRole)->widget() == &w1);
    QVERIFY(layout.itemAt(1, QFormLayout::FieldRole)->layout() == &l1);
    QVERIFY(layout.itemAt(2, QFormLayout::FieldRole)->widget() == &w2);
    QVERIFY(layout.itemAt(3, QFormLayout::FieldRole)->layout() == &l2);
//  ### should have a third role, FullRowRole?
//    QVERIFY(layout.itemAt(4, QFormLayout::FieldRole) == 0);
//    QVERIFY(layout.itemAt(5, QFormLayout::FieldRole) == 0);
}

void tst_QFormLayout::insertRow_QWidget_QWidget()
{
    QFormLayout layout;
    QLabel lbl1, lbl2, lbl3, lbl4;
    QLineEdit fld1, fld2, fld3, fld4;

    layout.insertRow(0, &lbl1, &fld1);
    QCOMPARE(layout.rowCount(), 1);

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&lbl1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&fld1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    // check that negative values append
    layout.insertRow(-2, &lbl2, &fld2);
    QCOMPARE(layout.rowCount(), 2);

    QVERIFY(layout.itemAt(0, QFormLayout::LabelRole)->widget() == &lbl1);
    QVERIFY(layout.itemAt(1, QFormLayout::LabelRole)->widget() == &lbl2);

    // check that too large values append
    layout.insertRow(100, &lbl3, &fld3);
    QCOMPARE(layout.rowCount(), 3);
    QCOMPARE(layout.count(), 6);

    layout.insertRow(3, (QWidget *)0, (QWidget *)0);
    QCOMPARE(layout.rowCount(), 4);
    QCOMPARE(layout.count(), 6);

    layout.insertRow(4, (QWidget *)0, &fld4);
    QCOMPARE(layout.rowCount(), 5);
    QCOMPARE(layout.count(), 7);

    layout.insertRow(5, &lbl4, (QWidget *)0);
    QCOMPARE(layout.rowCount(), 6);
    QCOMPARE(layout.count(), 8);

    QVERIFY(layout.itemAt(0, QFormLayout::LabelRole)->widget() == &lbl1);
    QVERIFY(layout.itemAt(1, QFormLayout::LabelRole)->widget() == &lbl2);
    QVERIFY(layout.itemAt(2, QFormLayout::LabelRole)->widget() == &lbl3);
    QVERIFY(layout.itemAt(3, QFormLayout::LabelRole) == 0);
    QVERIFY(layout.itemAt(4, QFormLayout::LabelRole) == 0);
    QVERIFY(layout.itemAt(5, QFormLayout::LabelRole)->widget() == &lbl4);

    QVERIFY(layout.itemAt(0, QFormLayout::FieldRole)->widget() == &fld1);
    QVERIFY(layout.itemAt(1, QFormLayout::FieldRole)->widget() == &fld2);
    QVERIFY(layout.itemAt(2, QFormLayout::FieldRole)->widget() == &fld3);
    QVERIFY(layout.itemAt(3, QFormLayout::FieldRole) == 0);
    QVERIFY(layout.itemAt(4, QFormLayout::FieldRole)->widget() == &fld4);
    QVERIFY(layout.itemAt(5, QFormLayout::FieldRole) == 0);
}

void tst_QFormLayout::insertRow_QWidget_QLayout()
{
    QFormLayout layout;
    QLabel lbl1, lbl2, lbl3, lbl4;
    QHBoxLayout fld1, fld2, fld3, fld4;

    layout.insertRow(0, &lbl1, &fld1);
    QCOMPARE(layout.rowCount(), 1);

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&lbl1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getLayoutPosition(&fld1, &row, &role);
        QCOMPARE(row, 0);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    // check that negative values append
    layout.insertRow(-2, &lbl2, &fld2);
    QCOMPARE(layout.rowCount(), 2);

    QVERIFY(layout.itemAt(0, QFormLayout::LabelRole)->widget() == &lbl1);
    QVERIFY(layout.itemAt(1, QFormLayout::LabelRole)->widget() == &lbl2);

    // check that too large values append
    layout.insertRow(100, &lbl3, &fld3);
    QCOMPARE(layout.rowCount(), 3);

    QVERIFY(layout.itemAt(0, QFormLayout::LabelRole)->widget() == &lbl1);
    QVERIFY(layout.itemAt(1, QFormLayout::LabelRole)->widget() == &lbl2);
    QVERIFY(layout.itemAt(2, QFormLayout::LabelRole)->widget() == &lbl3);

    QVERIFY(layout.itemAt(0, QFormLayout::FieldRole)->layout() == &fld1);
    QVERIFY(layout.itemAt(1, QFormLayout::FieldRole)->layout() == &fld2);
    QVERIFY(layout.itemAt(2, QFormLayout::FieldRole)->layout() == &fld3);
}

void tst_QFormLayout::insertRow_QString_QWidget()
{
    QFormLayout layout;
    QLineEdit fld1, fld2, fld3;

    layout.insertRow(-5, "&Name:", &fld1);
    QLabel *label1 = qobject_cast<QLabel *>(layout.itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label1 != 0);
    QVERIFY(label1->buddy() == &fld1);

    layout.insertRow(0, "&Email:", &fld2);
    QLabel *label2 = qobject_cast<QLabel *>(layout.itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label2 != 0);
    QVERIFY(label2->buddy() == &fld2);

    layout.insertRow(5, "&Age:", &fld3);
    QLabel *label3 = qobject_cast<QLabel *>(layout.itemAt(2, QFormLayout::LabelRole)->widget());
    QVERIFY(label3 != 0);
    QVERIFY(label3->buddy() == &fld3);
}

void tst_QFormLayout::insertRow_QString_QLayout()
{
    QFormLayout layout;
    QHBoxLayout fld1, fld2, fld3;

    layout.insertRow(-5, "&Name:", &fld1);
    QLabel *label1 = qobject_cast<QLabel *>(layout.itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label1 != 0);
    QVERIFY(label1->buddy() == 0);

    QCOMPARE(layout.rowCount(), 1);

    layout.insertRow(0, "&Email:", &fld2);
    QLabel *label2 = qobject_cast<QLabel *>(layout.itemAt(0, QFormLayout::LabelRole)->widget());
    QVERIFY(label2 != 0);
    QVERIFY(label2->buddy() == 0);

    QCOMPARE(layout.rowCount(), 2);

    layout.insertRow(5, "&Age:", &fld3);
    QLabel *label3 = qobject_cast<QLabel *>(layout.itemAt(2, QFormLayout::LabelRole)->widget());
    QVERIFY(label3 != 0);
    QVERIFY(label3->buddy() == 0);

    QCOMPARE(layout.rowCount(), 3);
}

void tst_QFormLayout::insertRow_QWidget()
{
    // ### come back to this later
}

void tst_QFormLayout::insertRow_QLayout()
{
    // ### come back to this later
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
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&w1, &row, &role);
        QCOMPARE(row, 5);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&w2, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&w3, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(&w4, &row, &role);
        QCOMPARE(row, -1);
        QCOMPARE(int(role), -123);
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getWidgetPosition(0, &row, &role);
        QCOMPARE(row, -1);
        QCOMPARE(int(role), -123);
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
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getLayoutPosition(&l1, &row, &role);
        QCOMPARE(row, 5);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getLayoutPosition(&l2, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::FieldRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getLayoutPosition(&l3, &row, &role);
        QCOMPARE(row, 3);
        QCOMPARE(int(role), int(QFormLayout::LabelRole));
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getLayoutPosition(&l4, &row, &role);
        QCOMPARE(row, -1);
        QCOMPARE(int(role), -123);
    }

    {
        int row = -1;
        QFormLayout::ItemRole role = QFormLayout::ItemRole(-123);
        layout.getLayoutPosition(0, &row, &role);
        QCOMPARE(row, -1);
        QCOMPARE(int(role), -123);
    }
}

void tst_QFormLayout::itemAt()
{
    QFormLayout layout;

    QWidget w1;
    QWidget w2;
    QWidget w3;
    QWidget w4;
    QWidget w5;
    QHBoxLayout l6;

    layout.setWidget(5, QFormLayout::LabelRole, &w1);
    layout.setWidget(3, QFormLayout::FieldRole, &w2);
    layout.setWidget(3, QFormLayout::LabelRole, &w3);
    layout.addRow(&w4, &w5);
    layout.addRow("Foo:", &l6);

    QCOMPARE(layout.count(), 7);

    QBitArray scoreBoard(7);
    for (int i = 0; i < 7; ++i) {
        QLayoutItem *item = layout.itemAt(i);
        QVERIFY(item != 0);

        if (item->widget() == &w1) {
            scoreBoard[0] = true;
        } else if (item->widget() == &w2) {
            scoreBoard[1] = true;
        } else if (item->widget() == &w3) {
            scoreBoard[2] = true;
        } else if (item->widget() == &w4) {
            scoreBoard[3] = true;
        } else if (item->widget() == &w5) {
            scoreBoard[4] = true;
        } else if (item->layout() == &l6) {
            scoreBoard[5] = true;
        } else if (qobject_cast<QLabel *>(item->widget())) {
            scoreBoard[6] = true;
        }
    }
    QCOMPARE(scoreBoard.count(false), 0);
}

void tst_QFormLayout::takeAt()
{
    QFormLayout layout;

    QWidget w1;
    QWidget w2;
    QWidget w3;
    QWidget w4;
    QWidget w5;
    QHBoxLayout l6;

    layout.setWidget(5, QFormLayout::LabelRole, &w1);
    layout.setWidget(3, QFormLayout::FieldRole, &w2);
    layout.setWidget(3, QFormLayout::LabelRole, &w3);
    layout.addRow(&w4, &w5);
    layout.addRow("Foo:", &l6);

    QCOMPARE(layout.count(), 7);

    for (int i = 6; i >= 0; --i) {
        layout.takeAt(0);
        QCOMPARE(layout.count(), i);
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
    w.show();
    layout.activate();
    QTest::qWait(500);
}

void tst_QFormLayout::taskQTBUG_27420_takeAtShouldUnparentLayout()
{
    QSharedPointer<QFormLayout> outer(new QFormLayout);
    QPointer<QFormLayout> inner = new QFormLayout;

    outer->addRow(inner);
    QCOMPARE(outer->count(), 1);
    QCOMPARE(inner->parent(), outer.data());

    QLayoutItem *item = outer->takeAt(0);
    QCOMPARE(item->layout(), inner.data());
    QVERIFY(!item->layout()->parent());

    outer.reset();

    if (inner)
        delete item; // success: a taken item/layout should not be deleted when the old parent is deleted
    else
        QVERIFY(!inner.isNull());
}

QTEST_MAIN(tst_QFormLayout)

#include "tst_qformlayout.moc"
