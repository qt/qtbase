// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spreadsheetitem.h"

QTableWidgetItem *SpreadSheetItem::clone() const
{
    SpreadSheetItem *item = new SpreadSheetItem;
    *item = *this;
    return item;
}

QVariant SpreadSheetItem::data(int role) const
{
    if (role == Qt::EditRole || role == Qt::StatusTipRole)
        return formula();

    if (role == Qt::DisplayRole)
        return display();

    const QString t = display().toString();

    if (role == Qt::ForegroundRole) {
        bool isNumber = false;
        const int number = t.toInt(&isNumber);
        QColor color = Qt::black;
        if (isNumber)
            color = (number < 0) ? Qt::red : Qt::blue;
        return QVariant::fromValue(color);
    }

    if (role == Qt::TextAlignmentRole)
        if (!t.isEmpty() && (t.at(0).isNumber() || t.at(0) == '-'))
            return int(Qt::AlignRight | Qt::AlignVCenter);

     return QTableWidgetItem::data(role);
 }

void SpreadSheetItem::setData(int role, const QVariant &value)
{
    QTableWidgetItem::setData(role, value);
    if (tableWidget())
        tableWidget()->viewport()->update();
}

QVariant SpreadSheetItem::display() const
{
    // avoid circular dependencies
    if (isResolving)
        return QVariant();

    isResolving = true;
    QVariant result = computeFormula(formula(), tableWidget(), this);
    isResolving = false;
    return result;
}

QVariant SpreadSheetItem::computeFormula(const QString &formula,
                                         const QTableWidget *widget,
                                         const QTableWidgetItem *self)
{
    // check if the string is actually a formula or not
    QStringList list = formula.split(' ');
    if (list.isEmpty() || !widget)
        return formula; // it is a normal string

    QString op = list.value(0).toLower();

    int firstRow = -1;
    int firstCol = -1;
    int secondRow = -1;
    int secondCol = -1;

    if (list.count() > 1)
        decode_pos(list.value(1), &firstRow, &firstCol);

    if (list.count() > 2)
        decode_pos(list.value(2), &secondRow, &secondCol);

    const QTableWidgetItem *start = widget->item(firstRow, firstCol);
    const QTableWidgetItem *end = widget->item(secondRow, secondCol);

    int firstVal = start ? start->text().toInt() : 0;
    int secondVal = end ? end->text().toInt() : 0;

    QVariant result;
    if (op == "sum") {
        int sum = 0;
        for (int r = firstRow; r <= secondRow; ++r) {
            for (int c = firstCol; c <= secondCol; ++c) {
                const QTableWidgetItem *tableItem = widget->item(r, c);
                if (tableItem && tableItem != self)
                    sum += tableItem->text().toInt();
            }
        }

        result = sum;
    } else if (op == "+") {
        result = (firstVal + secondVal);
    } else if (op == "-") {
        result = (firstVal - secondVal);
    } else if (op == "*") {
        result = (firstVal * secondVal);
    } else if (op == "/") {
        if (secondVal == 0)
            result = QString("nan");
        else
            result = (firstVal / secondVal);
    } else if (op == "=") {
        if (start)
            result = start->text();
    } else {
        result = formula;
    }

    return result;
}

