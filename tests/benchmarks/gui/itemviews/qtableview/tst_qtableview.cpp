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

#include <qtest.h>
#include <QDebug>
#include <QTableView>
#include <QImage>
#include <QPainter>
#include <QHeaderView>
#include <QStandardItemModel>

class QtTestTableModel: public QAbstractTableModel
{
    Q_OBJECT


public:
    QtTestTableModel(int rows = 0, int columns = 0, QObject *parent = 0)
        : QAbstractTableModel(parent),
          row_count(rows),
          column_count(columns) {}

    int rowCount(const QModelIndex& = QModelIndex()) const { return row_count; }
    int columnCount(const QModelIndex& = QModelIndex()) const { return column_count; }
    bool isEditable(const QModelIndex &) const { return true; }

    QVariant data(const QModelIndex &idx, int role) const
    {
        if (!idx.isValid() || idx.row() >= row_count || idx.column() >= column_count) {
            qWarning() << "Invalid modelIndex [%d,%d,%p]" << idx;
            return QVariant();
        }

        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return QString("[%1,%2,%3]").arg(idx.row()).arg(idx.column()).arg(0);

        return QVariant();
    }

    bool insertRows(int start, int count, const QModelIndex &parent = QModelIndex())
    {
        if (start < 0 || start > row_count)
            return false;

        beginInsertRows(parent, start, start + count - 1);
        row_count += count;
        endInsertRows();
        return true;
    }

    bool removeRows(int start, int count, const QModelIndex &parent = QModelIndex())
    {
        if (start < 0 || start >= row_count || row_count < count)
            return false;

        beginRemoveRows(parent, start, start + count - 1);
        row_count -= count;
        endRemoveRows();
        return true;
    }

    bool insertColumns(int start, int count, const QModelIndex &parent = QModelIndex())
    {
        if (start < 0 || start > column_count)
            return false;

        beginInsertColumns(parent, start, start + count - 1);
        column_count += count;
        endInsertColumns();
        return true;
    }

    bool removeColumns(int start, int count, const QModelIndex &parent = QModelIndex())
    {
        if (start < 0 || start >= column_count || column_count < count)
            return false;

        beginRemoveColumns(parent, start, start + count - 1);
        column_count -= count;
        endRemoveColumns();
        return true;
    }

    int row_count;
    int column_count;
};




class tst_QTableView : public QObject
{
    Q_OBJECT

public:
    tst_QTableView();
    virtual ~tst_QTableView();

public slots:
    void init();
    void cleanup();

private slots:
    void spanInit();
    void spanDraw();
    void spanSelectColumn();
    void spanSelectAll();
    void rowInsertion_data();
    void rowInsertion();
    void rowRemoval_data();
    void rowRemoval();
    void columnInsertion_data();
    void columnInsertion();
    void columnRemoval_data();
    void columnRemoval();
    void sizeHintForColumnWhenHidden();
private:
    static inline void spanInit_helper(QTableView *);
};

tst_QTableView::tst_QTableView()
{
}

tst_QTableView::~tst_QTableView()
{
}

void tst_QTableView::init()
{
}

void tst_QTableView::cleanup()
{
}

void tst_QTableView::spanInit_helper(QTableView *view)
{
    for (int i=0; i < 40; i++) {
        view->setSpan(1+i%2, 1+4*i, 1+i%3, 2);
    }

    for (int i=1; i < 40; i++) {
        view->setSpan(6 + i*7, 4, 4, 50);
    }
}

void tst_QTableView::spanInit()
{
    QtTestTableModel model(500, 500);
    QTableView v;
    v.setModel(&model);

    QBENCHMARK {
        spanInit_helper(&v);
    }
}

