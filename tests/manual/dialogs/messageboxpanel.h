// Copyright (C) 2013 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MESSAGEBOXPANEL_H
#define MESSAGEBOXPANEL_H

#include <QWidget>
#include <QCheckBox>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QValidator;
class QLabel;
class QMessageBox;
class QCheckBox;
QT_END_NAMESPACE

class MessageBoxPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MessageBoxPanel(QWidget *parent = nullptr);
    ~MessageBoxPanel();

public slots:
    void doExec();
    void doShowApply();

private:
    QComboBox *m_iconComboBox;
    QLineEdit *m_textInMsgBox;
    QLineEdit *m_informativeText;
    QLineEdit *m_detailedtext;
    QLineEdit *m_buttonsMask;
    QPushButton *m_btnExec;
    QPushButton *m_btnShowApply;
    QValidator *m_validator;
    QLabel *m_resultLabel;
    QCheckBox *m_chkReallocMsgBox;
    QLineEdit *m_checkboxText;
    QLabel *m_checkBoxResult;
    QMessageBox *m_msgbox;
    void setupMessageBox(QMessageBox &box);
};

#endif
