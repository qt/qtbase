/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    explicit FontDialogPanel(QWidget *parent = 0);

public slots:
    void execModal();
    void showModal();
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
