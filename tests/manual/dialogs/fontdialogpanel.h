// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FONTDIALOGPANEL_H
#define FONTDIALOGPANEL_H

#include <QPointer>
#include <QFontDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPushButton;
class QFontComboBox;
class QDoubleSpinBox;
QT_END_NAMESPACE

class FontDialogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FontDialogPanel(QWidget *parent = nullptr);

public slots:
    void execModal();
    void showModal(Qt::WindowModality modality);
    void showNonModal();
    void deleteNonModalDialog();
    void deleteModalDialog();
    void accepted();
    void showAcceptedResult();
    void restoreDefaults();

private slots:
    void enableDeleteNonModalDialogButton();
    void enableDeleteModalDialogButton();

private:
    void applySettings(QFontDialog *d) const;

    QFontComboBox *m_fontFamilyBox;
    QDoubleSpinBox *m_fontSizeBox;
    QCheckBox *m_noButtons;
    QCheckBox *m_dontUseNativeDialog;
    QCheckBox *m_scalableFilter;
    QCheckBox *m_nonScalableFilter;
    QCheckBox *m_monospacedFilter;
    QCheckBox *m_proportionalFilter;
    QPushButton *m_deleteNonModalDialogButton;
    QPushButton *m_deleteModalDialogButton;
    QString m_result;
    QPointer<QFontDialog> m_modalDialog;
    QPointer<QFontDialog> m_nonModalDialog;
};

#endif // FONTDIALOGPANEL_H
