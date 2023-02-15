// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtexthtmlparser_p.h"

#include <qbytearray.h>
#include <qstack.h>
#include <qdebug.h>
#include <qthread.h>
#include <qguiapplication.h>

#include "qtextdocument.h"
#include "qtextformat_p.h"
#include "qtextdocument_p.h"
#include "qtextcursor.h"
#include "qfont_p.h"

#include <algorithm>

#ifndef QT_NO_TEXTHTMLPARSER

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// see also tst_qtextdocumentfragment.cpp
#define MAX_ENTITY 258
static const struct QTextHtmlEntity { const char name[9]; char16_t code; } entities[]= {
    { "AElig", 0x00c6 },
    { "AMP", 38 },
    { "Aacute", 0x00c1 },
    { "Acirc", 0x00c2 },
    { "Agrave", 0x00c0 },
    { "Alpha", 0x0391 },
    { "Aring", 0x00c5 },
    { "Atilde", 0x00c3 },
    { "Auml", 0x00c4 },
    { "Beta", 0x0392 },
    { "Ccedil", 0x00c7 },
    { "Chi", 0x03a7 },
    { "Dagger", 0x2021 },
    { "Delta", 0x0394 },
    { "ETH", 0x00d0 },
    { "Eacute", 0x00c9 },
    { "Ecirc", 0x00ca },
    { "Egrave", 0x00c8 },
    { "Epsilon", 0x0395 },
    { "Eta", 0x0397 },
    { "Euml", 0x00cb },
    { "GT", 62 },
    { "Gamma", 0x0393 },
    { "Iacute", 0x00cd },
    { "Icirc", 0x00ce },
    { "Igrave", 0x00cc },
    { "Iota", 0x0399 },
    { "Iuml", 0x00cf },
    { "Kappa", 0x039a },
    { "LT", 60 },
    { "Lambda", 0x039b },
    { "Mu", 0x039c },
    { "Ntilde", 0x00d1 },
    { "Nu", 0x039d },
    { "OElig", 0x0152 },
    { "Oacute", 0x00d3 },
    { "Ocirc", 0x00d4 },
    { "Ograve", 0x00d2 },
    { "Omega", 0x03a9 },
    { "Omicron", 0x039f },
    { "Oslash", 0x00d8 },
    { "Otilde", 0x00d5 },
    { "Ouml", 0x00d6 },
    { "Phi", 0x03a6 },
    { "Pi", 0x03a0 },
    { "Prime", 0x2033 },
    { "Psi", 0x03a8 },
    { "QUOT", 34 },
    { "Rho", 0x03a1 },
    { "Scaron", 0x0160 },
    { "Sigma", 0x03a3 },
    { "THORN", 0x00de },
    { "Tau", 0x03a4 },
    { "Theta", 0x0398 },
    { "Uacute", 0x00da },
    { "Ucirc", 0x00db },
    { "Ugrave", 0x00d9 },
    { "Upsilon", 0x03a5 },
    { "Uuml", 0x00dc },
    { "Xi", 0x039e },
    { "Yacute", 0x00dd },
    { "Yuml", 0x0178 },
    { "Zeta", 0x0396 },
    { "aacute", 0x00e1 },
    { "acirc", 0x00e2 },
    { "acute", 0x00b4 },
    { "aelig", 0x00e6 },
    { "agrave", 0x00e0 },
    { "alefsym", 0x2135 },
    { "alpha", 0x03b1 },
    { "amp", 38 },
    { "and", 0x22a5 },
    { "ang", 0x2220 },
    { "apos", 0x0027 },
    { "aring", 0x00e5 },
    { "asymp", 0x2248 },
    { "atilde", 0x00e3 },
    { "auml", 0x00e4 },
    { "bdquo", 0x201e },
    { "beta", 0x03b2 },
    { "brvbar", 0x00a6 },
    { "bull", 0x2022 },
    { "cap", 0x2229 },
    { "ccedil", 0x00e7 },
    { "cedil", 0x00b8 },
    { "cent", 0x00a2 },
    { "chi", 0x03c7 },
    { "circ", 0x02c6 },
    { "clubs", 0x2663 },
    { "cong", 0x2245 },
    { "copy", 0x00a9 },
    { "crarr", 0x21b5 },
    { "cup", 0x222a },
    { "curren", 0x00a4 },
    { "dArr", 0x21d3 },
    { "dagger", 0x2020 },
    { "darr", 0x2193 },
    { "deg", 0x00b0 },
    { "delta", 0x03b4 },
    { "diams", 0x2666 },
    { "divide", 0x00f7 },
    { "eacute", 0x00e9 },
    { "ecirc", 0x00ea },
    { "egrave", 0x00e8 },
    { "empty", 0x2205 },
    { "emsp", 0x2003 },
    { "ensp", 0x2002 },
    { "epsilon", 0x03b5 },
    { "equiv", 0x2261 },
    { "eta", 0x03b7 },
    { "eth", 0x00f0 },
    { "euml", 0x00eb },
    { "euro", 0x20ac },
    { "exist", 0x2203 },
    { "fnof", 0x0192 },
    { "forall", 0x2200 },
    { "frac12", 0x00bd },
    { "frac14", 0x00bc },
    { "frac34", 0x00be },
    { "frasl", 0x2044 },
    { "gamma", 0x03b3 },
    { "ge", 0x2265 },
    { "gt", 62 },
    { "hArr", 0x21d4 },
    { "harr", 0x2194 },
    { "hearts", 0x2665 },
    { "hellip", 0x2026 },
    { "iacute", 0x00ed },
    { "icirc", 0x00ee },
    { "iexcl", 0x00a1 },
    { "igrave", 0x00ec },
    { "image", 0x2111 },
    { "infin", 0x221e },
    { "int", 0x222b },
    { "iota", 0x03b9 },
    { "iquest", 0x00bf },
    { "isin", 0x2208 },
    { "iuml", 0x00ef },
    { "kappa", 0x03ba },
    { "lArr", 0x21d0 },
    { "lambda", 0x03bb },
    { "lang", 0x2329 },
    { "laquo", 0x00ab },
    { "larr", 0x2190 },
    { "lceil", 0x2308 },
    { "ldquo", 0x201c },
    { "le", 0x2264 },
    { "lfloor", 0x230a },
    { "lowast", 0x2217 },
    { "loz", 0x25ca },
    { "lrm", 0x200e },
    { "lsaquo", 0x2039 },
    { "lsquo", 0x2018 },
    { "lt", 60 },
    { "macr", 0x00af },
    { "mdash", 0x2014 },
    { "micro", 0x00b5 },
    { "middot", 0x00b7 },
    { "minus", 0x2212 },
    { "mu", 0x03bc },
    { "nabla", 0x2207 },
    { "nbsp", 0x00a0 },
    { "ndash", 0x2013 },
    { "ne", 0x2260 },
    { "ni", 0x220b },
    { "not", 0x00ac },
    { "notin", 0x2209 },
    { "nsub", 0x2284 },
    { "ntilde", 0x00f1 },
    { "nu", 0x03bd },
    { "oacute", 0x00f3 },
    { "ocirc", 0x00f4 },
    { "oelig", 0x0153 },
    { "ograve", 0x00f2 },
    { "oline", 0x203e },
    { "omega", 0x03c9 },
    { "omicron", 0x03bf },
    { "oplus", 0x2295 },
    { "or", 0x22a6 },
    { "ordf", 0x00aa },
    { "ordm", 0x00ba },
    { "oslash", 0x00f8 },
    { "otilde", 0x00f5 },
    { "otimes", 0x2297 },
    { "ouml", 0x00f6 },
    { "para", 0x00b6 },
    { "part", 0x2202 },
    { "percnt", 0x0025 },
    { "permil", 0x2030 },
    { "perp", 0x22a5 },
    { "phi", 0x03c6 },
    { "pi", 0x03c0 },
    { "piv", 0x03d6 },
    { "plusmn", 0x00b1 },
    { "pound", 0x00a3 },
    { "prime", 0x2032 },
    { "prod", 0x220f },
    { "prop", 0x221d },
    { "psi", 0x03c8 },
    { "quot", 34 },
    { "rArr", 0x21d2 },
    { "radic", 0x221a },
    { "rang", 0x232a },
    { "raquo", 0x00bb },
    { "rarr", 0x2192 },
    { "rceil", 0x2309 },
    { "rdquo", 0x201d },
    { "real", 0x211c },
    { "reg", 0x00ae },
    { "rfloor", 0x230b },
    { "rho", 0x03c1 },
    { "rlm", 0x200f },
    { "rsaquo", 0x203a },
    { "rsquo", 0x2019 },
    { "sbquo", 0x201a },
    { "scaron", 0x0161 },
    { "sdot", 0x22c5 },
    { "sect", 0x00a7 },
    { "shy", 0x00ad },
    { "sigma", 0x03c3 },
    { "sigmaf", 0x03c2 },
    { "sim", 0x223c },
    { "spades", 0x2660 },
    { "sub", 0x2282 },
    { "sube", 0x2286 },
    { "sum", 0x2211 },
    { "sup", 0x2283 },
    { "sup1", 0x00b9 },
    { "sup2", 0x00b2 },
    { "sup3", 0x00b3 },
    { "supe", 0x2287 },
    { "szlig", 0x00df },
    { "tau", 0x03c4 },
    { "there4", 0x2234 },
    { "theta", 0x03b8 },
    { "thetasym", 0x03d1 },
    { "thinsp", 0x2009 },
    { "thorn", 0x00fe },
    { "tilde", 0x02dc },
    { "times", 0x00d7 },
    { "trade", 0x2122 },
    { "uArr", 0x21d1 },
    { "uacute", 0x00fa },
    { "uarr", 0x2191 },
    { "ucirc", 0x00fb },
    { "ugrave", 0x00f9 },
    { "uml", 0x00a8 },
    { "upsih", 0x03d2 },
    { "upsilon", 0x03c5 },
    { "uuml", 0x00fc },
    { "weierp", 0x2118 },
    { "xi", 0x03be },
    { "yacute", 0x00fd },
    { "yen", 0x00a5 },
    { "yuml", 0x00ff },
    { "zeta", 0x03b6 },
    { "zwj", 0x200d },
    { "zwnj", 0x200c }
};
static_assert(MAX_ENTITY == sizeof entities / sizeof *entities);

