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
#include <private/qapplication_p.h>
#include <qkeysequence.h>
#include <private/qkeysequence_p.h>
#include <QTranslator>
#include <QLibraryInfo>

//TESTED_CLASS=
//TESTED_FILES=

#ifdef Q_WS_MAC
#include <Carbon/Carbon.h>
struct MacSpecialKey {
    int key;
    ushort macSymbol;
};

static const int NumEntries = 21;
static const MacSpecialKey entries[NumEntries] = {
    { Qt::Key_Escape, 0x238B },
    { Qt::Key_Tab, 0x21E5 },
    { Qt::Key_Backtab, 0x21E4 },
    { Qt::Key_Backspace, 0x232B },
    { Qt::Key_Return, 0x21B5 },
    { Qt::Key_Enter, 0x21B5 },
    { Qt::Key_Delete, 0x2326 },
    { Qt::Key_Home, 0x2196 },
    { Qt::Key_End, 0x2198 },
    { Qt::Key_Left, 0x2190 },
    { Qt::Key_Up, 0x2191 },
    { Qt::Key_Right, 0x2192 },
    { Qt::Key_Down, 0x2193 },
    { Qt::Key_PageUp, 0x21DE },
    { Qt::Key_PageDown, 0x21DF },
    { Qt::Key_Shift, kShiftUnicode },
    { Qt::Key_Control, kCommandUnicode },
    { Qt::Key_Meta, kControlUnicode },
    { Qt::Key_Alt, kOptionUnicode },
    { Qt::Key_CapsLock, 0x21EA },
};

static bool operator<(const MacSpecialKey &entry, int key)
{
    return entry.key < key;
}

static bool operator<(int key, const MacSpecialKey &entry)
{
    return key < entry.key;
}

static const MacSpecialKey * const MacSpecialKeyEntriesEnd = entries + NumEntries;

static QChar macSymbolForQtKey(int key)
{
    const MacSpecialKey *i = qBinaryFind(entries, MacSpecialKeyEntriesEnd, key);
    if (i == MacSpecialKeyEntriesEnd)
        return QChar();
    return QChar(i->macSymbol);
}

#endif

class tst_QKeySequence : public QObject
{
    Q_OBJECT

public:
    tst_QKeySequence();
    virtual ~tst_QKeySequence();

private slots:
    void swap();
    void operatorQString_data();
    void operatorQString();
    void compareConstructors_data();
    void compareConstructors();
    void symetricConstructors_data();
    void symetricConstructors();
    void checkMultipleNames();
    void checkMultipleCodes();
    void mnemonic_data();
    void mnemonic();
    void toString_data();
    void toString();
    void streamOperators_data();
    void streamOperators();
    void fromString_data();
    void fromString();
    void ensureSorted();
    void standardKeys_data();
    void standardKeys();
    void keyBindings();
    void translated_data();
    void translated();
    void i18nKeys_data();
    void i18nKeys();


    void initTestCase();
private:
    QTranslator *ourTranslator;
    QTranslator *qtTranslator;
#ifdef Q_WS_MAC
    static const QString MacCtrl;
    static const QString MacMeta;
    static const QString MacAlt;
    static const QString MacShift;
#endif


};

#ifdef Q_WS_MAC
const QString tst_QKeySequence::MacCtrl = QString(QChar(0x2318));
const QString tst_QKeySequence::MacMeta = QString(QChar(0x2303));
const QString tst_QKeySequence::MacAlt = QString(QChar(0x2325));
const QString tst_QKeySequence::MacShift = QString(QChar(0x21E7));
#endif

tst_QKeySequence::tst_QKeySequence()
{
}

tst_QKeySequence::~tst_QKeySequence()
{

}

void tst_QKeySequence::initTestCase()
{
    ourTranslator = new QTranslator(this);
    ourTranslator->load(":/keys_de");
    qtTranslator = new QTranslator(this);
    qtTranslator->load(":/qt_de");
}

void tst_QKeySequence::swap()
{
    QKeySequence ks1(Qt::CTRL+Qt::Key_O);
    QKeySequence ks2(Qt::CTRL+Qt::Key_L);
    ks1.swap(ks2);
    QCOMPARE(ks1[0], int(Qt::CTRL+Qt::Key_L));
    QCOMPARE(ks2[0], int(Qt::CTRL+Qt::Key_O));
}

