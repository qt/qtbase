// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCOLORWELL_P_H
#define QCOLORWELL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qrect.h>
#include <QtWidgets/qwidget.h>

QT_REQUIRE_CONFIG(colordialog);

QT_BEGIN_NAMESPACE

class QWellArray : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int selectedColumn READ selectedColumn)
    Q_PROPERTY(int selectedRow READ selectedRow)

public:
    QWellArray(int rows, int cols, QWidget *parent = nullptr);
    ~QWellArray() { }

    int currentColumn() const { return curCol; }
    int currentRow() const { return curRow; }

    int selectedColumn() const { return selCol; }
    int selectedRow() const { return selRow; }

    virtual void setCurrent(int row, int col);
    virtual void setSelected(int row, int col);

    QSize sizeHint() const override;

    inline int cellWidth() const { return cellw; }

    inline int cellHeight() const { return cellh; }

    inline int rowAt(int y) const { return y / cellh; }

    inline int columnAt(int x) const
    {
        if (isRightToLeft())
            return ncols - (x / cellw) - 1;
        return x / cellw;
    }

    inline int rowY(int row) const { return cellh * row; }

    inline int columnX(int column) const
    {
        if (isRightToLeft())
            return cellw * (ncols - column - 1);
        return cellw * column;
    }

    inline int numRows() const { return nrows; }

    inline int numCols() const { return ncols; }

    inline int index(int row, int col) { return col * nrows + row; }

    inline QRect cellRect() const { return QRect(0, 0, cellw, cellh); }

    inline QSize gridSize() const { return QSize(ncols * cellw, nrows * cellh); }

    QRect cellGeometry(int row, int column)
    {
        QRect r;
        if (row >= 0 && row < nrows && column >= 0 && column < ncols)
            r.setRect(columnX(column), rowY(row), cellw, cellh);
        return r;
    }

    inline void updateCell(int row, int column) { update(cellGeometry(row, column)); }

signals:
    void selected(int row, int col);
    void currentChanged(int row, int col);
    void colorChanged(int index, QRgb color);

protected:
    virtual void paintCell(QPainter *, int row, int col, const QRect &);
    virtual void paintCellContents(QPainter *, int row, int col, const QRect &);

    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void paintEvent(QPaintEvent *) override;

    void sendAccessibleChildFocusEvent();

private:
    Q_DISABLE_COPY(QWellArray)

    int nrows;
    int ncols;
    int cellw;
    int cellh;
    int curRow;
    int curCol;
    int selRow;
    int selCol;
};

class QColorWell : public QWellArray
{
    Q_OBJECT
public:
    QColorWell(QWidget *parent, int r, int c, const QRgb *vals)
        : QWellArray(r, c, parent), values(vals), mousePressed(false), oldCurrent(-1, -1)
    {
        setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    }

    const QRgb *rgbValues() { return values; }

protected:
    void paintCellContents(QPainter *, int row, int col, const QRect &) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
#if QT_CONFIG(draganddrop)
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
#endif

private:
    const QRgb *values;
    bool mousePressed;
    QPoint pressPos;
    QPoint oldCurrent;
};

QT_END_NAMESPACE

#endif // QCOLORWELL_P_H
