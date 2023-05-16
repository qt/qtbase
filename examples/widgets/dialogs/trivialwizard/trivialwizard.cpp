// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

//! [0] //! [1]
QWizardPage *createIntroPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Introduction");

    QLabel *label = new QLabel("This wizard will help you register your copy "
                               "of Super Product Two.");
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);

    return page;
}
//! [0]

//! [2]
QWizardPage *createRegistrationPage()
//! [1] //! [3]
{
//! [3]
    QWizardPage *page = new QWizardPage;
    page->setTitle("Registration");
    page->setSubTitle("Please fill both fields.");

    QLabel *nameLabel = new QLabel("Name:");
    QLineEdit *nameLineEdit = new QLineEdit;

    QLabel *emailLabel = new QLabel("Email address:");
    QLineEdit *emailLineEdit = new QLineEdit;

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(nameLabel, 0, 0);
    layout->addWidget(nameLineEdit, 0, 1);
    layout->addWidget(emailLabel, 1, 0);
    layout->addWidget(emailLineEdit, 1, 1);
    page->setLayout(layout);

    return page;
//! [4]
}
//! [2] //! [4]

//! [5] //! [6]
QWizardPage *createConclusionPage()
//! [5] //! [7]
{
//! [7]
    QWizardPage *page = new QWizardPage;
    page->setTitle("Conclusion");

    QLabel *label = new QLabel("You are now successfully registered. Have a "
                               "nice day!");
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);

    return page;
//! [8]
}
//! [6] //! [8]

//! [9] //! [10]
int main(int argc, char *argv[])
//! [9] //! [11]
{
    QApplication app(argc, argv);

#ifndef QT_NO_TRANSLATION
    QString translatorFileName = QLatin1String("qtbase_");
    translatorFileName += QLocale::system().name();
    QTranslator *translator = new QTranslator(&app);
    if (translator->load(translatorFileName, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(translator);
#endif

//! [linearAddPage]
    QWizard wizard;
    wizard.addPage(createIntroPage());
    wizard.addPage(createRegistrationPage());
    wizard.addPage(createConclusionPage());
//! [linearAddPage]

    wizard.setWindowTitle("Trivial Wizard");
    wizard.show();

    return app.exec();
}
//! [10] //! [11]