void tst_QKeySequence::operatorQString_data()
{
    QTest::addColumn<int>("modifiers");
    QTest::addColumn<int>("keycode");
    QTest::addColumn<QString>("keystring");

    QTest::newRow( "No modifier" ) << 0 << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << QString( "\x0c5" );

#ifndef Q_WS_MAC
    QTest::newRow( "Ctrl+Left" ) << int(Qt::CTRL) << int(Qt::Key_Left) << QString( "Ctrl+Left" );
    QTest::newRow( "Ctrl+," ) << int(Qt::CTRL) << int(Qt::Key_Comma) << QString( "Ctrl+," );
    QTest::newRow( "Alt+Left" ) << int(Qt::ALT) << int(Qt::Key_Left) << QString( "Alt+Left" );
    QTest::newRow( "Alt+Shift+Left" ) << int(Qt::ALT | Qt::SHIFT) << int(Qt::Key_Left) << QString( "Alt+Shift+Left" );
    QTest::newRow( "Ctrl" ) << int(Qt::CTRL) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << QString( "Ctrl+\x0c5" );
    QTest::newRow( "Alt" ) << int(Qt::ALT) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << QString( "Alt+\x0c5" );
    QTest::newRow( "Shift" ) << int(Qt::SHIFT) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << QString( "Shift+\x0c5" );
    QTest::newRow( "Meta" ) << int(Qt::META) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << QString( "Meta+\x0c5" );
#else
    QTest::newRow( "Ctrl+Left" ) << int(Qt::CTRL) << int(Qt::Key_Left) << MacCtrl + macSymbolForQtKey(Qt::Key_Left);
    QTest::newRow( "Ctrl+," ) << int(Qt::CTRL) << int(Qt::Key_Comma) << MacCtrl + ",";
    QTest::newRow( "Alt+Left" ) << int(Qt::ALT) << int(Qt::Key_Left) << MacAlt + macSymbolForQtKey(Qt::Key_Left);
    QTest::newRow( "Alt+Shift+Left" ) << int(Qt::ALT | Qt::SHIFT) << int(Qt::Key_Left) << MacAlt + MacShift + macSymbolForQtKey(Qt::Key_Left);
    QTest::newRow( "Ctrl" ) << int(Qt::CTRL) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << MacCtrl + "\x0c5";
    QTest::newRow( "Alt" ) << int(Qt::ALT) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << MacAlt + "\x0c5";
    QTest::newRow( "Shift" ) << int(Qt::SHIFT) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << MacShift + "\x0c5";
    QTest::newRow( "Meta" ) << int(Qt::META) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL) << MacMeta + "\x0c5";
#endif
}

void tst_QKeySequence::symetricConstructors_data()
{
    QTest::addColumn<int>("modifiers");
    QTest::addColumn<int>("keycode");

    QTest::newRow( "No modifier" ) << 0 << int(Qt::Key_Aring | Qt::UNICODE_ACCEL);
    QTest::newRow( "Ctrl" ) << int(Qt::CTRL) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL);
    QTest::newRow( "Alt" ) << int(Qt::ALT) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL);
    QTest::newRow( "Shift" ) << int(Qt::SHIFT) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL);
    QTest::newRow( "Meta" ) << int(Qt::META) << int(Qt::Key_Aring | Qt::UNICODE_ACCEL);
}

void tst_QKeySequence::compareConstructors_data()
{
    operatorQString_data();
}

// operator QString()
void tst_QKeySequence::operatorQString()
{
    QKeySequence seq;
    QFETCH( int, modifiers );
    QFETCH( int, keycode );
    QFETCH( QString, keystring );

    seq = QKeySequence( modifiers | keycode );

    QCOMPARE( (QString)seq, keystring );
}

// this verifies that the constructors can handle the same strings in and out
void tst_QKeySequence::symetricConstructors()
{
    QFETCH( int, modifiers );
    QFETCH( int, keycode );

    QKeySequence seq1( modifiers | keycode );
    QKeySequence seq2( (QString)seq1 );

    QVERIFY( seq1 == seq2 );
}

/* Compares QKeySequence constructurs with int or QString arguments
   We don't do this for 3.0 since it doesn't support unicode accelerators */
