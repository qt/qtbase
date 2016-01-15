/****************************************************************************
**
** Copyright (C) 2013 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    explicit MessageBoxPanel(QWidget *parent = 0);
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
