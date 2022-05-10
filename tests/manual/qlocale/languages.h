// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LANGUAGES_H
#define LANGUAGES_H

#include <QWidget>
#include <QLocale>

class QLabel;
class QListWidget;

class LanguagesWidget : public QWidget
{
    Q_OBJECT
public:
    LanguagesWidget();

private:
    QLocale currentLocale;

    QLabel *languagesLabel;
    QListWidget *languagesList;

private slots:
    void localeChanged(QLocale locale);
};

#endif
