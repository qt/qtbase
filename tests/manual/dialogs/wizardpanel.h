// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WIZARDPANEL_H
#define WIZARDPANEL_H

#include <QWidget>

class WizardStyleControl;
class WizardOptionsControl;
QT_BEGIN_NAMESPACE
class QWizard;
QT_END_NAMESPACE

class WizardPanel : public QWidget
{
    Q_OBJECT
public:
    explicit WizardPanel(QWidget *parent = nullptr);

public slots:
    void execModal();
    void showModal(Qt::WindowModality modality);
    void showNonModal();
    void showEmbedded();

private:
    void applyParameters(QWizard *wizard) const;

    WizardStyleControl *m_styleControl;
    WizardOptionsControl *m_optionsControl;
};

#endif // WIZARDPANEL_H
