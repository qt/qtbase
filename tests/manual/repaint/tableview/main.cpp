/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include "../shared/shared.h"

class CellWidget : public QWidget
{
public:
    CellWidget (QWidget *parent = 0) : QWidget(parent) { }
    void paintEvent(QPaintEvent * event)
    {
        static int value = 200;
        value = (value + 41) % 205 + 50;
        QPainter p(this);
        p.fillRect(event->rect(), QColor::fromHsv(100, 255, value));
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTableWidget tableWidget;
//    tableWidget.setAttribute(Qt::WA_StaticContents);
    tableWidget.viewport()->setAttribute(Qt::WA_StaticContents);
    tableWidget.setRowCount(15);
    tableWidget.setColumnCount(4);
    for (int row = 0; row  < 15; ++row)
    for (int col = 0; col  < 4; ++col)
//        tableWidget.setCellWidget(row, col, new StaticWidget());
       tableWidget.setCellWidget(row, col, new CellWidget());
    tableWidget.resize(400, 600);
    tableWidget.show();
    
    
    return app.exec();
}