void tst_QKeySequence::compareConstructors()
{
    QFETCH( int, modifiers );
    QFETCH( int, keycode );
    QFETCH( QString, keystring );

    QKeySequence qstringSeq( keystring );
    QKeySequence intSeq( modifiers | keycode );

    QVERIFY( qstringSeq == intSeq );
}

void tst_QKeySequence::checkMultipleNames()
{
    QKeySequence oldK( "Ctrl+Page Up" );
    QKeySequence newK( "Ctrl+PgUp" );
    QVERIFY( oldK == newK );
}

//TODO: could test third constructor, or test fromString on all constructor-data
void tst_QKeySequence::checkMultipleCodes()
{
    QKeySequence seq1("Alt+d, l");
    QKeySequence seq2 = QKeySequence::fromString("Alt+d, l");
    QVERIFY( seq1 == seq2 );

    QKeySequence seq3("Alt+d,l");
    QKeySequence seq4 = QKeySequence::fromString("Alt+d,l");
    QVERIFY( seq3 == seq4 );
}

/*
* We must ensure that the keyBindings data is always sorted
* so that we can safely perform binary searches.
*/
void tst_QKeySequence::ensureSorted()
{
//### accessing static members from private classes does not work on msvc at the moment
#if defined(QT_BUILD_INTERNAL) && !defined(Q_WS_WIN)
    uint N = QKeySequencePrivate::numberOfKeyBindings;
    uint val = QKeySequencePrivate::keyBindings[0].shortcut;
    for ( uint i = 1 ; i < N ; ++i) {
        uint nextval = QKeySequencePrivate::keyBindings[i].shortcut;
        if (nextval < val)
            qDebug() << "Data not sorted at index " << i;
        QVERIFY(nextval >= val);
        val = nextval;
    }
#endif
}

void tst_QKeySequence::standardKeys_data()
{
    QTest::addColumn<int>("standardKey");
    QTest::addColumn<QString>("expected");
    QTest::newRow("unknownkey") << (int)QKeySequence::UnknownKey<< QString("");
    QTest::newRow("copy") << (int)QKeySequence::Copy << QString("CTRL+C");
    QTest::newRow("cut") << (int)QKeySequence::Cut << QString("CTRL+X");
    QTest::newRow("paste") << (int)QKeySequence::Paste << QString("CTRL+V");
    QTest::newRow("delete") << (int)QKeySequence::Delete<< QString("DEL");
    QTest::newRow("open") << (int)QKeySequence::Open << QString("CTRL+O");
    QTest::newRow("find") << (int)QKeySequence::Find<< QString("CTRL+F");
#ifdef Q_WS_WIN
    QTest::newRow("addTab") << (int)QKeySequence::AddTab<< QString("CTRL+T");
    QTest::newRow("findNext") << (int)QKeySequence::FindNext<< QString("F3");
    QTest::newRow("findPrevious") << (int)QKeySequence::FindPrevious << QString("SHIFT+F3");
    QTest::newRow("close") << (int)QKeySequence::Close<< QString("CTRL+F4");
    QTest::newRow("replace") << (int)QKeySequence::Replace<< QString("CTRL+H");
#endif
    QTest::newRow("bold") << (int)QKeySequence::Bold << QString("CTRL+B");
    QTest::newRow("italic") << (int)QKeySequence::Italic << QString("CTRL+I");
    QTest::newRow("underline") << (int)QKeySequence::Underline << QString("CTRL+U");
    QTest::newRow("selectall") << (int)QKeySequence::SelectAll << QString("CTRL+A");
    QTest::newRow("print") << (int)QKeySequence::Print << QString("CTRL+P");
    QTest::newRow("movenextchar") << (int)QKeySequence::MoveToNextChar<< QString("RIGHT");
    QTest::newRow("zoomIn") << (int)QKeySequence::ZoomIn<< QString("CTRL++");
    QTest::newRow("zoomOut") << (int)QKeySequence::ZoomOut<< QString("CTRL+-");
    QTest::newRow("whatsthis") << (int)QKeySequence::WhatsThis<< QString("SHIFT+F1");

#if defined(Q_WS_MAC)
    QTest::newRow("help") << (int)QKeySequence::HelpContents<< QString("Ctrl+?");
    QTest::newRow("nextChild") << (int)QKeySequence::NextChild << QString("CTRL+}");
    QTest::newRow("previousChild") << (int)QKeySequence::PreviousChild << QString("CTRL+{");
    QTest::newRow("MoveToEndOfBlock") << (int)QKeySequence::MoveToEndOfBlock << QString("ALT+DOWN");
    QTest::newRow("forward") << (int)QKeySequence::Forward << QString("CTRL+]");
    QTest::newRow("backward") << (int)QKeySequence::Back << QString("CTRL+[");
    QTest::newRow("SelectEndOfDocument") << (int)QKeySequence::SelectEndOfDocument<< QString("CTRL+SHIFT+DOWN"); //mac only
#elif defined(Q_WS_S60)
    QTest::newRow("help") << (int)QKeySequence::HelpContents<< QString("F2");
    QTest::newRow("SelectEndOfDocument") << (int)QKeySequence::SelectEndOfDocument<< QString("CTRL+SHIFT+END"); //mac only
#else
    QTest::newRow("help") << (int)QKeySequence::HelpContents<< QString("F1");
    QTest::newRow("nextChild") << (int)QKeySequence::NextChild<< QString("CTRL+Tab");
    QTest::newRow("previousChild") << (int)QKeySequence::PreviousChild<< QString("CTRL+SHIFT+BACKTAB");
    QTest::newRow("forward") << (int)QKeySequence::Forward << QString("ALT+RIGHT");
    QTest::newRow("backward") << (int)QKeySequence::Back << QString("ALT+LEFT");
    QTest::newRow("MoveToEndOfBlock") << (int)QKeySequence::MoveToEndOfBlock<< QString(""); //mac only
    QTest::newRow("SelectEndOfDocument") << (int)QKeySequence::SelectEndOfDocument<< QString("CTRL+SHIFT+END"); //mac only
#endif
}

