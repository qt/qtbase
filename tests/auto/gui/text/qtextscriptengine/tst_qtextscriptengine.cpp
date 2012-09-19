/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/



#ifdef Q_WS_X11
#define private public
#endif

// cannot do private -> public on windows since it seems to mess up some stl headers
#include <qfont.h>

#ifdef Q_WS_X11
#undef private
#endif

#include <QtTest/QtTest>



#if defined(Q_WS_X11) || defined(Q_OS_MAC)
#define private public
#include <private/qtextengine_p.h>
#include <qtextlayout.h>
#undef private
#else
#include <private/qtextengine_p.h>
#include <qtextlayout.h>
#endif

#include <qfontdatabase.h>
#include <qfontinfo.h>

class tst_QTextScriptEngine : public QObject
{
Q_OBJECT

public:
    tst_QTextScriptEngine();
    virtual ~tst_QTextScriptEngine();


public slots:
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void devanagari();
    void bengali();
    void gurmukhi();
    // gujarati missing
    void oriya();
    void tamil();
    void telugu();
    void kannada();
    void malayalam();
    void sinhala();
    void greek();

    void khmer();
    void linearB();
    void controlInSyllable_qtbug14204();
    void combiningMarks_qtbug15675();

    void mirroredChars_data();
    void mirroredChars();

    void thaiIsolatedSaraAm();
    void thaiWithZWJ();
    void thaiMultipleVowels();
private:
    bool haveTestFonts;
};

tst_QTextScriptEngine::tst_QTextScriptEngine()
    : haveTestFonts(qgetenv("QT_HAVE_TEST_FONTS") == QByteArray("1"))
{
}

tst_QTextScriptEngine::~tst_QTextScriptEngine()
{
}

void tst_QTextScriptEngine::initTestCase()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts) {
        qWarning(
            "Some of these tests depend on the internals of some test fonts which are not freely "
            "distributable.\n"
            "These tests will be skipped.\n"
            "If you have the fonts available, set QT_HAVE_TEST_FONTS=1 in your environment and "
            "run the test again."
        );
    }
#endif
}

void tst_QTextScriptEngine::init()
{
}

void tst_QTextScriptEngine::cleanup()
{
}

struct ShapeTable {
    unsigned short unicode[16];
    unsigned short glyphs[16];
};

#if defined(Q_WS_X11)
static bool shaping( const QFont &f, const ShapeTable *s)
{
    QString str = QString::fromUtf16( s->unicode );
    QTextLayout layout(str, f);
    QTextEngine *e = layout.d;
    e->itemize();
    e->shape(0);

    int nglyphs = 0;
    const unsigned short *g = s->glyphs;
    while ( *g ) {
	nglyphs++;
	g++;
    }

    if( nglyphs != e->layoutData->items[0].num_glyphs )
	goto error;

    for (int i = 0; i < nglyphs; ++i) {
	if ((e->layoutData->glyphLayout.glyphs[i] & 0xffffff) != s->glyphs[i])
	    goto error;
    }
    return true;
 error:
    str = "";
    const unsigned short *uc = s->unicode;
    while (*uc) {
	str += QString("%1 ").arg(*uc, 4, 16);
	++uc;
    }
    qDebug("%s: shaping of string %s failed, nglyphs=%d, expected %d",
           f.family().toLatin1().constData(),
           str.toLatin1().constData(),
           e->layoutData->items[0].num_glyphs, nglyphs);

    str = "";
    int i = 0;
    while (i < e->layoutData->items[0].num_glyphs) {
	str += QString("%1 ").arg(e->layoutData->glyphLayout.glyphs[i], 4, 16);
	++i;
    }
    qDebug("    glyph result = %s", str.toLatin1().constData());
    return false;
}
#endif

