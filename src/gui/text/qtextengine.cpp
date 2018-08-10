/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/private/qtguiglobal_p.h>
#include "qdebug.h"
#include "qtextformat.h"
#include "qtextformat_p.h"
#include "qtextengine_p.h"
#include "qabstracttextdocumentlayout.h"
#include "qtextlayout.h"
#include "qtextboundaryfinder.h"
#include <QtCore/private/qunicodetables_p.h>
#include "qvarlengtharray.h"
#include "qfont.h"
#include "qfont_p.h"
#include "qfontengine_p.h"
#include "qstring.h"
#include "qtextdocument_p.h"
#include "qrawfont.h"
#include "qrawfont_p.h"
#include <qguiapplication.h>
#include <qinputmethod.h>
#include <algorithm>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

static const float smallCapsFraction = 0.7f;

namespace {
// Helper class used in QTextEngine::itemize
// keep it out here to allow us to keep supporting various compilers.
class Itemizer {
public:
    Itemizer(const QString &string, const QScriptAnalysis *analysis, QScriptItemArray &items)
        : m_string(string),
        m_analysis(analysis),
        m_items(items),
        m_splitter(0)
    {
    }
    ~Itemizer()
    {
        delete m_splitter;
    }

    /// generate the script items
    /// The caps parameter is used to choose the algoritm of splitting text and assiging roles to the textitems
    void generate(int start, int length, QFont::Capitalization caps)
    {
        if (caps == QFont::SmallCaps)
            generateScriptItemsSmallCaps(reinterpret_cast<const ushort *>(m_string.unicode()), start, length);
        else if(caps == QFont::Capitalize)
            generateScriptItemsCapitalize(start, length);
        else if(caps != QFont::MixedCase) {
            generateScriptItemsAndChangeCase(start, length,
                caps == QFont::AllLowercase ? QScriptAnalysis::Lowercase : QScriptAnalysis::Uppercase);
        }
        else
            generateScriptItems(start, length);
    }

private:
    enum { MaxItemLength = 4096 };

    void generateScriptItemsAndChangeCase(int start, int length, QScriptAnalysis::Flags flags)
    {
        generateScriptItems(start, length);
        if (m_items.isEmpty()) // the next loop won't work in that case
            return;
        QScriptItemArray::Iterator iter = m_items.end();
        do {
            iter--;
            if (iter->analysis.flags < QScriptAnalysis::LineOrParagraphSeparator)
                iter->analysis.flags = flags;
        } while (iter->position > start);
    }

    void generateScriptItems(int start, int length)
    {
        if (!length)
            return;
        const int end = start + length;
        for (int i = start + 1; i < end; ++i) {
            if (m_analysis[i].bidiLevel == m_analysis[start].bidiLevel
                && m_analysis[i].flags == m_analysis[start].flags
                && (m_analysis[i].script == m_analysis[start].script || m_string[i] == QLatin1Char('.'))
                && m_analysis[i].flags < QScriptAnalysis::SpaceTabOrObject
                && i - start < MaxItemLength)
                continue;
            m_items.append(QScriptItem(start, m_analysis[start]));
            start = i;
        }
        m_items.append(QScriptItem(start, m_analysis[start]));
    }

    void generateScriptItemsCapitalize(int start, int length)
    {
        if (!length)
            return;

        if (!m_splitter)
            m_splitter = new QTextBoundaryFinder(QTextBoundaryFinder::Word,
                                                 m_string.constData(), m_string.length(),
                                                 /*buffer*/0, /*buffer size*/0);

        m_splitter->setPosition(start);
        QScriptAnalysis itemAnalysis = m_analysis[start];

        if (m_splitter->boundaryReasons() & QTextBoundaryFinder::StartOfItem)
            itemAnalysis.flags = QScriptAnalysis::Uppercase;

        m_splitter->toNextBoundary();

        const int end = start + length;
        for (int i = start + 1; i < end; ++i) {
            bool atWordStart = false;

            if (i == m_splitter->position()) {
                if (m_splitter->boundaryReasons() & QTextBoundaryFinder::StartOfItem) {
                    Q_ASSERT(m_analysis[i].flags < QScriptAnalysis::TabOrObject);
                    atWordStart = true;
                }

                m_splitter->toNextBoundary();
            }

            if (m_analysis[i] == itemAnalysis
                && m_analysis[i].flags < QScriptAnalysis::TabOrObject
                && !atWordStart
                && i - start < MaxItemLength)
                continue;

            m_items.append(QScriptItem(start, itemAnalysis));
            start = i;
            itemAnalysis = m_analysis[start];

            if (atWordStart)
                itemAnalysis.flags = QScriptAnalysis::Uppercase;
        }
        m_items.append(QScriptItem(start, itemAnalysis));
    }

    void generateScriptItemsSmallCaps(const ushort *uc, int start, int length)
    {
        if (!length)
            return;
        bool lower = (QChar::category(uc[start]) == QChar::Letter_Lowercase);
        const int end = start + length;
        // split text into parts that are already uppercase and parts that are lowercase, and mark the latter to be uppercased later.
        for (int i = start + 1; i < end; ++i) {
            bool l = (QChar::category(uc[i]) == QChar::Letter_Lowercase);
            if ((m_analysis[i] == m_analysis[start])
                && m_analysis[i].flags < QScriptAnalysis::TabOrObject
                && l == lower
                && i - start < MaxItemLength)
                continue;
            m_items.append(QScriptItem(start, m_analysis[start]));
            if (lower)
                m_items.last().analysis.flags = QScriptAnalysis::SmallCaps;

            start = i;
            lower = l;
        }
        m_items.append(QScriptItem(start, m_analysis[start]));
        if (lower)
            m_items.last().analysis.flags = QScriptAnalysis::SmallCaps;
    }

    const QString &m_string;
    const QScriptAnalysis * const m_analysis;
    QScriptItemArray &m_items;
    QTextBoundaryFinder *m_splitter;
};

// -----------------------------------------------------------------------------------------------------
//
// The Unicode Bidi algorithm.
// See http://www.unicode.org/reports/tr9/tr9-37.html
//
// -----------------------------------------------------------------------------------------------------

// #define DEBUG_BIDI
#ifndef DEBUG_BIDI
enum { BidiDebugEnabled = false };
#define BIDI_DEBUG if (1) ; else qDebug
#else
enum { BidiDebugEnabled = true };
static const char *directions[] = {
    "DirL", "DirR", "DirEN", "DirES", "DirET", "DirAN", "DirCS", "DirB", "DirS", "DirWS", "DirON",
    "DirLRE", "DirLRO", "DirAL", "DirRLE", "DirRLO", "DirPDF", "DirNSM", "DirBN",
    "DirLRI", "DirRLI", "DirFSI", "DirPDI"
};
#define BIDI_DEBUG qDebug
QDebug operator<<(QDebug d, QChar::Direction dir) {
    return (d << directions[dir]);
}
#endif

struct QBidiAlgorithm {
    template<typename T> using Vector = QVarLengthArray<T, 64>;

    QBidiAlgorithm(const QChar *text, QScriptAnalysis *analysis, int length, bool baseDirectionIsRtl)
        : text(text),
          analysis(analysis),
          length(length),
          baseLevel(baseDirectionIsRtl ? 1 : 0)
    {

    }

    struct IsolatePair {
        int start;
        int end;
    };

    void initScriptAnalysisAndIsolatePairs(Vector<IsolatePair> &isolatePairs)
    {
        isolatePairs.append({ -1, length }); // treat the whole string as one isolate

        int isolateStack[128];
        int isolateLevel = 0;
        // load directions of string, and determine isolate pairs
        for (int i = 0; i < length; ++i) {
            int pos = i;
            uint uc = text[i].unicode();
            if (QChar::isHighSurrogate(uc) && i < length - 1) {
                ++i;
                analysis[i].bidiDirection = QChar::DirNSM;
                uc = QChar::surrogateToUcs4(ushort(uc), text[i].unicode());
            }
            const QUnicodeTables::Properties *p = QUnicodeTables::properties(uc);
            analysis[pos].bidiDirection = QChar::Direction(p->direction);
            switch (QChar::Direction(p->direction)) {
            case QChar::DirON:
                // all mirrored chars are DirON
                if (p->mirrorDiff)
                    analysis[pos].bidiFlags = QScriptAnalysis::BidiMirrored;
                break;
            case QChar::DirLRE:
            case QChar::DirRLE:
            case QChar::DirLRO:
            case QChar::DirRLO:
            case QChar::DirPDF:
            case QChar::DirBN:
                analysis[pos].bidiFlags = QScriptAnalysis::BidiMaybeResetToParagraphLevel|QScriptAnalysis::BidiBN;
                break;
            case QChar::DirLRI:
            case QChar::DirRLI:
            case QChar::DirFSI:
                if (isolateLevel < 128) {
                    isolateStack[isolateLevel] = isolatePairs.size();
                    isolatePairs.append({ pos, length });
                }
                ++isolateLevel;
                analysis[pos].bidiFlags = QScriptAnalysis::BidiMaybeResetToParagraphLevel;
                break;
            case QChar::DirPDI:
                if (isolateLevel > 0) {
                    --isolateLevel;
                    if (isolateLevel < 128)
                        isolatePairs[isolateStack[isolateLevel]].end = pos;
                }
                Q_FALLTHROUGH();
            case QChar::DirWS:
                analysis[pos].bidiFlags = QScriptAnalysis::BidiMaybeResetToParagraphLevel;
                break;
            case QChar::DirS:
            case QChar::DirB:
                analysis[pos].bidiFlags = QScriptAnalysis::BidiResetToParagraphLevel;
                break;
            default:
                break;
            }
        }
    }

    struct DirectionalRun {
        int start;
        int end;
        int continuation;
        ushort level;
        bool isContinuation;
        bool hasContent;
    };

    void generateDirectionalRuns(const Vector<IsolatePair> &isolatePairs, Vector<DirectionalRun> &runs)
    {
        struct DirectionalStack {
            enum { MaxDepth = 125 };
            struct Item {
                ushort level;
                bool isOverride;
                bool isIsolate;
                int runBeforeIsolate;
            };
            Item items[128];
            int counter = 0;

            void push(Item i) {
                items[counter] = i;
                ++counter;
            }
            void pop() {
                --counter;
            }
            int depth() const {
                return counter;
            }
            const Item &top() const {
                return items[counter - 1];
            }
        } stack;
        int overflowIsolateCount = 0;
        int overflowEmbeddingCount = 0;
        int validIsolateCount = 0;

        ushort level = baseLevel;
        bool override = false;
        stack.push({ level, false, false, -1 });

        BIDI_DEBUG() << "resolving explicit levels";
        int runStart = 0;
        int continuationFrom = -1;
        int lastRunWithContent = -1;
        bool runHasContent = false;

        auto appendRun = [&](int runEnd) {
            if (runEnd < runStart)
                return;
            bool isContinuation = false;
            if (continuationFrom != -1) {
                runs[continuationFrom].continuation = runs.size();
                isContinuation = true;
            } else if (lastRunWithContent != -1 && level == runs.at(lastRunWithContent).level) {
                runs[lastRunWithContent].continuation = runs.size();
                isContinuation = true;
            }
            if (runHasContent)
                lastRunWithContent = runs.size();
            BIDI_DEBUG() << "   appending run start/end" << runStart << runEnd << "level" << level;
            runs.append({ runStart, runEnd, -1, level, isContinuation, runHasContent });
            runHasContent = false;
            runStart = runEnd + 1;
            continuationFrom = -1;
        };

        int isolatePairPosition = 0;

        for (int i = 0; i < length; ++i) {
            QChar::Direction dir = analysis[i].bidiDirection;


            auto doEmbed = [&](bool isRtl, bool isOverride, bool isIsolate) {
                if (isIsolate) {
                    if (override)
                        analysis[i].bidiDirection = (level & 1) ? QChar::DirR : QChar::DirL;
                    runHasContent = true;
                    lastRunWithContent = -1;
                }
                int runBeforeIsolate = runs.size();
                ushort newLevel = isRtl ? ((stack.top().level + 1) | 1) : ((stack.top().level + 2) & ~1);
                if (newLevel <= DirectionalStack::MaxDepth && !overflowEmbeddingCount && !overflowIsolateCount) {
                    if (isIsolate)
                        ++validIsolateCount;
                    else
                        runBeforeIsolate = -1;
                    appendRun(isIsolate ? i : i - 1);
                    BIDI_DEBUG() << "pushing new item on stack: level" << (int)newLevel << "isOverride" << isOverride << "isIsolate" << isIsolate << runBeforeIsolate;
                    stack.push({ newLevel, isOverride, isIsolate, runBeforeIsolate });
                    override = isOverride;
                    level = newLevel;
                } else {
                    if (isIsolate)
                        ++overflowIsolateCount;
                    else if (!overflowIsolateCount)
                        ++overflowEmbeddingCount;
                }
                if (!isIsolate) {
                    if (override)
                        analysis[i].bidiDirection = (level & 1) ? QChar::DirR : QChar::DirL;
                    else
                        analysis[i].bidiDirection = QChar::DirBN;
                }
            };

            switch (dir) {
            case QChar::DirLRE:
                doEmbed(false, false, false);
                break;
            case QChar::DirRLE:
                doEmbed(true, false, false);
                break;
            case QChar::DirLRO:
                doEmbed(false, true, false);
                break;
            case QChar::DirRLO:
                doEmbed(true, true, false);
                break;
            case QChar::DirLRI:
                ++isolatePairPosition;
                Q_ASSERT(isolatePairs.at(isolatePairPosition).start == i);
                doEmbed(false, false, true);
                break;
            case QChar::DirRLI:
                ++isolatePairPosition;
                Q_ASSERT(isolatePairs.at(isolatePairPosition).start == i);
                doEmbed(true, false, true);
                break;
            case QChar::DirFSI: {
                ++isolatePairPosition;
                const auto &pair = isolatePairs.at(isolatePairPosition);
                Q_ASSERT(pair.start == i);
                bool isRtl = QStringView(text + pair.start + 1, pair.end - pair.start - 1).isRightToLeft();
                doEmbed(isRtl, false, true);
                break;
            }

            case QChar::DirPDF:
                if (override)
                    analysis[i].bidiDirection = (level & 1) ? QChar::DirR : QChar::DirL;
                else
                    analysis[i].bidiDirection = QChar::DirBN;
                if (overflowIsolateCount) {
                    ; // do nothing
                } else if (overflowEmbeddingCount) {
                    --overflowEmbeddingCount;
                } else if (!stack.top().isIsolate && stack.depth() >= 2) {
                    appendRun(i);
                    stack.pop();
                    override = stack.top().isOverride;
                    level = stack.top().level;
                    BIDI_DEBUG() << "popped PDF from stack, level now" << (int)stack.top().level;
                }
                break;
            case QChar::DirPDI:
                runHasContent = true;
                if (overflowIsolateCount) {
                    --overflowIsolateCount;
                } else if (validIsolateCount == 0) {
                    ; // do nothing
                } else {
                    appendRun(i - 1);
                    overflowEmbeddingCount = 0;
                    while (!stack.top().isIsolate)
                        stack.pop();
                    continuationFrom = stack.top().runBeforeIsolate;
                    BIDI_DEBUG() << "popped PDI from stack, level now" << (int)stack.top().level << "continuation from" << continuationFrom;
                    stack.pop();
                    override = stack.top().isOverride;
                    level = stack.top().level;
                    lastRunWithContent = -1;
                    --validIsolateCount;
                }
                if (override)
                    analysis[i].bidiDirection = (level & 1) ? QChar::DirR : QChar::DirL;
                break;
            case QChar::DirB:
                // paragraph separator, go down to base direction
                appendRun(i - 1);
                while (stack.counter > 1) {
                    // there might be remaining isolates on the stack that are missing a PDI. Those need to get
                    // a continuation indicating to take the eos from the end of the string (ie. the paragraph level)
                    const auto &t = stack.top();
                    if (t.isIsolate) {
                        runs[t.runBeforeIsolate].continuation = -2;
                    }
                    --stack.counter;
                }
                break;
            default:
                runHasContent = true;
                Q_FALLTHROUGH();
            case QChar::DirBN:
                if (override)
                    analysis[i].bidiDirection = (level & 1) ? QChar::DirR : QChar::DirL;
                break;
            }
        }
        appendRun(length - 1);
        while (stack.counter > 1) {
            // there might be remaining isolates on the stack that are missing a PDI. Those need to get
            // a continuation indicating to take the eos from the end of the string (ie. the paragraph level)
            const auto &t = stack.top();
            if (t.isIsolate) {
                runs[t.runBeforeIsolate].continuation = -2;
            }
            --stack.counter;
        }
    }

    void resolveExplicitLevels(Vector<DirectionalRun> &runs)
    {
        Vector<IsolatePair> isolatePairs;

        initScriptAnalysisAndIsolatePairs(isolatePairs);
        generateDirectionalRuns(isolatePairs, runs);
    }

    struct IsolatedRunSequenceIterator {
        struct Position {
            int current = -1;
            int pos = -1;

            Position() = default;
            Position(int current, int pos) : current(current), pos(pos) {}

            bool isValid() const { return pos != -1; }
            void clear() { pos = -1; }
        };
        IsolatedRunSequenceIterator(const Vector<DirectionalRun> &runs, int i)
            : runs(runs),
              current(i)
        {
            pos = runs.at(current).start;
        }
        int operator *() const { return pos; }
        bool atEnd() const { return pos < 0; }
        void operator++() {
            ++pos;
            if (pos > runs.at(current).end) {
                current = runs.at(current).continuation;
                if (current > -1)
                    pos = runs.at(current).start;
                else
                    pos = -1;
            }
        }
        void setPosition(Position p) {
            current = p.current;
            pos = p.pos;
        }
        Position position() const {
            return Position(current, pos);
        }
        bool operator !=(int position) const {
            return pos != position;
        }