void tst_QKeySequence::standardKeys()
{
    QFETCH(int, standardKey);
    QFETCH(QString, expected);
    QKeySequence ks((QKeySequence::StandardKey)standardKey);
    QKeySequence ks2(expected);
    QVERIFY(ks == ks2);
}

void tst_QKeySequence::keyBindings()
{
    QList<QKeySequence> bindings = QKeySequence::keyBindings(QKeySequence::Copy);
    QList<QKeySequence> expected;
#if defined(Q_WS_MAC) || defined (Q_WS_S60)
    expected  << QKeySequence("CTRL+C");
#elif defined Q_WS_X11
    expected  << QKeySequence("CTRL+C") << QKeySequence("F16") << QKeySequence("CTRL+INSERT");
#else
    expected  << QKeySequence("CTRL+C") << QKeySequence("CTRL+INSERT");
#endif
	QVERIFY(bindings == expected);
}



void tst_QKeySequence::mnemonic_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("warning");

    QTest::newRow("1") << QString::fromLatin1("&bonjour") << QString::fromLatin1("ALT+B") << false;
    QTest::newRow("2") << QString::fromLatin1("&&bonjour") << QString() << false;
    QTest::newRow("3") << QString::fromLatin1("&&bon&jour") << QString::fromLatin1("ALT+J") << false;
    QTest::newRow("4") << QString::fromLatin1("&&bon&jo&ur") << QString::fromLatin1("ALT+J") << true;
    QTest::newRow("5") << QString::fromLatin1("b&on&&jour") << QString::fromLatin1("ALT+O") << false;
    QTest::newRow("6") << QString::fromLatin1("bonjour") << QString() << false;
    QTest::newRow("7") << QString::fromLatin1("&&&bonjour") << QString::fromLatin1("ALT+B") << false;
    QTest::newRow("8") << QString::fromLatin1("bonjour&&&") << QString() << false;
    QTest::newRow("9") << QString::fromLatin1("bo&&nj&o&&u&r") << QString::fromLatin1("ALT+O") << true;
    QTest::newRow("10") << QString::fromLatin1("BON&JOUR") << QString::fromLatin1("ALT+J") << false;
    QTest::newRow("11") << QString::fromUtf8("bonjour") << QString() << false;
}

void tst_QKeySequence::mnemonic()
{
#ifdef Q_WS_MAC
    QSKIP("mnemonics are not used on Mac OS X", SkipAll);
#endif
    QFETCH(QString, string);
    QFETCH(QString, key);
    QFETCH(bool, warning);

#ifndef QT_NO_DEBUG
    if (warning) {
        QString str = QString::fromLatin1("QKeySequence::mnemonic: \"%1\" contains multiple occurrences of '&'").arg(string);
        QTest::ignoreMessage(QtWarningMsg, qPrintable(str));
    //    qWarning(qPrintable(str));
    }
#endif
    QKeySequence seq = QKeySequence::mnemonic(string);
    QKeySequence res = QKeySequence(key);

    QCOMPARE(seq, res);
}



