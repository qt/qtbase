/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qapplication.h>
#include <QtCore/QSet>
#include <QtCore/QTranslator>
#include <private/qthread_p.h>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QDesktopWidget>


//TESTED_CLASS=
//TESTED_FILES=

class tst_languageChange : public QObject
{
    Q_OBJECT
public:
    tst_languageChange();

public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void retranslatability_data();
    void retranslatability();

};


tst_languageChange::tst_languageChange()

{
}

void tst_languageChange::initTestCase()
{
}

void tst_languageChange::cleanupTestCase()
{
}
/**
 * Records all calls to translate()
 */
class TransformTranslator : public QTranslator
{
    Q_OBJECT
public:
    TransformTranslator() : QTranslator() {}
    TransformTranslator(QObject *parent) : QTranslator(parent) {}
    virtual QString translate(const char *context, const char *sourceText, const char *comment = 0) const
    {
        QByteArray total(context);
        total.append("::");
        total.append(sourceText);
        if (comment) {
            total.append("::");
            total.append(comment);
        }
        m_translations.insert(total);
        QString res;
        for (int i = 0; i < int(qstrlen(sourceText)); ++i) {
            QChar ch = QLatin1Char(sourceText[i]);
            if (ch.isLower()) {
                res.append(ch.toUpper());
            } else if (ch.isUpper()) {
                res.append(ch.toLower());
            } else {
                res.append(ch);
            }
        }
        return res;
    }

    virtual bool isEmpty() const { return false; }

public slots:
    void install() {
        QCoreApplication::installTranslator(this);
        QTest::qWait(2500);
        QApplication::closeAllWindows();
    }
public:
    mutable QSet<QByteArray> m_translations;
};

enum DialogType {
    InputDialog = 1,
    ColorDialog,
    FileDialog
};

typedef QSet<QByteArray> TranslationSet;
Q_DECLARE_METATYPE(TranslationSet)

void tst_languageChange::retranslatability_data()
{
    QTest::addColumn<int>("dialogType");
    QTest::addColumn<TranslationSet >("expected");

    //next we fill it with data
    QTest::newRow( "QInputDialog" )
        << int(InputDialog) << (QSet<QByteArray>()
                    << "QDialogButtonBox::Cancel");

    QTest::newRow( "QColorDialog" )
        << int(ColorDialog) << (QSet<QByteArray>()
                    << "QDialogButtonBox::Cancel"
                    << "QColorDialog::&Sat:"
                    << "QColorDialog::&Add to Custom Colors"
                    << "QColorDialog::&Green:"
                    << "QColorDialog::&Red:"
                    << "QColorDialog::Bl&ue:"
                    << "QColorDialog::A&lpha channel:"
                    << "QColorDialog::&Basic colors"
                    << "QColorDialog::&Custom colors"
                    << "QColorDialog::&Val:"
                    << "QColorDialog::Hu&e:");

    QTest::newRow( "QFileDialog" )
        << int(FileDialog) << (QSet<QByteArray>()
                    << "QFileDialog::All Files (*)"
                    << "QFileDialog::Back"
                    << "QFileDialog::Create New Folder"
                    << "QFileDialog::Detail View"
#ifndef Q_OS_MAC
                    << "QFileDialog::File"
#endif
                    << "QFileDialog::Files of type:"
                    << "QFileDialog::Forward"
                    << "QFileDialog::List View"
                    << "QFileDialog::Look in:"
                    << "QFileDialog::Open"
                    << "QFileDialog::Parent Directory"
                    << "QFileDialog::Show "
                    << "QFileDialog::Show &hidden files"
                    << "QFileDialog::&Delete"
                    << "QFileDialog::&New Folder"
                    << "QFileDialog::&Rename"
                    << "QFileSystemModel::Date Modified"
#ifdef Q_OS_WIN
                    << "QFileSystemModel::My Computer"
#else
                    << "QFileSystemModel::Computer"
#endif
                    << "QFileSystemModel::Size"
#ifdef Q_OS_MAC
                    << "QFileSystemModel::Kind::Match OS X Finder"
#else
                    << "QFileSystemModel::Type::All other platforms"
#endif
//                    << "QFileSystemModel::%1 KB"
                    << "QDialogButtonBox::Cancel"
                    << "QDialogButtonBox::Open"
                    << "QFileDialog::File &name:");
}

void tst_languageChange::retranslatability()
{
    QFETCH( int, dialogType);
    QFETCH( TranslationSet, expected);

    // This will always be queried for when a language changes
    expected.insert("QApplication::QT_LAYOUT_DIRECTION::Translate this string to the string 'LTR' in left-to-right "
                       "languages or to 'RTL' in right-to-left languages (such as Hebrew and Arabic) to "
                       "get proper widget layout.");
    TransformTranslator translator;
    QTimer::singleShot(500, &translator, SLOT(install()));
    switch (dialogType) {
    case InputDialog:
        (void)QInputDialog::getInteger(0, QLatin1String("title"), QLatin1String("label"));
        break;

    case ColorDialog:
#ifdef Q_WS_MAC
        QSKIP("The native color dialog is used on Mac OS");
#else
        (void)QColorDialog::getColor();
#endif
        break;
    case FileDialog: {
#ifdef Q_WS_MAC
        QSKIP("The native file dialog is used on Mac OS");
#endif
        QFileDialog dlg;
        dlg.setOption(QFileDialog::DontUseNativeDialog);
        QString tmpParentDir = QDir::tempPath() + "/languagechangetestdir";
        QString tmpDir = tmpParentDir + "/finaldir";
        QString fooName = tmpParentDir + "/foo";
        QDir dir;
        QCOMPARE(dir.mkpath(tmpDir), true);
        QCOMPARE(QFile::copy(QApplication::applicationFilePath(), fooName), true);

        dlg.setDirectory(tmpParentDir);
#ifdef Q_OS_WINCE
        dlg.setDirectory("\\Windows");
#endif
        dlg.setFileMode(QFileDialog::ExistingFiles);
        dlg.setViewMode(QFileDialog::Detail);
        dlg.exec();
        QTest::qWait(3000);
        QCOMPARE(QFile::remove(fooName), true);
        QCOMPARE(dir.rmdir(tmpDir), true);
        QCOMPARE(dir.rmdir(tmpParentDir), true);
        break; }
    }
#if 0
    QList<QByteArray> list = translator.m_translations.toList();
    qSort(list);
    qDebug() << list;
#endif
    // In case we use a Color dialog, we do not want to test for
    // strings non existing in the dialog and which do not get
    // translated.
    if ((dialogType == ColorDialog) &&
#ifndef Q_OS_WINCE
        (qApp->desktop()->width() < 480 || qApp->desktop()->height() < 350)
#else
        true // On Qt/WinCE we always use compact mode
#endif
        ) {
        expected.remove("QColorDialog::&Basic colors");
        expected.remove("QColorDialog::&Custom colors");
        expected.remove("QColorDialog::&Define Custom Colors >>");
        expected.remove("QColorDialog::&Add to Custom Colors");
    }

    // see if all of our *expected* translations was translated.
    // (There might be more, but thats not that bad)
    QSet<QByteArray> commonTranslations = expected;
    commonTranslations.intersect(translator.m_translations);
    if (!expected.subtract(commonTranslations).isEmpty()) {
        qDebug() << "Missing:" << expected;
        if (!translator.m_translations.subtract(commonTranslations).isEmpty())
            qDebug() << "Unexpected:" << translator.m_translations;
    }

    QVERIFY(expected.isEmpty());
}

QTEST_MAIN(tst_languageChange)
#include "tst_languagechange.moc"
