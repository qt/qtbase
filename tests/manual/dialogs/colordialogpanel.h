// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef COLORDIALOGPANEL_H
#define COLORDIALOGPANEL_H

#include <QPointer>
#include <QColorDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

class ColorDialogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ColorDialogPanel(QWidget *parent = nullptr);

public slots:
    void execModal();
    void showModal(Qt::WindowModality modality);
    void showNonModal();
    void deleteNonModalDialog();
    void deleteModalDialog();
    void accepted();
    void rejected();
    void currentColorChanged(const QColor & color);
    void showAcceptedResult();
    void restoreDefaults();

private slots:
    void enableDeleteNonModalDialogButton();
    void enableDeleteModalDialogButton();

private:
    void applySettings(QColorDialog *d) const;

    QComboBox *m_colorComboBox;
    QCheckBox *m_showAlphaChannel;
    QCheckBox *m_noButtons;
    QCheckBox *m_dontUseNativeDialog;
    QPushButton *m_deleteNonModalDialogButton;
    QPushButton *m_deleteModalDialogButton;
    QString m_result;
    QPointer<QColorDialog> m_modalDialog;
    QPointer<QColorDialog> m_nonModalDialog;
};

#endif // COLORDIALOGPANEL_H