void tst_QKeySequence::toString_data()
{
    QTest::addColumn<QString>("strSequence");
    QTest::addColumn<QString>("neutralString");
    QTest::addColumn<QString>("platformString");


#ifndef Q_WS_MAC
    QTest::newRow("Ctrl+Left") << QString("Ctrl+Left") << QString("Ctrl+Left") << QString("Ctrl+Left");
    QTest::newRow("Alt+Left") << QString("Alt+Left") << QString("Alt+Left") << QString("Alt+Left");
    QTest::newRow("Alt+Shift+Left") << QString("Alt+Shift+Left") << QString("Alt+Shift+Left") << QString("Alt+Shift+Left");
    QTest::newRow("Ctrl") << QString("Ctrl+\x0c5") << QString("Ctrl+\x0c5") << QString("Ctrl+\x0c5");
    QTest::newRow("Alt") << QString("Alt+\x0c5") << QString("Alt+\x0c5") << QString("Alt+\x0c5");
    QTest::newRow("Shift") << QString("Shift+\x0c5") << QString("Shift+\x0c5") << QString("Shift+\x0c5");
    QTest::newRow("Meta") << QString("Meta+\x0c5") << QString("Meta+\x0c5") << QString("Meta+\x0c5");
    QTest::newRow("Ctrl+Plus") << QString("Ctrl++") << QString("Ctrl++") << QString("Ctrl++");
    QTest::newRow("Ctrl+,") << QString("Ctrl+,") << QString("Ctrl+,") << QString("Ctrl+,");
    QTest::newRow("Ctrl+,,Ctrl+,") << QString("Ctrl+,,Ctrl+,") << QString("Ctrl+,, Ctrl+,") << QString("Ctrl+,, Ctrl+,");
    QTest::newRow("MultiKey") << QString("Alt+X, Ctrl+Y, Z") << QString("Alt+X, Ctrl+Y, Z")
                           << QString("Alt+X, Ctrl+Y, Z");

    QTest::newRow("Invalid") << QString("Ctrly") << QString("") << QString("");
#else
    /*
    QTest::newRow("Ctrl+Left") << MacCtrl + "Left" << QString("Ctrl+Left") << MacCtrl + macSymbolForQtKey(Qt::Key_Left);
    QTest::newRow("Alt+Left") << MacAlt + "Left" << QString("Alt+Left") << MacAlt + macSymbolForQtKey(Qt::Key_Left);
    QTest::newRow("Alt+Shift+Left") << MacAlt + MacShift + "Left" << QString("Alt+Shift+Left")
                                 << MacAlt + MacShift + macSymbolForQtKey(Qt::Key_Left);
                                 */
    QTest::newRow("Ctrl+Right,Left") << MacCtrl + "Right, Left" << QString("Ctrl+Right, Left") << MacCtrl + macSymbolForQtKey(Qt::Key_Right) + QString(", ") + macSymbolForQtKey(Qt::Key_Left);
    QTest::newRow("Ctrl") << MacCtrl + "\x0c5" << QString("Ctrl+\x0c5") << MacCtrl + "\x0c5";
    QTest::newRow("Alt") << MacAlt + "\x0c5" << QString("Alt+\x0c5") << MacAlt + "\x0c5";
    QTest::newRow("Shift") << MacShift + "\x0c5" << QString("Shift+\x0c5") << MacShift + "\x0c5";
    QTest::newRow("Meta") << MacMeta + "\x0c5" << QString("Meta+\x0c5") << MacMeta + "\x0c5";
    QTest::newRow("Ctrl+Plus") << MacCtrl + "+" << QString("Ctrl++") << MacCtrl + "+";
    QTest::newRow("Ctrl+,") << MacCtrl + "," << QString("Ctrl+,") << MacCtrl + ",";
    QTest::newRow("Ctrl+,,Ctrl+,") << MacCtrl + ",, " + MacCtrl + "," << QString("Ctrl+,, Ctrl+,") << MacCtrl + ",, " + MacCtrl + ",";
    QTest::newRow("MultiKey") << MacAlt + "X, " + MacCtrl + "Y, Z" << QString("Alt+X, Ctrl+Y, Z")
                           << MacAlt + "X, " + MacCtrl + "Y, Z";
    QTest::newRow("Invalid") << QString("Ctrly") << QString("") << QString("");
#endif
}

