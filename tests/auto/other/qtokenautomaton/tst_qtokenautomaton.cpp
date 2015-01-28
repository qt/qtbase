/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include "tokenizers/basic/basic.h"
#include "tokenizers/basicNamespace/basicNamespace.h"
#include "tokenizers/boilerplate/boilerplate.h"
#include "tokenizers/noToString/noToString.h"

/*!
 \class tst_QTokenAutomaton
 \internal
 \brief Tests the QTokenAutomaton generator.

TODO:
    - Verify high Unicode codepoints
    - Verify that signatures are static.
 */
class tst_QTokenAutomaton : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tokenizerBasic() const;
};

void tst_QTokenAutomaton::tokenizerBasic() const
{
    typedef QPair<Basic::Token, QString> TokenPair;
    typedef QList<TokenPair> TokenPairList;

    /* Tokenizer basic. */
    {
        /* All the tokens matching tokens. */
        {
            TokenPairList tokens;
            tokens.append(qMakePair(Basic::_, QString::fromLatin1("abc")));
            tokens.append(qMakePair(Basic::_111, QString::fromLatin1("def")));
            tokens.append(qMakePair(Basic::wobbly, QString::fromLatin1("ghi")));
            tokens.append(qMakePair(Basic::FFFF, QString::fromLatin1("FFFF")));
            tokens.append(qMakePair(Basic::FFFG, QString::fromLatin1("FFFG")));
            tokens.append(qMakePair(Basic::FFGG, QString::fromLatin1("FFGG")));
            tokens.append(qMakePair(Basic::FFGF, QString::fromLatin1("FFGF")));
            tokens.append(qMakePair(Basic::FFLM, QString::fromLatin1("FFLM")));
            tokens.append(qMakePair(Basic::AReallyLongTokenIFreakinMeanItUKnowUKnowKnow, QString::fromLatin1("aReallyLongTokenIFreakinMeanItUKnowUKnowKnow")));
            tokens.append(qMakePair(Basic::WeHaveDashes, QString::fromLatin1("we-have-dashes")));
            tokens.append(qMakePair(Basic::WeHaveDashes2, QString::fromLatin1("we-have-dashes-")));

            /* toToken(). */
            for(int i = 0; i < tokens.count(); ++i)
            {
                const TokenPair &at = tokens.at(i);
                /* QString. */
                QCOMPARE(Basic::toToken(at.second), at.first);

                /* const QChar *, int. */
                QCOMPARE(Basic::toToken(at.second.constData(), at.second.length()), at.first);

                /* QStringRef. */
                const QStringRef ref(&at.second);
                QCOMPARE(Basic::toToken(ref), at.first);
            }

            /* toString(). */
            for(int i = 0; i < tokens.count(); ++i)
            {
                const TokenPair &at = tokens.at(i);
                QCOMPARE(Basic::toString(at.first), at.second);
            }
        }

        /* Tokens that we don't recognize. */
        {
            QStringList tokens;
            tokens.append(QLatin1String("NO-MATCH"));
            tokens.append(QLatin1String("NoKeyword"));
            tokens.append(QString());

            for(int i = 0; i < tokens.count(); ++i)
                QCOMPARE(Basic::toToken(tokens.at(i)), Basic::NoKeyword);
        }

        /* Weird values for toToken(). */
        {
            QCOMPARE(Basic::toString(Basic::Token(5000)), QString());
            QCOMPARE(Basic::toString(Basic::NoKeyword), QString());
        }

    }
}

QTEST_MAIN(tst_QTokenAutomaton)

#include "tst_qtokenautomaton.moc"
