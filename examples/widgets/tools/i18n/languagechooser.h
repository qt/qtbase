// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LANGUAGECHOOSER_H
#define LANGUAGECHOOSER_H

#include <QDialog>
#include <QHash>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QCheckBox;
class QDialogButtonBox;
class QGroupBox;
QT_END_NAMESPACE
class MainWindow;

class LanguageChooser : public QDialog
{
    Q_OBJECT

public:
    explicit LanguageChooser(const QString &defaultLang = QString(), QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void checkBoxToggled();
    void showAll();
    void hideAll();

private:
    static QStringList findQmFiles();
    static QString languageName(const QString &qmFile);
    static QColor colorForLanguage(const QString &language);
    static bool languageMatch(QStringView lang, QStringView qmFile);

    QGroupBox *groupBox;
    QDialogButtonBox *buttonBox;
    QAbstractButton *showAllButton;
    QAbstractButton *hideAllButton;
    QHash<QCheckBox *, QString> qmFileForCheckBoxMap;
    QHash<QCheckBox *, MainWindow *> mainWindowForCheckBoxMap;
};

#endif