        const Vector<DirectionalRun> &runs;
        int current;
        int pos;
    };


    void resolveW1W2W3(const Vector<DirectionalRun> &runs, int i, QChar::Direction sos)
    {
        QChar::Direction last = sos;
        QChar::Direction lastStrong = sos;
        IsolatedRunSequenceIterator it(runs, i);
        while (!it.atEnd()) {
            int pos = *it;

            // Rule W1: Resolve NSM
            QChar::Direction current = analysis[pos].bidiDirection;
            if (current == QChar::DirNSM) {
                current = last;
                analysis[pos].bidiDirection = current;
            } else if (current >= QChar::DirLRI) {
                last = QChar::DirON;
            } else if (current == QChar::DirBN) {
                current = last;
            } else {
                Q_ASSERT(current != QChar::DirLRE && current != QChar::DirRLE && current != QChar::DirLRO && current != QChar::DirRLO && current != QChar::DirPDF); // there shouldn't be any explicit embedding marks here
                last = current;
            }

            // Rule W2
            if (current == QChar::DirEN && lastStrong == QChar::DirAL) {
                current = QChar::DirAN;
                analysis[pos].bidiDirection = current;
            }

            // remember last strong char for rule W2
            if (current == QChar::DirL || current == QChar::DirR) {
                lastStrong = current;
            } else if (current == QChar::DirAL) {
                // Rule W3
                lastStrong = current;
                analysis[pos].bidiDirection = QChar::DirR;
            }
            last = current;
            ++it;
        }
    }


    void resolveW4(const Vector<DirectionalRun> &runs, int i, QChar::Direction sos)
    {
        // Rule W4
        QChar::Direction secondLast = sos;

        IsolatedRunSequenceIterator it(runs, i);
        int lastPos = *it;
        QChar::Direction last = analysis[lastPos].bidiDirection;

//            BIDI_DEBUG() << "Applying rule W4/W5";
        ++it;
        while (!it.atEnd()) {
            int pos = *it;
            QChar::Direction current = analysis[pos].bidiDirection;
            if (current == QChar::DirBN) {
                ++it;
                continue;
            }
//                BIDI_DEBUG() << pos << secondLast << last << current;
            if (last == QChar::DirES && current == QChar::DirEN && secondLast == QChar::DirEN) {
                last = QChar::DirEN;
                analysis[lastPos].bidiDirection = last;
            } else if (last == QChar::DirCS) {
                if (current == QChar::DirEN && secondLast == QChar::DirEN) {
                    last = QChar::DirEN;
                    analysis[lastPos].bidiDirection = last;
                } else if (current == QChar::DirAN && secondLast == QChar::DirAN) {
                    last = QChar::DirAN;
                    analysis[lastPos].bidiDirection = last;
                }
            }
            secondLast = last;
            last = current;
            lastPos = pos;
            ++it;
        }
    }

    void resolveW5(const Vector<DirectionalRun> &runs, int i)
    {
        // Rule W5
        IsolatedRunSequenceIterator::Position lastETPosition;

        IsolatedRunSequenceIterator it(runs, i);
        int lastPos = *it;
        QChar::Direction last = analysis[lastPos].bidiDirection;
        if (last == QChar::DirET || last == QChar::DirBN)
            lastETPosition = it.position();

        ++it;
        while (!it.atEnd()) {
            int pos = *it;
            QChar::Direction current = analysis[pos].bidiDirection;
            if (current == QChar::DirBN) {
                ++it;
                continue;
            }
            if (current == QChar::DirET) {
                if (last == QChar::DirEN) {
                    current = QChar::DirEN;
                    analysis[pos].bidiDirection = current;
                } else if (!lastETPosition.isValid()) {
                    lastETPosition = it.position();
                }
            } else if (lastETPosition.isValid()) {
                if (current == QChar::DirEN) {
                    it.setPosition(lastETPosition);
                    while (it != pos) {
                        int pos = *it;
                        analysis[pos].bidiDirection = QChar::DirEN;
                        ++it;
                    }
                } else {
                    lastETPosition.clear();
                }
            }
            last = current;
            lastPos = pos;
            ++it;
        }
    }

    void resolveW6W7(const Vector<DirectionalRun> &runs, int i, QChar::Direction sos)
    {
        QChar::Direction lastStrong = sos;
        IsolatedRunSequenceIterator it(runs, i);
        while (!it.atEnd()) {
            int pos = *it;

            // Rule W6
            QChar::Direction current = analysis[pos].bidiDirection;
            if (current == QChar::DirBN) {
                ++it;
                continue;
            }
            if (current == QChar::DirET || current == QChar::DirES || current == QChar::DirCS) {
                analysis[pos].bidiDirection = QChar::DirON;
            }

            // Rule W7
            else if (current == QChar::DirL || current == QChar::DirR) {
                lastStrong = current;
            } else if (current == QChar::DirEN && lastStrong == QChar::DirL) {
                analysis[pos].bidiDirection = lastStrong;
            }
            ++it;
        }
    }

    struct BracketPair {
        int first;
        int second;

        bool isValid() const { return second > 0; }

        QChar::Direction containedDirection(const QScriptAnalysis *analysis, QChar::Direction embeddingDir) const {
            int isolateCounter = 0;
            QChar::Direction containedDir = QChar::DirON;
            for (int i = first + 1; i < second; ++i) {
                QChar::Direction dir = analysis[i].bidiDirection;
                if (isolateCounter) {
                    if (dir == QChar::DirPDI)
                        --isolateCounter;
                    continue;
                }
                if (dir == QChar::DirL) {
                    containedDir = dir;
                    if (embeddingDir == dir)
                        break;
                } else if (dir == QChar::DirR || dir == QChar::DirAN || dir == QChar::DirEN) {
                    containedDir = QChar::DirR;
                    if (embeddingDir == QChar::DirR)
                        break;
                } else if (dir == QChar::DirLRI || dir == QChar::DirRLI || dir == QChar::DirFSI)
                    ++isolateCounter;
            }
            BIDI_DEBUG() << "    contained dir for backet pair" << first << "/" << second << "is" << containedDir;
            return containedDir;
        }
    };


    struct BracketStack {
        struct Item {
            Item() = default;
            Item(uint pairedBracked, int position) : pairedBracked(pairedBracked), position(position) {}
            uint pairedBracked = 0;
            int position = 0;
        };

        void push(uint closingUnicode, int pos) {
            if (position < MaxDepth)
                stack[position] = Item(closingUnicode, pos);
            ++position;
        }
        int match(uint unicode) {
            Q_ASSERT(!overflowed());
            int p = position;
            while (--p >= 0) {
                if (stack[p].pairedBracked == unicode ||
                    // U+3009 and U+2329 are canonical equivalents of each other. Fortunately it's the only pair in Unicode 10
                    (stack[p].pairedBracked == 0x3009 && unicode == 0x232a) ||
                    (stack[p].pairedBracked == 0x232a && unicode == 0x3009)) {
                    position = p;
                    return stack[p].position;
                }

            }
            return -1;
        }

        enum { MaxDepth = 63 };
        Item stack[MaxDepth];
        int position = 0;

        bool overflowed() const { return position > MaxDepth; }
    };

    void resolveN0(const Vector<DirectionalRun> &runs, int i, QChar::Direction sos)
    {
        ushort level = runs.at(i).level;

        Vector<BracketPair> bracketPairs;
        {
            BracketStack bracketStack;
            IsolatedRunSequenceIterator it(runs, i);
            while (!it.atEnd()) {
                int pos = *it;
                QChar::Direction dir = analysis[pos].bidiDirection;
                if (dir == QChar::DirON) {
                    const QUnicodeTables::Properties *p = QUnicodeTables::properties(text[pos].unicode());
                    if (p->mirrorDiff) {
                        // either opening or closing bracket
                        if (p->category == QChar::Punctuation_Open) {
                            // opening bracked
                            uint closingBracked = text[pos].unicode() + p->mirrorDiff;
                            bracketStack.push(closingBracked, bracketPairs.size());
                            if (bracketStack.overflowed()) {
                                bracketPairs.clear();
                                break;
                            }
                            bracketPairs.append({ pos, -1 });
                        } else if (p->category == QChar::Punctuation_Close) {
                            int pairPos = bracketStack.match(text[pos].unicode());
                            if (pairPos != -1)
                                bracketPairs[pairPos].second = pos;
                        }
                    }
                }
                ++it;
            }
        }

        if (BidiDebugEnabled && bracketPairs.size()) {
            BIDI_DEBUG() << "matched bracket pairs:";
            for (int i = 0; i < bracketPairs.size(); ++i)
                BIDI_DEBUG() << "   " << bracketPairs.at(i).first << bracketPairs.at(i).second;
        }

        QChar::Direction lastStrong = sos;
        IsolatedRunSequenceIterator it(runs, i);
        QChar::Direction embeddingDir = (level & 1) ? QChar::DirR : QChar::DirL;
        for (int i = 0; i < bracketPairs.size(); ++i) {
            const auto &pair = bracketPairs.at(i);
            if (!pair.isValid())
                continue;
            QChar::Direction containedDir = pair.containedDirection(analysis, embeddingDir);
            if (containedDir == QChar::DirON) {
                BIDI_DEBUG() << "    3: resolve bracket pair" << i << "to DirON";
                continue;
            } else if (containedDir == embeddingDir) {
                analysis[pair.first].bidiDirection = embeddingDir;
                analysis[pair.second].bidiDirection = embeddingDir;
                BIDI_DEBUG() << "    1: resolve bracket pair" << i << "to" << embeddingDir;
            } else {
                // case c.
                while (it.pos < pair.first) {
                    int pos = *it;
                    switch (analysis[pos].bidiDirection) {
                    case QChar::DirR:
                    case QChar::DirEN:
                    case QChar::DirAN:
                        lastStrong = QChar::DirR;
                        break;
                    case QChar::DirL:
                        lastStrong = QChar::DirL;
                        break;
                    default:
                        break;
                    }
                    ++it;
                }
                analysis[pair.first].bidiDirection = lastStrong;
                analysis[pair.second].bidiDirection = lastStrong;
                BIDI_DEBUG() << "    2: resolve bracket pair" << i << "to" << lastStrong;
            }
            for (int i = pair.second + 1; i < length; ++i) {
                if (text[i].direction() == QChar::DirNSM)
                    analysis[i].bidiDirection = analysis[pair.second].bidiDirection;
                else
                    break;
            }
        }
    }

    void resolveN1N2(const Vector<DirectionalRun> &runs, int i, QChar::Direction sos, QChar::Direction eos)
    {
        // Rule N1 & N2
        QChar::Direction lastStrong = sos;
        IsolatedRunSequenceIterator::Position niPos;
        IsolatedRunSequenceIterator it(runs, i);
//            QChar::Direction last = QChar::DirON;
        while (1) {
            int pos = *it;

            QChar::Direction current = pos >= 0 ? analysis[pos].bidiDirection : eos;
            QChar::Direction currentStrong = current;
            switch (current) {
            case QChar::DirEN:
            case QChar::DirAN:
                currentStrong = QChar::DirR;
                Q_FALLTHROUGH();
            case QChar::DirL:
            case QChar::DirR:
                if (niPos.isValid()) {
                    QChar::Direction dir = currentStrong;
                    if (lastStrong != currentStrong)
                        dir = (runs.at(i).level) & 1 ? QChar::DirR : QChar::DirL;
                    it.setPosition(niPos);
                    while (*it != pos) {
                        if (analysis[*it].bidiDirection != QChar::DirBN)
                            analysis[*it].bidiDirection = dir;
                        ++it;
                    }
                    niPos.clear();
                }
                lastStrong = currentStrong;
                break;

            case QChar::DirBN:
            case QChar::DirS:
            case QChar::DirWS:
            case QChar::DirON:
            case QChar::DirFSI:
            case QChar::DirLRI:
            case QChar::DirRLI:
            case QChar::DirPDI:
            case QChar::DirB:
                if (!niPos.isValid())
                    niPos = it.position();
                break;

            default:
                Q_UNREACHABLE();
            }
            if (it.atEnd())
                break;
//                last = current;
            ++it;
        }
    }

    void resolveImplicitLevelsForIsolatedRun(const Vector<DirectionalRun> &runs, int i)
    {
        // Rule X10
        int level = runs.at(i).level;
        int before = i - 1;
        while (before >= 0 && !runs.at(before).hasContent)
            --before;
        int level_before = (before >= 0) ? runs.at(before).level : baseLevel;
        int after = i;
        while (runs.at(after).continuation >= 0)
            after = runs.at(after).continuation;
        if (runs.at(after).continuation == -2) {
            after = runs.size();
        } else {
            ++after;
            while (after < runs.size() && !runs.at(after).hasContent)
                ++after;
        }
        int level_after = (after == runs.size()) ? baseLevel : runs.at(after).level;
        QChar::Direction sos = (qMax(level_before, level) & 1) ? QChar::DirR : QChar::DirL;
        QChar::Direction eos = (qMax(level_after, level) & 1) ? QChar::DirR : QChar::DirL;

        if (BidiDebugEnabled) {
            BIDI_DEBUG() << "Isolated run starting at" << i << "sos/eos" << sos << eos;
            BIDI_DEBUG() << "before implicit level processing:";
            IsolatedRunSequenceIterator it(runs, i);
            while (!it.atEnd()) {
                BIDI_DEBUG() << "    " << *it << hex << text[*it].unicode() << analysis[*it].bidiDirection;
                ++it;
            }
        }

        resolveW1W2W3(runs, i, sos);
        resolveW4(runs, i, sos);
        resolveW5(runs, i);

        if (BidiDebugEnabled) {
            BIDI_DEBUG() << "after W4/W5";
            IsolatedRunSequenceIterator it(runs, i);
            while (!it.atEnd()) {
                BIDI_DEBUG() << "    " << *it << hex << text[*it].unicode() << analysis[*it].bidiDirection;
                ++it;
            }
        }

        resolveW6W7(runs, i, sos);

        // Resolve neutral types

        // Rule N0
        resolveN0(runs, i, sos);
        resolveN1N2(runs, i, sos, eos);

        BIDI_DEBUG() << "setting levels (run at" << level << ")";
        // Rules I1 & I2: set correct levels
        {
            ushort level = runs.at(i).level;
            IsolatedRunSequenceIterator it(runs, i);
            while (!it.atEnd()) {
                int pos = *it;

                QChar::Direction current = analysis[pos].bidiDirection;
                switch (current) {
                case QChar::DirBN:
                    break;
                case QChar::DirL:
                    analysis[pos].bidiLevel = (level + 1) & ~1;
                    break;
                case QChar::DirR:
                    analysis[pos].bidiLevel = level | 1;
                    break;
                case QChar::DirAN:
                case QChar::DirEN:
                    analysis[pos].bidiLevel = (level + 2) & ~1;
                    break;
                default:
                    Q_UNREACHABLE();
                }
                BIDI_DEBUG() << "    " << pos << current << analysis[pos].bidiLevel;
                ++it;
            }
        }
    }

    void resolveImplicitLevels(const Vector<DirectionalRun> &runs)
    {
        for (int i = 0; i < runs.size(); ++i) {
            if (runs.at(i).isContinuation)
                continue;

            resolveImplicitLevelsForIsolatedRun(runs, i);
        }
    }

    bool checkForBidi() const
    {
        if (baseLevel != 0)
            return true;
        for (int i = 0; i < length; ++i) {
            if (text[i].unicode() >= 0x590) {
                switch (text[i].direction()) {
                case QChar::DirR: case QChar::DirAN:
                case QChar::DirLRE: case QChar::DirLRO: case QChar::DirAL:
                case QChar::DirRLE: case QChar::DirRLO: case QChar::DirPDF:
                case QChar::DirLRI: case QChar::DirRLI: case QChar::DirFSI: case QChar::DirPDI:
                    return true;
                default:
                    break;
                }
            }
        }
        return false;
    }

    bool process()
    {
        memset(analysis, 0, length * sizeof(QScriptAnalysis));

        bool hasBidi = checkForBidi();

        if (!hasBidi)
            return false;

        if (BidiDebugEnabled) {
            BIDI_DEBUG() << ">>>> start bidi, text length" << length;
            for (int i = 0; i < length; ++i)
                BIDI_DEBUG() << hex << "    (" << i << ")" << text[i].unicode() << text[i].direction();
        }

        {
            Vector<DirectionalRun> runs;
            resolveExplicitLevels(runs);

            if (BidiDebugEnabled) {
                BIDI_DEBUG() << "resolved explicit levels, nruns" << runs.size();
                for (int i = 0; i < runs.size(); ++i)
                    BIDI_DEBUG() << "    " << i << "start/end" << runs.at(i).start << runs.at(i).end << "level" << (int)runs.at(i).level << "continuation" << runs.at(i).continuation;
            }

            // now we have a list of isolated run sequences inside the vector of runs, that can be fed
            // through the implicit level resolving

            resolveImplicitLevels(runs);
        }

        // set directions for BN to the minimum of adjacent chars
        // This makes is possible to be conformant with the Bidi algorithm even though we don't
        // remove BN and explicit embedding chars from the stream of characters to reorder
        int lastLevel = baseLevel;
        int lastBNPos = -1;
        for (int i = 0; i < length; ++i) {
            if (analysis[i].bidiFlags & QScriptAnalysis::BidiBN) {
                if (lastBNPos < 0)
                    lastBNPos = i;
                analysis[i].bidiLevel = lastLevel;
            } else {
                int l = analysis[i].bidiLevel;
                if (lastBNPos >= 0) {
                    if (l < lastLevel) {
                        while (lastBNPos < i) {
                            analysis[lastBNPos].bidiLevel = l;
                            ++lastBNPos;
                        }
                    }
                    lastBNPos = -1;
                }
                lastLevel = l;
            }
        }
        if (lastBNPos >= 0 && baseLevel < lastLevel) {
            while (lastBNPos < length) {
                analysis[lastBNPos].bidiLevel = baseLevel;
                ++lastBNPos;
            }
        }

        BIDI_DEBUG() << "Rule L1:";
        // Rule L1:
        bool resetLevel = true;
        for (int i = length - 1; i >= 0; --i) {
            if (analysis[i].bidiFlags & QScriptAnalysis::BidiResetToParagraphLevel) {
                BIDI_DEBUG() << "resetting pos" << i << "to baselevel";
                analysis[i].bidiLevel = baseLevel;
                resetLevel = true;
            } else if (resetLevel && analysis[i].bidiFlags & QScriptAnalysis::BidiMaybeResetToParagraphLevel) {
                BIDI_DEBUG() << "resetting pos" << i << "to baselevel (maybereset flag)";
                analysis[i].bidiLevel = baseLevel;
            } else {
                resetLevel = false;
            }
        }

        if (BidiDebugEnabled) {
            BIDI_DEBUG() << "final resolved levels:";
            for (int i = 0; i < length; ++i)
                BIDI_DEBUG() << "    " << i << hex << text[i].unicode() << dec << (int)analysis[i].bidiLevel;
        }

        return true;
    }


