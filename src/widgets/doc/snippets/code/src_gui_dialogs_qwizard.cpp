// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
int LicenseWizard::nextId() const
{
    switch (currentId()) {
    case Page_Intro:
        if (field("intro.evaluate").toBool()) {
            return Page_Evaluate;
        } else {
            return Page_Register;
        }
    case Page_Evaluate:
        return Page_Conclusion;
    case Page_Register:
        if (field("register.upgradeKey").toString().isEmpty()) {
            return Page_Details;
        } else {
            return Page_Conclusion;
        }
    case Page_Details:
        return Page_Conclusion;
    case Page_Conclusion:
    default:
        return -1;
    }
}
//! [0]


//! [1]
MyWizard::MyWizard(QWidget *parent)
    : QWizard(parent)
{
    ...
    QList<QWizard::WizardButton> layout;
    layout << QWizard::Stretch << QWizard::BackButton << QWizard::CancelButton
           << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(layout);
    ...
}
//! [1]
