/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    LicenseWizard(QWidget *parent = 0);

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
    IntroPage(QWidget *parent = 0);

    int nextId() const;

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
    EvaluatePage(QWidget *parent = 0);

    int nextId() const;

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
    RegisterPage(QWidget *parent = 0);

    int nextId() const;

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
    DetailsPage(QWidget *parent = 0);

    int nextId() const;

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
    ConclusionPage(QWidget *parent = 0);

    void initializePage();
    int nextId() const;
    void setVisible(bool visible);

private slots:
    void printButtonClicked();

private:
    QLabel *bottomLabel;
    QCheckBox *agreeCheckBox;
};
//! [6]

#endif