    const QChar *text;
    QScriptAnalysis *analysis;
    int length;
    char baseLevel;
};

} // namespace

void QTextEngine::bidiReorder(int numItems, const quint8 *levels, int *visualOrder)
{

    // first find highest and lowest levels
    quint8 levelLow = 128;
    quint8 levelHigh = 0;
    int i = 0;
    while (i < numItems) {
        //printf("level = %d\n", r->level);
        if (levels[i] > levelHigh)
            levelHigh = levels[i];
        if (levels[i] < levelLow)
            levelLow = levels[i];
        i++;
    }

    // implements reordering of the line (L2 according to BiDi spec):
    // L2. From the highest level found in the text to the lowest odd level on each line,
    // reverse any contiguous sequence of characters that are at that level or higher.

    // reversing is only done up to the lowest odd level
    if(!(levelLow%2)) levelLow++;

    BIDI_DEBUG() << "reorderLine: lineLow = " << (uint)levelLow << ", lineHigh = " << (uint)levelHigh;

    int count = numItems - 1;
    for (i = 0; i < numItems; i++)
        visualOrder[i] = i;

    while(levelHigh >= levelLow) {
        int i = 0;
        while (i < count) {
            while(i < count && levels[i] < levelHigh) i++;
            int start = i;
            while(i <= count && levels[i] >= levelHigh) i++;
            int end = i-1;

            if(start != end) {
                //qDebug() << "reversing from " << start << " to " << end;
                for(int j = 0; j < (end-start+1)/2; j++) {
                    int tmp = visualOrder[start+j];
                    visualOrder[start+j] = visualOrder[end-j];
                    visualOrder[end-j] = tmp;
                }
            }
            i++;
        }
        levelHigh--;
    }

//     BIDI_DEBUG("visual order is:");
//     for (i = 0; i < numItems; i++)
//         BIDI_DEBUG() << visualOrder[i];
}


enum JustificationClass {
    Justification_Prohibited      = 0,   // Justification can not be applied after this glyph
    Justification_Arabic_Space    = 1,   // This glyph represents a space inside arabic text
    Justification_Character       = 2,   // Inter-character justification point follows this glyph
    Justification_Space           = 4,   // This glyph represents a blank outside an Arabic run
    Justification_Arabic_Normal   = 7,   // Normal Middle-Of-Word glyph that connects to the right (begin)
    Justification_Arabic_Waw      = 8,   // Next character is final form of Waw/Ain/Qaf/Feh
    Justification_Arabic_BaRa     = 9,   // Next two characters are Ba + Ra/Ya/AlefMaksura
    Justification_Arabic_Alef     = 10,  // Next character is final form of Alef/Tah/Lam/Kaf/Gaf
    Justification_Arabic_HahDal   = 11,  // Next character is final form of Hah/Dal/Teh Marbuta
    Justification_Arabic_Seen     = 12,  // Initial or medial form of Seen/Sad
    Justification_Arabic_Kashida  = 13   // User-inserted Kashida(U+0640)
};

#if QT_CONFIG(harfbuzz)

/*
    Adds an inter character justification opportunity after the number or letter
    character and a space justification opportunity after the space character.
*/
static inline void qt_getDefaultJustificationOpportunities(const ushort *string, int length, const QGlyphLayout &g, ushort *log_clusters, int spaceAs)
{
    int str_pos = 0;
    while (str_pos < length) {
        int glyph_pos = log_clusters[str_pos];

        Q_ASSERT(glyph_pos < g.numGlyphs && g.attributes[glyph_pos].clusterStart);

        uint ucs4 = string[str_pos];
        if (QChar::isHighSurrogate(ucs4) && str_pos + 1 < length) {
            ushort low = string[str_pos + 1];
            if (QChar::isLowSurrogate(low)) {
                ++str_pos;
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
            }
        }

        // skip whole cluster
        do {
            ++str_pos;
        } while (str_pos < length && log_clusters[str_pos] == glyph_pos);
        do {
            ++glyph_pos;
        } while (glyph_pos < g.numGlyphs && !g.attributes[glyph_pos].clusterStart);
        --glyph_pos;

        // justification opportunity at the end of cluster
        if (Q_LIKELY(QChar::isLetterOrNumber(ucs4)))
            g.attributes[glyph_pos].justification = Justification_Character;
        else if (Q_LIKELY(QChar::isSpace(ucs4)))
            g.attributes[glyph_pos].justification = spaceAs;
    }
}

static inline void qt_getJustificationOpportunities(const ushort *string, int length, const QScriptItem &si, const QGlyphLayout &g, ushort *log_clusters)
{
    Q_ASSERT(length > 0 && g.numGlyphs > 0);

    for (int glyph_pos = 0; glyph_pos < g.numGlyphs; ++glyph_pos)
        g.attributes[glyph_pos].justification = Justification_Prohibited;

    int spaceAs;

    switch (si.analysis.script) {
    case QChar::Script_Arabic:
    case QChar::Script_Syriac:
    case QChar::Script_Nko:
    case QChar::Script_Mandaic:
    case QChar::Script_Mongolian:
    case QChar::Script_PhagsPa:
    case QChar::Script_Manichaean:
    case QChar::Script_PsalterPahlavi:
        // same as default but inter character justification takes precedence
        spaceAs = Justification_Arabic_Space;
        break;

    case QChar::Script_Tibetan:
    case QChar::Script_Hiragana:
    case QChar::Script_Katakana:
    case QChar::Script_Bopomofo:
    case QChar::Script_Han:
        // same as default but inter character justification is the only option
        spaceAs = Justification_Character;
        break;

    default:
        spaceAs = Justification_Space;
        break;
    }

    qt_getDefaultJustificationOpportunities(string, length, g, log_clusters, spaceAs);
}

#endif // harfbuzz


// shape all the items that intersect with the line, taking tab widths into account to find out what text actually fits in the line.
void QTextEngine::shapeLine(const QScriptLine &line)
{
    QFixed x;
    bool first = true;
    int item = findItem(line.from);
    if (item == -1)
        return;

    const int end = findItem(line.from + line.length + line.trailingSpaces - 1, item);
    for ( ; item <= end; ++item) {
        QScriptItem &si = layoutData->items[item];
        if (si.analysis.flags == QScriptAnalysis::Tab) {
            ensureSpace(1);
            si.width = calculateTabWidth(item, x);
        } else {
            shape(item);
        }
        if (first && si.position != line.from) { // that means our x position has to be offset
            QGlyphLayout glyphs = shapedGlyphs(&si);
            Q_ASSERT(line.from > si.position);
            for (int i = line.from - si.position - 1; i >= 0; i--) {
                x -= glyphs.effectiveAdvance(i);
            }
        }
        first = false;

        x += si.width;
    }
}

#if QT_CONFIG(harfbuzz)
extern bool qt_useHarfbuzzNG(); // defined in qfontengine.cpp
#endif

void QTextEngine::shapeText(int item) const
{
    Q_ASSERT(item < layoutData->items.size());
    QScriptItem &si = layoutData->items[item];

    if (si.num_glyphs)
        return;

    si.width = 0;
    si.glyph_data_offset = layoutData->used;

    const ushort *string = reinterpret_cast<const ushort *>(layoutData->string.constData()) + si.position;
    const int itemLength = length(item);

    QString casedString;
    if (si.analysis.flags && si.analysis.flags <= QScriptAnalysis::SmallCaps) {
        casedString.resize(itemLength);
        ushort *uc = reinterpret_cast<ushort *>(casedString.data());
        for (int i = 0; i < itemLength; ++i) {
            uint ucs4 = string[i];
            if (QChar::isHighSurrogate(ucs4) && i + 1 < itemLength) {
                uint low = string[i + 1];
                if (QChar::isLowSurrogate(low)) {
                    ++i;
                    ucs4 = QChar::surrogateToUcs4(ucs4, low);
                    ucs4 = si.analysis.flags == QScriptAnalysis::Lowercase ? QChar::toLower(ucs4)
                                                                           : QChar::toUpper(ucs4);
                    // high part never changes in simple casing
                    uc[i] = QChar::lowSurrogate(ucs4);
                }
            } else {
                uc[i] = si.analysis.flags == QScriptAnalysis::Lowercase ? QChar::toLower(ucs4)
                                                                        : QChar::toUpper(ucs4);
            }
        }
        string = reinterpret_cast<const ushort *>(casedString.constData());
    }

    if (Q_UNLIKELY(!ensureSpace(itemLength))) {
        Q_UNREACHABLE(); // ### report OOM error somehow
        return;
    }

    QFontEngine *fontEngine = this->fontEngine(si, &si.ascent, &si.descent, &si.leading);

    bool kerningEnabled;
    bool letterSpacingIsAbsolute;
    bool shapingEnabled;
    QFixed letterSpacing, wordSpacing;
#ifndef QT_NO_RAWFONT
    if (useRawFont) {
        QTextCharFormat f = format(&si);
        kerningEnabled = f.fontKerning();
        shapingEnabled = QFontEngine::scriptRequiresOpenType(QChar::Script(si.analysis.script))
                || (f.fontStyleStrategy() & QFont::PreferNoShaping) == 0;
        wordSpacing = QFixed::fromReal(f.fontWordSpacing());
        letterSpacing = QFixed::fromReal(f.fontLetterSpacing());
        letterSpacingIsAbsolute = true;
    } else
#endif
    {
        QFont font = this->font(si);
        kerningEnabled = font.d->kerning;
        shapingEnabled = QFontEngine::scriptRequiresOpenType(QChar::Script(si.analysis.script))
                || (font.d->request.styleStrategy & QFont::PreferNoShaping) == 0;
        letterSpacingIsAbsolute = font.d->letterSpacingIsAbsolute;
        letterSpacing = font.d->letterSpacing;
        wordSpacing = font.d->wordSpacing;

        if (letterSpacingIsAbsolute && letterSpacing.value())
            letterSpacing *= font.d->dpi / qt_defaultDpiY();
    }

    // split up the item into parts that come from different font engines
    // k * 3 entries, array[k] == index in string, array[k + 1] == index in glyphs, array[k + 2] == engine index
    QVector<uint> itemBoundaries;
    itemBoundaries.reserve(24);

    QGlyphLayout initialGlyphs = availableGlyphs(&si);
    int nGlyphs = initialGlyphs.numGlyphs;
    if (fontEngine->type() == QFontEngine::Multi || !shapingEnabled) {
        // ask the font engine to find out which glyphs (as an index in the specific font)
        // to use for the text in one item.
        QFontEngine::ShaperFlags shaperFlags =
                shapingEnabled
                    ? QFontEngine::GlyphIndicesOnly
                    : QFontEngine::ShaperFlag(0);
        if (!fontEngine->stringToCMap(reinterpret_cast<const QChar *>(string), itemLength, &initialGlyphs, &nGlyphs, shaperFlags))
            Q_UNREACHABLE();
    }

    if (fontEngine->type() == QFontEngine::Multi) {
        uint lastEngine = ~0u;
        for (int i = 0, glyph_pos = 0; i < itemLength; ++i, ++glyph_pos) {
            const uint engineIdx = initialGlyphs.glyphs[glyph_pos] >> 24;
            if (lastEngine != engineIdx) {
                itemBoundaries.append(i);
                itemBoundaries.append(glyph_pos);
                itemBoundaries.append(engineIdx);

                if (engineIdx != 0) {
                    QFontEngine *actualFontEngine = static_cast<QFontEngineMulti *>(fontEngine)->engine(engineIdx);
                    si.ascent = qMax(actualFontEngine->ascent(), si.ascent);
                    si.descent = qMax(actualFontEngine->descent(), si.descent);
                    si.leading = qMax(actualFontEngine->leading(), si.leading);
                }

                lastEngine = engineIdx;
            }

            if (QChar::isHighSurrogate(string[i]) && i + 1 < itemLength && QChar::isLowSurrogate(string[i + 1]))
                ++i;
        }
    } else {
        itemBoundaries.append(0);
        itemBoundaries.append(0);
        itemBoundaries.append(0);
    }

    if (Q_UNLIKELY(!shapingEnabled)) {
        ushort *log_clusters = logClusters(&si);

        int glyph_pos = 0;
        for (int i = 0; i < itemLength; ++i, ++glyph_pos) {
            log_clusters[i] = glyph_pos;
            initialGlyphs.attributes[glyph_pos].clusterStart = true;
            if (QChar::isHighSurrogate(string[i])
                    && i + 1 < itemLength
                    && QChar::isLowSurrogate(string[i + 1])) {
                ++i;
                log_clusters[i] = glyph_pos;
            }
        }

        si.num_glyphs = glyph_pos;
#if QT_CONFIG(harfbuzz)
    } else if (Q_LIKELY(qt_useHarfbuzzNG())) {
        si.num_glyphs = shapeTextWithHarfbuzzNG(si, string, itemLength, fontEngine, itemBoundaries, kerningEnabled, letterSpacing != 0);
#endif
    } else {
        si.num_glyphs = shapeTextWithHarfbuzz(si, string, itemLength, fontEngine, itemBoundaries, kerningEnabled);
    }
    if (Q_UNLIKELY(si.num_glyphs == 0)) {
        Q_UNREACHABLE(); // ### report shaping errors somehow
        return;
    }


    layoutData->used += si.num_glyphs;

    QGlyphLayout glyphs = shapedGlyphs(&si);

#if QT_CONFIG(harfbuzz)
    if (Q_LIKELY(qt_useHarfbuzzNG()))
        qt_getJustificationOpportunities(string, itemLength, si, glyphs, logClusters(&si));
#endif

    if (letterSpacing != 0) {
        for (int i = 1; i < si.num_glyphs; ++i) {
            if (glyphs.attributes[i].clusterStart) {
                if (letterSpacingIsAbsolute)
                    glyphs.advances[i - 1] += letterSpacing;
                else {
                    QFixed &advance = glyphs.advances[i - 1];
                    advance += (letterSpacing - 100) * advance / 100;
                }
            }
        }
        if (letterSpacingIsAbsolute)
            glyphs.advances[si.num_glyphs - 1] += letterSpacing;
        else {
            QFixed &advance = glyphs.advances[si.num_glyphs - 1];
            advance += (letterSpacing - 100) * advance / 100;
        }
    }
    if (wordSpacing != 0) {
        for (int i = 0; i < si.num_glyphs; ++i) {
            if (glyphs.attributes[i].justification == Justification_Space
                || glyphs.attributes[i].justification == Justification_Arabic_Space) {
                // word spacing only gets added once to a consecutive run of spaces (see CSS spec)
                if (i + 1 == si.num_glyphs
                    ||(glyphs.attributes[i+1].justification != Justification_Space
                       && glyphs.attributes[i+1].justification != Justification_Arabic_Space))
                    glyphs.advances[i] += wordSpacing;
            }
        }
    }

    for (int i = 0; i < si.num_glyphs; ++i)
        si.width += glyphs.advances[i] * !glyphs.attributes[i].dontPrint;
}

#if QT_CONFIG(harfbuzz)

QT_BEGIN_INCLUDE_NAMESPACE

#include "qharfbuzzng_p.h"

QT_END_INCLUDE_NAMESPACE

int QTextEngine::shapeTextWithHarfbuzzNG(const QScriptItem &si,
                                         const ushort *string,
                                         int itemLength,
                                         QFontEngine *fontEngine,
                                         const QVector<uint> &itemBoundaries,
                                         bool kerningEnabled,
                                         bool hasLetterSpacing) const
{
    uint glyphs_shaped = 0;

    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buffer, hb_qt_get_unicode_funcs());
    hb_buffer_pre_allocate(buffer, itemLength);
    if (Q_UNLIKELY(!hb_buffer_allocation_successful(buffer))) {
        hb_buffer_destroy(buffer);
        return 0;
    }

    hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
    props.direction = si.analysis.bidiLevel % 2 ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
    QChar::Script script = QChar::Script(si.analysis.script);
    props.script = hb_qt_script_to_script(script);
    // ### props.language = hb_language_get_default_for_script(props.script);

    for (int k = 0; k < itemBoundaries.size(); k += 3) {
        const uint item_pos = itemBoundaries[k];
        const uint item_length = (k + 4 < itemBoundaries.size() ? itemBoundaries[k + 3] : itemLength) - item_pos;
        const uint engineIdx = itemBoundaries[k + 2];

        QFontEngine *actualFontEngine = fontEngine->type() != QFontEngine::Multi ? fontEngine
                                                                                 : static_cast<QFontEngineMulti *>(fontEngine)->engine(engineIdx);


        // prepare buffer
        hb_buffer_clear_contents(buffer);
        hb_buffer_add_utf16(buffer, reinterpret_cast<const uint16_t *>(string) + item_pos, item_length, 0, item_length);

#if defined(Q_OS_DARWIN)
        // ### temporary workaround for QTBUG-38113
        // CoreText throws away the PDF token, while the OpenType backend will replace it with
        // a zero-advance glyph. This becomes a real issue when PDF is the last character,
        // since it gets treated like if it were a grapheme extender, so we
        // temporarily replace it with some visible grapheme starter.
        bool endsWithPDF = actualFontEngine->type() == QFontEngine::Mac && string[item_pos + item_length - 1] == 0x202c;
        if (Q_UNLIKELY(endsWithPDF)) {
            uint num_glyphs;
            hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &num_glyphs);
            infos[num_glyphs - 1].codepoint = '.';
        }
