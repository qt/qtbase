// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QGraphicsView>

class tst_QGraphicsLinearLayout : public QObject
{
    Q_OBJECT
public:
    tst_QGraphicsLinearLayout() {}
    ~tst_QGraphicsLinearLayout() {}

private slots:
    void heightForWidth_data();
    void heightForWidth();
};


struct MySquareWidget : public QGraphicsWidget
{
    MySquareWidget() {}
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override
    {
        if (which != Qt::PreferredSize)
            return QGraphicsWidget::sizeHint(which, constraint);
        if (constraint.width() < 0)
            return QGraphicsWidget::sizeHint(which, constraint);
        return QSizeF(constraint.width(), constraint.width());
    }
};

void tst_QGraphicsLinearLayout::heightForWidth_data()
{
    QTest::addColumn<bool>("hfw");
    QTest::addColumn<bool>("nested");

    QTest::newRow("hfw") << true << false;
    QTest::newRow("hfw, nested") << true << true;
    QTest::newRow("not hfw") << false << false;
    QTest::newRow("not hfw, nested") << false << true;
}

void tst_QGraphicsLinearLayout::heightForWidth()
{
    QFETCH(bool, hfw);
    QFETCH(bool, nested);

    QGraphicsScene scene;
    QGraphicsWidget *form = new QGraphicsWidget;
    scene.addItem(form);

    QGraphicsLinearLayout *outerlayout = 0;
    if (nested) {
       outerlayout = new QGraphicsLinearLayout(form);
       for (int i = 0; i < 8; i++) {
           QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
           outerlayout->addItem(layout);
           outerlayout = layout;
       }
    }

    QGraphicsLinearLayout *qlayout = 0;
    qlayout = new QGraphicsLinearLayout(Qt::Vertical);
    if (nested)
        outerlayout->addItem(qlayout);
    else
        form->setLayout(qlayout);

    MySquareWidget *widget = new MySquareWidget;
    for (int i = 0; i < 1; i++) {
        widget = new MySquareWidget;
        QSizePolicy sizepolicy = widget->sizePolicy();
        sizepolicy.setHeightForWidth(hfw);
        widget->setSizePolicy(sizepolicy);
        qlayout->addItem(widget);
    }
    // make sure only one iteration is done.
    // run with tst_QGraphicsLinearLayout.exe "heightForWidth" -tickcounter -iterations 6
    // this will iterate 6 times the whole test, (not only the benchmark)
    // which should reduce warmup time and give a realistic picture of the performance of
    // effectiveSizeHint()
    QSizeF constraint(hfw ? 100 : -1, -1);
    QBENCHMARK {
        (void)form->effectiveSizeHint(Qt::PreferredSize, constraint);
    }

}


QTEST_MAIN(tst_QGraphicsLinearLayout)

#include "tst_qgraphicslinearlayout.moc"
