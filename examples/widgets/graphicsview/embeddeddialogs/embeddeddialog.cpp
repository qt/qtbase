/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "embeddeddialog.h"
#include "ui_embeddeddialog.h"

#include <QStyleFactory>

EmbeddedDialog::EmbeddedDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EmbeddedDialog)
{
    ui->setupUi(this);
    ui->layoutDirection->setCurrentIndex(layoutDirection() != Qt::LeftToRight);

    foreach (QString styleName, QStyleFactory::keys()) {
        ui->style->addItem(styleName);
        if (style()->objectName().toLower() == styleName.toLower())
            ui->style->setCurrentIndex(ui->style->count() - 1);
    }

    connect(ui->layoutDirection, SIGNAL(activated(int)),
            this, SLOT(layoutDirectionChanged(int)));
    connect(ui->spacing, SIGNAL(valueChanged(int)),
            this, SLOT(spacingChanged(int)));
    connect(ui->fontComboBox, SIGNAL(currentFontChanged(QFont)),
            this, SLOT(fontChanged(QFont)));
    connect(ui->style, SIGNAL(activated(QString)),
            this, SLOT(styleChanged(QString)));
}

EmbeddedDialog::~EmbeddedDialog()
{
    delete ui;
}

void EmbeddedDialog::layoutDirectionChanged(int index)
{
    setLayoutDirection(index == 0 ? Qt::LeftToRight : Qt::RightToLeft);
}

void EmbeddedDialog::spacingChanged(int spacing)
{
    layout()->setSpacing(spacing);
    adjustSize();
}

void EmbeddedDialog::fontChanged(const QFont &font)
{
    setFont(font);
}

static void setStyleHelper(QWidget *widget, QStyle *style)
{
    widget->setStyle(style);
    widget->setPalette(style->standardPalette());
    foreach (QObject *child, widget->children()) {
        if (QWidget *childWidget = qobject_cast<QWidget *>(child))
            setStyleHelper(childWidget, style);
    }
}

void EmbeddedDialog::styleChanged(const QString &styleName)
{
    QStyle *style = QStyleFactory::create(styleName);
    if (style)
        setStyleHelper(this, style);
}