#if defined(Q_CC_MSVC_ONLY) && _MSC_VER < 1600
bool operator<(const QTextHtmlEntity &entity1, const QTextHtmlEntity &entity2)
{
    return QLatin1StringView(entity1.name) < QLatin1StringView(entity2.name);
}
#endif

static bool operator<(QStringView entityStr, const QTextHtmlEntity &entity)
{
    return entityStr < QLatin1StringView(entity.name);
}

static bool operator<(const QTextHtmlEntity &entity, QStringView entityStr)
{
    return QLatin1StringView(entity.name) < entityStr;
}

static QChar resolveEntity(QStringView entity)
{
    const QTextHtmlEntity *start = &entities[0];
    const QTextHtmlEntity *end = &entities[MAX_ENTITY];
    const QTextHtmlEntity *e = std::lower_bound(start, end, entity);
    if (e == end || (entity < *e))
        return QChar();
    return e->code;
}

static const ushort windowsLatin1ExtendedCharacters[0xA0 - 0x80] = {
    0x20ac, // 0x80
    0x0081, // 0x81 direct mapping
    0x201a, // 0x82
    0x0192, // 0x83
    0x201e, // 0x84
    0x2026, // 0x85
    0x2020, // 0x86
    0x2021, // 0x87
    0x02C6, // 0x88
    0x2030, // 0x89
    0x0160, // 0x8A
    0x2039, // 0x8B
    0x0152, // 0x8C
    0x008D, // 0x8D direct mapping
    0x017D, // 0x8E
    0x008F, // 0x8F directmapping
    0x0090, // 0x90 directmapping
    0x2018, // 0x91
    0x2019, // 0x92
    0x201C, // 0x93
    0X201D, // 0x94
    0x2022, // 0x95
    0x2013, // 0x96
    0x2014, // 0x97
    0x02DC, // 0x98
    0x2122, // 0x99
    0x0161, // 0x9A
    0x203A, // 0x9B
    0x0153, // 0x9C
    0x009D, // 0x9D direct mapping
    0x017E, // 0x9E
    0x0178  // 0x9F
};

// the displayMode value is according to the what are blocks in the piecetable, not
// what the w3c defines.
static const QTextHtmlElement elements[Html_NumElements]= {
    { "a",          Html_a,          QTextHtmlElement::DisplayInline },
    { "address",    Html_address,    QTextHtmlElement::DisplayInline },
    { "b",          Html_b,          QTextHtmlElement::DisplayInline },
    { "big",        Html_big,        QTextHtmlElement::DisplayInline },
    { "blockquote", Html_blockquote, QTextHtmlElement::DisplayBlock },
    { "body",       Html_body,       QTextHtmlElement::DisplayBlock },
    { "br",         Html_br,         QTextHtmlElement::DisplayInline },
    { "caption",    Html_caption,    QTextHtmlElement::DisplayBlock },
    { "center",     Html_center,     QTextHtmlElement::DisplayBlock },
    { "cite",       Html_cite,       QTextHtmlElement::DisplayInline },
    { "code",       Html_code,       QTextHtmlElement::DisplayInline },
    { "dd",         Html_dd,         QTextHtmlElement::DisplayBlock },
    { "dfn",        Html_dfn,        QTextHtmlElement::DisplayInline },
    { "div",        Html_div,        QTextHtmlElement::DisplayBlock },
    { "dl",         Html_dl,         QTextHtmlElement::DisplayBlock },
    { "dt",         Html_dt,         QTextHtmlElement::DisplayBlock },
    { "em",         Html_em,         QTextHtmlElement::DisplayInline },
    { "font",       Html_font,       QTextHtmlElement::DisplayInline },
    { "h1",         Html_h1,         QTextHtmlElement::DisplayBlock },
    { "h2",         Html_h2,         QTextHtmlElement::DisplayBlock },
    { "h3",         Html_h3,         QTextHtmlElement::DisplayBlock },
    { "h4",         Html_h4,         QTextHtmlElement::DisplayBlock },
    { "h5",         Html_h5,         QTextHtmlElement::DisplayBlock },
    { "h6",         Html_h6,         QTextHtmlElement::DisplayBlock },
    { "head",       Html_head,       QTextHtmlElement::DisplayNone },
    { "hr",         Html_hr,         QTextHtmlElement::DisplayBlock },
    { "html",       Html_html,       QTextHtmlElement::DisplayInline },
    { "i",          Html_i,          QTextHtmlElement::DisplayInline },
    { "img",        Html_img,        QTextHtmlElement::DisplayInline },
    { "kbd",        Html_kbd,        QTextHtmlElement::DisplayInline },
    { "li",         Html_li,         QTextHtmlElement::DisplayBlock },
    { "link",       Html_link,       QTextHtmlElement::DisplayNone },
    { "meta",       Html_meta,       QTextHtmlElement::DisplayNone },
    { "nobr",       Html_nobr,       QTextHtmlElement::DisplayInline },
    { "ol",         Html_ol,         QTextHtmlElement::DisplayBlock },
    { "p",          Html_p,          QTextHtmlElement::DisplayBlock },
    { "pre",        Html_pre,        QTextHtmlElement::DisplayBlock },
    { "qt",         Html_body /*deliberate mapping*/, QTextHtmlElement::DisplayBlock },
    { "s",          Html_s,          QTextHtmlElement::DisplayInline },
    { "samp",       Html_samp,       QTextHtmlElement::DisplayInline },
    { "script",     Html_script,     QTextHtmlElement::DisplayNone },
    { "small",      Html_small,      QTextHtmlElement::DisplayInline },
    { "span",       Html_span,       QTextHtmlElement::DisplayInline },
    { "strong",     Html_strong,     QTextHtmlElement::DisplayInline },
    { "style",      Html_style,      QTextHtmlElement::DisplayNone },
    { "sub",        Html_sub,        QTextHtmlElement::DisplayInline },
    { "sup",        Html_sup,        QTextHtmlElement::DisplayInline },
    { "table",      Html_table,      QTextHtmlElement::DisplayTable },
    { "tbody",      Html_tbody,      QTextHtmlElement::DisplayTable },
    { "td",         Html_td,         QTextHtmlElement::DisplayBlock },
    { "tfoot",      Html_tfoot,      QTextHtmlElement::DisplayTable },
    { "th",         Html_th,         QTextHtmlElement::DisplayBlock },
    { "thead",      Html_thead,      QTextHtmlElement::DisplayTable },
    { "title",      Html_title,      QTextHtmlElement::DisplayNone },
    { "tr",         Html_tr,         QTextHtmlElement::DisplayTable },
    { "tt",         Html_tt,         QTextHtmlElement::DisplayInline },
    { "u",          Html_u,          QTextHtmlElement::DisplayInline },
    { "ul",         Html_ul,         QTextHtmlElement::DisplayBlock },
    { "var",        Html_var,        QTextHtmlElement::DisplayInline },
};

static bool operator<(const QString &str, const QTextHtmlElement &e)
{
    return str < QLatin1StringView(e.name);
}

static bool operator<(const QTextHtmlElement &e, const QString &str)
{
    return QLatin1StringView(e.name) < str;
}

static const QTextHtmlElement *lookupElementHelper(const QString &element)
{
    const QTextHtmlElement *start = &elements[0];
    const QTextHtmlElement *end = &elements[Html_NumElements];
    const QTextHtmlElement *e = std::lower_bound(start, end, element);
    if ((e == end) || (element < *e))
        return nullptr;
    return e;
}

int QTextHtmlParser::lookupElement(const QString &element)
{
    const QTextHtmlElement *e = lookupElementHelper(element);
    if (!e)
        return -1;
    return e->id;
}

// quotes newlines as "\\n"
static QString quoteNewline(const QString &s)
{
    QString n = s;
    n.replace(u'\n', "\\n"_L1);
    return n;
}

QTextHtmlParserNode::QTextHtmlParserNode()
    : parent(0), id(Html_unknown),
      cssFloat(QTextFrameFormat::InFlow), hasOwnListStyle(false), hasOwnLineHeightType(false), hasLineHeightMultiplier(false),
      hasCssListIndent(false), isEmptyParagraph(false), isTextFrame(false), isRootFrame(false),
      displayMode(QTextHtmlElement::DisplayInline), hasHref(false),
      listStyle(QTextListFormat::ListStyleUndefined), imageWidth(-1), imageHeight(-1), tableBorder(0),
      tableCellRowSpan(1), tableCellColSpan(1), tableCellSpacing(2), tableCellPadding(0),
      borderBrush(Qt::darkGray), borderStyle(QTextFrameFormat::BorderStyle_Outset),
      borderCollapse(false),
      userState(-1), cssListIndent(0), wsm(WhiteSpaceModeUndefined)
{
    margin[QTextHtmlParser::MarginLeft] = 0;
    margin[QTextHtmlParser::MarginRight] = 0;
    margin[QTextHtmlParser::MarginTop] = 0;
    margin[QTextHtmlParser::MarginBottom] = 0;

    for (int i = 0; i < 4; ++i) {
        tableCellBorderStyle[i] = QTextFrameFormat::BorderStyle_None;
        tableCellBorder[i] = 0;
        tableCellBorderBrush[i] = Qt::NoBrush;
    }
}

void QTextHtmlParser::dumpHtml()
{
    for (int i = 0; i < count(); ++i) {
        qDebug().nospace() << qPrintable(QString(depth(i) * 4, u' '))
                           << qPrintable(at(i).tag) << ':'
                           << quoteNewline(at(i).text);
    }
}