void tst_QKeySequence::toString()
{
    QFETCH(QString, strSequence);
    QFETCH(QString, neutralString);
    QFETCH(QString, platformString);

    QKeySequence ks1(strSequence);

    QCOMPARE(ks1.toString(QKeySequence::NativeText), platformString);
    QCOMPARE(ks1.toString(QKeySequence::PortableText), neutralString);

}

void tst_QKeySequence::streamOperators_data()
{
	operatorQString_data();
}

void tst_QKeySequence::streamOperators()
{
    QFETCH( int, modifiers );
    QFETCH( int, keycode );

	QByteArray data;
	QKeySequence refK( modifiers | keycode );
	QKeySequence orgK( "Ctrl+A" );
	QKeySequence copyOrgK = orgK;
	QVERIFY( copyOrgK == orgK );

	QDataStream in(&data, QIODevice::WriteOnly);
	in << refK;
        QDataStream out(&data, QIODevice::ReadOnly);
        out >> orgK;

	QVERIFY( orgK == refK );

	// check if detached
	QVERIFY( orgK != copyOrgK );
}

void tst_QKeySequence::fromString_data()
{
    toString_data();
}

void tst_QKeySequence::fromString()
{
    QFETCH(QString, strSequence);
    QFETCH(QString, neutralString);
    QFETCH(QString, platformString);

    QKeySequence ks1(strSequence);
    QKeySequence ks2 = QKeySequence::fromString(ks1.toString());
    QKeySequence ks3 = QKeySequence::fromString(neutralString, QKeySequence::PortableText);
    QKeySequence ks4 = QKeySequence::fromString(platformString, QKeySequence::NativeText);


    // assume the transitive property exists here.
    QCOMPARE(ks2, ks1);
    QCOMPARE(ks3, ks1);
    QCOMPARE(ks4, ks1);
}

void tst_QKeySequence::translated_data()
{
    qApp->installTranslator(ourTranslator);
    qApp->installTranslator(qtTranslator);

    QTest::addColumn<QString>("transKey");
    QTest::addColumn<QString>("compKey");

    QTest::newRow("Shift++") << tr("Shift++") << QString("Umschalt++");
    QTest::newRow("Ctrl++")  << tr("Ctrl++") << QString("Strg++");
    QTest::newRow("Alt++")   << tr("Alt++") << QString("Alt++");
    QTest::newRow("Meta++")  << tr("Meta++") << QString("Meta++");

    QTest::newRow("Shift+,, Shift++") << tr("Shift+,, Shift++") << QString("Umschalt+,, Umschalt++");
    QTest::newRow("Shift+,, Ctrl++")  << tr("Shift+,, Ctrl++") << QString("Umschalt+,, Strg++");
    QTest::newRow("Shift+,, Alt++")   << tr("Shift+,, Alt++") << QString("Umschalt+,, Alt++");
    QTest::newRow("Shift+,, Meta++")  << tr("Shift+,, Meta++") << QString("Umschalt+,, Meta++");

    QTest::newRow("Ctrl+,, Shift++") << tr("Ctrl+,, Shift++") << QString("Strg+,, Umschalt++");
    QTest::newRow("Ctrl+,, Ctrl++")  << tr("Ctrl+,, Ctrl++") << QString("Strg+,, Strg++");
    QTest::newRow("Ctrl+,, Alt++")   << tr("Ctrl+,, Alt++") << QString("Strg+,, Alt++");
    QTest::newRow("Ctrl+,, Meta++")  << tr("Ctrl+,, Meta++") << QString("Strg+,, Meta++");

    qApp->removeTranslator(ourTranslator);
    qApp->removeTranslator(qtTranslator);
}

void tst_QKeySequence::translated()
{
    QFETCH(QString, transKey);
    QFETCH(QString, compKey);
#ifdef Q_WS_MAC
    QSKIP("No need to translate modifiers on Mac OS X", SkipAll);
#elif defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QSKIP("No need to translate modifiers on WinCE or Symbian", SkipAll);
#endif

    qApp->installTranslator(ourTranslator);
    qApp->installTranslator(qtTranslator);

    QKeySequence ks1(transKey);
    QCOMPARE(ks1.toString(QKeySequence::NativeText), compKey);

    qApp->removeTranslator(ourTranslator);
    qApp->removeTranslator(qtTranslator);
}


