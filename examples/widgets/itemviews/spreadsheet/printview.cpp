// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "printview.h"

#if defined(QT_PRINTSUPPORT_LIB)
#  include <QtPrintSupport/qtprintsupportglobal.h>

#  if QT_CONFIG(printer)
#    include <QPrinter>
#  endif
#endif

PrintView::PrintView()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void PrintView::print(QPrinter *printer)
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printer)
    resize(printer->width(), printer->height());
    render(printer);
#else
    Q_UNUSED(printer);
#endif
}