QTextHtmlParserNode *QTextHtmlParser::newNode(int parent)
{
    QTextHtmlParserNode *lastNode = nodes.last();
    QTextHtmlParserNode *newNode = nullptr;

    bool reuseLastNode = true;

    if (nodes.size() == 1) {
        reuseLastNode = false;
    } else if (lastNode->tag.isEmpty()) {

        if (lastNode->text.isEmpty()) {
            reuseLastNode = true;
        } else { // last node is a text node (empty tag) with some text

            if (lastNode->text.size() == 1 && lastNode->text.at(0).isSpace()) {

                int lastSibling = count() - 2;
                while (lastSibling
                       && at(lastSibling).parent != lastNode->parent
                       && at(lastSibling).displayMode == QTextHtmlElement::DisplayInline) {
                    lastSibling = at(lastSibling).parent;
                }

                if (at(lastSibling).displayMode == QTextHtmlElement::DisplayInline) {
                    reuseLastNode = false;
                } else {
                    reuseLastNode = true;
                }
            } else {
                // text node with real (non-whitespace) text -> nothing to re-use
                reuseLastNode = false;
            }

        }

    } else {
        // last node had a proper tag -> nothing to re-use
        reuseLastNode = false;
    }

    if (reuseLastNode) {
        newNode = lastNode;
        newNode->tag.clear();
        newNode->text.clear();
        newNode->id = Html_unknown;
    } else {
        nodes.append(new QTextHtmlParserNode);
        newNode = nodes.last();
    }

    newNode->parent = parent;
    return newNode;
}

void QTextHtmlParser::parse(const QString &text, const QTextDocument *_resourceProvider)
{
    qDeleteAll(nodes);
    nodes.clear();
    nodes.append(new QTextHtmlParserNode);
    txt = text;
    pos = 0;
    len = txt.size();
    textEditMode = false;
    resourceProvider = _resourceProvider;
    parse();
    //dumpHtml();
}

int QTextHtmlParser::depth(int i) const
{
    int depth = 0;
    while (i) {
        i = at(i).parent;
        ++depth;
    }
    return depth;
}

int QTextHtmlParser::margin(int i, int mar) const {
    int m = 0;
    const QTextHtmlParserNode *node;
    if (mar == MarginLeft
        || mar == MarginRight) {
        while (i) {
            node = &at(i);
            if (!node->isBlock() && node->id != Html_table)
                break;
            if (node->isTableCell())
                break;
            m += node->margin[mar];
            i = node->parent;
        }
    }
    return m;
}

int QTextHtmlParser::topMargin(int i) const
{
    if (!i)
        return 0;
    return at(i).margin[MarginTop];
}

int QTextHtmlParser::bottomMargin(int i) const
{
    if (!i)
        return 0;
    return at(i).margin[MarginBottom];
}

void QTextHtmlParser::eatSpace()
{
    while (pos < len && txt.at(pos).isSpace() && txt.at(pos) != QChar::ParagraphSeparator)
        pos++;
}

void QTextHtmlParser::parse()
{
    while (pos < len) {
        QChar c = txt.at(pos++);
        if (c == u'<') {
            parseTag();
        } else if (c == u'&') {
            nodes.last()->text += parseEntity();
        } else {
            nodes.last()->text += c;
        }
    }
}

// parses a tag after "<"
void QTextHtmlParser::parseTag()
{
    eatSpace();

    // handle comments and other exclamation mark declarations
    if (hasPrefix(u'!')) {
        parseExclamationTag();
        if (nodes.last()->wsm != QTextHtmlParserNode::WhiteSpacePre
            && nodes.last()->wsm != QTextHtmlParserNode::WhiteSpacePreWrap
                && !textEditMode)
            eatSpace();
        return;
    }

    // if close tag just close
    if (hasPrefix(u'/')) {
        if (nodes.last()->id == Html_style) {
#ifndef QT_NO_CSSPARSER
            QCss::Parser parser(nodes.constLast()->text);
            QCss::StyleSheet sheet;
            sheet.origin = QCss::StyleSheetOrigin_Author;
            parser.parse(&sheet, Qt::CaseInsensitive);
            inlineStyleSheets.append(sheet);
            resolveStyleSheetImports(sheet);
#endif
        }
        parseCloseTag();
        return;
    }

    int p = last();
    while (p && at(p).tag.size() == 0)
        p = at(p).parent;

    QTextHtmlParserNode *node = newNode(p);

    // parse tag name
    node->tag = parseWord().toLower();

    const QTextHtmlElement *elem = lookupElementHelper(node->tag);
    if (elem) {
        node->id = elem->id;
        node->displayMode = elem->displayMode;
    } else {
        node->id = Html_unknown;
    }

    node->attributes.clear();
    // _need_ at least one space after the tag name, otherwise there can't be attributes
    if (pos < len && txt.at(pos).isSpace())
        node->attributes = parseAttributes();

    // resolveParent() may have to change the order in the tree and
    // insert intermediate nodes for buggy HTML, so re-initialize the 'node'
    // pointer through the return value
    node = resolveParent();
    resolveNode();

#ifndef QT_NO_CSSPARSER
    const int nodeIndex = nodes.size() - 1; // this new node is always the last
    node->applyCssDeclarations(declarationsForNode(nodeIndex), resourceProvider);
#endif
    applyAttributes(node->attributes);

    // finish tag
    bool tagClosed = false;
    while (pos < len && txt.at(pos) != u'>') {
        if (txt.at(pos) == u'/')
            tagClosed = true;

        pos++;
    }
    pos++;

    // in a white-space preserving environment strip off a initial newline
    // since the element itself already generates a newline
    if ((node->wsm == QTextHtmlParserNode::WhiteSpacePre
         || node->wsm == QTextHtmlParserNode::WhiteSpacePreWrap
         || node->wsm == QTextHtmlParserNode::WhiteSpacePreLine)
        && node->isBlock()) {
        if (pos < len - 1 && txt.at(pos) == u'\n')
            ++pos;
    }

    if (node->mayNotHaveChildren() || tagClosed) {
        newNode(node->parent);
        resolveNode();
    }
}

// parses a tag beginning with "/"
void QTextHtmlParser::parseCloseTag()
{
    ++pos;
    QString tag = parseWord().toLower().trimmed();
    while (pos < len) {
        QChar c = txt.at(pos++);
        if (c == u'>')
            break;
    }

    // find corresponding open node
    int p = last();
    if (p > 0
        && at(p - 1).tag == tag
        && at(p - 1).mayNotHaveChildren())
        p--;

    while (p && at(p).tag != tag)
        p = at(p).parent;

    // simply ignore the tag if we can't find
    // a corresponding open node, for broken
    // html such as <font>blah</font></font>
    if (!p)
        return;

    // in a white-space preserving environment strip off a trailing newline
    // since the closing of the opening block element will automatically result
    // in a new block for elements following the <pre>
    // ...foo\n</pre><p>blah -> foo</pre><p>blah
    if ((at(p).wsm == QTextHtmlParserNode::WhiteSpacePre
         || at(p).wsm == QTextHtmlParserNode::WhiteSpacePreWrap
         || at(p).wsm == QTextHtmlParserNode::WhiteSpacePreLine)
        && at(p).isBlock()) {
        if (at(last()).text.endsWith(u'\n'))
            nodes[last()]->text.chop(1);
    }

    newNode(at(p).parent);
    resolveNode();
}

// parses a tag beginning with "!"
void QTextHtmlParser::parseExclamationTag()
{
    ++pos;
    if (hasPrefix(u'-') && hasPrefix(u'-', 1)) {
        pos += 2;
        // eat comments
        int end = txt.indexOf("-->"_L1, pos);
        pos = (end >= 0 ? end + 3 : len);
    } else {
        // eat internal tags
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == u'>')
                break;
        }
    }
}

QString QTextHtmlParser::parseEntity(QStringView entity)
{
    QChar resolved = resolveEntity(entity);
    if (!resolved.isNull())
        return QString(resolved);

    if (entity.size() > 1 && entity.at(0) == u'#') {
        entity = entity.mid(1); // removing leading #

        int base = 10;
        bool ok = false;

        if (entity.at(0).toLower() == u'x') { // hex entity?
            entity = entity.mid(1);
            base = 16;
        }

        uint uc = entity.toUInt(&ok, base);
        if (ok) {
            if (uc >= 0x80 && uc < 0x80 + (sizeof(windowsLatin1ExtendedCharacters)/sizeof(windowsLatin1ExtendedCharacters[0])))
                uc = windowsLatin1ExtendedCharacters[uc - 0x80];
            return QStringView{QChar::fromUcs4(uc)}.toString();
        }
    }
    return {};
}

// parses an entity after "&", and returns it
QString QTextHtmlParser::parseEntity()
{
    const int recover = pos;
    int entityLen = 0;
    while (pos < len) {
        QChar c = txt.at(pos++);
        if (c.isSpace() || pos - recover > 9) {
            goto error;
        }
        if (c == u';')
            break;
        ++entityLen;
    }
    if (entityLen) {
        const QStringView entity = QStringView(txt).mid(recover, entityLen);
        QString parsedEntity = parseEntity(entity);
        if (!parsedEntity.isNull()) {
            return parsedEntity;
        }
    }
error:
    pos = recover;
    return "&"_L1;
}

// parses one word, possibly quoted, and returns it
QString QTextHtmlParser::parseWord()
{
    QString word;
    if (hasPrefix(u'\"')) { // double quotes
        ++pos;
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == u'\"')
                break;
            else if (c == u'&')
                word += parseEntity();
            else
                word += c;
        }
    } else if (hasPrefix(u'\'')) { // single quotes
        ++pos;
        while (pos < len) {
            QChar c = txt.at(pos++);
            // Allow for escaped single quotes as they may be part of the string
            if (c == u'\'' && (txt.size() > 1 && txt.at(pos - 2) != u'\\'))
                break;
            else
                word += c;
        }
    } else { // normal text
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == u'>' || (c == u'/' && hasPrefix(u'>'))
                    || c == u'<' || c == u'=' || c.isSpace()) {
                --pos;
                break;
            }
            if (c == u'&')
                word += parseEntity();
            else
                word += c;
        }
    }
    return word;
}