void tst_QKeySequence::i18nKeys_data()
{
    QTest::addColumn<int>("keycode");
    QTest::addColumn<QString>("keystring");

    // Japanese keyboard support
    QTest::newRow("Kanji") << (int)Qt::Key_Kanji << QString("Kanji");
    QTest::newRow("Muhenkan") << (int)Qt::Key_Muhenkan << QString("Muhenkan");
    QTest::newRow("Henkan") << (int)Qt::Key_Henkan << QString("Henkan");
    QTest::newRow("Romaji") << (int)Qt::Key_Romaji << QString("Romaji");
    QTest::newRow("Hiragana") << (int)Qt::Key_Hiragana << QString("Hiragana");
    QTest::newRow("Katakana") << (int)Qt::Key_Katakana << QString("Katakana");
    QTest::newRow("Hiragana Katakana") << (int)Qt::Key_Hiragana_Katakana << QString("Hiragana Katakana");
    QTest::newRow("Zenkaku") << (int)Qt::Key_Zenkaku << QString("Zenkaku");
    QTest::newRow("Hankaku") << (int)Qt::Key_Hankaku << QString("Hankaku");
    QTest::newRow("Zenkaku Hankaku") << (int)Qt::Key_Zenkaku_Hankaku << QString("Zenkaku Hankaku");
    QTest::newRow("Touroku") << (int)Qt::Key_Touroku << QString("Touroku");
    QTest::newRow("Massyo") << (int)Qt::Key_Massyo << QString("Massyo");
    QTest::newRow("Kana Lock") << (int)Qt::Key_Kana_Lock << QString("Kana Lock");
    QTest::newRow("Kana Shift") << (int)Qt::Key_Kana_Shift << QString("Kana Shift");
    QTest::newRow("Eisu Shift") << (int)Qt::Key_Eisu_Shift << QString("Eisu Shift");
    QTest::newRow("Eisu_toggle") << (int)Qt::Key_Eisu_toggle << QString("Eisu toggle");
    QTest::newRow("Code input") << (int)Qt::Key_Codeinput << QString("Code input");
    QTest::newRow("Multiple Candidate") << (int)Qt::Key_MultipleCandidate << QString("Multiple Candidate");
    QTest::newRow("Previous Candidate") << (int)Qt::Key_PreviousCandidate << QString("Previous Candidate");

    // Korean keyboard support
    QTest::newRow("Hangul") << (int)Qt::Key_Hangul << QString("Hangul");
    QTest::newRow("Hangul Start") << (int)Qt::Key_Hangul_Start << QString("Hangul Start");
    QTest::newRow("Hangul End") << (int)Qt::Key_Hangul_End << QString("Hangul End");
    QTest::newRow("Hangul Hanja") << (int)Qt::Key_Hangul_Hanja << QString("Hangul Hanja");
    QTest::newRow("Hangul Jamo") << (int)Qt::Key_Hangul_Jamo << QString("Hangul Jamo");
    QTest::newRow("Hangul Romaja") << (int)Qt::Key_Hangul_Romaja << QString("Hangul Romaja");
    QTest::newRow("Hangul Jeonja") << (int)Qt::Key_Hangul_Jeonja << QString("Hangul Jeonja");
    QTest::newRow("Hangul Banja") << (int)Qt::Key_Hangul_Banja << QString("Hangul Banja");
    QTest::newRow("Hangul PreHanja") << (int)Qt::Key_Hangul_PreHanja << QString("Hangul PreHanja");
    QTest::newRow("Hangul PostHanja") << (int)Qt::Key_Hangul_PostHanja << QString("Hangul PostHanja");
    QTest::newRow("Hangul Special") << (int)Qt::Key_Hangul_Special << QString("Hangul Special");
}

void tst_QKeySequence::i18nKeys()
{
    QFETCH(int, keycode);
    QFETCH(QString, keystring);
    QKeySequence seq(keycode);

    QCOMPARE(seq, QKeySequence(keystring));
    QCOMPARE(seq.toString(), keystring);
}

QTEST_MAIN(tst_QKeySequence)
#include "tst_qkeysequence.moc"
