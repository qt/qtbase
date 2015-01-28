/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "spreadsheetitem.h"

SpreadSheetItem::SpreadSheetItem()
        : QTableWidgetItem(), isResolving(false)
{
}

SpreadSheetItem::SpreadSheetItem(const QString &text)
        : QTableWidgetItem(text), isResolving(false)
{
}

QTableWidgetItem *SpreadSheetItem::clone() const
{
    SpreadSheetItem *item = new SpreadSheetItem();
    *item = *this;
    return item;
}

QVariant SpreadSheetItem::data(int role) const
{
    if (role == Qt::EditRole || role == Qt::StatusTipRole)
        return formula();

    if (role == Qt::DisplayRole)
        return display();

    QString t = display().toString();
    bool isNumber = false;
    int number = t.toInt(&isNumber);

    if (role == Qt::TextColorRole) {
        if (!isNumber)
            return QVariant::fromValue(QColor(Qt::black));
        else if (number < 0)
            return QVariant::fromValue(QColor(Qt::red));
        return QVariant::fromValue(QColor(Qt::blue));
    }

     if (role == Qt::TextAlignmentRole)
         if (!t.isEmpty() && (t.at(0).isNumber() || t.at(0) == '-'))
             return (int)(Qt::AlignRight | Qt::AlignVCenter);

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
    // check if the s tring is actually a formula or not
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