// gives the new node the right parent
QTextHtmlParserNode *QTextHtmlParser::resolveParent()
{
    QTextHtmlParserNode *node = nodes.last();

    int p = node->parent;

    // Excel gives us buggy HTML with just tr without surrounding table tags
    // or with just td tags

    if (node->id == Html_td) {
        int n = p;
        while (n && at(n).id != Html_tr)
            n = at(n).parent;

        if (!n) {
            nodes.insert(nodes.size() - 1, new QTextHtmlParserNode);
            nodes.insert(nodes.size() - 1, new QTextHtmlParserNode);

            QTextHtmlParserNode *table = nodes[nodes.size() - 3];
            table->parent = p;
            table->id = Html_table;
            table->tag = "table"_L1;
            table->children.append(nodes.size() - 2); // add row as child

            QTextHtmlParserNode *row = nodes[nodes.size() - 2];
            row->parent = nodes.size() - 3; // table as parent
            row->id = Html_tr;
            row->tag = "tr"_L1;

            p = nodes.size() - 2;
            node = nodes.last(); // re-initialize pointer
        }
    }

    if (node->id == Html_tr) {
        int n = p;
        while (n && at(n).id != Html_table)
            n = at(n).parent;

        if (!n) {
            nodes.insert(nodes.size() - 1, new QTextHtmlParserNode);
            QTextHtmlParserNode *table = nodes[nodes.size() - 2];
            table->parent = p;
            table->id = Html_table;
            table->tag = "table"_L1;
            p = nodes.size() - 2;
            node = nodes.last(); // re-initialize pointer
        }
    }

    // permit invalid html by letting block elements be children
    // of inline elements with the exception of paragraphs:
    //
    // a new paragraph closes parent inline elements (while loop),
    // unless they themselves are children of a non-paragraph block
    // element (if statement)
    //
    // For example:
    //
    // <body><p><b>Foo<p>Bar <-- second <p> implicitly closes <b> that
    //                           belongs to the first <p>. The self-nesting
    //                           check further down prevents the second <p>
    //                           from nesting into the first one then.
    //                           so Bar is not bold.
    //
    // <body><b><p>Foo <-- Foo should be bold.
    //
    // <body><b><p>Foo<p>Bar <-- Foo and Bar should be bold.
    //
    if (node->id == Html_p) {
        while (p && !at(p).isBlock())
            p = at(p).parent;

        if (!p || at(p).id != Html_p)
            p = node->parent;
    }

    // some elements are not self nesting
    if (node->id == at(p).id
        && node->isNotSelfNesting())
        p = at(p).parent;

    // some elements are not allowed in certain contexts
    while ((p && !node->allowedInContext(at(p).id))
           // ### make new styles aware of empty tags
           || at(p).mayNotHaveChildren()
       ) {
        p = at(p).parent;
    }

    node->parent = p;

    // makes it easier to traverse the tree, later
    nodes[p]->children.append(nodes.size() - 1);
    return node;
}

// sets all properties on the new node
void QTextHtmlParser::resolveNode()
{
    QTextHtmlParserNode *node = nodes.last();
    const QTextHtmlParserNode *parent = nodes.at(node->parent);
    node->initializeProperties(parent, this);
}

bool QTextHtmlParserNode::isNestedList(const QTextHtmlParser *parser) const
{
    if (!isListStart())
        return false;

    int p = parent;
    while (p) {
        if (parser->at(p).isListStart())
            return true;
        p = parser->at(p).parent;
    }
    return false;
}

void QTextHtmlParserNode::initializeProperties(const QTextHtmlParserNode *parent, const QTextHtmlParser *parser)
{
    // inherit properties from parent element
    charFormat = parent->charFormat;

    if (id == Html_html)
        blockFormat.setLayoutDirection(Qt::LeftToRight); // HTML default
    else if (parent->blockFormat.hasProperty(QTextFormat::LayoutDirection))
        blockFormat.setLayoutDirection(parent->blockFormat.layoutDirection());

    if (parent->displayMode == QTextHtmlElement::DisplayNone)
        displayMode = QTextHtmlElement::DisplayNone;

    if (parent->id != Html_table || id == Html_caption) {
        if (parent->blockFormat.hasProperty(QTextFormat::BlockAlignment))
            blockFormat.setAlignment(parent->blockFormat.alignment());
        else
            blockFormat.clearProperty(QTextFormat::BlockAlignment);
    }
    // we don't paint per-row background colors, yet. so as an
    // exception inherit the background color here
    // we also inherit the background between inline elements
    // we also inherit from non-body block elements since we merge them together
    if ((parent->id != Html_tr || !isTableCell())
        && (displayMode != QTextHtmlElement::DisplayInline || parent->displayMode != QTextHtmlElement::DisplayInline)
        && (parent->id == Html_body || displayMode != QTextHtmlElement::DisplayBlock || parent->displayMode != QTextHtmlElement::DisplayBlock)
       ) {
        charFormat.clearProperty(QTextFormat::BackgroundBrush);
    }

    listStyle = parent->listStyle;
    // makes no sense to inherit that property, a named anchor is a single point
    // in the document, which is set by the DocumentFragment
    charFormat.clearProperty(QTextFormat::AnchorName);
    wsm = parent->wsm;

    // initialize remaining properties
    margin[QTextHtmlParser::MarginLeft] = 0;
    margin[QTextHtmlParser::MarginRight] = 0;
    margin[QTextHtmlParser::MarginTop] = 0;
    margin[QTextHtmlParser::MarginBottom] = 0;
    cssFloat = QTextFrameFormat::InFlow;

    for (int i = 0; i < 4; ++i)
        padding[i] = -1;

    // set element specific attributes
    switch (id) {
        case Html_a:
            for (int i = 0; i < attributes.size(); i += 2) {
                const QString key = attributes.at(i);
                if (key.compare("href"_L1, Qt::CaseInsensitive) == 0
                    && !attributes.at(i + 1).isEmpty()) {
                    hasHref = true;
                }
            }
            charFormat.setAnchor(true);
            break;
        case Html_big:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(1));
            break;
        case Html_small:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(-1));
            break;
        case Html_h1:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(3));
            margin[QTextHtmlParser::MarginTop] = 18;
            margin[QTextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h2:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(2));
            margin[QTextHtmlParser::MarginTop] = 16;
            margin[QTextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h3:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(1));
            margin[QTextHtmlParser::MarginTop] = 14;
            margin[QTextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h4:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(0));
            margin[QTextHtmlParser::MarginTop] = 12;
            margin[QTextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h5:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(-1));
            margin[QTextHtmlParser::MarginTop] = 12;
            margin[QTextHtmlParser::MarginBottom] = 4;
            break;
        case Html_p:
            margin[QTextHtmlParser::MarginTop] = 12;
            margin[QTextHtmlParser::MarginBottom] = 12;
            break;
        case Html_ul:
            // nested lists don't have margins, except for the toplevel one
            if (!isNestedList(parser)) {
                margin[QTextHtmlParser::MarginTop] = 12;
                margin[QTextHtmlParser::MarginBottom] = 12;
            }
            // no left margin as we use indenting instead
            break;
        case Html_ol:
            // nested lists don't have margins, except for the toplevel one
            if (!isNestedList(parser)) {
                margin[QTextHtmlParser::MarginTop] = 12;
                margin[QTextHtmlParser::MarginBottom] = 12;
            }
            // no left margin as we use indenting instead
            break;
        case Html_br:
            text = QChar(QChar::LineSeparator);
            break;
        case Html_pre:
            margin[QTextHtmlParser::MarginTop] = 12;
            margin[QTextHtmlParser::MarginBottom] = 12;
            break;
        case Html_blockquote:
            margin[QTextHtmlParser::MarginTop] = 12;
            margin[QTextHtmlParser::MarginBottom] = 12;
            margin[QTextHtmlParser::MarginLeft] = 40;
            margin[QTextHtmlParser::MarginRight] = 40;
            blockFormat.setProperty(QTextFormat::BlockQuoteLevel, 1);
            break;
        case Html_dl:
            margin[QTextHtmlParser::MarginTop] = 8;
            margin[QTextHtmlParser::MarginBottom] = 8;
            break;
        case Html_dd:
            margin[QTextHtmlParser::MarginLeft] = 30;
            break;
        default: break;
    }
}

#ifndef QT_NO_CSSPARSER
void QTextHtmlParserNode::setListStyle(const QList<QCss::Value> &cssValues)
{
    for (int i = 0; i < cssValues.size(); ++i) {
        if (cssValues.at(i).type == QCss::Value::KnownIdentifier) {
            switch (static_cast<QCss::KnownValue>(cssValues.at(i).variant.toInt())) {
                case QCss::Value_None: hasOwnListStyle = true; listStyle = QTextListFormat::ListStyleUndefined; break;
                case QCss::Value_Disc: hasOwnListStyle = true; listStyle = QTextListFormat::ListDisc; break;
                case QCss::Value_Square: hasOwnListStyle = true; listStyle = QTextListFormat::ListSquare; break;
                case QCss::Value_Circle: hasOwnListStyle = true; listStyle = QTextListFormat::ListCircle; break;
                case QCss::Value_Decimal: hasOwnListStyle = true; listStyle = QTextListFormat::ListDecimal; break;
                case QCss::Value_LowerAlpha: hasOwnListStyle = true; listStyle = QTextListFormat::ListLowerAlpha; break;
                case QCss::Value_UpperAlpha: hasOwnListStyle = true; listStyle = QTextListFormat::ListUpperAlpha; break;
                case QCss::Value_LowerRoman: hasOwnListStyle = true; listStyle = QTextListFormat::ListLowerRoman; break;
                case QCss::Value_UpperRoman: hasOwnListStyle = true; listStyle = QTextListFormat::ListUpperRoman; break;
                default: break;
            }
        }
    }
    // allow individual list items to override the style
    if (id == Html_li && hasOwnListStyle)
        blockFormat.setProperty(QTextFormat::ListStyle, listStyle);
}

