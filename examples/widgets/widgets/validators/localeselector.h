// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LOCALESELECTOR_H
#define LOCALESELECTOR_H

#include <QComboBox>

class LocaleSelector : public QComboBox
{
    Q_OBJECT

public:
    LocaleSelector(QWidget *parent = nullptr);

signals:
    void localeSelected(const QLocale &locale);

private slots:
    void emitLocaleSelected(int index);
};

#endif //LOCALESELECTOR_H
