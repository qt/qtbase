// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include <QWidget>
#include <QLocale>

class QLineEdit;
class QLabel;

class MiscWidget : public QWidget
{
    Q_OBJECT
public:
    MiscWidget();

private:
    void createLineEdit(const QString &label, QLabel **labelWidget = 0, QLineEdit **lineEditWidget = 0);

    QLabel *textToQuoteLabel;
    QLabel *standardQuotedTextLabel;
    QLabel *alternateQuotedTextLabel;
    QLabel *textDirectionLabel;
    QLabel *listToSeparatedStringLabel;
    QLineEdit *textToQuote;
    QLineEdit *standardQuotedText;
    QLineEdit *alternateQuotedText;
    QLineEdit *textDirection;
    QLineEdit *listToSeparatedStringText;

private slots:
    void localeChanged(QLocale locale);
    void updateQuotedText(QString str);
    void updateListToSeparatedStringText();
};

#endif // MISCELLANEOUS_H
