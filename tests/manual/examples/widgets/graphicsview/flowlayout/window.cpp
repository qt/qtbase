// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include "flowlayout.h"

#include <QGraphicsProxyWidget>
#include <QLabel>

Window::Window(QGraphicsItem *parent) : QGraphicsWidget(parent, Qt::Window)
{
    FlowLayout *lay = new FlowLayout;
    const QString sentence(QLatin1String("I am not bothered by the fact that I am unknown."
                                         " I am bothered when I do not know others. (Confucius)"));
    const QList<QStringView> words = QStringView{ sentence }.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QStringView &word : words) {
        QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(this);
        QLabel *label = new QLabel(word.toString());
        label->setFrameStyle(QFrame::Box | QFrame::Plain);
        proxy->setWidget(label);
        lay->addItem(proxy);
    }
    setLayout(lay);
}