static QTextFrameFormat::BorderStyle toQTextFrameFormat(QCss::BorderStyle cssStyle)
{
    switch (cssStyle) {
    case QCss::BorderStyle::BorderStyle_Dotted:
        return QTextFrameFormat::BorderStyle::BorderStyle_Dotted;
    case QCss::BorderStyle::BorderStyle_Dashed:
        return QTextFrameFormat::BorderStyle::BorderStyle_Dashed;
    case QCss::BorderStyle::BorderStyle_Solid:
        return QTextFrameFormat::BorderStyle::BorderStyle_Solid;
    case QCss::BorderStyle::BorderStyle_Double:
        return QTextFrameFormat::BorderStyle::BorderStyle_Double;
    case QCss::BorderStyle::BorderStyle_DotDash:
        return QTextFrameFormat::BorderStyle::BorderStyle_DotDash;
    case QCss::BorderStyle::BorderStyle_DotDotDash:
        return QTextFrameFormat::BorderStyle::BorderStyle_DotDotDash;
    case QCss::BorderStyle::BorderStyle_Groove:
        return QTextFrameFormat::BorderStyle::BorderStyle_Groove;
    case QCss::BorderStyle::BorderStyle_Ridge:
        return QTextFrameFormat::BorderStyle::BorderStyle_Ridge;
    case QCss::BorderStyle::BorderStyle_Inset:
        return QTextFrameFormat::BorderStyle::BorderStyle_Inset;
    case QCss::BorderStyle::BorderStyle_Outset:
        return QTextFrameFormat::BorderStyle::BorderStyle_Outset;
    case QCss::BorderStyle::BorderStyle_Unknown:
    case QCss::BorderStyle::BorderStyle_None:
    case QCss::BorderStyle::BorderStyle_Native:
        return QTextFrameFormat::BorderStyle::BorderStyle_None;
    case QCss::BorderStyle::NumKnownBorderStyles:
        break;
    // Intentionally no "default" to allow a compiler warning when extending the enum
    // without updating this here. clang gives such a warning.
    }
    // Must not happen, intentionally trigger undefined behavior which sanitizers will detect.
    // Having all cases covered in switch is not sufficient:
    // MSVC would warn when there is no "default".
    return static_cast<QTextFrameFormat::BorderStyle>(-1);
}

void QTextHtmlParserNode::applyCssDeclarations(const QList<QCss::Declaration> &declarations, const QTextDocument *resourceProvider)
{
    QCss::ValueExtractor extractor(declarations);
    extractor.extractBox(margin, padding);

    if (id == Html_td || id == Html_th) {
        QCss::BorderStyle cssStyles[4];
        int cssBorder[4];
        QSize cssRadii[4]; // unused
        for (int i = 0; i < 4; ++i) {
            cssStyles[i] = QCss::BorderStyle_None;
            cssBorder[i] = 0;
        }
        // this will parse (and cache) "border-width" as a list so the
        // QCss::BorderWidth parsing below which expects a single value
        // will not work as expected - which in this case does not matter
        // because tableBorder is not relevant for cells.
        extractor.extractBorder(cssBorder, tableCellBorderBrush, cssStyles, cssRadii);
        for (int i = 0; i < 4; ++i) {
            tableCellBorderStyle[i] = toQTextFrameFormat(cssStyles[i]);
            tableCellBorder[i] = static_cast<qreal>(cssBorder[i]);
        }
    }

    for (int i = 0; i < declarations.size(); ++i) {
        const QCss::Declaration &decl = declarations.at(i);
        if (decl.d->values.isEmpty()) continue;

        QCss::KnownValue identifier = QCss::UnknownValue;
        if (decl.d->values.first().type == QCss::Value::KnownIdentifier)
            identifier = static_cast<QCss::KnownValue>(decl.d->values.first().variant.toInt());

        switch (decl.d->propertyId) {
        case QCss::BorderColor: borderBrush = QBrush(decl.colorValue()); break;
        case QCss::BorderStyles:
            if (decl.styleValue() != QCss::BorderStyle_Unknown && decl.styleValue() != QCss::BorderStyle_Native)
                borderStyle = static_cast<QTextFrameFormat::BorderStyle>(decl.styleValue() - 1);
            break;
        case QCss::BorderWidth: {
            int borders[4];
            extractor.lengthValues(decl, borders);
            tableBorder = borders[0];
            }
            break;
        case QCss::BorderCollapse:
            borderCollapse = decl.borderCollapseValue();
            break;
        case QCss::Color: charFormat.setForeground(decl.colorValue()); break;
        case QCss::Float:
            cssFloat = QTextFrameFormat::InFlow;
            switch (identifier) {
            case QCss::Value_Left: cssFloat = QTextFrameFormat::FloatLeft; break;
            case QCss::Value_Right: cssFloat = QTextFrameFormat::FloatRight; break;
            default: break;
            }
            break;
        case QCss::QtBlockIndent:
            blockFormat.setIndent(decl.d->values.first().variant.toInt());
            break;
        case QCss::QtLineHeightType: {
            QString lineHeightTypeName = decl.d->values.first().variant.toString();
            QTextBlockFormat::LineHeightTypes lineHeightType;
            if (lineHeightTypeName.compare("proportional"_L1, Qt::CaseInsensitive) == 0)
                lineHeightType = QTextBlockFormat::ProportionalHeight;
            else if (lineHeightTypeName.compare("fixed"_L1, Qt::CaseInsensitive) == 0)
                lineHeightType = QTextBlockFormat::FixedHeight;
            else if (lineHeightTypeName.compare("minimum"_L1, Qt::CaseInsensitive) == 0)
                lineHeightType = QTextBlockFormat::MinimumHeight;
            else if (lineHeightTypeName.compare("line-distance"_L1, Qt::CaseInsensitive) == 0)
                lineHeightType = QTextBlockFormat::LineDistanceHeight;
            else
                lineHeightType = QTextBlockFormat::SingleHeight;

            if (hasLineHeightMultiplier) {
                qreal lineHeight = blockFormat.lineHeight() / 100.0;
                blockFormat.setProperty(QTextBlockFormat::LineHeight, lineHeight);
            }

            blockFormat.setProperty(QTextBlockFormat::LineHeightType, lineHeightType);
            hasOwnLineHeightType = true;
        }
        break;
        case QCss::LineHeight: {
            qreal lineHeight;
            QTextBlockFormat::LineHeightTypes lineHeightType;
            if (decl.realValue(&lineHeight, "px")) {
                lineHeightType = QTextBlockFormat::MinimumHeight;
            } else {
                bool ok;
                QCss::Value cssValue = decl.d->values.first();
                QString value = cssValue.toString();
                lineHeight = value.toDouble(&ok);
                if (ok) {
                    if (!hasOwnLineHeightType && cssValue.type == QCss::Value::Number) {
                        lineHeight *= 100.0;
                        hasLineHeightMultiplier = true;
                    }
                    lineHeightType = QTextBlockFormat::ProportionalHeight;
                } else {
                    lineHeight = 0.0;
                    lineHeightType = QTextBlockFormat::SingleHeight;
                }
            }

            // Only override line height type if specified in same node
            if (hasOwnLineHeightType)
                lineHeightType = QTextBlockFormat::LineHeightTypes(blockFormat.lineHeightType());

            blockFormat.setLineHeight(lineHeight, lineHeightType);
            break;
        }
        case QCss::TextIndent: {
            qreal indent = 0;
            if (decl.realValue(&indent, "px"))
                blockFormat.setTextIndent(indent);
            break; }
        case QCss::QtListIndent:
            if (decl.intValue(&cssListIndent))
                hasCssListIndent = true;
            break;
        case QCss::QtParagraphType:
            if (decl.d->values.first().variant.toString().compare("empty"_L1, Qt::CaseInsensitive) == 0)
                isEmptyParagraph = true;
            break;
        case QCss::QtTableType:
            if (decl.d->values.first().variant.toString().compare("frame"_L1, Qt::CaseInsensitive) == 0)
                isTextFrame = true;
            else if (decl.d->values.first().variant.toString().compare("root"_L1, Qt::CaseInsensitive) == 0) {
                isTextFrame = true;
                isRootFrame = true;
            }
            break;
        case QCss::QtUserState:
            userState = decl.d->values.first().variant.toInt();
            break;
        case QCss::Whitespace:
            switch (identifier) {
            case QCss::Value_Normal: wsm = QTextHtmlParserNode::WhiteSpaceNormal; break;
            case QCss::Value_Pre: wsm = QTextHtmlParserNode::WhiteSpacePre; break;
            case QCss::Value_NoWrap: wsm = QTextHtmlParserNode::WhiteSpaceNoWrap; break;
            case QCss::Value_PreWrap: wsm = QTextHtmlParserNode::WhiteSpacePreWrap; break;
            case QCss::Value_PreLine: wsm = QTextHtmlParserNode::WhiteSpacePreLine; break;
            default: break;
            }
            break;
        case QCss::VerticalAlignment:
            switch (identifier) {
            case QCss::Value_Sub: charFormat.setVerticalAlignment(QTextCharFormat::AlignSubScript); break;
            case QCss::Value_Super: charFormat.setVerticalAlignment(QTextCharFormat::AlignSuperScript); break;
            case QCss::Value_Middle: charFormat.setVerticalAlignment(QTextCharFormat::AlignMiddle); break;
            case QCss::Value_Top: charFormat.setVerticalAlignment(QTextCharFormat::AlignTop); break;
            case QCss::Value_Bottom: charFormat.setVerticalAlignment(QTextCharFormat::AlignBottom); break;
            default: charFormat.setVerticalAlignment(QTextCharFormat::AlignNormal); break;
            }
            break;
        case QCss::PageBreakBefore:
            switch (identifier) {
            case QCss::Value_Always: blockFormat.setPageBreakPolicy(blockFormat.pageBreakPolicy() | QTextFormat::PageBreak_AlwaysBefore); break;
            case QCss::Value_Auto: blockFormat.setPageBreakPolicy(blockFormat.pageBreakPolicy() & ~QTextFormat::PageBreak_AlwaysBefore); break;
            default: break;
            }
            break;
        case QCss::PageBreakAfter:
            switch (identifier) {
            case QCss::Value_Always: blockFormat.setPageBreakPolicy(blockFormat.pageBreakPolicy() | QTextFormat::PageBreak_AlwaysAfter); break;
            case QCss::Value_Auto: blockFormat.setPageBreakPolicy(blockFormat.pageBreakPolicy() & ~QTextFormat::PageBreak_AlwaysAfter); break;
            default: break;
            }
            break;
        case QCss::TextUnderlineStyle:
            switch (identifier) {
            case QCss::Value_None: charFormat.setUnderlineStyle(QTextCharFormat::NoUnderline); break;
            case QCss::Value_Solid: charFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline); break;
            case QCss::Value_Dashed: charFormat.setUnderlineStyle(QTextCharFormat::DashUnderline); break;
            case QCss::Value_Dotted: charFormat.setUnderlineStyle(QTextCharFormat::DotLine); break;
            case QCss::Value_DotDash: charFormat.setUnderlineStyle(QTextCharFormat::DashDotLine); break;
            case QCss::Value_DotDotDash: charFormat.setUnderlineStyle(QTextCharFormat::DashDotDotLine); break;
            case QCss::Value_Wave: charFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline); break;
            default: break;
            }
            break;
        case QCss::TextDecorationColor: charFormat.setUnderlineColor(decl.colorValue()); break;
        case QCss::ListStyleType:
        case QCss::ListStyle:
            setListStyle(decl.d->values);
            break;
        case QCss::QtListNumberPrefix:
            textListNumberPrefix = decl.d->values.first().variant.toString();
            break;
        case QCss::QtListNumberSuffix:
            textListNumberSuffix = decl.d->values.first().variant.toString();
            break;
        case QCss::TextAlignment:
            switch (identifier) {
            case QCss::Value_Left: blockFormat.setAlignment(Qt::AlignLeft); break;
            case QCss::Value_Center: blockFormat.setAlignment(Qt::AlignCenter); break;
            case QCss::Value_Right: blockFormat.setAlignment(Qt::AlignRight); break;
            default: break;
            }
            break;

        case QCss::QtForegroundTextureCacheKey:
        {
            if (resourceProvider != nullptr && QTextDocumentPrivate::get(resourceProvider) != nullptr) {
                bool ok;
                qint64 searchKey = decl.d->values.first().variant.toLongLong(&ok);
                if (ok)
                    applyForegroundImage(searchKey, resourceProvider);
            }
            break;
        }
        default: break;
        }
    }

    QFont f;
    int adjustment = -255;
    extractor.extractFont(&f, &adjustment);
    if (f.pixelSize() > INT32_MAX / 2)
        f.setPixelSize(INT32_MAX / 2);   // avoid even more extreme values
    charFormat.setFont(f, QTextCharFormat::FontPropertiesSpecifiedOnly);

    if (adjustment >= -1)
        charFormat.setProperty(QTextFormat::FontSizeAdjustment, adjustment);

    {
        Qt::Alignment ignoredAlignment;
        QCss::Repeat ignoredRepeat;
        QString bgImage;
        QBrush bgBrush;
        QCss::Origin ignoredOrigin, ignoredClip;
        QCss::Attachment ignoredAttachment;
        extractor.extractBackground(&bgBrush, &bgImage, &ignoredRepeat, &ignoredAlignment,
                                    &ignoredOrigin, &ignoredAttachment, &ignoredClip);

        if (!bgImage.isEmpty() && resourceProvider) {
            applyBackgroundImage(bgImage, resourceProvider);
        } else if (bgBrush.style() != Qt::NoBrush) {
            charFormat.setBackground(bgBrush);
            if (id == Html_hr)
                blockFormat.setProperty(QTextFormat::BackgroundBrush, bgBrush);
        }
    }
}