void tst_QTextScriptEngine::devanagari()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Devanagari).contains("Raghindi")) {
            QFont f("Raghindi");
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0915, 0x0 },
		  { 0x0080, 0x0 } },
		// Ka Halant
		{ { 0x0915, 0x094d, 0x0 },
		  { 0x0080, 0x0051, 0x0 } },
		// Ka Halant Ka
		{ { 0x0915, 0x094d, 0x0915, 0x0 },
		  { 0x00c8, 0x0080, 0x0 } },
		// Ka MatraI
		{ { 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x0 } },
		// Ra Halant Ka
		{ { 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0080, 0x005b, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x0930, 0x094d, 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x005b, 0x0 } },
		// MatraI
		{ { 0x093f, 0x0 },
		  { 0x01d4, 0x029c, 0x0 } },
		// Ka Nukta
		{ { 0x0915, 0x093c, 0x0 },
		  { 0x00a4, 0x0 } },
		// Ka Halant Ra
		{ { 0x0915, 0x094d, 0x0930, 0x0 },
		  { 0x0110, 0x0 } },
		// Ka Halant Ra Halant Ka
		{ { 0x0915, 0x094d, 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0158, 0x0080, 0x0 } },
		{ { 0x0930, 0x094d, 0x200d, 0x0 },
		  { 0x00e2, 0x0 } },
		{ { 0x0915, 0x094d, 0x0930, 0x094d, 0x200d, 0x0 },
		  { 0x0158, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Raghindi");
    }

    {
        if (QFontDatabase().families(QFontDatabase::Devanagari).contains("Mangal")) {
            QFont f("Mangal");
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0915, 0x0 },
		  { 0x0080, 0x0 } },
		// Ka Halant
		{ { 0x0915, 0x094d, 0x0 },
		  { 0x0080, 0x0051, 0x0 } },
		// Ka Halant Ka
		{ { 0x0915, 0x094d, 0x0915, 0x0 },
		  { 0x00c8, 0x0080, 0x0 } },
		// Ka MatraI
		{ { 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x0 } },
		// Ra Halant Ka
		{ { 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0080, 0x005b, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x0930, 0x094d, 0x0915, 0x093f, 0x0 },
		  { 0x01d1, 0x0080, 0x005b, 0x0 } },
		// MatraI
		{ { 0x093f, 0x0 },
		  { 0x01d4, 0x029c, 0x0 } },
		// Ka Nukta
		{ { 0x0915, 0x093c, 0x0 },
		  { 0x00a4, 0x0 } },
		// Ka Halant Ra
		{ { 0x0915, 0x094d, 0x0930, 0x0 },
		  { 0x0110, 0x0 } },
		// Ka Halant Ra Halant Ka
		{ { 0x0915, 0x094d, 0x0930, 0x094d, 0x0915, 0x0 },
		  { 0x0158, 0x0080, 0x0 } },

                { { 0x92b, 0x94d, 0x930, 0x0 },
                  { 0x125, 0x0 } },
                { { 0x92b, 0x93c, 0x94d, 0x930, 0x0 },
                  { 0x149, 0x0 } }, 
		{ {0}, {0} }
	    };

	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find mangal");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::bengali()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Bengali).contains("Akaash")) {
            QFont f("Akaash");
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0995, 0x0 },
		  { 0x0151, 0x0 } },
		// Ka Halant
		{ { 0x0995, 0x09cd, 0x0 },
		  { 0x0151, 0x017d, 0x0 } },
		// Ka Halant Ka
		{ { 0x0995, 0x09cd, 0x0995, 0x0 },
		  { 0x019b, 0x0 } },
		// Ka MatraI
		{ { 0x0995, 0x09bf, 0x0 },
		  { 0x0173, 0x0151, 0x0 } },
		// Ra Halant Ka
		{ { 0x09b0, 0x09cd, 0x0995, 0x0 },
		  { 0x0151, 0x0276, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x09b0, 0x09cd, 0x0995, 0x09bf, 0x0 },
		  { 0x0173, 0x0151, 0x0276, 0x0 } },
		// Ka Nukta
		{ { 0x0995, 0x09bc, 0x0 },
		  { 0x0151, 0x0171, 0x0 } },
		// Ka Halant Ra
		{ { 0x0995, 0x09cd, 0x09b0, 0x0 },
		  { 0x01f4, 0x0 } },
		// Ka Halant Ra Halant Ka
		{ { 0x0995, 0x09cd, 0x09b0, 0x09cd, 0x0995, 0x0 },
		  { 0x025c, 0x0276, 0x0151, 0x0 } },
		// Ya + Halant
		{ { 0x09af, 0x09cd, 0x0 },
		  { 0x016a, 0x017d, 0x0 } },
		// Da Halant Ya -> Da Ya-Phala
		{ { 0x09a6, 0x09cd, 0x09af, 0x0 },
		  { 0x01e5, 0x0 } },
		// A Halant Ya -> A Ya-phala
		{ { 0x0985, 0x09cd, 0x09af, 0x0 },
		  { 0x0145, 0x01cf, 0x0 } },
		// Na Halant Ka
		{ { 0x09a8, 0x09cd, 0x0995, 0x0 },
		  { 0x026f, 0x0151, 0x0 } },
		// Na Halant ZWNJ Ka
		{ { 0x09a8, 0x09cd, 0x200c, 0x0995, 0x0 },
		  { 0x0164, 0x017d, 0x0151, 0x0 } },
		// Na Halant ZWJ Ka
		{ { 0x09a8, 0x09cd, 0x200d, 0x0995, 0x0 },
		  { 0x026f, 0x0151, 0x0 } },
		// Ka Halant ZWNJ Ka
		{ { 0x0995, 0x09cd, 0x200c, 0x0995, 0x0 },
		  { 0x0151, 0x017d, 0x0151, 0x0 } },
		// Ka Halant ZWJ Ka
		{ { 0x0995, 0x09cd, 0x200d, 0x0995, 0x0 },
		  { 0x025c, 0x0151, 0x0 } },
		// Na Halant Ra
		{ { 0x09a8, 0x09cd, 0x09b0, 0x0 },
		  { 0x0207, 0x0 } },
		// Na Halant ZWNJ Ra
		{ { 0x09a8, 0x09cd, 0x200c, 0x09b0, 0x0 },
		  { 0x0164, 0x017d, 0x016b, 0x0 } },
		// Na Halant ZWJ Ra
		{ { 0x09a8, 0x09cd, 0x200d, 0x09b0, 0x0 },
		  { 0x026f, 0x016b, 0x0 } },
		// Na Halant Ba
		{ { 0x09a8, 0x09cd, 0x09ac, 0x0 },
		  { 0x022f, 0x0 } },
		// Na Halant ZWNJ Ba
		{ { 0x09a8, 0x09cd, 0x200c, 0x09ac, 0x0 },
		  { 0x0164, 0x017d, 0x0167, 0x0 } },
		// Na Halant ZWJ Ba
		{ { 0x09a8, 0x09cd, 0x200d, 0x09ac, 0x0 },
		  { 0x026f, 0x0167, 0x0 } },
		// Na Halant Dha
		{ { 0x09a8, 0x09cd, 0x09a7, 0x0 },
		  { 0x01d3, 0x0 } },
		// Na Halant ZWNJ Dha
		{ { 0x09a8, 0x09cd, 0x200c, 0x09a7, 0x0 },
		  { 0x0164, 0x017d, 0x0163, 0x0 } },
		// Na Halant ZWJ Dha
		{ { 0x09a8, 0x09cd, 0x200d, 0x09a7, 0x0 },
		  { 0x026f, 0x0163, 0x0 } },
		// Ra Halant Ka MatraAU
		{ { 0x09b0, 0x09cd, 0x0995, 0x09cc, 0x0 },
		  { 0x0179, 0x0151, 0x0276, 0x017e, 0x0 } },
		// Ra Halant Ba Halant Ba
		{ { 0x09b0, 0x09cd, 0x09ac, 0x09cd, 0x09ac, 0x0 },
		  { 0x0232, 0x0276, 0x0 } },
                { { 0x9b0, 0x9cd, 0x995, 0x9be, 0x982, 0x0 },
                  { 0x151, 0x276, 0x172, 0x143, 0x0 } },
                { { 0x9b0, 0x9cd, 0x995, 0x9be, 0x983, 0x0 },
                  { 0x151, 0x276, 0x172, 0x144, 0x0 } }, 
                // test decomposed two parts matras
                { { 0x995, 0x9c7, 0x9be, 0x0 },
                  { 0x179, 0x151, 0x172, 0x0 } },
                { { 0x995, 0x9c7, 0x9d7, 0x0 },
                  { 0x179, 0x151, 0x17e, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Akaash");
    }
    {
        if (QFontDatabase().families(QFontDatabase::Bengali).contains("Mukti Narrow")) {
            QFont f("Mukti Narrow");
	    const ShapeTable shape_table [] = {
		// Ka
		{ { 0x0995, 0x0 },
		  { 0x0073, 0x0 } },
		// Ka Halant
		{ { 0x0995, 0x09cd, 0x0 },
		  { 0x00b9, 0x0 } },
		// Ka Halant Ka
		{ { 0x0995, 0x09cd, 0x0995, 0x0 },
		  { 0x0109, 0x0 } },
		// Ka MatraI
		{ { 0x0995, 0x09bf, 0x0 },
		  { 0x0095, 0x0073, 0x0 } },
		// Ra Halant Ka
		{ { 0x09b0, 0x09cd, 0x0995, 0x0 },
		  { 0x0073, 0x00e1, 0x0 } },
		// Ra Halant Ka MatraI
		{ { 0x09b0, 0x09cd, 0x0995, 0x09bf, 0x0 },
		  { 0x0095, 0x0073, 0x00e1, 0x0 } },
		// MatraI
 		{ { 0x09bf, 0x0 },
		  { 0x0095, 0x01c8, 0x0 } },
		// Ka Nukta
		{ { 0x0995, 0x09bc, 0x0 },
		  { 0x0073, 0x0093, 0x0 } },
		// Ka Halant Ra
		{ { 0x0995, 0x09cd, 0x09b0, 0x0 },
		  { 0x00e5, 0x0 } },
		// Ka Halant Ra Halant Ka
                { { 0x995, 0x9cd, 0x9b0, 0x9cd, 0x995, 0x0 },
                  { 0x234, 0x24e, 0x73, 0x0 } }, 
		// Ya + Halant
		{ { 0x09af, 0x09cd, 0x0 },
		  { 0x00d2, 0x0 } },
		// Da Halant Ya -> Da Ya-Phala
		{ { 0x09a6, 0x09cd, 0x09af, 0x0 },
		  { 0x0084, 0x00e2, 0x0 } },
		// A Halant Ya -> A Ya-phala
		{ { 0x0985, 0x09cd, 0x09af, 0x0 },
		  { 0x0067, 0x00e2, 0x0 } },
		// Na Halant Ka
		{ { 0x09a8, 0x09cd, 0x0995, 0x0 },
		  { 0x0188, 0x0 } },
		// Na Halant ZWNJ Ka
                { { 0x9a8, 0x9cd, 0x200c, 0x995, 0x0 },
                  { 0xcc, 0x73, 0x0 } }, 
		// Na Halant ZWJ Ka
                { { 0x9a8, 0x9cd, 0x200d, 0x995, 0x0 },
                  { 0x247, 0x73, 0x0 } }, 
		// Ka Halant ZWNJ Ka
                { { 0x9a8, 0x9cd, 0x200d, 0x995, 0x0 },
                  { 0x247, 0x73, 0x0 } }, 
		// Ka Halant ZWJ Ka
                { { 0x9a8, 0x9cd, 0x200d, 0x995, 0x0 },
                  { 0x247, 0x73, 0x0 } }, 
		// Na Halant Ra
		{ { 0x09a8, 0x09cd, 0x09b0, 0x0 },
		  { 0x00f8, 0x0 } },
		// Na Halant ZWNJ Ra
		{ { 0x09a8, 0x09cd, 0x200c, 0x09b0, 0x0 },
		  { 0xcc, 0x8d, 0x0 } },
		// Na Halant ZWJ Ra
                { { 0x9a8, 0x9cd, 0x200d, 0x9b0, 0x0 },
                  { 0x247, 0x8d, 0x0 } }, 
		// Na Halant Ba
		{ { 0x09a8, 0x09cd, 0x09ac, 0x0 },
		  { 0x0139, 0x0 } },
		// Na Halant ZWNJ Ba
                { { 0x9a8, 0x9cd, 0x200c, 0x9ac, 0x0 },
                  { 0xcc, 0x89, 0x0 } }, 
		// Na Halant ZWJ Ba
                { { 0x9a8, 0x9cd, 0x200d, 0x9ac, 0x0 },
                  { 0x247, 0x89, 0x0 } }, 
		// Na Halant Dha
		{ { 0x09a8, 0x09cd, 0x09a7, 0x0 },
		  { 0x0145, 0x0 } },
		// Na Halant ZWNJ Dha
		{ { 0x09a8, 0x09cd, 0x200c, 0x09a7, 0x0 },
		  { 0xcc, 0x85, 0x0 } },
		// Na Halant ZWJ Dha
		{ { 0x09a8, 0x09cd, 0x200d, 0x09a7, 0x0 },
		  { 0x247, 0x85, 0x0 } },
		// Ra Halant Ka MatraAU
                { { 0x9b0, 0x9cd, 0x995, 0x9cc, 0x0 },
                  { 0x232, 0x73, 0xe1, 0xa0, 0x0 } }, 
		// Ra Halant Ba Halant Ba
		{ { 0x09b0, 0x09cd, 0x09ac, 0x09cd, 0x09ac, 0x0 },
		  { 0x013b, 0x00e1, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Mukti");
    }
    {
        if (QFontDatabase().families(QFontDatabase::Bengali).contains("Likhan")) {
            QFont f("Likhan");
	    const ShapeTable shape_table [] = {
                { { 0x9a8, 0x9cd, 0x9af, 0x0 },
                  { 0x1ca, 0x0 } },
		{ { 0x09b8, 0x09cd, 0x09af, 0x0 },
                  { 0x020e, 0x0 } },
		{ { 0x09b6, 0x09cd, 0x09af, 0x0 },
                  { 0x01f4, 0x0 } },
		{ { 0x09b7, 0x09cd, 0x09af, 0x0 },
                  { 0x01fe, 0x0 } },
		{ { 0x09b0, 0x09cd, 0x09a8, 0x09cd, 0x200d, 0x0 },
                  { 0x10b, 0x167, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Likhan");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::gurmukhi()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Gurmukhi).contains("Lohit Punjabi")) {
            QFont f("Lohit Punjabi");
	    const ShapeTable shape_table [] = {
		{ { 0xA15, 0xA4D, 0xa39, 0x0 },
		  { 0x3b, 0x8b, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Lohit Punjabi");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::oriya()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Oriya).contains("utkal")) {
            QFont f("utkal");
	    const ShapeTable shape_table [] = {
                { { 0xb15, 0xb4d, 0xb24, 0xb4d, 0xb30, 0x0 },
                  { 0x150, 0x125, 0x0 } }, 
                { { 0xb24, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0x0 },
                  { 0x151, 0x120, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0x0 },
                  { 0x152, 0x120, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb2c, 0x0 },
                  { 0x152, 0x120, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb30, 0x0 },
                  { 0x176, 0x0 } }, 
                { { 0xb38, 0xb4d, 0xb24, 0xb4d, 0xb30, 0x0 },
                  { 0x177, 0x0 } }, 
                { { 0xb28, 0xb4d, 0xb24, 0xb4d, 0xb30, 0xb4d, 0xb2f, 0x0 },
                  { 0x176, 0x124, 0x0 } }, 
                { {0}, {0} }

            };

	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find utkal");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::tamil()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Tamil).contains("AkrutiTml1")) {
            QFont f("AkrutiTml1");
	    const ShapeTable shape_table [] = {
		{ { 0x0b95, 0x0bc2, 0x0 },
		  { 0x004e, 0x0 } },
		{ { 0x0bae, 0x0bc2, 0x0 },
		  { 0x009e, 0x0 } },
		{ { 0x0b9a, 0x0bc2, 0x0 },
		  { 0x0058, 0x0 } },
		{ { 0x0b99, 0x0bc2, 0x0 },
		  { 0x0053, 0x0 } },
		{ { 0x0bb0, 0x0bc2, 0x0 },
		  { 0x00a8, 0x0 } },
		{ { 0x0ba4, 0x0bc2, 0x0 },
		  { 0x008e, 0x0 } },
		{ { 0x0b9f, 0x0bc2, 0x0 },
		  { 0x0062, 0x0 } },
		{ { 0x0b95, 0x0bc6, 0x0 },
		  { 0x000a, 0x0031, 0x0 } },
		{ { 0x0b95, 0x0bca, 0x0 },
		  { 0x000a, 0x0031, 0x0007, 0x0 } },
		{ { 0x0b95, 0x0bc6, 0x0bbe, 0x0 },
		  { 0x000a, 0x0031, 0x007, 0x0 } },
		{ { 0x0b95, 0x0bcd, 0x0bb7, 0x0 },
		  { 0x0049, 0x0 } },
		{ { 0x0b95, 0x0bcd, 0x0bb7, 0x0bca, 0x0 },
		  { 0x000a, 0x0049, 0x007, 0x0 } },
		{ { 0x0b95, 0x0bcd, 0x0bb7, 0x0bc6, 0x0bbe, 0x0 },
		  { 0x000a, 0x0049, 0x007, 0x0 } },
		{ { 0x0b9f, 0x0bbf, 0x0 },
		  { 0x005f, 0x0 } },
		{ { 0x0b9f, 0x0bc0, 0x0 },
		  { 0x0060, 0x0 } },
		{ { 0x0bb2, 0x0bc0, 0x0 },
		  { 0x00ab, 0x0 } },
		{ { 0x0bb2, 0x0bbf, 0x0 },
		  { 0x00aa, 0x0 } },
		{ { 0x0bb0, 0x0bcd, 0x0 },
		  { 0x00a4, 0x0 } },
		{ { 0x0bb0, 0x0bbf, 0x0 },
		  { 0x00a5, 0x0 } },
		{ { 0x0bb0, 0x0bc0, 0x0 },
		  { 0x00a6, 0x0 } },
		{ { 0x0b83, 0x0 },
		  { 0x0025, 0x0 } },
		{ { 0x0b83, 0x0b95, 0x0 },
		  { 0x0025, 0x0031, 0x0 } },
                { { 0xb95, 0xbc6, 0xbbe, 0x0 },
                  { 0xa, 0x31, 0x7, 0x0 } },
                { { 0xb95, 0xbc7, 0xbbe, 0x0 },
                  { 0xb, 0x31, 0x7, 0x0 } },
                { { 0xb95, 0xbc6, 0xbd7, 0x0 },
                  { 0xa, 0x31, 0x40, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find AkrutiTml1");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::telugu()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Telugu).contains("Pothana2000")) {
            QFont f("Pothana2000");
	    const ShapeTable shape_table [] = {
                { { 0xc15, 0xc4d, 0x0 },
                  { 0xbb, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc37, 0x0 },
                  { 0x4b, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc37, 0xc4d, 0x0 },
                  { 0xe0, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc37, 0xc4d, 0xc23, 0x0 },
                  { 0x4b, 0x91, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0x0 },
                  { 0x5a, 0xb2, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0xc4d, 0x0 },
                  { 0xbb, 0xb2, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0xc4d, 0xc15, 0x0 },
                  { 0x5a, 0xb2, 0x83, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc30, 0xc3f, 0x0 },
                  { 0xe2, 0xb2, 0x0 } }, 
                { { 0xc15, 0xc4d, 0xc15, 0xc48, 0x0 },
                  { 0xe6, 0xb3, 0x83, 0x0 } },
                { { 0xc15, 0xc4d, 0xc30, 0xc48, 0x0 },
                  { 0xe6, 0xb3, 0x9f, 0x0 } }, 
                { { 0xc15, 0xc46, 0xc56, 0x0 },
                  { 0xe6, 0xb3, 0x0 } },
                { {0}, {0} }

            };

	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Pothana2000");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::kannada()
{
#if defined(Q_WS_X11)
    {
        if (QFontDatabase().families(QFontDatabase::Kannada).contains("Sampige")) {
            QFont f("Sampige");
	    const ShapeTable shape_table [] = {
		{ { 0x0ca8, 0x0ccd, 0x0ca8, 0x0 },
		  { 0x0049, 0x00ba, 0x0 } },
		{ { 0x0ca8, 0x0ccd, 0x0ca1, 0x0 },
		  { 0x0049, 0x00b3, 0x0 } },
		{ { 0x0caf, 0x0cc2, 0x0 },
		  { 0x004f, 0x005d, 0x0 } },
		{ { 0x0ce0, 0x0 },
		  { 0x006a, 0x0 } },
		{ { 0x0ce6, 0x0ce7, 0x0ce8, 0x0 },
		  { 0x006b, 0x006c, 0x006d, 0x0 } },
		{ { 0x0cb5, 0x0ccb, 0x0 },
		  { 0x015f, 0x0067, 0x0 } },
		{ { 0x0cb0, 0x0ccd, 0x0cae, 0x0 },
		  { 0x004e, 0x0082, 0x0 } },
		{ { 0x0cb0, 0x0ccd, 0x0c95, 0x0 },
		  { 0x0036, 0x0082, 0x0 } },
		{ { 0x0c95, 0x0ccd, 0x0cb0, 0x0 },
		  { 0x0036, 0x00c1, 0x0 } },
		{ { 0x0cb0, 0x0ccd, 0x200d, 0x0c95, 0x0 },
		  { 0x0050, 0x00a7, 0x0 } },

		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Sampige");
    }
    {
        if (QFontDatabase().families(QFontDatabase::Kannada).contains("Tunga")) {
            QFont f("Tunga");
	    const ShapeTable shape_table [] = {
		{ { 0x0cb7, 0x0cc6, 0x0 },
		  { 0x00b0, 0x006c, 0x0 } },
		{ { 0x0cb7, 0x0ccd, 0x0 },
		  { 0x0163, 0x0 } },
                { { 0xc95, 0xcbf, 0xcd5, 0x0 },
                  { 0x114, 0x73, 0x0 } },
                { { 0xc95, 0xcc6, 0xcd5, 0x0 },
                  { 0x90, 0x6c, 0x73, 0x0 } },
                { { 0xc95, 0xcc6, 0xcd6, 0x0 },
                  { 0x90, 0x6c, 0x74, 0x0 } },
                { { 0xc95, 0xcc6, 0xcc2, 0x0 },
                  { 0x90, 0x6c, 0x69, 0x0 } },
                { { 0xc95, 0xcca, 0xcd5, 0x0 },
                  { 0x90, 0x6c, 0x69, 0x73, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Tunga");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::malayalam()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Malayalam).contains("AkrutiMal2")) {
            QFont f("AkrutiMal2");
	    const ShapeTable shape_table [] = {
		{ { 0x0d15, 0x0d46, 0x0 },
		  { 0x005e, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d47, 0x0 },
		  { 0x005f, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d4b, 0x0 },
		  { 0x005f, 0x0034, 0x0058, 0x0 } },
		{ { 0x0d15, 0x0d48, 0x0 },
		  { 0x0060, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d4a, 0x0 },
		  { 0x005e, 0x0034, 0x0058, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x0d15, 0x0 },
		  { 0x009e, 0x0034, 0x0 } },
		{ { 0x0d15, 0x0d4d, 0x0d35, 0x0 },
		  { 0x0034, 0x007a, 0x0 } },
		{ { 0x0d15, 0x0d4d, 0x0d2f, 0x0 },
		  { 0x0034, 0x00a2, 0x0 } },
		{ { 0x0d1f, 0x0d4d, 0x0d1f, 0x0 },
		  { 0x0069, 0x0 } },
		{ { 0x0d26, 0x0d4d, 0x0d26, 0x0 },
		  { 0x0074, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x0 },
		  { 0x009e, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x200c, 0x0 },
		  { 0x009e, 0x0 } },
		{ { 0x0d30, 0x0d4d, 0x200d, 0x0 },
		  { 0x009e, 0x0 } },
                { { 0xd15, 0xd46, 0xd3e, 0x0 },
                  { 0x5e, 0x34, 0x58, 0x0 } },
                { { 0xd15, 0xd47, 0xd3e, 0x0 },
                  { 0x5f, 0x34, 0x58, 0x0 } },
                { { 0xd15, 0xd46, 0xd57, 0x0 },
                  { 0x5e, 0x34, 0x65, 0x0 } },
                { { 0xd15, 0xd57, 0x0 },
                  { 0x34, 0x65, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find AkrutiMal2");
    }
    {
        if (QFontDatabase().families(QFontDatabase::Malayalam).contains("Rachana")) {
            QFont f("Rachana");
            const ShapeTable shape_table [] = {
                { { 0xd37, 0xd4d, 0xd1f, 0xd4d, 0xd30, 0xd40, 0x0 },
                  { 0x385, 0xa3, 0x0 } },
                { { 0xd2f, 0xd4d, 0xd15, 0xd4d, 0xd15, 0xd41, 0x0 },
                  { 0x2ff, 0x0 } },
                { { 0xd33, 0xd4d, 0xd33, 0x0 },
                  { 0x3f8, 0x0 } },
                { { 0xd2f, 0xd4d, 0xd15, 0xd4d, 0xd15, 0xd41, 0x0 },
                  { 0x2ff, 0x0 } },
                { { 0xd30, 0xd4d, 0x200d, 0xd35, 0xd4d, 0xd35, 0x0 },
                  { 0xf3, 0x350, 0x0 } },

                { {0}, {0} }
            };


            const ShapeTable *s = shape_table;
            while (s->unicode[0]) {
                QVERIFY( shaping(f, s) );
                ++s;
            }
        } else
            QSKIP("couldn't find Rachana");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::sinhala()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Sinhala).contains("Malithi Web")) {
            QFont f("Malithi Web");
            const ShapeTable shape_table [] = {
                { { 0xd9a, 0xdd9, 0xdcf, 0x0 },
                  { 0x4a, 0x61, 0x42, 0x0 } },
                { { 0xd9a, 0xdd9, 0xddf, 0x0 },
                  { 0x4a, 0x61, 0x50, 0x0 } },
                { { 0xd9a, 0xdd9, 0xdca, 0x0 },
                  { 0x4a, 0x62, 0x0 } },
                { { 0xd9a, 0xddc, 0xdca, 0x0 },
                  { 0x4a, 0x61, 0x42, 0x41, 0x0 } },
                { { 0xd9a, 0xdda, 0x0 },
                  { 0x4a, 0x62, 0x0 } },
                { { 0xd9a, 0xddd, 0x0 },
                  { 0x4a, 0x61, 0x42, 0x41, 0x0 } },
                { {0}, {0} }
            };


            const ShapeTable *s = shape_table;
            while (s->unicode[0]) {
                QVERIFY( shaping(f, s) );
                ++s;
            }
        } else
            QSKIP("couldn't find Malithi Web");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::khmer()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Khmer).contains("Khmer OS")) {
            QFont f("Khmer OS");
	    const ShapeTable shape_table [] = {
		{ { 0x179a, 0x17cd, 0x0 },
		  { 0x24c, 0x27f, 0x0 } },
		{ { 0x179f, 0x17c5, 0x0 },
		  { 0x273, 0x203, 0x0 } },
		{ { 0x1790, 0x17d2, 0x1784, 0x17c3, 0x0 },
		  { 0x275, 0x242, 0x182, 0x0 } },
		{ { 0x179a, 0x0 },
		  { 0x24c, 0x0 } },
		{ { 0x1781, 0x17d2, 0x1798, 0x17c2, 0x0 },
		  { 0x274, 0x233, 0x197, 0x0 } },
		{ { 0x1798, 0x17b6, 0x0 },
		  { 0x1cb, 0x0 } },
		{ { 0x179a, 0x17b8, 0x0 },
		  { 0x24c, 0x26a, 0x0 } },
		{ { 0x1787, 0x17b6, 0x0 },
		  { 0x1ba, 0x0 } },
		{ { 0x1798, 0x17d2, 0x1796, 0x17bb, 0x0 },
		  { 0x24a, 0x195, 0x26d, 0x0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Khmer OS");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::linearB()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Any).contains("Penuturesu")) {
            QFont f("Penuturesu");
	    const ShapeTable shape_table [] = {
		{ { 0xd800, 0xdc01, 0xd800, 0xdc02, 0xd800, 0xdc03,  0 },
                  { 0x5, 0x6, 0x7, 0 } },
		{ {0}, {0} }
	    };


	    const ShapeTable *s = shape_table;
	    while (s->unicode[0]) {
		QVERIFY( shaping(f, s) );
		++s;
	    }
	} else
	    QSKIP("couldn't find Penuturesu");
    }
#else
    QSKIP("X11 specific test");
#endif
}

#if defined(Q_WS_X11)
static bool decomposedShaping(const QFont &f, QChar ch)
{
    QString str = QString().append(ch);
    QTextLayout layout(str, f);
    QTextEngine *e = layout.d;
    e->itemize();
    e->shape(0);

    QTextLayout decomposed(str.normalized(QString::NormalizationForm_D), f);
    QTextEngine *de = decomposed.d;
    de->itemize();
    de->shape(0);

    if( e->layoutData->items[0].num_glyphs != de->layoutData->items[0].num_glyphs )
        goto error;

    for (int i = 0; i < e->layoutData->items[0].num_glyphs; ++i) {
        if ((e->layoutData->glyphLayout.glyphs[i] & 0xffffff) != (de->layoutData->glyphLayout.glyphs[i] & 0xffffff))
            goto error;
    }
    return true;
 error:
    qDebug("%s: decomposedShaping of char %4x failed, nglyphs=%d, decomposed nglyphs %d",
           f.family().toLatin1().constData(),
           ch.unicode(),
           e->layoutData->items[0].num_glyphs,
           de->layoutData->items[0].num_glyphs);

    str = "";
    int i = 0;
    while (i < e->layoutData->items[0].num_glyphs) {
        str += QString("%1 ").arg(e->layoutData->glyphLayout.glyphs[i], 4, 16);
        ++i;
    }
    qDebug("    composed glyph result   = %s", str.toLatin1().constData());
    str = "";
    i = 0;
    while (i < de->layoutData->items[0].num_glyphs) {
        str += QString("%1 ").arg(de->layoutData->glyphLayout.glyphs[i], 4, 16);
        ++i;
    }
    qDebug("    decomposed glyph result = %s", str.toLatin1().constData());
    return false;
}
#endif

void tst_QTextScriptEngine::greek()
{
#if defined(Q_WS_X11)
    if (!haveTestFonts)
        QSKIP("Test fonts are not available");

    {
        if (QFontDatabase().families(QFontDatabase::Any).contains("DejaVu Sans")) {
            QFont f("DejaVu Sans");
            for (int uc = 0x1f00; uc <= 0x1fff; ++uc) {
                QString str;
                str.append(uc);
                if (str.normalized(QString::NormalizationForm_D).normalized(QString::NormalizationForm_C) != str) {
                    //qDebug() << "skipping" << hex << uc;
                    continue;
                }
                if (uc == 0x1fc1 || uc == 0x1fed)
                    continue;
                QVERIFY( decomposedShaping(f, QChar(uc)) );
            }
        } else
            QSKIP("couldn't find DejaVu Sans");
    }

    {
        if (QFontDatabase().families(QFontDatabase::Any).contains("SBL Greek")) {
            QFont f("SBL Greek");
            for (int uc = 0x1f00; uc <= 0x1fff; ++uc) {
                QString str;
                str.append(uc);
                if (str.normalized(QString::NormalizationForm_D).normalized(QString::NormalizationForm_C) != str) {
                    //qDebug() << "skipping" << hex << uc;
                    continue;
                }
                if (uc == 0x1fc1 || uc == 0x1fed)
                    continue;
                QVERIFY( decomposedShaping(f, QChar(uc) ) );

            }

            const ShapeTable shape_table [] = {
                { { 0x3b1, 0x300, 0x313, 0x0 },
                  { 0xb8, 0x3d3, 0x3c7, 0x0 } },
                { { 0x3b1, 0x313, 0x300, 0x0 },
                  { 0xd4, 0x0 } },

                { {0}, {0} }
            };


            const ShapeTable *s = shape_table;
            while (s->unicode[0]) {
                QVERIFY( shaping(f, s) );
                ++s;
            }
        } else
            QSKIP("couldn't find SBL_grk");
    }
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::controlInSyllable_qtbug14204()
{
#if defined(Q_WS_X11)
    QString s;
    s.append(QChar(0x0915));
    s.append(QChar(0x094d));
    s.append(QChar(0x200d));
    s.append(QChar(0x0915));

    QTextLayout layout(s);
    QTextEngine *e = layout.d;
    e->itemize();
    e->shape(0);

    QVERIFY(e->layoutData->items[0].num_glyphs == 2);
    QVERIFY(e->layoutData->glyphLayout.advances_x[1] != 0);
#else
    QSKIP("X11 specific test");
#endif
}

void tst_QTextScriptEngine::combiningMarks_qtbug15675()
{
#if defined(Q_OS_MAC)
    QString s;
    s.append(QChar(0x0061));
    s.append(QChar(0x0062));
    s.append(QChar(0x0300));
    s.append(QChar(0x0063));

    QFont font("Monaco");
    QTextLayout layout(s, font);
    QTextEngine *e = layout.engine();
    e->itemize();
    e->shape(0);

    QVERIFY(e->layoutData->items[0].num_glyphs == 4);
    QEXPECT_FAIL("", "QTBUG-23064", Abort);
    QVERIFY(e->layoutData->glyphLayout.advances_y[2] > 0);
#elif defined(Q_WS_X11)
    QFontDatabase db;

    if (!db.families().contains("DejaVu Sans Mono"))
        QSKIP("Required font (DejaVu Sans Mono) doesn't exist, skip test.");

    QString s;
    s.append(QChar(0x0062));
    s.append(QChar(0x0332));
    s.append(QChar(0x0063));

    QTextLayout layout(s, QFont("DejaVu Sans Mono"));
    QTextEngine *e = layout.d;
    e->itemize();
    e->shape(0);

    QVERIFY(e->layoutData->items[0].num_glyphs == 3);
    QVERIFY(e->layoutData->glyphLayout.advances_x[1] == 0);
#else
    QSKIP("X11/Mac specific test");
#endif
}

void tst_QTextScriptEngine::mirroredChars_data()
{
    QTest::addColumn<int>("hintingPreference");

    QTest::newRow("Default hinting") << int(QFont::PreferDefaultHinting);
    QTest::newRow("No hinting") << int(QFont::PreferNoHinting);
    QTest::newRow("Vertical hinting") << int(QFont::PreferVerticalHinting);
    QTest::newRow("Full hinting") << int(QFont::PreferFullHinting);
}

void tst_QTextScriptEngine::mirroredChars()
{
#if defined(Q_OS_MAC)
    QSKIP("Not supported on Mac");
#endif
    QFETCH(int, hintingPreference);

    QFont font;
    font.setHintingPreference(QFont::HintingPreference(hintingPreference));

    QString s;
    s.append(QLatin1Char('('));
    s.append(QLatin1Char(')'));

    HB_Glyph leftParenthesis;
    HB_Glyph rightParenthesis;
    {
        QTextLayout layout(s);
        layout.setCacheEnabled(true);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QTextEngine *e = layout.engine();
        e->itemize();
        e->shape(0);
        QCOMPARE(e->layoutData->items[0].num_glyphs, ushort(2));

        const QGlyphLayout &glyphLayout = e->layoutData->glyphLayout;
        leftParenthesis = glyphLayout.glyphs[0];
        rightParenthesis = glyphLayout.glyphs[1];
    }

    {
        QTextLayout layout(s);
        layout.setFlags(Qt::TextForceRightToLeft);

        QTextEngine *e = layout.engine();
        e->itemize();
        e->shape(0);
        QCOMPARE(e->layoutData->items[0].num_glyphs, ushort(2));

        const QGlyphLayout &glyphLayout = e->layoutData->glyphLayout;
        QCOMPARE(glyphLayout.glyphs[0], rightParenthesis);
        QCOMPARE(glyphLayout.glyphs[1], leftParenthesis);
    }
}

void tst_QTextScriptEngine::thaiIsolatedSaraAm()
{
    if (QFontDatabase().families(QFontDatabase::Any).contains("Waree")) {
        QString s;
        s.append(QChar(0x0e33));

        QTextLayout layout(s, QFont("Waree"));
        layout.setCacheEnabled(true);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QTextEngine *e = layout.engine();
        e->itemize();
        e->shape(0);
        QCOMPARE(e->layoutData->items[0].num_glyphs, ushort(3));

        unsigned short *logClusters = e->layoutData->logClustersPtr;
        QCOMPARE(logClusters[0], ushort(0));
    } else
        QSKIP("Cannot find Waree.");
}

void tst_QTextScriptEngine::thaiWithZWJ()
{
#ifdef Q_OS_WIN
    QSKIP("This test currently fails on Windows - QTBUG-24565");
#endif
    QString s(QString::fromUtf8("\xe0\xb8\xa3\xe2\x80\x8d\xe0\xb8\xa3\xe2\x80"
                                "\x8c\x2e\xe0\xb8\xa3\x2e\xe2\x80\x9c\xe0\xb8"
                                "\xa3\xe2\x80\xa6\xe0\xb8\xa3\xe2\x80\x9d\xe0"
                                "\xb8\xa3\xa0\xe0\xb8\xa3\xe6\x9c\xac\xe0\xb8\xa3")
              + QChar(0x0363)/*superscript 'a', for testing Inherited class*/);
    QTextLayout layout(s);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *e = layout.engine();
    e->width(0, s.length()); //force itemize and shape

    // A thai implementation could either remove the ZWJ and ZWNJ characters, or hide them.
    // The current implementation hides them, so we test for that.
    // But make sure that we don't hide anything else
    QCOMPARE(e->layoutData->items.size(), 11);
    QCOMPARE(e->layoutData->items[0].num_glyphs, ushort(7));  // Thai: The ZWJ and ZWNJ characters are inherited, so should be part of the thai script
    QCOMPARE(e->layoutData->items[1].num_glyphs, ushort(1));  // Common: The smart quotes cannot be handled by thai, so should be a separate item
    QCOMPARE(e->layoutData->items[2].num_glyphs, ushort(1));  // Thai: Thai character
    QCOMPARE(e->layoutData->items[3].num_glyphs, ushort(1));  // Common: Ellipsis
    QCOMPARE(e->layoutData->items[4].num_glyphs, ushort(1));  // Thai: Thai character
    QCOMPARE(e->layoutData->items[5].num_glyphs, ushort(1));  // Common: Smart quote
    QCOMPARE(e->layoutData->items[6].num_glyphs, ushort(1));  // Thai: Thai character
    QCOMPARE(e->layoutData->items[7].num_glyphs, ushort(1));  // Common: \xA0 = non-breaking space. Could be useful to have in thai, but not currently implemented
    QCOMPARE(e->layoutData->items[8].num_glyphs, ushort(1));  // Thai: Thai character
    QCOMPARE(e->layoutData->items[9].num_glyphs, ushort(1));  // Japanese: Kanji for tree
    QCOMPARE(e->layoutData->items[10].num_glyphs, ushort(2)); // Thai: Thai character followed by superscript "a" which is of inherited type

    //A quick sanity check - check all the characters are individual clusters
    unsigned short *logClusters = e->layoutData->logClustersPtr;
    for (int i = 0; i < 7; i++)
        QCOMPARE(logClusters[i], ushort(i));
    for (int i = 0; i < 10; i++)
        QCOMPARE(logClusters[i+7], ushort(0));
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23064", Abort);
#endif
    QCOMPARE(logClusters[17], ushort(1));

    // The only characters that we should be hiding are the ZWJ and ZWNJ characters in position 1
    // and 3.
    for (int i = 0; i < 18; i++)
        QCOMPARE((bool)e->layoutData->glyphLayout.attributes[i].dontPrint, (i == 1 || i == 3));
}

void tst_QTextScriptEngine::thaiMultipleVowels()
{
    QString s(QString::fromUtf8("\xe0\xb8\xaa"));
    for (int i = 0; i < 100; i++)
        s += QChar(0x0E47); // Add lots of "VOWEL SIGN MAI TAI KHU N/S-T"  stacked on top of the character
    s += QChar(0x200D); // Now add a zero width joiner (which adds a circle which is hidden)
    for (int i = 0; i < 100; i++)
        s += QChar(0x0E47); //Add lots of "VOWEL SIGN MAI TAI KHU N/S-T"  stacked on top of the ZWJ

    for (int i = 0; i < 10; i++)
        s += s; //Repeat the string to make it more likely to crash if we have a buffer overflow
    QTextLayout layout(s);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *e = layout.engine();
    e->width(0, s.length()); //force itemize and shape

    // If we haven't crashed at this point, then the test has passed.
}

QTEST_MAIN(tst_QTextScriptEngine)
#include "tst_qtextscriptengine.moc"
