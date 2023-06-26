// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef STYLESHEETEDITOR_H
#define STYLESHEETEDITOR_H

#include <QDialog>

#include "ui_stylesheeteditor.h"

class StyleSheetEditor : public QDialog
{
    Q_OBJECT

public:
    StyleSheetEditor(QWidget *parent = nullptr);

private slots:
    void setStyleName(const QString &styleName);
    void setStyleSheetName(const QString &styleSheetName);
    void setModified();
    void apply();

private:
    void loadStyleSheet(const QString &sheetName);

    Ui::StyleSheetEditor ui;
};

#endif