void tst_QTableView::spanDraw()
{
    QtTestTableModel model(500, 500);
    QTableView v;
    v.setModel(&model);

    spanInit_helper(&v);
    v.show();
    v.resize(500,500);
    QTest::qWait(30);

    QImage image(500, 500, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    QBENCHMARK {
        v.render(&painter);
    }
}

void tst_QTableView::spanSelectAll()
{
    QtTestTableModel model(500, 500);
    QTableView v;
    v.setModel(&model);

    spanInit_helper(&v);
    v.show();
    QTest::qWait(30);

    QBENCHMARK {
        v.selectAll();
    }
}

void tst_QTableView::spanSelectColumn()
{
    QtTestTableModel model(500, 500);
    QTableView v;
    v.setModel(&model);

    spanInit_helper(&v);
    v.show();
    QTest::qWait(30);

    QBENCHMARK {
        v.selectColumn(22);
    }
}

typedef QVector<QRect> SpanList;
Q_DECLARE_METATYPE(SpanList)

void spansData()
{
    QTest::addColumn<SpanList>("spans");

    QTest::newRow("Without spans")
        << SpanList();

    QTest::newRow("With spans")
            << (SpanList()
                  << QRect(0, 1, 1, 2)
                  << QRect(1, 2, 1, 2)
                  << QRect(2, 2, 1, 5)
                  << QRect(2, 8, 1, 2)
                  << QRect(3, 4, 1, 2)
                  << QRect(4, 4, 1, 4)
                  << QRect(5, 6, 1, 3)
                  << QRect(6, 7, 1, 3));
}

void tst_QTableView::rowInsertion_data()
{
    spansData();
}

void tst_QTableView::rowInsertion()
{
    QFETCH(SpanList, spans);

    QtTestTableModel model(10, 10);
    QTableView view;
    view.setModel(&model);

    foreach (QRect span, spans)
        view.setSpan(span.top(), span.left(), span.height(), span.width());
    view.show();
    QTest::qWait(50);

    QBENCHMARK_ONCE {
        view.model()->insertRows(0, 2);
        view.model()->insertRows(5, 2);
        view.model()->insertRows(8, 2);
        view.model()->insertRows(12, 2);
    }
}

void tst_QTableView::rowRemoval_data()
{
    spansData();
}

void tst_QTableView::rowRemoval()
{
    QFETCH(SpanList, spans);

    QtTestTableModel model(10, 10);
    QTableView view;
    view.setModel(&model);

    foreach (QRect span, spans)
        view.setSpan(span.top(), span.left(), span.height(), span.width());
    view.show();
    QTest::qWait(50);

    QBENCHMARK_ONCE {
        view.model()->removeRows(3, 3);
    }
}

void tst_QTableView::columnInsertion_data()
{
    spansData();
}

void tst_QTableView::columnInsertion()
{
    QFETCH(SpanList, spans);

    QtTestTableModel model(10, 10);
    QTableView view;
    view.setModel(&model);

    // Same set as for rowInsertion, just swapping columns and rows.
    foreach (QRect span, spans)
        view.setSpan(span.left(), span.top(), span.width(), span.height());
    view.show();
    QTest::qWait(50);

    QBENCHMARK_ONCE {
        view.model()->insertColumns(0, 2);
        view.model()->insertColumns(5, 2);
        view.model()->insertColumns(8, 2);
        view.model()->insertColumns(12, 2);
    }
}

void tst_QTableView::columnRemoval_data()
{
    spansData();
}

void tst_QTableView::columnRemoval()
{
    QFETCH(SpanList, spans);

    QtTestTableModel model(10, 10);
    QTableView view;
    view.setModel(&model);

    // Same set as for rowRemoval, just swapping columns and rows.
    foreach (QRect span, spans)
        view.setSpan(span.left(), span.top(), span.width(), span.height());
    view.show();
    QTest::qWait(50);

    QBENCHMARK_ONCE {
        view.model()->removeColumns(3, 3);
    }
}

void tst_QTableView::sizeHintForColumnWhenHidden()
{
    QTableView view;
    QStandardItemModel model(12500, 6);
    for (int r = 0; r < model.rowCount(); ++r)
        for (int c = 0; c < model.columnCount(); ++c) {
            QStandardItem *item = new QStandardItem(QString("row %0, column %1").arg(r).arg(c));
            model.setItem(r, c, item);
        }

    view.horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view.setModel(&model);
    QBENCHMARK_ONCE {
        view.horizontalHeader()->resizeSection(0, 10); // this force resizeSections - on a hidden view.
    }

}

QTEST_MAIN(tst_QTableView)
#include "tst_qtableview.moc"