#endif // QT_NO_CSSPARSER

void QTextHtmlParserNode::applyForegroundImage(qint64 searchKey, const QTextDocument *resourceProvider)
{
    const QTextDocumentPrivate *priv = QTextDocumentPrivate::get(resourceProvider);
    for (int i = 0; i < priv->formats.numFormats(); ++i) {
        QTextCharFormat format = priv->formats.charFormat(i);
        if (format.isValid()) {
            QBrush brush = format.foreground();
            if (brush.style() == Qt::TexturePattern) {
                const bool isPixmap = qHasPixmapTexture(brush);

                if (isPixmap && QCoreApplication::instance()->thread() != QThread::currentThread()) {
                    qWarning("Can't apply QPixmap outside of GUI thread");
                    return;
                }

                const qint64 cacheKey = isPixmap ? brush.texture().cacheKey() : brush.textureImage().cacheKey();
                if (cacheKey == searchKey) {
                    QBrush b;
                    if (isPixmap)
                        b.setTexture(brush.texture());
                    else
                        b.setTextureImage(brush.textureImage());
                    b.setStyle(Qt::TexturePattern);
                    charFormat.setForeground(b);
                }
            }
        }
    }

}

void QTextHtmlParserNode::applyBackgroundImage(const QString &url, const QTextDocument *resourceProvider)
{
    if (!url.isEmpty() && resourceProvider) {
        QVariant val = resourceProvider->resource(QTextDocument::ImageResource, url);

        if (QCoreApplication::instance()->thread() != QThread::currentThread()) {
            // must use images in non-GUI threads
            if (val.userType() == QMetaType::QImage) {
                QImage image = qvariant_cast<QImage>(val);
                charFormat.setBackground(image);
            } else if (val.userType() == QMetaType::QByteArray) {
                QImage image;
                if (image.loadFromData(val.toByteArray())) {
                    charFormat.setBackground(image);
                }
            }
        } else {
            if (val.userType() == QMetaType::QImage || val.userType() == QMetaType::QPixmap) {
                charFormat.setBackground(qvariant_cast<QPixmap>(val));
            } else if (val.userType() == QMetaType::QByteArray) {
                QPixmap pm;
                if (pm.loadFromData(val.toByteArray())) {
                    charFormat.setBackground(pm);
                }
            }
        }
    }
    if (!url.isEmpty())
        charFormat.setProperty(QTextFormat::BackgroundImageUrl, url);
}

bool QTextHtmlParserNode::hasOnlyWhitespace() const
{
    for (int i = 0; i < text.size(); ++i)
        if (!text.at(i).isSpace() || text.at(i) == QChar::LineSeparator)
            return false;
    return true;
}

static bool setIntAttribute(int *destination, const QString &value)
{
    bool ok = false;
    int val = value.toInt(&ok);
    if (ok)
        *destination = val;

    return ok;
}

static bool setFloatAttribute(qreal *destination, const QString &value)
{
    bool ok = false;
    qreal val = value.toDouble(&ok);
    if (ok)
        *destination = val;

    return ok;
}

static void setWidthAttribute(QTextLength *width, const QString &valueStr)
{
    bool ok = false;
    qreal realVal = valueStr.toDouble(&ok);
    if (ok) {
        *width = QTextLength(QTextLength::FixedLength, realVal);
    } else {
        auto value = QStringView(valueStr).trimmed();
        if (!value.isEmpty() && value.endsWith(u'%')) {
            value.truncate(value.size() - 1);
            realVal = value.toDouble(&ok);
            if (ok)
                *width = QTextLength(QTextLength::PercentageLength, realVal);
        }
    }
}

#ifndef QT_NO_CSSPARSER
void QTextHtmlParserNode::parseStyleAttribute(const QString &value, const QTextDocument *resourceProvider)
{
    const QString css = "* {"_L1 + value + u'}';
    QCss::Parser parser(css);
    QCss::StyleSheet sheet;
    parser.parse(&sheet, Qt::CaseInsensitive);
    if (sheet.styleRules.size() != 1) return;
    applyCssDeclarations(sheet.styleRules.at(0).declarations, resourceProvider);
}
#endif

QStringList QTextHtmlParser::parseAttributes()
{
    QStringList attrs;

    while (pos < len) {
        eatSpace();
        if (hasPrefix(u'>') || hasPrefix(u'/'))
            break;
        QString key = parseWord().toLower();
        QString value = "1"_L1;
        if (key.size() == 0)
            break;
        eatSpace();
        if (hasPrefix(u'=')){
            pos++;
            eatSpace();
            value = parseWord();
        }
        if (value.size() == 0)
            continue;
        attrs << key << value;
    }

    return attrs;
}

