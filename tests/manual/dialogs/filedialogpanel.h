/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef FILEDIALOGPANEL_H
#define FILEDIALOGPANEL_H

#include <QGroupBox>
#include <QFileDialog>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QPushButton;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QFormLayout;
QT_END_NAMESPACE
class LabelLineEdit;

class FileDialogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FileDialogPanel(QWidget *parent = 0);

public slots:
    void execModal();
    void showModal();
    void showNonModal();
    void deleteNonModalDialog();
    void deleteModalDialog();
    void getOpenFileNames();
    void getOpenFileUrls();
    void getOpenFileName();
    void getOpenFileUrl();
    void getSaveFileName();
    void getSaveFileUrl();
    void getExistingDirectory();
    void getExistingDirectoryUrl();
    void accepted();
    void showAcceptedResult();
    void restoreDefaults();

private slots:
    void enableDeleteNonModalDialogButton();
    void enableDeleteModalDialogButton();
    void useMimeTypeFilters(bool);

private:
    QString filterString() const;
    QFileDialog::Options options() const;
    QStringList allowedSchemes() const;
    void applySettings(QFileDialog *d) const;

    QFormLayout *filesLayout;
    QCheckBox *m_showDirsOnly;
    QCheckBox *m_readOnly;
    QCheckBox *m_confirmOverWrite;
    QCheckBox *m_nameFilterDetailsVisible;
    QCheckBox *m_resolveSymLinks;
    QCheckBox *m_native;
    QCheckBox *m_customDirIcons;
    QComboBox *m_acceptMode;
    QComboBox *m_fileMode;
    QComboBox *m_viewMode;
    QLineEdit *m_allowedSchemes;
    QLineEdit *m_defaultSuffix;
    QLineEdit *m_directory;
    QLineEdit *m_selectedFileName;
    QList<LabelLineEdit *> m_labelLineEdits;
    QCheckBox *m_useMimeTypeFilters;
    QPlainTextEdit *m_nameFilters;
    QLineEdit *m_selectedNameFilter;
    QPushButton *m_deleteNonModalDialogButton;
    QPushButton *m_deleteModalDialogButton;
    QString m_result;
    QPointer<QFileDialog> m_modalDialog;
    QPointer<QFileDialog> m_nonModalDialog;
};

#endif // FILEDIALOGPANEL_H
