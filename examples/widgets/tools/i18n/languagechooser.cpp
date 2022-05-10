// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "languagechooser.h"
#include "mainwindow.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QTranslator>

LanguageChooser::LanguageChooser(const QString &defaultLang, QWidget *parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint)
{
    groupBox = new QGroupBox("Languages");

    QGridLayout *groupBoxLayout = new QGridLayout;

    const QStringList qmFiles = findQmFiles();
    for (int i = 0; i < qmFiles.size(); ++i) {
        const QString &qmlFile = qmFiles.at(i);
        QCheckBox *checkBox = new QCheckBox(languageName(qmlFile));
        qmFileForCheckBoxMap.insert(checkBox, qmlFile);
        connect(checkBox, &QCheckBox::toggled,
                this, &LanguageChooser::checkBoxToggled);
        if (languageMatch(defaultLang, qmlFile))
            checkBox->setCheckState(Qt::Checked);
        groupBoxLayout->addWidget(checkBox, i / 2, i % 2);
    }
    groupBox->setLayout(groupBoxLayout);

    buttonBox = new QDialogButtonBox;
    showAllButton = buttonBox->addButton("Show All",
                                         QDialogButtonBox::ActionRole);
    hideAllButton = buttonBox->addButton("Hide All",
                                         QDialogButtonBox::ActionRole);

    connect(showAllButton, &QAbstractButton::clicked, this, &LanguageChooser::showAll);
    connect(hideAllButton, &QAbstractButton::clicked, this, &LanguageChooser::hideAll);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle("I18N");
}

bool LanguageChooser::languageMatch(QStringView lang, QStringView qmFile)
{
    //qmFile: i18n_xx.qm
    const QStringView prefix{ u"i18n_" };
    const int langTokenLength = 2; /*FIXME: is checking two chars enough?*/
    return qmFile.mid(qmFile.indexOf(prefix) + prefix.length(), langTokenLength) == lang.left(langTokenLength);
}

bool LanguageChooser::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        MainWindow *window = qobject_cast<MainWindow *>(object);
        if (window) {
            QCheckBox *checkBox = mainWindowForCheckBoxMap.key(window);
            if (checkBox)
                checkBox->setChecked(false);
        }
    }
    return QDialog::eventFilter(object, event);
}

void LanguageChooser::closeEvent(QCloseEvent * /* event */)
{
    QCoreApplication::quit();
}

void LanguageChooser::checkBoxToggled()
{
    QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());
    MainWindow *window = mainWindowForCheckBoxMap.value(checkBox);
    if (!window) {
        QTranslator translator;
        const QString qmlFile = qmFileForCheckBoxMap.value(checkBox);
        if (translator.load(qmlFile))
            QCoreApplication::installTranslator(&translator);
        else
            qWarning("Unable to load %s", qPrintable(QDir::toNativeSeparators(qmlFile)));

        window = new MainWindow;
        window->setPalette(colorForLanguage(checkBox->text()));

        window->installEventFilter(this);
        mainWindowForCheckBoxMap.insert(checkBox, window);
    }
    window->setVisible(checkBox->isChecked());
}

void LanguageChooser::showAll()
{
    for (auto it = qmFileForCheckBoxMap.keyBegin(); it != qmFileForCheckBoxMap.keyEnd(); ++it)
        (*it)->setChecked(true);
}

void LanguageChooser::hideAll()
{
    for (auto it = qmFileForCheckBoxMap.keyBegin(); it != qmFileForCheckBoxMap.keyEnd(); ++it)
        (*it)->setChecked(false);
}

QStringList LanguageChooser::findQmFiles()
{
    QDir dir(":/translations");
    QStringList fileNames = dir.entryList(QStringList("*.qm"), QDir::Files,
                                          QDir::Name);
    for (QString &fileName : fileNames)
        fileName = dir.filePath(fileName);
    return fileNames;
}

QString LanguageChooser::languageName(const QString &qmFile)
{
    QTranslator translator;
    if (!translator.load(qmFile)) {
        qWarning("Unable to load %s", qPrintable(QDir::toNativeSeparators(qmFile)));
        return {};
    }
    return translator.translate("MainWindow", "English");
}

QColor LanguageChooser::colorForLanguage(const QString &language)
{
    size_t hashValue = qHash(language);
    int red = 156 + (hashValue & 0x3F);
    int green = 156 + ((hashValue >> 6) & 0x3F);
    int blue = 156 + ((hashValue >> 12) & 0x3F);
    return QColor(red, green, blue);
}
