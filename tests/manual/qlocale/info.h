// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INFO_H
#define INFO_H

#include <QWidget>
#include <QLocale>

class QLineEdit;
class QScrollArea;
class QGridLayout;

class InfoWidget : public QWidget
{
    Q_OBJECT
public:
    InfoWidget();

private:
    void addItem(const QString &label, QWidget *);
    QLineEdit *addItem(const QString &label);

    QScrollArea *scrollArea;
    QWidget *scrollAreaWidget;
    QGridLayout *layout;

    QLineEdit *name;
    QLineEdit *bcp47Name;
    QLineEdit *languageName;
    QLineEdit *nativeLanguageName;
    QLineEdit *scriptName;
    QLineEdit *territoryName;
    QLineEdit *nativeTerritoryName;

private slots:
    void localeChanged(QLocale locale);
};

#endif