void QTextHtmlParser::applyAttributes(const QStringList &attributes)
{
    // local state variable for qt3 textedit mode
    bool seenQt3Richtext = false;
    QString linkHref;
    QString linkType;

    if (attributes.size() % 2 == 1)
        return;

    QTextHtmlParserNode *node = nodes.last();

    for (int i = 0; i < attributes.size(); i += 2) {
        QString key = attributes.at(i);
        QString value = attributes.at(i + 1);

        switch (node->id) {
            case Html_font:
                // the infamous font tag
                if (key == "size"_L1 && value.size()) {
                    int n = value.toInt();
                    if (value.at(0) != u'+' && value.at(0) != u'-')
                        n -= 3;
                    node->charFormat.setProperty(QTextFormat::FontSizeAdjustment, n);
                } else if (key == "face"_L1) {
                    if (value.contains(u',')) {
                        const QStringList values = value.split(u',');
                        QStringList families;
                        for (const QString &family : values)
                            families << family.trimmed();
                        node->charFormat.setFontFamilies(families);
                    } else {
                        node->charFormat.setFontFamilies(QStringList(value));
                    }
                } else if (key == "color"_L1) {
                    QColor c = QColor::fromString(value);
                    if (!c.isValid())
                        qWarning("QTextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setForeground(c);
                }
                break;
            case Html_ol:
            case Html_ul:
                if (key == "type"_L1) {
                    node->hasOwnListStyle = true;
                    if (value == "1"_L1) {
                        node->listStyle = QTextListFormat::ListDecimal;
                    } else if (value == "a"_L1) {
                        node->listStyle = QTextListFormat::ListLowerAlpha;
                    } else if (value == "A"_L1) {
                        node->listStyle = QTextListFormat::ListUpperAlpha;
                    } else if (value == "i"_L1) {
                        node->listStyle = QTextListFormat::ListLowerRoman;
                    } else if (value == "I"_L1) {
                        node->listStyle = QTextListFormat::ListUpperRoman;
                    } else {
                        value = std::move(value).toLower();
                        if (value == "square"_L1)
                            node->listStyle = QTextListFormat::ListSquare;
                        else if (value == "disc"_L1)
                            node->listStyle = QTextListFormat::ListDisc;
                        else if (value == "circle"_L1)
                            node->listStyle = QTextListFormat::ListCircle;
                        else if (value == "none"_L1)
                            node->listStyle = QTextListFormat::ListStyleUndefined;
                    }
                } else if (key == "start"_L1) {
                    setIntAttribute(&node->listStart, value);
                }
                break;
            case Html_li:
                if (key == "class"_L1) {
                    if (value == "unchecked"_L1)
                        node->blockFormat.setMarker(QTextBlockFormat::MarkerType::Unchecked);
                    else if (value == "checked"_L1)
                        node->blockFormat.setMarker(QTextBlockFormat::MarkerType::Checked);
                }
                break;
            case Html_a:
                if (key == "href"_L1)
                    node->charFormat.setAnchorHref(value);
                else if (key == "name"_L1)
                    node->charFormat.setAnchorNames({value});
                break;
            case Html_img:
                if (key == "src"_L1 || key == "source"_L1) {
                    node->imageName = value;
                } else if (key == "width"_L1) {
                    node->imageWidth = -2; // register that there is a value for it.
                    setFloatAttribute(&node->imageWidth, value);
                } else if (key == "height"_L1) {
                    node->imageHeight = -2; // register that there is a value for it.
                    setFloatAttribute(&node->imageHeight, value);
                } else if (key == "alt"_L1) {
                    node->imageAlt = value;
                } else if (key == "title"_L1) {
                    node->text = value;
                }
                break;
            case Html_tr:
            case Html_body:
                if (key == "bgcolor"_L1) {
                    QColor c = QColor::fromString(value);
                    if (!c.isValid())
                        qWarning("QTextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setBackground(c);
                } else if (key == "background"_L1) {
                    node->applyBackgroundImage(value, resourceProvider);
                }
                break;
            case Html_th:
            case Html_td:
                if (key == "width"_L1) {
                    setWidthAttribute(&node->width, value);
                } else if (key == "bgcolor"_L1) {
                    QColor c = QColor::fromString(value);
                    if (!c.isValid())
                        qWarning("QTextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setBackground(c);
                } else if (key == "background"_L1) {
                    node->applyBackgroundImage(value, resourceProvider);
                } else if (key == "rowspan"_L1) {
                    if (setIntAttribute(&node->tableCellRowSpan, value))
                        node->tableCellRowSpan = qMax(1, node->tableCellRowSpan);
                } else if (key == "colspan"_L1) {
                    if (setIntAttribute(&node->tableCellColSpan, value))
                        node->tableCellColSpan = qBound(1, node->tableCellColSpan, 20480);
                }
                break;
            case Html_table:
                if (key == "border"_L1) {
                    setFloatAttribute(&node->tableBorder, value);
                } else if (key == "bgcolor"_L1) {
                    QColor c = QColor::fromString(value);
                    if (!c.isValid())
                        qWarning("QTextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setBackground(c);
                } else if (key == "bordercolor"_L1) {
                    QColor c = QColor::fromString(value);
                    if (!c.isValid())
                        qWarning("QTextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->borderBrush = c;
                } else if (key == "background"_L1) {
                    node->applyBackgroundImage(value, resourceProvider);
                } else if (key == "cellspacing"_L1) {
                    setFloatAttribute(&node->tableCellSpacing, value);
                } else if (key == "cellpadding"_L1) {
                    setFloatAttribute(&node->tableCellPadding, value);
                } else if (key == "width"_L1) {
                    setWidthAttribute(&node->width, value);
                } else if (key == "height"_L1) {
                    setWidthAttribute(&node->height, value);
                }
                break;
            case Html_meta:
                if (key == "name"_L1 && value == "qrichtext"_L1)
                    seenQt3Richtext = true;

                if (key == "content"_L1 && value == "1"_L1 && seenQt3Richtext)
                    textEditMode = true;
                break;
            case Html_hr:
                if (key == "width"_L1)
                    setWidthAttribute(&node->width, value);
                break;
            case Html_link:
                if (key == "href"_L1)
                    linkHref = value;
                else if (key == "type"_L1)
                    linkType = value;
                break;
            case Html_pre:
                if (key == "class"_L1 && value.startsWith("language-"_L1))
                    node->blockFormat.setProperty(QTextFormat::BlockCodeLanguage, value.mid(9));
                break;
            default:
                break;
        }

        if (key == "style"_L1) {
#ifndef QT_NO_CSSPARSER
            node->parseStyleAttribute(value, resourceProvider);
#endif
        } else if (key == "align"_L1) {
            value = std::move(value).toLower();
            bool alignmentSet = true;

            if (value == "left"_L1)
                node->blockFormat.setAlignment(Qt::AlignLeft|Qt::AlignAbsolute);
            else if (value == "right"_L1)
                node->blockFormat.setAlignment(Qt::AlignRight|Qt::AlignAbsolute);
            else if (value == "center"_L1)
                node->blockFormat.setAlignment(Qt::AlignHCenter);
            else if (value == "justify"_L1)
                node->blockFormat.setAlignment(Qt::AlignJustify);
            else
                alignmentSet = false;

            if (node->id == Html_img) {
                // HTML4 compat
                if (alignmentSet) {
                    if (node->blockFormat.alignment() & Qt::AlignLeft)
                        node->cssFloat = QTextFrameFormat::FloatLeft;
                    else if (node->blockFormat.alignment() & Qt::AlignRight)
                        node->cssFloat = QTextFrameFormat::FloatRight;
                } else if (value == "middle"_L1) {
                    node->charFormat.setVerticalAlignment(QTextCharFormat::AlignMiddle);
                } else if (value == "top"_L1) {
                    node->charFormat.setVerticalAlignment(QTextCharFormat::AlignTop);
                }
            }
        } else if (key == "valign"_L1) {
            value = std::move(value).toLower();
            if (value == "top"_L1)
                node->charFormat.setVerticalAlignment(QTextCharFormat::AlignTop);
            else if (value == "middle"_L1)
                node->charFormat.setVerticalAlignment(QTextCharFormat::AlignMiddle);
            else if (value == "bottom"_L1)
                node->charFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        } else if (key == "dir"_L1) {
            value = std::move(value).toLower();
            if (value == "ltr"_L1)
                node->blockFormat.setLayoutDirection(Qt::LeftToRight);
            else if (value == "rtl"_L1)
                node->blockFormat.setLayoutDirection(Qt::RightToLeft);
        } else if (key == "title"_L1) {
            node->charFormat.setToolTip(value);
        } else if (key == "id"_L1) {
            node->charFormat.setAnchor(true);
            node->charFormat.setAnchorNames({value});
        }
    }

#ifndef QT_NO_CSSPARSER
    if (resourceProvider && !linkHref.isEmpty() && linkType == "text/css"_L1)
        importStyleSheet(linkHref);
#endif
}

#ifndef QT_NO_CSSPARSER
class QTextHtmlStyleSelector : public QCss::StyleSelector
{
public:
    inline QTextHtmlStyleSelector(const QTextHtmlParser *parser)
        : parser(parser) { nameCaseSensitivity = Qt::CaseInsensitive; }

    QStringList nodeNames(NodePtr node) const override;
    QString attributeValue(NodePtr node, const QCss::AttributeSelector &aSelector) const override;
    bool hasAttributes(NodePtr node) const override;
    bool isNullNode(NodePtr node) const override;
    NodePtr parentNode(NodePtr node) const override;
    NodePtr previousSiblingNode(NodePtr node) const override;
    NodePtr duplicateNode(NodePtr node) const override;
    void freeNode(NodePtr node) const override;

private:
    const QTextHtmlParser *parser;
};

QStringList QTextHtmlStyleSelector::nodeNames(NodePtr node) const
{
    return QStringList(parser->at(node.id).tag.toLower());
}

#endif // QT_NO_CSSPARSER

#ifndef QT_NO_CSSPARSER

static inline int findAttribute(const QStringList &attributes, const QString &name)
{
    int idx = -1;
    do {
        idx = attributes.indexOf(name, idx + 1);
    } while (idx != -1 && (idx % 2 == 1));
    return idx;
}

QString QTextHtmlStyleSelector::attributeValue(NodePtr node, const QCss::AttributeSelector &aSelector) const
{
    const QStringList &attributes = parser->at(node.id).attributes;
    const int idx = findAttribute(attributes, aSelector.name);
    if (idx == -1)
        return QString();
    return attributes.at(idx + 1);
}

bool QTextHtmlStyleSelector::hasAttributes(NodePtr node) const
{
   const QStringList &attributes = parser->at(node.id).attributes;
   return !attributes.isEmpty();
}

bool QTextHtmlStyleSelector::isNullNode(NodePtr node) const
{
    return node.id == 0;
}

QCss::StyleSelector::NodePtr QTextHtmlStyleSelector::parentNode(NodePtr node) const
{
    NodePtr parent;
    parent.id = 0;
    if (node.id) {
        parent.id = parser->at(node.id).parent;
    }
    return parent;
}

QCss::StyleSelector::NodePtr QTextHtmlStyleSelector::duplicateNode(NodePtr node) const
{
    return node;
}

QCss::StyleSelector::NodePtr QTextHtmlStyleSelector::previousSiblingNode(NodePtr node) const
{
    NodePtr sibling;
    sibling.id = 0;
    if (!node.id)
        return sibling;
    int parent = parser->at(node.id).parent;
    if (!parent)
        return sibling;
    const int childIdx = parser->at(parent).children.indexOf(node.id);
    if (childIdx <= 0)
        return sibling;
    sibling.id = parser->at(parent).children.at(childIdx - 1);
    return sibling;
}

void QTextHtmlStyleSelector::freeNode(NodePtr) const
{
}

void QTextHtmlParser::resolveStyleSheetImports(const QCss::StyleSheet &sheet)
{
    for (int i = 0; i < sheet.importRules.size(); ++i) {
        const QCss::ImportRule &rule = sheet.importRules.at(i);
        if (rule.media.isEmpty() || rule.media.contains("screen"_L1, Qt::CaseInsensitive))
            importStyleSheet(rule.href);
    }
}

void QTextHtmlParser::importStyleSheet(const QString &href)
{
    if (!resourceProvider)
        return;
    for (int i = 0; i < externalStyleSheets.size(); ++i)
        if (externalStyleSheets.at(i).url == href)
            return;

    QVariant res = resourceProvider->resource(QTextDocument::StyleSheetResource, href);
    QString css;
    if (res.userType() == QMetaType::QString) {
        css = res.toString();
    } else if (res.userType() == QMetaType::QByteArray) {
        // #### detect @charset
        css = QString::fromUtf8(res.toByteArray());
    }
    if (!css.isEmpty()) {
        QCss::Parser parser(css);
        QCss::StyleSheet sheet;
        parser.parse(&sheet, Qt::CaseInsensitive);
        externalStyleSheets.append(ExternalStyleSheet(href, sheet));
        resolveStyleSheetImports(sheet);
    }
}

QList<QCss::Declaration> standardDeclarationForNode(const QTextHtmlParserNode &node)
{
    QList<QCss::Declaration> decls;
    QCss::Declaration decl;
    QCss::Value val;
    switch (node.id) {
    case Html_a:
    case Html_u: {
        bool needsUnderline = (node.id == Html_u) ? true : false;
        if (node.id == Html_a) {
            for (int i = 0; i < node.attributes.size(); i += 2) {
                const QString key = node.attributes.at(i);
                if (key.compare("href"_L1, Qt::CaseInsensitive) == 0
                    && !node.attributes.at(i + 1).isEmpty()) {
                    needsUnderline = true;
                    decl.d->property = "color"_L1;
                    decl.d->propertyId = QCss::Color;
                    val.type = QCss::Value::Function;
                    val.variant = QStringList() << "palette"_L1 << "link"_L1;
                    decl.d->values = QList<QCss::Value> { val };
                    decl.d->inheritable = true;
                    decls << decl;
                    break;
                }
            }
        }
        if (needsUnderline) {
            decl = QCss::Declaration();
            decl.d->property = "text-decoration"_L1;
            decl.d->propertyId = QCss::TextDecoration;
            val.type = QCss::Value::KnownIdentifier;
            val.variant = QVariant(QCss::Value_Underline);
            decl.d->values = QList<QCss::Value> { val };
            decl.d->inheritable = true;
            decls << decl;
        }
        break;
    }
    case Html_b:
    case Html_strong:
    case Html_h1:
    case Html_h2:
    case Html_h3:
    case Html_h4:
    case Html_h5:
    case Html_th:
        decl = QCss::Declaration();
        decl.d->property = "font-weight"_L1;
        decl.d->propertyId = QCss::FontWeight;
        val.type = QCss::Value::KnownIdentifier;
        val.variant = QVariant(QCss::Value_Bold);
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        if (node.id == Html_b || node.id == Html_strong)
            break;
        Q_FALLTHROUGH();
    case Html_big:
    case Html_small:
        if (node.id != Html_th) {
            decl = QCss::Declaration();
            decl.d->property = "font-size"_L1;
            decl.d->propertyId = QCss::FontSize;
            decl.d->inheritable = false;
            val.type = QCss::Value::KnownIdentifier;
            switch (node.id) {
            case Html_h1: val.variant = QVariant(QCss::Value_XXLarge); break;
            case Html_h2: val.variant = QVariant(QCss::Value_XLarge); break;
            case Html_h3: case Html_big: val.variant = QVariant(QCss::Value_Large); break;
            case Html_h4: val.variant = QVariant(QCss::Value_Medium); break;
            case Html_h5: case Html_small: val.variant = QVariant(QCss::Value_Small); break;
            default: break;
            }
            decl.d->values = QList<QCss::Value> { val };
            decls << decl;
            break;
        }
        Q_FALLTHROUGH();
    case Html_center:
    case Html_td:
        decl = QCss::Declaration();
        decl.d->property = "text-align"_L1;
        decl.d->propertyId = QCss::TextAlignment;
        val.type = QCss::Value::KnownIdentifier;
        val.variant = (node.id == Html_td) ? QVariant(QCss::Value_Left) : QVariant(QCss::Value_Center);
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        break;
    case Html_s:
        decl = QCss::Declaration();
        decl.d->property = "text-decoration"_L1;
        decl.d->propertyId = QCss::TextDecoration;
        val.type = QCss::Value::KnownIdentifier;
        val.variant = QVariant(QCss::Value_LineThrough);
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        break;
    case Html_em:
    case Html_i:
    case Html_cite:
    case Html_address:
    case Html_var:
    case Html_dfn:
        decl = QCss::Declaration();
        decl.d->property = "font-style"_L1;
        decl.d->propertyId = QCss::FontStyle;
        val.type = QCss::Value::KnownIdentifier;
        val.variant = QVariant(QCss::Value_Italic);
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        break;
    case Html_sub:
    case Html_sup:
        decl = QCss::Declaration();
        decl.d->property = "vertical-align"_L1;
        decl.d->propertyId = QCss::VerticalAlignment;
        val.type = QCss::Value::KnownIdentifier;
        val.variant = (node.id == Html_sub) ? QVariant(QCss::Value_Sub) : QVariant(QCss::Value_Super);
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        break;
    case Html_ul:
    case Html_ol:
        decl = QCss::Declaration();
        decl.d->property = "list-style"_L1;
        decl.d->propertyId = QCss::ListStyle;
        val.type = QCss::Value::KnownIdentifier;
        val.variant = (node.id == Html_ul) ? QVariant(QCss::Value_Disc) : QVariant(QCss::Value_Decimal);
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        break;
    case Html_code:
    case Html_tt:
    case Html_kbd:
    case Html_samp:
    case Html_pre: {
        decl = QCss::Declaration();
        decl.d->property = "font-family"_L1;
        decl.d->propertyId = QCss::FontFamily;
        QList<QCss::Value> values;
        val.type = QCss::Value::String;
        val.variant = QFontDatabase::systemFont(QFontDatabase::FixedFont).families().first();
        values << val;
        decl.d->values = values;
        decl.d->inheritable = true;
        decls << decl;
        }
        if (node.id != Html_pre)
            break;
        Q_FALLTHROUGH();
    case Html_br:
    case Html_nobr:
        decl = QCss::Declaration();
        decl.d->property = "whitespace"_L1;
        decl.d->propertyId = QCss::Whitespace;
        val.type = QCss::Value::KnownIdentifier;
        switch (node.id) {
        case Html_br: val.variant = QVariant(QCss::Value_PreWrap); break;
        case Html_nobr: val.variant = QVariant(QCss::Value_NoWrap); break;
        case Html_pre: val.variant = QVariant(QCss::Value_Pre); break;
        default: break;
        }
        decl.d->values = QList<QCss::Value> { val };
        decl.d->inheritable = true;
        decls << decl;
        break;
    default:
        break;
    }
    return decls;
}

QList<QCss::Declaration> QTextHtmlParser::declarationsForNode(int node) const
{
    QList<QCss::Declaration> decls;

    QTextHtmlStyleSelector selector(this);

    int idx = 0;
    selector.styleSheets.resize((resourceProvider ? 1 : 0)
                                + externalStyleSheets.size()
                                + inlineStyleSheets.size());
    if (resourceProvider)
        selector.styleSheets[idx++] = QTextDocumentPrivate::get(resourceProvider)->parsedDefaultStyleSheet;

    for (int i = 0; i < externalStyleSheets.size(); ++i, ++idx)
        selector.styleSheets[idx] = externalStyleSheets.at(i).sheet;

    for (int i = 0; i < inlineStyleSheets.size(); ++i, ++idx)
        selector.styleSheets[idx] = inlineStyleSheets.at(i);

    selector.medium = resourceProvider ? resourceProvider->metaInformation(QTextDocument::CssMedia) : "screen"_L1;

    QCss::StyleSelector::NodePtr n;
    n.id = node;

    const char *extraPseudo = nullptr;
    if (nodes.at(node)->id == Html_a && nodes.at(node)->hasHref)
        extraPseudo = "link";
    // Ensure that our own style is taken into consideration
    decls = standardDeclarationForNode(*nodes.at(node));
    decls += selector.declarationsForNode(n, extraPseudo);
    n = selector.parentNode(n);
    while (!selector.isNullNode(n)) {
        QList<QCss::Declaration> inheritedDecls;
        inheritedDecls = selector.declarationsForNode(n, extraPseudo);
        for (int i = 0; i < inheritedDecls.size(); ++i) {
            const QCss::Declaration &decl = inheritedDecls.at(i);
            if (decl.d->inheritable)
                decls.prepend(decl);
        }
        n = selector.parentNode(n);
    }
    return decls;
}

bool QTextHtmlParser::nodeIsChildOf(int i, QTextHTMLElements id) const
{
    while (i) {
        if (at(i).id == id)
            return true;
        i = at(i).parent;
    }
    return false;
}

QT_END_NAMESPACE
#endif // QT_NO_CSSPARSER

#endif // QT_NO_TEXTHTMLPARSER