#endif

        hb_buffer_set_segment_properties(buffer, &props);
        hb_buffer_guess_segment_properties(buffer);

        uint buffer_flags = HB_BUFFER_FLAG_DEFAULT;
        // Symbol encoding used to encode various crap in the 32..255 character code range,
        // and thus might override U+00AD [SHY]; avoid hiding default ignorables
        if (Q_UNLIKELY(actualFontEngine->symbol))
            buffer_flags |= HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES;
        hb_buffer_set_flags(buffer, hb_buffer_flags_t(buffer_flags));


        // shape
        {
            hb_font_t *hb_font = hb_qt_font_get_for_engine(actualFontEngine);
            Q_ASSERT(hb_font);
            hb_qt_font_set_use_design_metrics(hb_font, option.useDesignMetrics() ? uint(QFontEngine::DesignMetrics) : 0); // ###

            // Ligatures are incompatible with custom letter spacing, so when a letter spacing is set,
            // we disable them for writing systems where they are purely cosmetic.
            bool scriptRequiresOpenType = ((script >= QChar::Script_Syriac && script <= QChar::Script_Sinhala)
                                         || script == QChar::Script_Khmer || script == QChar::Script_Nko);

            bool dontLigate = hasLetterSpacing && !scriptRequiresOpenType;
            const hb_feature_t features[5] = {
                { HB_TAG('k','e','r','n'), !!kerningEnabled, 0, uint(-1) },
                { HB_TAG('l','i','g','a'), !dontLigate, 0, uint(-1) },
                { HB_TAG('c','l','i','g'), !dontLigate, 0, uint(-1) },
                { HB_TAG('d','l','i','g'), !dontLigate, 0, uint(-1) },
                { HB_TAG('h','l','i','g'), !dontLigate, 0, uint(-1) } };
            const int num_features = dontLigate ? 5 : 1;

            const char *const *shaper_list = nullptr;
#if defined(Q_OS_DARWIN)
            // What's behind QFontEngine::FaceData::user_data isn't compatible between different font engines
            // - specifically functions in hb-coretext.cc would run into undefined behavior with data
            // from non-CoreText engine. The other shapers works with that engine just fine.
            if (actualFontEngine->type() != QFontEngine::Mac) {
                static const char *s_shaper_list_without_coretext[] = {
                    "graphite2",
                    "ot",
                    "fallback",
                    nullptr
                };
                shaper_list = s_shaper_list_without_coretext;
            }
#endif

            bool shapedOk = hb_shape_full(hb_font, buffer, features, num_features, shaper_list);
            if (Q_UNLIKELY(!shapedOk)) {
                hb_buffer_destroy(buffer);
                return 0;
            }

            if (Q_UNLIKELY(HB_DIRECTION_IS_BACKWARD(props.direction)))
                hb_buffer_reverse(buffer);
        }

        const uint num_glyphs = hb_buffer_get_length(buffer);
        // ensure we have enough space for shaped glyphs and metrics
        if (Q_UNLIKELY(num_glyphs == 0 || !ensureSpace(glyphs_shaped + num_glyphs))) {
            hb_buffer_destroy(buffer);
            return 0;
        }

        // fetch the shaped glyphs and metrics
        QGlyphLayout g = availableGlyphs(&si).mid(glyphs_shaped, num_glyphs);
        ushort *log_clusters = logClusters(&si) + item_pos;

        hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, 0);
        hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, 0);
        uint str_pos = 0;
        uint last_cluster = ~0u;
        uint last_glyph_pos = glyphs_shaped;
        for (uint i = 0; i < num_glyphs; ++i, ++infos, ++positions) {
            g.glyphs[i] = infos->codepoint;

            g.advances[i] = QFixed::fromFixed(positions->x_advance);
            g.offsets[i].x = QFixed::fromFixed(positions->x_offset);
            g.offsets[i].y = QFixed::fromFixed(positions->y_offset);

            uint cluster = infos->cluster;
            if (Q_LIKELY(last_cluster != cluster)) {
                g.attributes[i].clusterStart = true;

                // fix up clusters so that the cluster indices will be monotonic
                // and thus we never return out-of-order indices
                while (last_cluster++ < cluster && str_pos < item_length)
                    log_clusters[str_pos++] = last_glyph_pos;
                last_glyph_pos = i + glyphs_shaped;
                last_cluster = cluster;

                // hide characters that should normally be invisible
                switch (string[item_pos + str_pos]) {
                case QChar::LineFeed:
                case 0x000c: // FormFeed
                case QChar::CarriageReturn:
                case QChar::LineSeparator:
                case QChar::ParagraphSeparator:
                    g.attributes[i].dontPrint = true;
                    break;
                case QChar::SoftHyphen:
                    if (!actualFontEngine->symbol) {
                        // U+00AD [SOFT HYPHEN] is a default ignorable codepoint,
                        // so we replace its glyph and metrics with ones for
                        // U+002D [HYPHEN-MINUS] and make it visible if it appears at line-break
                        g.glyphs[i] = actualFontEngine->glyphIndex('-');
                        if (Q_LIKELY(g.glyphs[i] != 0)) {
                            QGlyphLayout tmp = g.mid(i, 1);
                            actualFontEngine->recalcAdvances(&tmp, 0);
                        }
                        g.attributes[i].dontPrint = true;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        while (str_pos < item_length)
            log_clusters[str_pos++] = last_glyph_pos;

#if defined(Q_OS_DARWIN)
        if (Q_UNLIKELY(endsWithPDF)) {
            int last_glyph_idx = num_glyphs - 1;
            g.glyphs[last_glyph_idx] = 0xffff;
            g.advances[last_glyph_idx] = QFixed();
            g.offsets[last_glyph_idx].x = QFixed();
            g.offsets[last_glyph_idx].y = QFixed();
            g.attributes[last_glyph_idx].clusterStart = true;
            g.attributes[last_glyph_idx].dontPrint = true;

            log_clusters[item_length - 1] = glyphs_shaped + last_glyph_idx;
        }
#endif

        if (Q_UNLIKELY(engineIdx != 0)) {
            for (quint32 i = 0; i < num_glyphs; ++i)
                g.glyphs[i] |= (engineIdx << 24);
        }

#ifdef Q_OS_DARWIN
        if (actualFontEngine->type() == QFontEngine::Mac) {
            if (actualFontEngine->fontDef.stretch != 100) {
                QFixed stretch = QFixed(int(actualFontEngine->fontDef.stretch)) / QFixed(100);
                for (uint i = 0; i < num_glyphs; ++i)
                    g.advances[i] *= stretch;
            }
        }
#endif

        if (!actualFontEngine->supportsSubPixelPositions() || (actualFontEngine->fontDef.styleStrategy & QFont::ForceIntegerMetrics)) {
            for (uint i = 0; i < num_glyphs; ++i)
                g.advances[i] = g.advances[i].round();
        }

        glyphs_shaped += num_glyphs;
    }

    hb_buffer_destroy(buffer);

    return glyphs_shaped;
}

#endif // harfbuzz


QT_BEGIN_INCLUDE_NAMESPACE

#include <private/qharfbuzz_p.h>

QT_END_INCLUDE_NAMESPACE

Q_STATIC_ASSERT(sizeof(HB_Glyph) == sizeof(glyph_t));
Q_STATIC_ASSERT(sizeof(HB_Fixed) == sizeof(QFixed));
Q_STATIC_ASSERT(sizeof(HB_FixedPoint) == sizeof(QFixedPoint));

static inline void moveGlyphData(const QGlyphLayout &destination, const QGlyphLayout &source, int num)
{
    if (num > 0 && destination.glyphs != source.glyphs)
        memmove(destination.glyphs, source.glyphs, num * sizeof(glyph_t));
}

int QTextEngine::shapeTextWithHarfbuzz(const QScriptItem &si, const ushort *string, int itemLength, QFontEngine *fontEngine, const QVector<uint> &itemBoundaries, bool kerningEnabled) const
{
    HB_ShaperItem entire_shaper_item;
    memset(&entire_shaper_item, 0, sizeof(entire_shaper_item));
    entire_shaper_item.string = reinterpret_cast<const HB_UChar16 *>(string);
    entire_shaper_item.stringLength = itemLength;
    entire_shaper_item.item.script = script_to_hbscript(si.analysis.script);
    entire_shaper_item.item.pos = 0;
    entire_shaper_item.item.length = itemLength;
    entire_shaper_item.item.bidiLevel = si.analysis.bidiLevel;

    entire_shaper_item.shaperFlags = 0;
    if (!kerningEnabled)
        entire_shaper_item.shaperFlags |= HB_ShaperFlag_NoKerning;
    if (option.useDesignMetrics())
        entire_shaper_item.shaperFlags |= HB_ShaperFlag_UseDesignMetrics;

    // ensure we are not asserting in HB_HeuristicSetGlyphAttributes()
    entire_shaper_item.num_glyphs = 0;
    for (int i = 0; i < itemLength; ++i, ++entire_shaper_item.num_glyphs) {
        if (QChar::isHighSurrogate(string[i]) && i + 1 < itemLength && QChar::isLowSurrogate(string[i + 1]))
            ++i;
    }


    int remaining_glyphs = entire_shaper_item.num_glyphs;
    int glyph_pos = 0;
    // for each item shape using harfbuzz and store the results in our layoutData's glyphs array.
    for (int k = 0; k < itemBoundaries.size(); k += 3) {
        HB_ShaperItem shaper_item = entire_shaper_item;
        shaper_item.item.pos = itemBoundaries[k];
        if (k + 4 < itemBoundaries.size()) {
            shaper_item.item.length = itemBoundaries[k + 3] - shaper_item.item.pos;
            shaper_item.num_glyphs = itemBoundaries[k + 4] - itemBoundaries[k + 1];
        } else { // last combo in the list, avoid out of bounds access.
            shaper_item.item.length -= shaper_item.item.pos - entire_shaper_item.item.pos;
            shaper_item.num_glyphs -= itemBoundaries[k + 1];
        }
        shaper_item.initialGlyphCount = shaper_item.num_glyphs;
        if (shaper_item.num_glyphs < shaper_item.item.length)
            shaper_item.num_glyphs = shaper_item.item.length;

        uint engineIdx = itemBoundaries[k + 2];
        QFontEngine *actualFontEngine = fontEngine;
        if (fontEngine->type() == QFontEngine::Multi) {
            actualFontEngine = static_cast<QFontEngineMulti *>(fontEngine)->engine(engineIdx);

            if ((si.analysis.bidiLevel % 2) == 0)
                shaper_item.glyphIndicesPresent = true;
        }

        shaper_item.font = (HB_Font)actualFontEngine->harfbuzzFont();
        shaper_item.face = (HB_Face)actualFontEngine->harfbuzzFace();

        remaining_glyphs -= shaper_item.initialGlyphCount;

        QVarLengthArray<HB_GlyphAttributes, 128> hbGlyphAttributes;
        do {
            if (!ensureSpace(glyph_pos + shaper_item.num_glyphs + remaining_glyphs))
                return 0;
            if (hbGlyphAttributes.size() < int(shaper_item.num_glyphs)) {
                hbGlyphAttributes.resize(shaper_item.num_glyphs);
                memset(hbGlyphAttributes.data(), 0, hbGlyphAttributes.size() * sizeof(HB_GlyphAttributes));
            }

            const QGlyphLayout g = availableGlyphs(&si).mid(glyph_pos);
            if (fontEngine->type() == QFontEngine::Multi && shaper_item.num_glyphs > shaper_item.item.length)
                moveGlyphData(g.mid(shaper_item.num_glyphs), g.mid(shaper_item.initialGlyphCount), remaining_glyphs);

            shaper_item.glyphs = reinterpret_cast<HB_Glyph *>(g.glyphs);
            shaper_item.advances = reinterpret_cast<HB_Fixed *>(g.advances);
            shaper_item.offsets = reinterpret_cast<HB_FixedPoint *>(g.offsets);
            shaper_item.attributes = hbGlyphAttributes.data();

            if (engineIdx != 0 && shaper_item.glyphIndicesPresent) {
                for (quint32 i = 0; i < shaper_item.initialGlyphCount; ++i)
                    shaper_item.glyphs[i] &= 0x00ffffff;
            }

            shaper_item.log_clusters = logClusters(&si) + shaper_item.item.pos - entire_shaper_item.item.pos;
        } while (!qShapeItem(&shaper_item)); // this does the actual shaping via harfbuzz.

        QGlyphLayout g = availableGlyphs(&si).mid(glyph_pos, shaper_item.num_glyphs);
        if (fontEngine->type() == QFontEngine::Multi)
            moveGlyphData(g.mid(shaper_item.num_glyphs), g.mid(shaper_item.initialGlyphCount), remaining_glyphs);

        for (quint32 i = 0; i < shaper_item.num_glyphs; ++i) {
            HB_GlyphAttributes hbAttrs = hbGlyphAttributes.at(i);
            QGlyphAttributes &attrs = g.attributes[i];
            attrs.clusterStart = hbAttrs.clusterStart;
            attrs.dontPrint = hbAttrs.dontPrint;
            attrs.justification = hbAttrs.justification;
        }

        for (quint32 i = 0; i < shaper_item.item.length; ++i) {
            // Workaround wrong log_clusters for surrogates (i.e. QTBUG-39875)
            if (shaper_item.log_clusters[i] >= shaper_item.num_glyphs)
                shaper_item.log_clusters[i] = shaper_item.num_glyphs - 1;
            shaper_item.log_clusters[i] += glyph_pos;
        }

        if (kerningEnabled && !shaper_item.kerning_applied)
            actualFontEngine->doKerning(&g, option.useDesignMetrics() ? QFontEngine::DesignMetrics : QFontEngine::ShaperFlags(0));

        if (engineIdx != 0) {
            for (quint32 i = 0; i < shaper_item.num_glyphs; ++i)
                g.glyphs[i] |= (engineIdx << 24);
        }

        glyph_pos += shaper_item.num_glyphs;
    }

    return glyph_pos;
}

void QTextEngine::init(QTextEngine *e)
{
    e->ignoreBidi = false;
    e->cacheGlyphs = false;
    e->forceJustification = false;
    e->visualMovement = false;
    e->delayDecorations = false;

    e->layoutData = 0;

    e->minWidth = 0;
    e->maxWidth = 0;

    e->specialData = 0;
    e->stackEngine = false;
#ifndef QT_NO_RAWFONT
    e->useRawFont = false;
#endif
}

QTextEngine::QTextEngine()
{
    init(this);
}

QTextEngine::QTextEngine(const QString &str, const QFont &f)
    : text(str),
      fnt(f)
{
    init(this);
}

QTextEngine::~QTextEngine()
{
    if (!stackEngine)
        delete layoutData;
    delete specialData;
    resetFontEngineCache();
}

const QCharAttributes *QTextEngine::attributes() const
{
    if (layoutData && layoutData->haveCharAttributes)
        return (QCharAttributes *) layoutData->memory;

    itemize();
    if (! ensureSpace(layoutData->string.length()))
        return NULL;

    QVarLengthArray<QUnicodeTools::ScriptItem> scriptItems(layoutData->items.size());
    for (int i = 0; i < layoutData->items.size(); ++i) {
        const QScriptItem &si = layoutData->items[i];
        scriptItems[i].position = si.position;
        scriptItems[i].script = si.analysis.script;
    }

    QUnicodeTools::initCharAttributes(reinterpret_cast<const ushort *>(layoutData->string.constData()),
                                      layoutData->string.length(),
                                      scriptItems.data(), scriptItems.size(),
                                      (QCharAttributes *)layoutData->memory);


    layoutData->haveCharAttributes = true;
    return (QCharAttributes *) layoutData->memory;
}

void QTextEngine::shape(int item) const
{
    if (layoutData->items[item].analysis.flags == QScriptAnalysis::Object) {
        ensureSpace(1);
        if (block.docHandle()) {
            docLayout()->resizeInlineObject(QTextInlineObject(item, const_cast<QTextEngine *>(this)),
                                            layoutData->items[item].position + block.position(),
                                            format(&layoutData->items[item]));
        }
    } else if (layoutData->items[item].analysis.flags == QScriptAnalysis::Tab) {
        // set up at least the ascent/descent/leading of the script item for the tab
        fontEngine(layoutData->items[item],
                   &layoutData->items[item].ascent,
                   &layoutData->items[item].descent,
                   &layoutData->items[item].leading);
    } else {
        shapeText(item);
    }
}

static inline void releaseCachedFontEngine(QFontEngine *fontEngine)
{
    if (fontEngine && !fontEngine->ref.deref())
        delete fontEngine;
}

void QTextEngine::resetFontEngineCache()
{
    releaseCachedFontEngine(feCache.prevFontEngine);
    releaseCachedFontEngine(feCache.prevScaledFontEngine);
    feCache.reset();
}

void QTextEngine::invalidate()
{
    freeMemory();
    minWidth = 0;
    maxWidth = 0;

    resetFontEngineCache();
}

void QTextEngine::clearLineData()
{
    lines.clear();
}

void QTextEngine::validate() const
{
    if (layoutData)
        return;
    layoutData = new LayoutData();
    if (block.docHandle()) {
        layoutData->string = block.text();
        const bool nextBlockValid = block.next().isValid();
        if (!nextBlockValid && option.flags() & QTextOption::ShowDocumentTerminator) {
            layoutData->string += QChar(0xA7);
        } else if (option.flags() & QTextOption::ShowLineAndParagraphSeparators) {
            layoutData->string += QLatin1Char(nextBlockValid ? 0xb6 : 0x20);
        }

    } else {
        layoutData->string = text;
    }
    if (specialData && specialData->preeditPosition != -1)
        layoutData->string.insert(specialData->preeditPosition, specialData->preeditText);
}

void QTextEngine::itemize() const
{
    validate();
    if (layoutData->items.size())
        return;

    int length = layoutData->string.length();
    if (!length)
        return;

    const ushort *string = reinterpret_cast<const ushort *>(layoutData->string.unicode());

    bool rtl = isRightToLeft();

    QVarLengthArray<QScriptAnalysis, 4096> scriptAnalysis(length);
    QScriptAnalysis *analysis = scriptAnalysis.data();

    QBidiAlgorithm bidi(layoutData->string.constData(), analysis, length, rtl);
    layoutData->hasBidi = bidi.process();

    {
        QVarLengthArray<uchar> scripts(length);
        QUnicodeTools::initScripts(string, length, scripts.data());
        for (int i = 0; i < length; ++i)
            analysis[i].script = scripts.at(i);
    }

    const ushort *uc = string;
    const ushort *e = uc + length;
    while (uc < e) {
        switch (*uc) {
        case QChar::ObjectReplacementCharacter:
            analysis->flags = QScriptAnalysis::Object;
            break;
        case QChar::LineSeparator:
            if (analysis->bidiLevel % 2)
                --analysis->bidiLevel;
            analysis->flags = QScriptAnalysis::LineOrParagraphSeparator;
            if (option.flags() & QTextOption::ShowLineAndParagraphSeparators) {
                const int offset = uc - string;
                layoutData->string.detach();
                string = reinterpret_cast<const ushort *>(layoutData->string.unicode());
                uc = string + offset;
                e = string + length;
                *const_cast<ushort*>(uc) = 0x21B5; // visual line separator
            }
            break;
        case QChar::Tabulation:
            analysis->flags = QScriptAnalysis::Tab;
            analysis->bidiLevel = bidi.baseLevel;
            break;
        case QChar::Space:
        case QChar::Nbsp:
            if (option.flags() & QTextOption::ShowTabsAndSpaces) {
                analysis->flags = QScriptAnalysis::Space;
                break;
            }
            Q_FALLTHROUGH();
        default:
            analysis->flags = QScriptAnalysis::None;
            break;
        }
#if !QT_CONFIG(harfbuzz)
        analysis->script = hbscript_to_script(script_to_hbscript(analysis->script));
#endif
        ++uc;
        ++analysis;
    }
    if (option.flags() & QTextOption::ShowLineAndParagraphSeparators) {
        (analysis-1)->flags = QScriptAnalysis::LineOrParagraphSeparator; // to exclude it from width
    }
#if QT_CONFIG(harfbuzz)
    analysis = scriptAnalysis.data();
    if (qt_useHarfbuzzNG()) {
        // ### pretend HB-old behavior for now
        for (int i = 0; i < length; ++i) {
            switch (analysis[i].script) {
            case QChar::Script_Latin:
            case QChar::Script_Hiragana:
            case QChar::Script_Katakana:
            case QChar::Script_Bopomofo:
            case QChar::Script_Han:
                analysis[i].script = QChar::Script_Common;
                break;
            default:
                break;
            }
        }
    } else {
        for (int i = 0; i < length; ++i)
            analysis[i].script = hbscript_to_script(script_to_hbscript(analysis[i].script));
    }
#endif

    Itemizer itemizer(layoutData->string, scriptAnalysis.data(), layoutData->items);

    const QTextDocumentPrivate *p = block.docHandle();
    if (p) {
        SpecialData *s = specialData;

        QTextDocumentPrivate::FragmentIterator it = p->find(block.position());
        QTextDocumentPrivate::FragmentIterator end = p->find(block.position() + block.length() - 1); // -1 to omit the block separator char
        int format = it.value()->format;

        int prevPosition = 0;
        int position = prevPosition;
        while (1) {
            const QTextFragmentData * const frag = it.value();
            if (it == end || format != frag->format) {
                if (s && position >= s->preeditPosition) {
                    position += s->preeditText.length();
                    s = 0;
                }
                Q_ASSERT(position <= length);
                QFont::Capitalization capitalization =
                        formatCollection()->charFormat(format).hasProperty(QTextFormat::FontCapitalization)
                        ? formatCollection()->charFormat(format).fontCapitalization()
                        : formatCollection()->defaultFont().capitalization();
                itemizer.generate(prevPosition, position - prevPosition, capitalization);
                if (it == end) {
                    if (position < length)
                        itemizer.generate(position, length - position, capitalization);
                    break;
                }
                format = frag->format;
                prevPosition = position;
            }
            position += frag->size_array[0];
            ++it;
        }
    } else {
#ifndef QT_NO_RAWFONT
        if (useRawFont && specialData) {
            int lastIndex = 0;
            for (int i = 0; i < specialData->formats.size(); ++i) {
                const QTextLayout::FormatRange &range = specialData->formats.at(i);
                const QTextCharFormat &format = range.format;
                if (format.hasProperty(QTextFormat::FontCapitalization)) {
                    itemizer.generate(lastIndex, range.start - lastIndex, QFont::MixedCase);
                    itemizer.generate(range.start, range.length, format.fontCapitalization());
                    lastIndex = range.start + range.length;
                }
            }
            itemizer.generate(lastIndex, length - lastIndex, QFont::MixedCase);
        } else
#endif
            itemizer.generate(0, length, static_cast<QFont::Capitalization> (fnt.d->capital));
    }

    addRequiredBoundaries();
    resolveFormats();
}

bool QTextEngine::isRightToLeft() const
{
    switch (option.textDirection()) {
    case Qt::LeftToRight:
        return false;
    case Qt::RightToLeft:
        return true;
    default:
        break;
    }
    if (!layoutData)
        itemize();
    // this places the cursor in the right position depending on the keyboard layout
    if (layoutData->string.isEmpty())
        return QGuiApplication::inputMethod()->inputDirection() == Qt::RightToLeft;
    return layoutData->string.isRightToLeft();
}


int QTextEngine::findItem(int strPos, int firstItem) const
{
    itemize();
    if (strPos < 0 || strPos >= layoutData->string.size() || firstItem < 0)
        return -1;

    int left = firstItem + 1;
    int right = layoutData->items.size()-1;
    while(left <= right) {
        int middle = ((right-left)/2)+left;
        if (strPos > layoutData->items[middle].position)
            left = middle+1;
        else if(strPos < layoutData->items[middle].position)
            right = middle-1;
        else {
            return middle;
        }
    }
    return right;
}

QFixed QTextEngine::width(int from, int len) const
{
    itemize();

    QFixed w = 0;

//     qDebug("QTextEngine::width(from = %d, len = %d), numItems=%d, strleng=%d", from,  len, items.size(), string.length());
    for (int i = 0; i < layoutData->items.size(); i++) {
        const QScriptItem *si = layoutData->items.constData() + i;
        int pos = si->position;
        int ilen = length(i);
//          qDebug("item %d: from %d len %d", i, pos, ilen);
        if (pos >= from + len)
            break;
        if (pos + ilen > from) {
            if (!si->num_glyphs)
                shape(i);

            if (si->analysis.flags == QScriptAnalysis::Object) {
                w += si->width;
                continue;
            } else if (si->analysis.flags == QScriptAnalysis::Tab) {
                w += calculateTabWidth(i, w);
                continue;
            }


            QGlyphLayout glyphs = shapedGlyphs(si);
            unsigned short *logClusters = this->logClusters(si);

//             fprintf(stderr, "  logclusters:");
//             for (int k = 0; k < ilen; k++)
//                 fprintf(stderr, " %d", logClusters[k]);
//             fprintf(stderr, "\n");
            // do the simple thing for now and give the first glyph in a cluster the full width, all other ones 0.
            int charFrom = from - pos;
            if (charFrom < 0)
                charFrom = 0;
            int glyphStart = logClusters[charFrom];
            if (charFrom > 0 && logClusters[charFrom-1] == glyphStart)
                while (charFrom < ilen && logClusters[charFrom] == glyphStart)
                    charFrom++;
            if (charFrom < ilen) {
                glyphStart = logClusters[charFrom];
                int charEnd = from + len - 1 - pos;
                if (charEnd >= ilen)
                    charEnd = ilen-1;
                int glyphEnd = logClusters[charEnd];
                while (charEnd < ilen && logClusters[charEnd] == glyphEnd)
                    charEnd++;
                glyphEnd = (charEnd == ilen) ? si->num_glyphs : logClusters[charEnd];

//                 qDebug("char: start=%d end=%d / glyph: start = %d, end = %d", charFrom, charEnd, glyphStart, glyphEnd);
                for (int i = glyphStart; i < glyphEnd; i++)
                    w += glyphs.advances[i] * !glyphs.attributes[i].dontPrint;
            }
        }
    }
//     qDebug("   --> w= %d ", w);
    return w;
}

glyph_metrics_t QTextEngine::boundingBox(int from,  int len) const
{
    itemize();

    glyph_metrics_t gm;

    for (int i = 0; i < layoutData->items.size(); i++) {
        const QScriptItem *si = layoutData->items.constData() + i;

        int pos = si->position;
        int ilen = length(i);
        if (pos > from + len)
            break;
        if (pos + ilen > from) {
            if (!si->num_glyphs)
                shape(i);

            if (si->analysis.flags == QScriptAnalysis::Object) {
                gm.width += si->width;
                continue;
            } else if (si->analysis.flags == QScriptAnalysis::Tab) {
                gm.width += calculateTabWidth(i, gm.width);
                continue;
            }

            unsigned short *logClusters = this->logClusters(si);
            QGlyphLayout glyphs = shapedGlyphs(si);

            // do the simple thing for now and give the first glyph in a cluster the full width, all other ones 0.
            int charFrom = from - pos;
            if (charFrom < 0)
                charFrom = 0;
            int glyphStart = logClusters[charFrom];
            if (charFrom > 0 && logClusters[charFrom-1] == glyphStart)
                while (charFrom < ilen && logClusters[charFrom] == glyphStart)
                    charFrom++;
            if (charFrom < ilen) {
                QFontEngine *fe = fontEngine(*si);
                glyphStart = logClusters[charFrom];
                int charEnd = from + len - 1 - pos;
                if (charEnd >= ilen)
                    charEnd = ilen-1;
                int glyphEnd = logClusters[charEnd];
                while (charEnd < ilen && logClusters[charEnd] == glyphEnd)
                    charEnd++;
                glyphEnd = (charEnd == ilen) ? si->num_glyphs : logClusters[charEnd];
                if (glyphStart <= glyphEnd ) {
                    glyph_metrics_t m = fe->boundingBox(glyphs.mid(glyphStart, glyphEnd - glyphStart));
                    gm.x = qMin(gm.x, m.x + gm.xoff);
                    gm.y = qMin(gm.y, m.y + gm.yoff);
                    gm.width = qMax(gm.width, m.width+gm.xoff);
                    gm.height = qMax(gm.height, m.height+gm.yoff);
                    gm.xoff += m.xoff;
                    gm.yoff += m.yoff;
                }
            }
        }
    }
    return gm;
}

glyph_metrics_t QTextEngine::tightBoundingBox(int from,  int len) const
{
    itemize();

    glyph_metrics_t gm;

    for (int i = 0; i < layoutData->items.size(); i++) {
        const QScriptItem *si = layoutData->items.constData() + i;
        int pos = si->position;
        int ilen = length(i);
        if (pos > from + len)
            break;
        if (pos + len > from) {
            if (!si->num_glyphs)
                shape(i);
            unsigned short *logClusters = this->logClusters(si);
            QGlyphLayout glyphs = shapedGlyphs(si);

            // do the simple thing for now and give the first glyph in a cluster the full width, all other ones 0.
            int charFrom = from - pos;
            if (charFrom < 0)
                charFrom = 0;
            int glyphStart = logClusters[charFrom];
            if (charFrom > 0 && logClusters[charFrom-1] == glyphStart)
                while (charFrom < ilen && logClusters[charFrom] == glyphStart)
                    charFrom++;
            if (charFrom < ilen) {
                glyphStart = logClusters[charFrom];
                int charEnd = from + len - 1 - pos;
                if (charEnd >= ilen)
                    charEnd = ilen-1;
                int glyphEnd = logClusters[charEnd];
                while (charEnd < ilen && logClusters[charEnd] == glyphEnd)
                    charEnd++;
                glyphEnd = (charEnd == ilen) ? si->num_glyphs : logClusters[charEnd];
                if (glyphStart <= glyphEnd ) {
                    QFontEngine *fe = fontEngine(*si);
                    glyph_metrics_t m = fe->tightBoundingBox(glyphs.mid(glyphStart, glyphEnd - glyphStart));
                    gm.x = qMin(gm.x, m.x + gm.xoff);
                    gm.y = qMin(gm.y, m.y + gm.yoff);
                    gm.width = qMax(gm.width, m.width+gm.xoff);
                    gm.height = qMax(gm.height, m.height+gm.yoff);
                    gm.xoff += m.xoff;
                    gm.yoff += m.yoff;
                }
            }
        }
    }
    return gm;
}

QFont QTextEngine::font(const QScriptItem &si) const
{
    QFont font = fnt;
    if (hasFormats()) {
        QTextCharFormat f = format(&si);
        font = f.font();

        if (block.docHandle() && block.docHandle()->layout()) {
            // Make sure we get the right dpi on printers
            QPaintDevice *pdev = block.docHandle()->layout()->paintDevice();
            if (pdev)
                font = QFont(font, pdev);
        } else {
            font = font.resolve(fnt);
        }
        QTextCharFormat::VerticalAlignment valign = f.verticalAlignment();
        if (valign == QTextCharFormat::AlignSuperScript || valign == QTextCharFormat::AlignSubScript) {
            if (font.pointSize() != -1)
                font.setPointSize((font.pointSize() * 2) / 3);
            else
                font.setPixelSize((font.pixelSize() * 2) / 3);
        }
    }

    if (si.analysis.flags == QScriptAnalysis::SmallCaps)
        font = font.d->smallCapsFont();

    return font;
}

QTextEngine::FontEngineCache::FontEngineCache()
{
    reset();
}

//we cache the previous results of this function, as calling it numerous times with the same effective
//input is common (and hard to cache at a higher level)
QFontEngine *QTextEngine::fontEngine(const QScriptItem &si, QFixed *ascent, QFixed *descent, QFixed *leading) const
{
    QFontEngine *engine = 0;
    QFontEngine *scaledEngine = 0;
    int script = si.analysis.script;

    QFont font = fnt;
#ifndef QT_NO_RAWFONT
    if (useRawFont && rawFont.isValid()) {
        if (feCache.prevFontEngine && feCache.prevFontEngine->type() == QFontEngine::Multi && feCache.prevScript == script) {
            engine = feCache.prevFontEngine;
        } else {
            engine = QFontEngineMulti::createMultiFontEngine(rawFont.d->fontEngine, script);
            feCache.prevFontEngine = engine;
            feCache.prevScript = script;
            engine->ref.ref();
            if (feCache.prevScaledFontEngine) {
                releaseCachedFontEngine(feCache.prevScaledFontEngine);
                feCache.prevScaledFontEngine = 0;
            }
        }
        if (si.analysis.flags == QScriptAnalysis::SmallCaps) {
            if (feCache.prevScaledFontEngine) {
                scaledEngine = feCache.prevScaledFontEngine;
            } else {
                QFontEngine *scEngine = rawFont.d->fontEngine->cloneWithSize(smallCapsFraction * rawFont.pixelSize());
                scEngine->ref.ref();
                scaledEngine = QFontEngineMulti::createMultiFontEngine(scEngine, script);
                scaledEngine->ref.ref();
                feCache.prevScaledFontEngine = scaledEngine;
                // If scEngine is not ref'ed by scaledEngine, make sure it is deallocated and not leaked.
                if (!scEngine->ref.deref())
                    delete scEngine;

            }
        }
    } else
#endif
    {
        if (hasFormats()) {
            if (feCache.prevFontEngine && feCache.prevPosition == si.position && feCache.prevLength == length(&si) && feCache.prevScript == script) {
                engine = feCache.prevFontEngine;
                scaledEngine = feCache.prevScaledFontEngine;
            } else {
                QTextCharFormat f = format(&si);
                font = f.font();

                if (block.docHandle() && block.docHandle()->layout()) {
                    // Make sure we get the right dpi on printers
                    QPaintDevice *pdev = block.docHandle()->layout()->paintDevice();
                    if (pdev)
                        font = QFont(font, pdev);
                } else {
                    font = font.resolve(fnt);
                }
                engine = font.d->engineForScript(script);
                if (engine)
                    engine->ref.ref();

                QTextCharFormat::VerticalAlignment valign = f.verticalAlignment();
                if (valign == QTextCharFormat::AlignSuperScript || valign == QTextCharFormat::AlignSubScript) {
                    if (font.pointSize() != -1)
                        font.setPointSize((font.pointSize() * 2) / 3);
                    else
                        font.setPixelSize((font.pixelSize() * 2) / 3);
                    scaledEngine = font.d->engineForScript(script);
                    if (scaledEngine)
                        scaledEngine->ref.ref();
                }

                if (feCache.prevFontEngine)
                    releaseCachedFontEngine(feCache.prevFontEngine);
                feCache.prevFontEngine = engine;

                if (feCache.prevScaledFontEngine)
                    releaseCachedFontEngine(feCache.prevScaledFontEngine);
                feCache.prevScaledFontEngine = scaledEngine;

                feCache.prevScript = script;
                feCache.prevPosition = si.position;
                feCache.prevLength = length(&si);
            }
        } else {
            if (feCache.prevFontEngine && feCache.prevScript == script && feCache.prevPosition == -1)
                engine = feCache.prevFontEngine;
            else {
                engine = font.d->engineForScript(script);

                if (engine)
                    engine->ref.ref();
                if (feCache.prevFontEngine)
                    releaseCachedFontEngine(feCache.prevFontEngine);
                feCache.prevFontEngine = engine;

                feCache.prevScript = script;
                feCache.prevPosition = -1;
                feCache.prevLength = -1;
                feCache.prevScaledFontEngine = 0;
            }
        }

        if (si.analysis.flags == QScriptAnalysis::SmallCaps) {
            QFontPrivate *p = font.d->smallCapsFontPrivate();
            scaledEngine = p->engineForScript(script);
        }
    }

    if (ascent) {
        *ascent = engine->ascent();
        *descent = engine->descent();
        *leading = engine->leading();
    }

    if (scaledEngine)
        return scaledEngine;
    return engine;
}

struct QJustificationPoint {
    int type;
    QFixed kashidaWidth;
    QGlyphLayout glyph;
};

Q_DECLARE_TYPEINFO(QJustificationPoint, Q_PRIMITIVE_TYPE);

static void set(QJustificationPoint *point, int type, const QGlyphLayout &glyph, QFontEngine *fe)
{
    point->type = type;
    point->glyph = glyph;

    if (type >= Justification_Arabic_Normal) {
        QChar ch(0x640); // Kashida character

        glyph_t kashidaGlyph = fe->glyphIndex(ch.unicode());
        if (kashidaGlyph != 0) {
            QGlyphLayout g;
            g.numGlyphs = 1;
            g.glyphs = &kashidaGlyph;
            g.advances = &point->kashidaWidth;
            fe->recalcAdvances(&g, 0);

            if (point->kashidaWidth == 0)
                point->type = Justification_Prohibited;
        } else {
            point->type = Justification_Prohibited;
            point->kashidaWidth = 0;
        }
    }
}


void QTextEngine::justify(const QScriptLine &line)
{
//     qDebug("justify: line.gridfitted = %d, line.justified=%d", line.gridfitted, line.justified);
    if (line.gridfitted && line.justified)
        return;

    if (!line.gridfitted) {
        // redo layout in device metrics, then adjust
        const_cast<QScriptLine &>(line).gridfitted = true;
    }

    if ((option.alignment() & Qt::AlignHorizontal_Mask) != Qt::AlignJustify)
        return;

    itemize();

    if (!forceJustification) {
        int end = line.from + (int)line.length + line.trailingSpaces;
        if (end == layoutData->string.length())
            return; // no justification at end of paragraph
        if (end && layoutData->items[findItem(end-1)].analysis.flags == QScriptAnalysis::LineOrParagraphSeparator)
            return; // no justification at the end of an explicitly separated line
    }

    // justify line
    int maxJustify = 0;

    // don't include trailing white spaces when doing justification
    int line_length = line.length;
    const QCharAttributes *a = attributes();
    if (! a)
        return;
    a += line.from;
    while (line_length && a[line_length-1].whiteSpace)
        --line_length;
    // subtract one char more, as we can't justfy after the last character
    --line_length;

    if (line_length <= 0)
        return;

    int firstItem = findItem(line.from);
    int lastItem = findItem(line.from + line_length - 1, firstItem);
    int nItems = (firstItem >= 0 && lastItem >= firstItem)? (lastItem-firstItem+1) : 0;

    QVarLengthArray<QJustificationPoint> justificationPoints;
    int nPoints = 0;
//     qDebug("justifying from %d len %d, firstItem=%d, nItems=%d (%s)", line.from, line_length, firstItem, nItems, layoutData->string.mid(line.from, line_length).toUtf8().constData());
    QFixed minKashida = 0x100000;

    // we need to do all shaping before we go into the next loop, as we there
    // store pointers to the glyph data that could get reallocated by the shaping
    // process.
    for (int i = 0; i < nItems; ++i) {
        QScriptItem &si = layoutData->items[firstItem + i];
        if (!si.num_glyphs)
            shape(firstItem + i);
    }

    for (int i = 0; i < nItems; ++i) {
        QScriptItem &si = layoutData->items[firstItem + i];

        int kashida_type = Justification_Arabic_Normal;
        int kashida_pos = -1;

        int start = qMax(line.from - si.position, 0);
        int end = qMin(line.from + line_length - (int)si.position, length(firstItem+i));

        unsigned short *log_clusters = logClusters(&si);

        int gs = log_clusters[start];
        int ge = (end == length(firstItem+i) ? si.num_glyphs : log_clusters[end]);

        Q_ASSERT(ge <= si.num_glyphs);

        const QGlyphLayout g = shapedGlyphs(&si);

        for (int i = gs; i < ge; ++i) {
            g.justifications[i].type = QGlyphJustification::JustifyNone;
            g.justifications[i].nKashidas = 0;
            g.justifications[i].space_18d6 = 0;

            justificationPoints.resize(nPoints+3);
            int justification = g.attributes[i].justification;

            switch(justification) {
            case Justification_Prohibited:
                break;
            case Justification_Space:
            case Justification_Arabic_Space:
                if (kashida_pos >= 0) {
//                     qDebug("kashida position at %d in word", kashida_pos);
                    set(&justificationPoints[nPoints], kashida_type, g.mid(kashida_pos), fontEngine(si));
                    if (justificationPoints[nPoints].kashidaWidth > 0) {
                        minKashida = qMin(minKashida, justificationPoints[nPoints].kashidaWidth);
                        maxJustify = qMax(maxJustify, justificationPoints[nPoints].type);
                        ++nPoints;
                    }
                }
                kashida_pos = -1;
                kashida_type = Justification_Arabic_Normal;
                Q_FALLTHROUGH();
            case Justification_Character:
                set(&justificationPoints[nPoints++], justification, g.mid(i), fontEngine(si));
                maxJustify = qMax(maxJustify, justification);
                break;
            case Justification_Arabic_Normal:
            case Justification_Arabic_Waw:
            case Justification_Arabic_BaRa:
            case Justification_Arabic_Alef:
            case Justification_Arabic_HahDal:
            case Justification_Arabic_Seen:
            case Justification_Arabic_Kashida:
                if (justification >= kashida_type) {
                    kashida_pos = i;
                    kashida_type = justification;
                }
            }
        }
        if (kashida_pos >= 0) {
            set(&justificationPoints[nPoints], kashida_type, g.mid(kashida_pos), fontEngine(si));
            if (justificationPoints[nPoints].kashidaWidth > 0) {
                minKashida = qMin(minKashida, justificationPoints[nPoints].kashidaWidth);
                maxJustify = qMax(maxJustify, justificationPoints[nPoints].type);
                ++nPoints;
            }
        }
    }

    QFixed leading = leadingSpaceWidth(line);
    QFixed need = line.width - line.textWidth - leading;
    if (need < 0) {
        // line overflows already!
        const_cast<QScriptLine &>(line).justified = true;
        return;
    }

//     qDebug("doing justification: textWidth=%x, requested=%x, maxJustify=%d", line.textWidth.value(), line.width.value(), maxJustify);
//     qDebug("     minKashida=%f, need=%f", minKashida.toReal(), need.toReal());

    // distribute in priority order
    if (maxJustify >= Justification_Arabic_Normal) {
        while (need >= minKashida) {
            for (int type = maxJustify; need >= minKashida && type >= Justification_Arabic_Normal; --type) {
                for (int i = 0; need >= minKashida && i < nPoints; ++i) {
                    if (justificationPoints[i].type == type && justificationPoints[i].kashidaWidth <= need) {
                        justificationPoints[i].glyph.justifications->nKashidas++;
                        // ############
                        justificationPoints[i].glyph.justifications->space_18d6 += justificationPoints[i].kashidaWidth.value();
                        need -= justificationPoints[i].kashidaWidth;
//                         qDebug("adding kashida type %d with width %x, neednow %x", type, justificationPoints[i].kashidaWidth, need.value());
                    }
                }
            }
        }
    }
    Q_ASSERT(need >= 0);
    if (!need)
        goto end;

    maxJustify = qMin(maxJustify, int(Justification_Space));
    for (int type = maxJustify; need != 0 && type > 0; --type) {
        int n = 0;
        for (int i = 0; i < nPoints; ++i) {
            if (justificationPoints[i].type == type)
                ++n;
        }
//          qDebug("number of points for justification type %d: %d", type, n);


        if (!n)
            continue;

        for (int i = 0; i < nPoints; ++i) {
            if (justificationPoints[i].type == type) {
                QFixed add = need/n;
//                  qDebug("adding %x to glyph %x", add.value(), justificationPoints[i].glyph->glyph);
                justificationPoints[i].glyph.justifications[0].space_18d6 = add.value();
                need -= add;
                --n;
            }
        }

        Q_ASSERT(!need);
    }
 end:
    const_cast<QScriptLine &>(line).justified = true;
}

void QScriptLine::setDefaultHeight(QTextEngine *eng)
{
    QFont f;
    QFontEngine *e;

    if (eng->block.docHandle() && eng->block.docHandle()->layout()) {
        f = eng->block.charFormat().font();
        // Make sure we get the right dpi on printers
        QPaintDevice *pdev = eng->block.docHandle()->layout()->paintDevice();
        if (pdev)
            f = QFont(f, pdev);
        e = f.d->engineForScript(QChar::Script_Common);
    } else {
        e = eng->fnt.d->engineForScript(QChar::Script_Common);
    }

    QFixed other_ascent = e->ascent();
    QFixed other_descent = e->descent();
    QFixed other_leading = e->leading();
    leading = qMax(leading + ascent, other_leading + other_ascent) - qMax(ascent, other_ascent);
    ascent = qMax(ascent, other_ascent);
    descent = qMax(descent, other_descent);
}

QTextEngine::LayoutData::LayoutData()
{
    memory = 0;
    allocated = 0;
    memory_on_stack = false;
    used = 0;
    hasBidi = false;
    layoutState = LayoutEmpty;
    haveCharAttributes = false;
    logClustersPtr = 0;
    available_glyphs = 0;
}

QTextEngine::LayoutData::LayoutData(const QString &str, void **stack_memory, int _allocated)
    : string(str)
{
    allocated = _allocated;

    int space_charAttributes = sizeof(QCharAttributes)*string.length()/sizeof(void*) + 1;
    int space_logClusters = sizeof(unsigned short)*string.length()/sizeof(void*) + 1;
    available_glyphs = ((int)allocated - space_charAttributes - space_logClusters)*(int)sizeof(void*)/(int)QGlyphLayout::SpaceNeeded;

    if (available_glyphs < str.length()) {
        // need to allocate on the heap
        allocated = 0;

        memory_on_stack = false;
        memory = 0;
        logClustersPtr = 0;
    } else {
        memory_on_stack = true;
        memory = stack_memory;
        logClustersPtr = (unsigned short *)(memory + space_charAttributes);

        void *m = memory + space_charAttributes + space_logClusters;
        glyphLayout = QGlyphLayout(reinterpret_cast<char *>(m), str.length());
        glyphLayout.clear();
        memset(memory, 0, space_charAttributes*sizeof(void *));
    }
    used = 0;
    hasBidi = false;
    layoutState = LayoutEmpty;
    haveCharAttributes = false;
}

QTextEngine::LayoutData::~LayoutData()
{
    if (!memory_on_stack)
        free(memory);
    memory = 0;
}

bool QTextEngine::LayoutData::reallocate(int totalGlyphs)
{
    Q_ASSERT(totalGlyphs >= glyphLayout.numGlyphs);
    if (memory_on_stack && available_glyphs >= totalGlyphs) {
        glyphLayout.grow(glyphLayout.data(), totalGlyphs);
        return true;
    }

    int space_charAttributes = sizeof(QCharAttributes)*string.length()/sizeof(void*) + 1;
    int space_logClusters = sizeof(unsigned short)*string.length()/sizeof(void*) + 1;
    int space_glyphs = (totalGlyphs * QGlyphLayout::SpaceNeeded) / sizeof(void *) + 2;

    int newAllocated = space_charAttributes + space_glyphs + space_logClusters;
    // These values can be negative if the length of string/glyphs causes overflow,
    // we can't layout such a long string all at once, so return false here to
    // indicate there is a failure
    if (space_charAttributes < 0 || space_logClusters < 0 || space_glyphs < 0 || newAllocated < allocated) {
        layoutState = LayoutFailed;
        return false;
    }

    void **newMem = (void **)::realloc(memory_on_stack ? 0 : memory, newAllocated*sizeof(void *));
    if (!newMem) {
        layoutState = LayoutFailed;
        return false;
    }
    if (memory_on_stack)
        memcpy(newMem, memory, allocated*sizeof(void *));
    memory = newMem;
    memory_on_stack = false;

    void **m = memory;
    m += space_charAttributes;
    logClustersPtr = (unsigned short *) m;
    m += space_logClusters;

    const int space_preGlyphLayout = space_charAttributes + space_logClusters;
    if (allocated < space_preGlyphLayout)
        memset(memory + allocated, 0, (space_preGlyphLayout - allocated)*sizeof(void *));

    glyphLayout.grow(reinterpret_cast<char *>(m), totalGlyphs);

    allocated = newAllocated;
    return true;
}

// grow to the new size, copying the existing data to the new layout
void QGlyphLayout::grow(char *address, int totalGlyphs)
{
    QGlyphLayout oldLayout(address, numGlyphs);
    QGlyphLayout newLayout(address, totalGlyphs);

    if (numGlyphs) {
        // move the existing data
        memmove(newLayout.attributes, oldLayout.attributes, numGlyphs * sizeof(QGlyphAttributes));
        memmove(newLayout.justifications, oldLayout.justifications, numGlyphs * sizeof(QGlyphJustification));
        memmove(newLayout.advances, oldLayout.advances, numGlyphs * sizeof(QFixed));
        memmove(newLayout.glyphs, oldLayout.glyphs, numGlyphs * sizeof(glyph_t));
    }

    // clear the new data
    newLayout.clear(numGlyphs);

    *this = newLayout;
}

void QTextEngine::freeMemory()
{
    if (!stackEngine) {
        delete layoutData;
        layoutData = 0;
    } else {
        layoutData->used = 0;
        layoutData->hasBidi = false;
        layoutData->layoutState = LayoutEmpty;
        layoutData->haveCharAttributes = false;
        layoutData->items.clear();
    }
    if (specialData)
        specialData->resolvedFormats.clear();
    for (int i = 0; i < lines.size(); ++i) {
        lines[i].justified = 0;
        lines[i].gridfitted = 0;
    }
}

int QTextEngine::formatIndex(const QScriptItem *si) const
{
    if (specialData && !specialData->resolvedFormats.isEmpty()) {
        QTextFormatCollection *collection = formatCollection();
        Q_ASSERT(collection);
        return collection->indexForFormat(specialData->resolvedFormats.at(si - &layoutData->items[0]));
    }

    QTextDocumentPrivate *p = block.docHandle();
    if (!p)
        return -1;
    int pos = si->position;
    if (specialData && si->position >= specialData->preeditPosition) {
        if (si->position < specialData->preeditPosition + specialData->preeditText.length())
            pos = qMax(qMin(block.length(), specialData->preeditPosition) - 1, 0);
        else
            pos -= specialData->preeditText.length();
    }
    QTextDocumentPrivate::FragmentIterator it = p->find(block.position() + pos);
    return it.value()->format;
}


QTextCharFormat QTextEngine::format(const QScriptItem *si) const
{
    if (const QTextFormatCollection *collection = formatCollection())
        return collection->charFormat(formatIndex(si));
    return QTextCharFormat();
}

void QTextEngine::addRequiredBoundaries() const
{
    if (specialData) {
        for (int i = 0; i < specialData->formats.size(); ++i) {
            const QTextLayout::FormatRange &r = specialData->formats.at(i);
            setBoundary(r.start);
            setBoundary(r.start + r.length);
            //qDebug("adding boundaries %d %d", r.start, r.start+r.length);
        }
    }
}

bool QTextEngine::atWordSeparator(int position) const
{
    const QChar c = layoutData->string.at(position);
    switch (c.unicode()) {
    case '.':
    case ',':
    case '?':
    case '!':
    case '@':
    case '#':
    case '$':
    case ':':
    case ';':
    case '-':
    case '<':
    case '>':
    case '[':
    case ']':
    case '(':
    case ')':
    case '{':
    case '}':
    case '=':
    case '/':
    case '+':
    case '%':
    case '&':
    case '^':
    case '*':
    case '\'':
    case '"':
    case '`':
    case '~':
    case '|':
    case '\\':
        return true;
    default:
        break;
    }
    return false;
}

void QTextEngine::setPreeditArea(int position, const QString &preeditText)
{
    if (preeditText.isEmpty()) {
        if (!specialData)
            return;
        if (specialData->formats.isEmpty()) {
            delete specialData;
            specialData = 0;
        } else {
            specialData->preeditText = QString();
            specialData->preeditPosition = -1;
        }
    } else {
        if (!specialData)
            specialData = new SpecialData;
        specialData->preeditPosition = position;
        specialData->preeditText = preeditText;
    }
    invalidate();
    clearLineData();
}

void QTextEngine::setFormats(const QVector<QTextLayout::FormatRange> &formats)
{
    if (formats.isEmpty()) {
        if (!specialData)
            return;
        if (specialData->preeditText.isEmpty()) {
            delete specialData;
            specialData = 0;
        } else {
            specialData->formats.clear();
        }
    } else {
        if (!specialData) {
            specialData = new SpecialData;
            specialData->preeditPosition = -1;
        }
        specialData->formats = formats;
        indexFormats();
    }
    invalidate();
    clearLineData();
}

void QTextEngine::indexFormats()
{
    QTextFormatCollection *collection = formatCollection();
    if (!collection) {
        Q_ASSERT(!block.docHandle());
        specialData->formatCollection.reset(new QTextFormatCollection);
        collection = specialData->formatCollection.data();
    }

    // replace with shared copies
    for (int i = 0; i < specialData->formats.size(); ++i) {
        QTextCharFormat &format = specialData->formats[i].format;
        format = collection->charFormat(collection->indexForFormat(format));
    }
}

/* These two helper functions are used to determine whether we need to insert a ZWJ character
   between the text that gets truncated and the ellipsis. This is important to get
   correctly shaped results for arabic text.
*/
static inline bool nextCharJoins(const QString &string, int pos)
{
    while (pos < string.length() && string.at(pos).category() == QChar::Mark_NonSpacing)
        ++pos;
    if (pos == string.length())
        return false;
    QChar::JoiningType joining = string.at(pos).joiningType();
    return joining != QChar::Joining_None && joining != QChar::Joining_Transparent;
}

static inline bool prevCharJoins(const QString &string, int pos)
{
    while (pos > 0 && string.at(pos - 1).category() == QChar::Mark_NonSpacing)
        --pos;
    if (pos == 0)
        return false;
    QChar::JoiningType joining = string.at(pos - 1).joiningType();
    return joining == QChar::Joining_Dual || joining == QChar::Joining_Causing;
}

static inline bool isRetainableControlCode(QChar c)
{
    return (c.unicode() >= 0x202a && c.unicode() <= 0x202e) // LRE, RLE, PDF, LRO, RLO
            || (c.unicode() >= 0x200e && c.unicode() <= 0x200f) // LRM, RLM
            || (c.unicode() >= 0x2066 && c.unicode() <= 0x2069); // LRI, RLI, FSI, PDI
}

static QString stringMidRetainingBidiCC(const QString &string,
                                        const QString &ellidePrefix,
                                        const QString &ellideSuffix,
                                        int subStringFrom,
                                        int subStringTo,
                                        int midStart,
                                        int midLength)
{
    QString prefix;
    for (int i=subStringFrom; i<midStart; ++i) {
        QChar c = string.at(i);
        if (isRetainableControlCode(c))
            prefix += c;
    }

    QString suffix;
    for (int i=midStart + midLength; i<subStringTo; ++i) {
        QChar c = string.at(i);
        if (isRetainableControlCode(c))
            suffix += c;
    }

    return prefix + ellidePrefix + string.midRef(midStart, midLength) + ellideSuffix + suffix;
}

QString QTextEngine::elidedText(Qt::TextElideMode mode, const QFixed &width, int flags, int from, int count) const
{
//    qDebug() << "elidedText; available width" << width.toReal() << "text width:" << this->width(0, layoutData->string.length()).toReal();

    if (flags & Qt::TextShowMnemonic) {
        itemize();
        QCharAttributes *attributes = const_cast<QCharAttributes *>(this->attributes());
        if (!attributes)
            return QString();
        for (int i = 0; i < layoutData->items.size(); ++i) {
            QScriptItem &si = layoutData->items[i];
            if (!si.num_glyphs)
                shape(i);

            unsigned short *logClusters = this->logClusters(&si);
            QGlyphLayout glyphs = shapedGlyphs(&si);

            const int end = si.position + length(&si);
            for (int i = si.position; i < end - 1; ++i) {
                if (layoutData->string.at(i) == QLatin1Char('&')
                    && !attributes[i + 1].whiteSpace && attributes[i + 1].graphemeBoundary) {
                    const int gp = logClusters[i - si.position];
                    glyphs.attributes[gp].dontPrint = true;
                    // emulate grapheme cluster
                    attributes[i] = attributes[i + 1];
                    memset(attributes + i + 1, 0, sizeof(QCharAttributes));
                    if (layoutData->string.at(i + 1) == QLatin1Char('&'))
                        ++i;
                }
            }
        }
    }

    validate();

    const int to = count >= 0 && count <= layoutData->string.length() - from
            ? from + count
            : layoutData->string.length();

    if (mode == Qt::ElideNone
        || this->width(from, layoutData->string.length()) <= width
        || to - from <= 1)
        return layoutData->string.mid(from, from - to);

    QFixed ellipsisWidth;
    QString ellipsisText;
    {
        QFontEngine *engine = fnt.d->engineForScript(QChar::Script_Common);

        QChar ellipsisChar(0x2026);

        glyph_t glyph = engine->glyphIndex(ellipsisChar.unicode());

        QGlyphLayout glyphs;
        glyphs.numGlyphs = 1;
        glyphs.glyphs = &glyph;
        glyphs.advances = &ellipsisWidth;

        if (glyph != 0) {
            engine->recalcAdvances(&glyphs, 0);

            ellipsisText = ellipsisChar;
        } else {
            glyph = engine->glyphIndex('.');
            if (glyph != 0) {
                engine->recalcAdvances(&glyphs, 0);

                ellipsisWidth *= 3;
                ellipsisText = QStringLiteral("...");
            }
        }
    }

    const QFixed availableWidth = width - ellipsisWidth;
    if (availableWidth < 0)
        return QString();

    const QCharAttributes *attributes = this->attributes();
    if (!attributes)
        return QString();

    if (mode == Qt::ElideRight) {
        QFixed currentWidth;
        int pos;
        int nextBreak = from;

        do {
            pos = nextBreak;

            ++nextBreak;
            while (nextBreak < layoutData->string.length() && !attributes[nextBreak].graphemeBoundary)
                ++nextBreak;

            currentWidth += this->width(pos, nextBreak - pos);
        } while (nextBreak < to
                 && currentWidth < availableWidth);

        if (nextCharJoins(layoutData->string, pos))
            ellipsisText.prepend(QChar(0x200d) /* ZWJ */);

        return stringMidRetainingBidiCC(layoutData->string,
                                        QString(), ellipsisText,
                                        from, to,
                                        from, pos - from);
    } else if (mode == Qt::ElideLeft) {
        QFixed currentWidth;
        int pos;
        int nextBreak = to;

        do {
            pos = nextBreak;

            --nextBreak;
            while (nextBreak > 0 && !attributes[nextBreak].graphemeBoundary)
                --nextBreak;

            currentWidth += this->width(nextBreak, pos - nextBreak);
        } while (nextBreak > from
                 && currentWidth < availableWidth);

        if (prevCharJoins(layoutData->string, pos))
            ellipsisText.append(QChar(0x200d) /* ZWJ */);

        return stringMidRetainingBidiCC(layoutData->string,
                                        ellipsisText, QString(),
                                        from, to,
                                        pos, to - pos);
    } else if (mode == Qt::ElideMiddle) {
        QFixed leftWidth;
        QFixed rightWidth;

        int leftPos = from;
        int nextLeftBreak = from;

        int rightPos = to;
        int nextRightBreak = to;

        do {
            leftPos = nextLeftBreak;
            rightPos = nextRightBreak;

            ++nextLeftBreak;
            while (nextLeftBreak < layoutData->string.length() && !attributes[nextLeftBreak].graphemeBoundary)
                ++nextLeftBreak;

            --nextRightBreak;
            while (nextRightBreak > from && !attributes[nextRightBreak].graphemeBoundary)
                --nextRightBreak;

            leftWidth += this->width(leftPos, nextLeftBreak - leftPos);
            rightWidth += this->width(nextRightBreak, rightPos - nextRightBreak);
        } while (nextLeftBreak < to
                 && nextRightBreak > from
                 && leftWidth + rightWidth < availableWidth);

        if (nextCharJoins(layoutData->string, leftPos))
            ellipsisText.prepend(QChar(0x200d) /* ZWJ */);
        if (prevCharJoins(layoutData->string, rightPos))
            ellipsisText.append(QChar(0x200d) /* ZWJ */);

        return layoutData->string.midRef(from, leftPos - from) + ellipsisText + layoutData->string.midRef(rightPos, to - rightPos);
    }

    return layoutData->string.mid(from, to - from);
}

void QTextEngine::setBoundary(int strPos) const
{
    const int item = findItem(strPos);
    if (item < 0)
        return;

    QScriptItem newItem = layoutData->items.at(item);
    if (newItem.position != strPos) {
        newItem.position = strPos;
        layoutData->items.insert(item + 1, newItem);
    }
}

QFixed QTextEngine::calculateTabWidth(int item, QFixed x) const
{
    const QScriptItem &si = layoutData->items[item];

    QFixed dpiScale = 1;
    if (block.docHandle() && block.docHandle()->layout()) {
        QPaintDevice *pdev = block.docHandle()->layout()->paintDevice();
        if (pdev)
            dpiScale = QFixed::fromReal(pdev->logicalDpiY() / qreal(qt_defaultDpiY()));
    } else {
        dpiScale = QFixed::fromReal(fnt.d->dpi / qreal(qt_defaultDpiY()));
    }

    QList<QTextOption::Tab> tabArray = option.tabs();
    if (!tabArray.isEmpty()) {
        if (isRightToLeft()) { // rebase the tabArray positions.
            QList<QTextOption::Tab> newTabs;
            newTabs.reserve(tabArray.count());
            QList<QTextOption::Tab>::Iterator iter = tabArray.begin();
            while(iter != tabArray.end()) {
                QTextOption::Tab tab = *iter;
                if (tab.type == QTextOption::LeftTab)
                    tab.type = QTextOption::RightTab;
                else if (tab.type == QTextOption::RightTab)
                    tab.type = QTextOption::LeftTab;
                newTabs << tab;
                ++iter;
            }
            tabArray = newTabs;
        }
        for (int i = 0; i < tabArray.size(); ++i) {
            QFixed tab = QFixed::fromReal(tabArray[i].position) * dpiScale;
            if (tab > x) {  // this is the tab we need.
                QTextOption::Tab tabSpec = tabArray[i];
                int tabSectionEnd = layoutData->string.count();
                if (tabSpec.type == QTextOption::RightTab || tabSpec.type == QTextOption::CenterTab) {
                    // find next tab to calculate the width required.
                    tab = QFixed::fromReal(tabSpec.position);
                    for (int i=item + 1; i < layoutData->items.count(); i++) {
                        const QScriptItem &item = layoutData->items[i];
                        if (item.analysis.flags == QScriptAnalysis::TabOrObject) { // found it.
                            tabSectionEnd = item.position;
                            break;
                        }
                    }
                }
                else if (tabSpec.type == QTextOption::DelimiterTab)
                    // find delimitor character to calculate the width required
                    tabSectionEnd = qMax(si.position, layoutData->string.indexOf(tabSpec.delimiter, si.position) + 1);

                if (tabSectionEnd > si.position) {
                    QFixed length;
                    // Calculate the length of text between this tab and the tabSectionEnd
                    for (int i=item; i < layoutData->items.count(); i++) {
                        QScriptItem &item = layoutData->items[i];
                        if (item.position > tabSectionEnd || item.position <= si.position)
                            continue;
                        shape(i); // first, lets make sure relevant text is already shaped
                        if (item.analysis.flags == QScriptAnalysis::Object) {
                            length += item.width;
                            continue;
                        }
                        QGlyphLayout glyphs = this->shapedGlyphs(&item);
                        const int end = qMin(item.position + item.num_glyphs, tabSectionEnd) - item.position;
                        for (int i=0; i < end; i++)
                            length += glyphs.advances[i] * !glyphs.attributes[i].dontPrint;
                        if (end + item.position == tabSectionEnd && tabSpec.type == QTextOption::DelimiterTab) // remove half of matching char
                            length -= glyphs.advances[end] / 2 * !glyphs.attributes[end].dontPrint;
                    }

                    switch (tabSpec.type) {
                    case QTextOption::CenterTab:
                        length /= 2;
                        Q_FALLTHROUGH();
                    case QTextOption::DelimiterTab:
                    case QTextOption::RightTab:
                        tab = QFixed::fromReal(tabSpec.position) * dpiScale - length;
                        if (tab < x) // default to tab taking no space
                            return QFixed();
                        break;
                    case QTextOption::LeftTab:
                        break;
                    }
                }
                return tab - x;
            }
        }
    }
    QFixed tab = QFixed::fromReal(option.tabStopDistance());
    if (tab <= 0)
        tab = 80; // default
    tab *= dpiScale;
    QFixed nextTabPos = ((x / tab).truncate() + 1) * tab;
    QFixed tabWidth = nextTabPos - x;

    return tabWidth;
}

namespace {
class FormatRangeComparatorByStart {
    const QVector<QTextLayout::FormatRange> &list;
public:
    FormatRangeComparatorByStart(const QVector<QTextLayout::FormatRange> &list) : list(list) { }
    bool operator()(int a, int b) {
        return list.at(a).start < list.at(b).start;
    }
};
class FormatRangeComparatorByEnd {
    const QVector<QTextLayout::FormatRange> &list;
public:
    FormatRangeComparatorByEnd(const QVector<QTextLayout::FormatRange> &list) : list(list) { }
    bool operator()(int a, int b) {
        return list.at(a).start + list.at(a).length < list.at(b).start + list.at(b).length;
    }
};
}

void QTextEngine::resolveFormats() const
{
    if (!specialData || specialData->formats.isEmpty())
        return;
    Q_ASSERT(specialData->resolvedFormats.isEmpty());

    QTextFormatCollection *collection = formatCollection();

    QVector<QTextCharFormat> resolvedFormats(layoutData->items.count());

    QVarLengthArray<int, 64> formatsSortedByStart;
    formatsSortedByStart.reserve(specialData->formats.size());
    for (int i = 0; i < specialData->formats.size(); ++i) {
        if (specialData->formats.at(i).length >= 0)
            formatsSortedByStart.append(i);
    }
    QVarLengthArray<int, 64> formatsSortedByEnd = formatsSortedByStart;
    std::sort(formatsSortedByStart.begin(), formatsSortedByStart.end(),
              FormatRangeComparatorByStart(specialData->formats));
    std::sort(formatsSortedByEnd.begin(), formatsSortedByEnd.end(),
              FormatRangeComparatorByEnd(specialData->formats));

    QVarLengthArray<int, 16>  currentFormats;
    const int *startIt = formatsSortedByStart.constBegin();
    const int *endIt = formatsSortedByEnd.constBegin();

    for (int i = 0; i < layoutData->items.count(); ++i) {
        const QScriptItem *si = &layoutData->items.at(i);
        int end = si->position + length(si);

        while (startIt != formatsSortedByStart.constEnd() &&
            specialData->formats.at(*startIt).start <= si->position) {
            currentFormats.insert(std::upper_bound(currentFormats.begin(), currentFormats.end(), *startIt),
                                  *startIt);
            ++startIt;
        }
        while (endIt != formatsSortedByEnd.constEnd() &&
            specialData->formats.at(*endIt).start + specialData->formats.at(*endIt).length < end) {
            int *currentFormatIterator = std::lower_bound(currentFormats.begin(), currentFormats.end(), *endIt);
            if (*endIt < *currentFormatIterator)
                currentFormatIterator = currentFormats.end();
            currentFormats.remove(currentFormatIterator - currentFormats.begin());
            ++endIt;
        }

        QTextCharFormat &format = resolvedFormats[i];
        if (block.docHandle()) {
            // when we have a docHandle, formatIndex might still return a valid index based
            // on the preeditPosition. for all other cases, we cleared the resolved format indices
            format = collection->charFormat(formatIndex(si));
        }
        if (!currentFormats.isEmpty()) {
            for (int cur : currentFormats) {
                const QTextLayout::FormatRange &range = specialData->formats.at(cur);
                Q_ASSERT(range.start <= si->position && range.start + range.length >= end);
                format.merge(range.format);
            }
            format = collection->charFormat(collection->indexForFormat(format)); // get shared copy
        }
    }

    specialData->resolvedFormats = resolvedFormats;
}

QFixed QTextEngine::leadingSpaceWidth(const QScriptLine &line)
{
    if (!line.hasTrailingSpaces
        || (option.flags() & QTextOption::IncludeTrailingSpaces)
        || !isRightToLeft())
        return QFixed();

    return width(line.from + line.length, line.trailingSpaces);
}

QFixed QTextEngine::alignLine(const QScriptLine &line)
{
    QFixed x = 0;
    justify(line);
    // if width is QFIXED_MAX that means we used setNumColumns() and that implicitly makes this line left aligned.
    if (!line.justified && line.width != QFIXED_MAX) {
        int align = option.alignment();
        if (align & Qt::AlignJustify && isRightToLeft())
            align = Qt::AlignRight;
        if (align & Qt::AlignRight)
            x = line.width - (line.textAdvance);
        else if (align & Qt::AlignHCenter)
            x = (line.width - line.textAdvance)/2;
    }
    return x;
}

QFixed QTextEngine::offsetInLigature(const QScriptItem *si, int pos, int max, int glyph_pos)
{
    unsigned short *logClusters = this->logClusters(si);
    const QGlyphLayout &glyphs = shapedGlyphs(si);

    int offsetInCluster = 0;
    for (int i = pos - 1; i >= 0; i--) {
        if (logClusters[i] == glyph_pos)
            offsetInCluster++;
        else
            break;
    }

    // in the case that the offset is inside a (multi-character) glyph,
    // interpolate the position.
    if (offsetInCluster > 0) {
        int clusterLength = 0;
        for (int i = pos - offsetInCluster; i < max; i++) {
            if (logClusters[i] == glyph_pos)
                clusterLength++;
            else
                break;
        }
        if (clusterLength)
            return glyphs.advances[glyph_pos] * offsetInCluster / clusterLength;
    }

    return 0;
}

// Scan in logClusters[from..to-1] for glyph_pos
int QTextEngine::getClusterLength(unsigned short *logClusters,
                                  const QCharAttributes *attributes,
                                  int from, int to, int glyph_pos, int *start)
{
    int clusterLength = 0;
    for (int i = from; i < to; i++) {
        if (logClusters[i] == glyph_pos && attributes[i].graphemeBoundary) {
            if (*start < 0)
                *start = i;
            clusterLength++;
        }
        else if (clusterLength)
            break;
    }
    return clusterLength;
}

int QTextEngine::positionInLigature(const QScriptItem *si, int end,
                                    QFixed x, QFixed edge, int glyph_pos,
                                    bool cursorOnCharacter)
{
    unsigned short *logClusters = this->logClusters(si);
    int clusterStart = -1;
    int clusterLength = 0;

    if (si->analysis.script != QChar::Script_Common &&
        si->analysis.script != QChar::Script_Greek) {
        if (glyph_pos == -1)
            return si->position + end;
        else {
            int i;
            for (i = 0; i < end; i++)
                if (logClusters[i] == glyph_pos)
                    break;
            return si->position + i;
        }
    }

    if (glyph_pos == -1 && end > 0)
        glyph_pos = logClusters[end - 1];
    else {
        if (x <= edge)
            glyph_pos--;
    }

    const QCharAttributes *attrs = attributes() + si->position;
    logClusters = this->logClusters(si);
    clusterLength = getClusterLength(logClusters, attrs, 0, end, glyph_pos, &clusterStart);

    if (clusterLength) {
        const QGlyphLayout &glyphs = shapedGlyphs(si);
        QFixed glyphWidth = glyphs.effectiveAdvance(glyph_pos);
        // the approximate width of each individual element of the ligature
        QFixed perItemWidth = glyphWidth / clusterLength;
        if (perItemWidth <= 0)
            return si->position + clusterStart;
        QFixed left = x > edge ? edge : edge - glyphWidth;
        int n = ((x - left) / perItemWidth).floor().toInt();
        QFixed dist = x - left - n * perItemWidth;
        int closestItem = dist > (perItemWidth / 2) ? n + 1 : n;
        if (cursorOnCharacter && closestItem > 0)
            closestItem--;
        int pos = clusterStart + closestItem;
        // Jump to the next grapheme boundary
        while (pos < end && !attrs[pos].graphemeBoundary)
            pos++;
        return si->position + pos;
    }
    return si->position + end;
}

int QTextEngine::previousLogicalPosition(int oldPos) const
{
    const QCharAttributes *attrs = attributes();
    int len = block.isValid() ? block.length() - 1
                              : layoutData->string.length();
    Q_ASSERT(len <= layoutData->string.length());
    if (!attrs || oldPos <= 0 || oldPos > len)
        return oldPos;

    oldPos--;
    while (oldPos && !attrs[oldPos].graphemeBoundary)
        oldPos--;
    return oldPos;
}

int QTextEngine::nextLogicalPosition(int oldPos) const
{
    const QCharAttributes *attrs = attributes();
    int len = block.isValid() ? block.length() - 1
                              : layoutData->string.length();
    Q_ASSERT(len <= layoutData->string.length());
    if (!attrs || oldPos < 0 || oldPos >= len)
        return oldPos;

    oldPos++;
    while (oldPos < len && !attrs[oldPos].graphemeBoundary)
        oldPos++;
    return oldPos;
}

int QTextEngine::lineNumberForTextPosition(int pos)
{
    if (!layoutData)
        itemize();
    if (pos == layoutData->string.length() && lines.size())
        return lines.size() - 1;
    for (int i = 0; i < lines.size(); ++i) {
        const QScriptLine& line = lines[i];
        if (line.from + line.length + line.trailingSpaces > pos)
            return i;
    }
    return -1;
}

void QTextEngine::insertionPointsForLine(int lineNum, QVector<int> &insertionPoints)
{
    QTextLineItemIterator iterator(this, lineNum);

    insertionPoints.reserve(iterator.line.length);

    bool lastLine = lineNum >= lines.size() - 1;

    while (!iterator.atEnd()) {
        const QScriptItem &si = iterator.next();

        int end = iterator.itemEnd;
        if (lastLine && iterator.item == iterator.lastItem)
            ++end; // the last item in the last line -> insert eol position
        if (si.analysis.bidiLevel % 2) {
            for (int i = end - 1; i >= iterator.itemStart; --i)
                insertionPoints.push_back(i);
        } else {
            for (int i = iterator.itemStart; i < end; ++i)
                insertionPoints.push_back(i);
        }
    }
}

int QTextEngine::endOfLine(int lineNum)
{
    QVector<int> insertionPoints;
    insertionPointsForLine(lineNum, insertionPoints);

    if (insertionPoints.size() > 0)
        return insertionPoints.constLast();
    return 0;
}

int QTextEngine::beginningOfLine(int lineNum)
{
    QVector<int> insertionPoints;
    insertionPointsForLine(lineNum, insertionPoints);

    if (insertionPoints.size() > 0)
        return insertionPoints.constFirst();
    return 0;
}

int QTextEngine::positionAfterVisualMovement(int pos, QTextCursor::MoveOperation op)
{
    itemize();

    bool moveRight = (op == QTextCursor::Right);
    bool alignRight = isRightToLeft();
    if (!layoutData->hasBidi)
        return moveRight ^ alignRight ? nextLogicalPosition(pos) : previousLogicalPosition(pos);

    int lineNum = lineNumberForTextPosition(pos);
    if (lineNum < 0)
        return pos;

    QVector<int> insertionPoints;
    insertionPointsForLine(lineNum, insertionPoints);
    int i, max = insertionPoints.size();
    for (i = 0; i < max; i++)
        if (pos == insertionPoints[i]) {
            if (moveRight) {
                if (i + 1 < max)
                    return insertionPoints[i + 1];
            } else {
                if (i > 0)
                    return insertionPoints[i - 1];
            }

            if (moveRight ^ alignRight) {
                if (lineNum + 1 < lines.size())
                    return alignRight ? endOfLine(lineNum + 1) : beginningOfLine(lineNum + 1);
            }
            else {
                if (lineNum > 0)
                    return alignRight ? beginningOfLine(lineNum - 1) : endOfLine(lineNum - 1);
            }

            break;
        }

    return pos;
}

void QTextEngine::addItemDecoration(QPainter *painter, const QLineF &line, ItemDecorationList *decorationList)
{
    if (delayDecorations) {
        decorationList->append(ItemDecoration(line.x1(), line.x2(), line.y1(), painter->pen()));
    } else {
        painter->drawLine(line);
    }
}

void QTextEngine::addUnderline(QPainter *painter, const QLineF &line)
{
    // qDebug() << "Adding underline:" << line;
    addItemDecoration(painter, line, &underlineList);
}

void QTextEngine::addStrikeOut(QPainter *painter, const QLineF &line)
{
    addItemDecoration(painter, line, &strikeOutList);
}

void QTextEngine::addOverline(QPainter *painter, const QLineF &line)
{
    addItemDecoration(painter, line, &overlineList);
}

void QTextEngine::drawItemDecorationList(QPainter *painter, const ItemDecorationList &decorationList)
{
    // qDebug() << "Drawing" << decorationList.size() << "decorations";
    if (decorationList.isEmpty())
        return;

    for (const ItemDecoration &decoration : decorationList) {
        painter->setPen(decoration.pen);
        painter->drawLine(QLineF(decoration.x1, decoration.y, decoration.x2, decoration.y));
    }
}

void QTextEngine::drawDecorations(QPainter *painter)
{
    QPen oldPen = painter->pen();

    bool wasCompatiblePainting = painter->renderHints()
            & QPainter::Qt4CompatiblePainting;

    if (wasCompatiblePainting)
        painter->setRenderHint(QPainter::Qt4CompatiblePainting, false);

    adjustUnderlines();
    drawItemDecorationList(painter, underlineList);
    drawItemDecorationList(painter, strikeOutList);
    drawItemDecorationList(painter, overlineList);

    clearDecorations();

    if (wasCompatiblePainting)
        painter->setRenderHint(QPainter::Qt4CompatiblePainting);

    painter->setPen(oldPen);
}

void QTextEngine::clearDecorations()
{
    underlineList.clear();
    strikeOutList.clear();
    overlineList.clear();
}

void QTextEngine::adjustUnderlines()
{
    // qDebug() << __PRETTY_FUNCTION__ << underlineList.count() << "underlines";
    if (underlineList.isEmpty())
        return;

    ItemDecorationList::iterator start = underlineList.begin();
    ItemDecorationList::iterator end   = underlineList.end();
    ItemDecorationList::iterator it = start;
    qreal underlinePos = start->y;
    qreal penWidth = start->pen.widthF();
    qreal lastLineEnd = start->x1;

    while (it != end) {
        if (qFuzzyCompare(lastLineEnd, it->x1)) { // no gap between underlines
            underlinePos = qMax(underlinePos, it->y);
            penWidth = qMax(penWidth, it->pen.widthF());
        } else { // gap between this and the last underline
            adjustUnderlines(start, it, underlinePos, penWidth);
            start = it;
            underlinePos = start->y;
            penWidth = start->pen.widthF();
        }
        lastLineEnd = it->x2;
        ++it;
    }

    adjustUnderlines(start, end, underlinePos, penWidth);
}

void QTextEngine::adjustUnderlines(ItemDecorationList::iterator start,
                                   ItemDecorationList::iterator end,
                                   qreal underlinePos, qreal penWidth)
{
    for (ItemDecorationList::iterator it = start; it != end; ++it) {
        it->y = underlinePos;
        it->pen.setWidthF(penWidth);
    }
}

QStackTextEngine::QStackTextEngine(const QString &string, const QFont &f)
    : QTextEngine(string, f),
      _layoutData(string, _memory, MemSize)
{
    stackEngine = true;
    layoutData = &_layoutData;
}

QTextItemInt::QTextItemInt(const QScriptItem &si, QFont *font, const QTextCharFormat &format)
    : justified(false),
      underlineStyle(QTextCharFormat::NoUnderline),
      charFormat(format),
      num_chars(0),
      chars(nullptr),
      logClusters(nullptr),
      f(font),
      fontEngine(font->d->engineForScript(si.analysis.script))
{
    Q_ASSERT(fontEngine);

    initWithScriptItem(si);
}

QTextItemInt::QTextItemInt(const QGlyphLayout &g, QFont *font, const QChar *chars_, int numChars, QFontEngine *fe, const QTextCharFormat &format)
    : flags(0),
      justified(false),
      underlineStyle(QTextCharFormat::NoUnderline),
      charFormat(format),
      num_chars(numChars),
      chars(chars_),
      logClusters(nullptr),
      f(font),
      glyphs(g),
      fontEngine(fe)
{
}

// Fix up flags and underlineStyle with given info
void QTextItemInt::initWithScriptItem(const QScriptItem &si)
{
    // explicitly initialize flags so that initFontAttributes can be called
    // multiple times on the same TextItem
    flags = 0;
    if (si.analysis.bidiLevel %2)
        flags |= QTextItem::RightToLeft;
    ascent = si.ascent;
    descent = si.descent;

    if (charFormat.hasProperty(QTextFormat::TextUnderlineStyle)) {
        underlineStyle = charFormat.underlineStyle();
    } else if (charFormat.boolProperty(QTextFormat::FontUnderline)
               || f->d->underline) {
        underlineStyle = QTextCharFormat::SingleUnderline;
    }

    // compat
    if (underlineStyle == QTextCharFormat::SingleUnderline)
        flags |= QTextItem::Underline;

    if (f->d->overline || charFormat.fontOverline())
        flags |= QTextItem::Overline;
    if (f->d->strikeOut || charFormat.fontStrikeOut())
        flags |= QTextItem::StrikeOut;
}

QTextItemInt QTextItemInt::midItem(QFontEngine *fontEngine, int firstGlyphIndex, int numGlyphs) const
{
    QTextItemInt ti = *this;
    const int end = firstGlyphIndex + numGlyphs;
    ti.glyphs = glyphs.mid(firstGlyphIndex, numGlyphs);
    ti.fontEngine = fontEngine;

    if (logClusters && chars) {
        const int logClusterOffset = logClusters[0];
        while (logClusters[ti.chars - chars] - logClusterOffset < firstGlyphIndex)
            ++ti.chars;

        ti.logClusters += (ti.chars - chars);

        ti.num_chars = 0;
        int char_start = ti.chars - chars;
        while (char_start + ti.num_chars < num_chars && ti.logClusters[ti.num_chars] - logClusterOffset < end)
            ++ti.num_chars;
    }
    return ti;
}


QTransform qt_true_matrix(qreal w, qreal h, const QTransform &x)
{
    QRectF rect = x.mapRect(QRectF(0, 0, w, h));
    return x * QTransform::fromTranslate(-rect.x(), -rect.y());
}


glyph_metrics_t glyph_metrics_t::transformed(const QTransform &matrix) const
{
    if (matrix.type() < QTransform::TxTranslate)
        return *this;

    glyph_metrics_t m = *this;

    qreal w = width.toReal();
    qreal h = height.toReal();
    QTransform xform = qt_true_matrix(w, h, matrix);

    QRectF rect(0, 0, w, h);
    rect = xform.mapRect(rect);
    m.width = QFixed::fromReal(rect.width());
    m.height = QFixed::fromReal(rect.height());

    QLineF l = xform.map(QLineF(x.toReal(), y.toReal(), xoff.toReal(), yoff.toReal()));

    m.x = QFixed::fromReal(l.x1());
    m.y = QFixed::fromReal(l.y1());

    // The offset is relative to the baseline which is why we use dx/dy of the line
    m.xoff = QFixed::fromReal(l.dx());
    m.yoff = QFixed::fromReal(l.dy());

    return m;
}

QTextLineItemIterator::QTextLineItemIterator(QTextEngine *_eng, int _lineNum, const QPointF &pos,
                                             const QTextLayout::FormatRange *_selection)
    : eng(_eng),
      line(eng->lines[_lineNum]),
      si(0),
      lineNum(_lineNum),
      lineEnd(line.from + line.length),
      firstItem(eng->findItem(line.from)),
      lastItem(eng->findItem(lineEnd - 1, firstItem)),
      nItems((firstItem >= 0 && lastItem >= firstItem)? (lastItem-firstItem+1) : 0),
      logicalItem(-1),
      item(-1),
      visualOrder(nItems),
      selection(_selection)
{
    x = QFixed::fromReal(pos.x());

    x += line.x;

    x += eng->alignLine(line);

    QVarLengthArray<uchar> levels(nItems);
    for (int i = 0; i < nItems; ++i)
        levels[i] = eng->layoutData->items[i+firstItem].analysis.bidiLevel;
    QTextEngine::bidiReorder(nItems, levels.data(), visualOrder.data());

    eng->shapeLine(line);
}

QScriptItem &QTextLineItemIterator::next()
{
    x += itemWidth;

    ++logicalItem;
    item = visualOrder[logicalItem] + firstItem;
    itemLength = eng->length(item);
    si = &eng->layoutData->items[item];
    if (!si->num_glyphs)
        eng->shape(item);

    itemStart = qMax(line.from, si->position);
    itemEnd = qMin(lineEnd, si->position + itemLength);

    if (si->analysis.flags >= QScriptAnalysis::TabOrObject) {
        glyphsStart = 0;
        glyphsEnd = 1;
        itemWidth = si->width;
        return *si;
    }

    unsigned short *logClusters = eng->logClusters(si);
    QGlyphLayout glyphs = eng->shapedGlyphs(si);

    glyphsStart = logClusters[itemStart - si->position];
    glyphsEnd = (itemEnd == si->position + itemLength) ? si->num_glyphs : logClusters[itemEnd - si->position];

    // show soft-hyphen at line-break
    if (si->position + itemLength >= lineEnd
        && eng->layoutData->string.at(lineEnd - 1).unicode() == QChar::SoftHyphen)
        glyphs.attributes[glyphsEnd - 1].dontPrint = false;

    itemWidth = 0;
    for (int g = glyphsStart; g < glyphsEnd; ++g)
        itemWidth += glyphs.effectiveAdvance(g);

    return *si;
}

bool QTextLineItemIterator::getSelectionBounds(QFixed *selectionX, QFixed *selectionWidth) const
{
    *selectionX = *selectionWidth = 0;

    if (!selection)
        return false;

    if (si->analysis.flags >= QScriptAnalysis::TabOrObject) {
        if (si->position >= selection->start + selection->length
            || si->position + itemLength <= selection->start)
            return false;

        *selectionX = x;
        *selectionWidth = itemWidth;
    } else {
        unsigned short *logClusters = eng->logClusters(si);
        QGlyphLayout glyphs = eng->shapedGlyphs(si);

        int from = qMax(itemStart, selection->start) - si->position;
        int to = qMin(itemEnd, selection->start + selection->length) - si->position;
        if (from >= to)
            return false;

        int start_glyph = logClusters[from];
        int end_glyph = (to == itemLength) ? si->num_glyphs : logClusters[to];
        QFixed soff;
        QFixed swidth;
        if (si->analysis.bidiLevel %2) {
            for (int g = glyphsEnd - 1; g >= end_glyph; --g)
                soff += glyphs.effectiveAdvance(g);
            for (int g = end_glyph - 1; g >= start_glyph; --g)
                swidth += glyphs.effectiveAdvance(g);
        } else {
            for (int g = glyphsStart; g < start_glyph; ++g)
                soff += glyphs.effectiveAdvance(g);
            for (int g = start_glyph; g < end_glyph; ++g)
                swidth += glyphs.effectiveAdvance(g);
        }

        // If the starting character is in the middle of a ligature,
        // selection should only contain the right part of that ligature
        // glyph, so we need to get the width of the left part here and
        // add it to *selectionX
        QFixed leftOffsetInLigature = eng->offsetInLigature(si, from, to, start_glyph);
        *selectionX = x + soff + leftOffsetInLigature;
        *selectionWidth = swidth - leftOffsetInLigature;
        // If the ending character is also part of a ligature, swidth does
        // not contain that part yet, we also need to find out the width of
        // that left part
        *selectionWidth += eng->offsetInLigature(si, to, itemLength, end_glyph);
    }
    return true;
}

QT_END_NAMESPACE
