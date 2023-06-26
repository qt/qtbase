// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "embeddeddialog.h"
#include "ui_embeddeddialog.h"

#include <QStyleFactory>

EmbeddedDialog::EmbeddedDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EmbeddedDialog)
{
    ui->setupUi(this);
    ui->layoutDirection->setCurrentIndex(layoutDirection() != Qt::LeftToRight);

    const QStringList styleKeys = QStyleFactory::keys();
    for (const QString &styleName : styleKeys) {
        ui->style->addItem(styleName);
        if (style()->objectName().toLower() == styleName.toLower())
            ui->style->setCurrentIndex(ui->style->count() - 1);
    }

    connect(ui->layoutDirection, &QComboBox::activated,
            this, &EmbeddedDialog::layoutDirectionChanged);
    connect(ui->spacing, &QSlider::valueChanged,
            this, &EmbeddedDialog::spacingChanged);
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged,
            this, &EmbeddedDialog::fontChanged);
    connect(ui->style, &QComboBox::textActivated,
            this, &EmbeddedDialog::styleChanged);
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
    const QObjectList children = widget->children();
    for (QObject *child : children) {
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
