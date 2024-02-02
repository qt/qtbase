// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FILEDIALOGPANEL_H
#define FILEDIALOGPANEL_H

#include <QGroupBox>
#include <QFileDialog>
#include <QPointer>

QT_BEGIN_NAMESPACE

class QAbstractFileIconProvider;
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
    explicit FileDialogPanel(QWidget *parent = nullptr);

public slots:
    void execModal();
    void showModal(Qt::WindowModality modality);
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
    QUrl currentDirectoryUrl() const;
    QString filterString() const;
    QFileDialog::Options options() const;
    QStringList allowedSchemes() const;
    void applySettings(QFileDialog *d);

    QFormLayout *filesLayout;
    QCheckBox *m_showDirsOnly;
    QCheckBox *m_readOnly;
    QCheckBox *m_confirmOverWrite;
    QCheckBox *m_nameFilterDetailsVisible;
    QCheckBox *m_resolveSymLinks;
    QCheckBox *m_native;
    QCheckBox *m_customDirIcons;
    QCheckBox *m_noIconProvider = nullptr;
    QAbstractFileIconProvider *m_origIconProvider = nullptr;

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
