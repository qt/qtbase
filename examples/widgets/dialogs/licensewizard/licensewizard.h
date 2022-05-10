// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LICENSEWIZARD_H
#define LICENSEWIZARD_H

#include <QWizard>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

//! [0] //! [1]
class LicenseWizard : public QWizard
{
//! [0]
    Q_OBJECT

public:
//! [2]
    enum { Page_Intro, Page_Evaluate, Page_Register, Page_Details,
           Page_Conclusion };
//! [2]

    LicenseWizard(QWidget *parent = nullptr);

private slots:
    void showHelp();
//! [3]
};
//! [1] //! [3]

//! [4]
class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = nullptr);

    int nextId() const override;

private:
    QLabel *topLabel;
    QRadioButton *registerRadioButton;
    QRadioButton *evaluateRadioButton;
};
//! [4]

//! [5]
class EvaluatePage : public QWizardPage
{
    Q_OBJECT

public:
    EvaluatePage(QWidget *parent = nullptr);

    int nextId() const override;

private:
    QLabel *nameLabel;
    QLabel *emailLabel;
    QLineEdit *nameLineEdit;
    QLineEdit *emailLineEdit;
};
//! [5]

class RegisterPage : public QWizardPage
{
    Q_OBJECT

public:
    RegisterPage(QWidget *parent = nullptr);

    int nextId() const override;

private:
    QLabel *nameLabel;
    QLabel *upgradeKeyLabel;
    QLineEdit *nameLineEdit;
    QLineEdit *upgradeKeyLineEdit;
};

class DetailsPage : public QWizardPage
{
    Q_OBJECT

public:
    DetailsPage(QWidget *parent = nullptr);

    int nextId() const override;

private:
    QLabel *companyLabel;
    QLabel *emailLabel;
    QLabel *postalLabel;
    QLineEdit *companyLineEdit;
    QLineEdit *emailLineEdit;
    QLineEdit *postalLineEdit;
};

//! [6]
class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = nullptr);

    void initializePage() override;
    int nextId() const override;
    void setVisible(bool visible) override;

private slots:
    void printButtonClicked();

private:
    QLabel *bottomLabel;
    QCheckBox *agreeCheckBox;
};
//! [6]

#endif
