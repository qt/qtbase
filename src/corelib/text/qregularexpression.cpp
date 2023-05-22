// Copyright (C) 2020 Giuseppe D'Angelo <dangelog@gmail.com>.
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qregularexpression.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qdatastream.h>

#if defined(Q_OS_MACOS)
#include <QtCore/private/qcore_mac_p.h>
#endif

#define PCRE2_CODE_UNIT_WIDTH 16

#include <pcre2.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QRegularExpression
    \inmodule QtCore
    \reentrant

    \brief The QRegularExpression class provides pattern matching using regular
    expressions.

    \since 5.0

    \ingroup tools
    \ingroup shared

    \keyword regular expression

    Regular expressions, or \e{regexps}, are a very powerful tool to handle
    strings and texts. This is useful in many contexts, e.g.,

    \table
    \row \li Validation
         \li A regexp can test whether a substring meets some criteria,
         e.g. is an integer or contains no whitespace.
    \row \li Searching
         \li A regexp provides more powerful pattern matching than
         simple substring matching, e.g., match one of the words
         \e{mail}, \e{letter} or \e{correspondence}, but none of the
         words \e{email}, \e{mailman}, \e{mailer}, \e{letterbox}, etc.
    \row \li Search and Replace
         \li A regexp can replace all occurrences of a substring with a
         different substring, e.g., replace all occurrences of \e{&}
         with \e{\&amp;} except where the \e{&} is already followed by
         an \e{amp;}.
    \row \li String Splitting
         \li A regexp can be used to identify where a string should be
         split apart, e.g. splitting tab-delimited strings.
    \endtable

    This document is by no means a complete reference to pattern matching using
    regular expressions, and the following parts will require the reader to
    have some basic knowledge about Perl-like regular expressions and their
    pattern syntax.

    Good references about regular expressions include:

    \list
    \li \e {Mastering Regular Expressions} (Third Edition) by Jeffrey E. F.
    Friedl, ISBN 0-596-52812-4;
    \li the \l{https://pcre.org/original/doc/html/pcrepattern.html}
    {pcrepattern(3)} man page, describing the pattern syntax supported by PCRE
    (the reference implementation of Perl-compatible regular expressions);
    \li the \l{http://perldoc.perl.org/perlre.html} {Perl's regular expression
    documentation} and the \l{http://perldoc.perl.org/perlretut.html} {Perl's
    regular expression tutorial}.
    \endlist

    \tableofcontents

    \section1 Introduction

    QRegularExpression implements Perl-compatible regular expressions. It fully
    supports Unicode. For an overview of the regular expression syntax
    supported by QRegularExpression, please refer to the aforementioned
    pcrepattern(3) man page. A regular expression is made up of two things: a
    \b{pattern string} and a set of \b{pattern options} that change the
    meaning of the pattern string.

    You can set the pattern string by passing a string to the QRegularExpression
    constructor:

    \snippet code/src_corelib_text_qregularexpression.cpp 0

    This sets the pattern string to \c{a pattern}. You can also use the
    setPattern() function to set a pattern on an existing QRegularExpression
    object:

    \snippet code/src_corelib_text_qregularexpression.cpp 1

    Note that due to C++ literal strings rules, you must escape all backslashes
    inside the pattern string with another backslash:

    \snippet code/src_corelib_text_qregularexpression.cpp 2

    Alternatively, you can use a
    \l {https://en.cppreference.com/w/cpp/language/string_literal} {raw string literal},
    in which case you don't need to escape backslashes in the pattern, all characters
    between \c {R"(...)"} are considered raw characters. As you can see in the following
    example, this simplifies writing patterns:

    \snippet code/src_corelib_text_qregularexpression.cpp 35

    The pattern() function returns the pattern that is currently set for a
    QRegularExpression object:

    \snippet code/src_corelib_text_qregularexpression.cpp 3

    \section1 Pattern Options

    The meaning of the pattern string can be modified by setting one or more
    \e{pattern options}. For instance, it is possible to set a pattern to match
    case insensitively by setting the QRegularExpression::CaseInsensitiveOption.

    You can set the options by passing them to the QRegularExpression
    constructor, as in:

    \snippet code/src_corelib_text_qregularexpression.cpp 4

    Alternatively, you can use the setPatternOptions() function on an existing
    QRegularExpressionObject:

    \snippet code/src_corelib_text_qregularexpression.cpp 5

    It is possible to get the pattern options currently set on a
    QRegularExpression object by using the patternOptions() function:

    \snippet code/src_corelib_text_qregularexpression.cpp 6

    Please refer to the QRegularExpression::PatternOption enum documentation for
    more information about each pattern option.

    \section1 Match Type and Match Options

    The last two arguments of the match() and the globalMatch() functions set
    the match type and the match options. The match type is a value of the
    QRegularExpression::MatchType enum; the "traditional" matching algorithm is
    chosen by using the NormalMatch match type (the default). It is also
    possible to enable partial matching of the regular expression against a
    subject string: see the \l{partial matching} section for more details.

    The match options are a set of one or more QRegularExpression::MatchOption
    values. They change the way a specific match of a regular expression
    against a subject string is done. Please refer to the
    QRegularExpression::MatchOption enum documentation for more details.

    \target normal matching
    \section1 Normal Matching

    In order to perform a match you can simply invoke the match() function
    passing a string to match against. We refer to this string as the
    \e{subject string}. The result of the match() function is a
    QRegularExpressionMatch object that can be used to inspect the results of
    the match. For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 7

    If a match is successful, the (implicit) capturing group number 0 can be
    used to retrieve the substring matched by the entire pattern (see also the
    section about \l{extracting captured substrings}):

    \snippet code/src_corelib_text_qregularexpression.cpp 8

    It's also possible to start a match at an arbitrary offset inside the
    subject string by passing the offset as an argument of the
    match() function. In the following example \c{"12 abc"}
    is not matched because the match is started at offset 1:

    \snippet code/src_corelib_text_qregularexpression.cpp 9

    \target extracting captured substrings
    \section2 Extracting captured substrings

    The QRegularExpressionMatch object contains also information about the
    substrings captured by the capturing groups in the pattern string. The
    \l{QRegularExpressionMatch::}{captured()} function will return the string
    captured by the n-th capturing group:

    \snippet code/src_corelib_text_qregularexpression.cpp 10

    Capturing groups in the pattern are numbered starting from 1, and the
    implicit capturing group 0 is used to capture the substring that matched
    the entire pattern.

    It's also possible to retrieve the starting and the ending offsets (inside
    the subject string) of each captured substring, by using the
    \l{QRegularExpressionMatch::}{capturedStart()} and the
    \l{QRegularExpressionMatch::}{capturedEnd()} functions:

    \snippet code/src_corelib_text_qregularexpression.cpp 11

    All of these functions have an overload taking a QString as a parameter
    in order to extract \e{named} captured substrings. For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 12

    \target global matching
    \section1 Global Matching

    \e{Global matching} is useful to find all the occurrences of a given
    regular expression inside a subject string. Suppose that we want to extract
    all the words from a given string, where a word is a substring matching
    the pattern \c{\w+}.

    QRegularExpression::globalMatch returns a QRegularExpressionMatchIterator,
    which is a Java-like forward iterator that can be used to iterate over the
    results. For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 13

    Since it's a Java-like iterator, the QRegularExpressionMatchIterator will
    point immediately before the first result. Every result is returned as a
    QRegularExpressionMatch object. The
    \l{QRegularExpressionMatchIterator::}{hasNext()} function will return true
    if there's at least one more result, and
    \l{QRegularExpressionMatchIterator::}{next()} will return the next result
    and advance the iterator. Continuing from the previous example:

    \snippet code/src_corelib_text_qregularexpression.cpp 14

    You can also use \l{QRegularExpressionMatchIterator::}{peekNext()} to get
    the next result without advancing the iterator.

    It is also possible to simply use the result of
    QRegularExpression::globalMatch in a range-based for loop, for instance
    like this:

    \snippet code/src_corelib_text_qregularexpression.cpp 34

    It is possible to pass a starting offset and one or more match options to
    the globalMatch() function, exactly like normal matching with match().

    \target partial matching
    \section1 Partial Matching

    A \e{partial match} is obtained when the end of the subject string is
    reached, but more characters are needed to successfully complete the match.
    Note that a partial match is usually much more inefficient than a normal
    match because many optimizations of the matching algorithm cannot be
    employed.

    A partial match must be explicitly requested by specifying a match type of
    PartialPreferCompleteMatch or PartialPreferFirstMatch when calling
    QRegularExpression::match or QRegularExpression::globalMatch. If a partial
    match is found, then calling the \l{QRegularExpressionMatch::}{hasMatch()}
    function on the QRegularExpressionMatch object returned by match() will
    return \c{false}, but \l{QRegularExpressionMatch::}{hasPartialMatch()} will return
    \c{true}.

    When a partial match is found, no captured substrings are returned, and the
    (implicit) capturing group 0 corresponding to the whole match captures the
    partially matched substring of the subject string.

    Note that asking for a partial match can still lead to a complete match, if
    one is found; in this case, \l{QRegularExpressionMatch::}{hasMatch()} will
    return \c{true} and \l{QRegularExpressionMatch::}{hasPartialMatch()}
    \c{false}. It never happens that a QRegularExpressionMatch reports both a
    partial and a complete match.

    Partial matching is mainly useful in two scenarios: validating user input
    in real time and incremental/multi-segment matching.

    \target validating user input
    \section2 Validating user input

    Suppose that we would like the user to input a date in a specific
    format, for instance "MMM dd, yyyy". We can check the input validity with
    a pattern like:

    \c{^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \d\d?, \d\d\d\d$}

    (This pattern doesn't catch invalid days, but let's keep it for the
    example's purposes).

    We would like to validate the input with this regular expression \e{while}
    the user is typing it, so that we can report an error in the input as soon
    as it is committed (for instance, the user typed the wrong key). In order
    to do so we must distinguish three cases:

    \list
    \li the input cannot possibly match the regular expression;
    \li the input does match the regular expression;
    \li the input does not match the regular expression right now,
    but it will if more characters will be added to it.
    \endlist

    Note that these three cases represent exactly the possible states of a
    QValidator (see the QValidator::State enum).

    In particular, in the last case we want the regular expression engine to
    report a partial match: we are successfully matching the pattern against
    the subject string but the matching cannot continue because the end of the
    subject is encountered. Notice, however, that the matching algorithm should
    continue and try all possibilities, and in case a complete (non-partial)
    match is found, then this one should be reported, and the input string
    accepted as fully valid.

    This behavior is implemented by the PartialPreferCompleteMatch match type.
    For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 15

    If matching the same regular expression against the subject string leads to
    a complete match, it is reported as usual:

    \snippet code/src_corelib_text_qregularexpression.cpp 16

    Another example with a different pattern, showing the behavior of
    preferring a complete match over a partial one:

    \snippet code/src_corelib_text_qregularexpression.cpp 17

    In this case, the subpattern \c{abc\\w+X} partially matches the subject
    string; however, the subpattern \c{def} matches the subject string
    completely, and therefore a complete match is reported.

    If multiple partial matches are found when matching (but no complete
    match), then the QRegularExpressionMatch object will report the first one
    that is found. For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 18

    \section2 Incremental/multi-segment matching

    Incremental matching is another use case of partial matching. Suppose that
    we want to find the occurrences of a regular expression inside a large text
    (that is, substrings matching the regular expression). In order to do so we
    would like to "feed" the large text to the regular expression engines in
    smaller chunks. The obvious problem is what happens if the substring that
    matches the regular expression spans across two or more chunks.

    In this case, the regular expression engine should report a partial match,
    so that we can match again adding new data and (eventually) get a complete
    match. This implies that the regular expression engine may assume that
    there are other characters \e{beyond the end} of the subject string. This
    is not to be taken literally -- the engine will never try to access
    any character after the last one in the subject.

    QRegularExpression implements this behavior when using the
    PartialPreferFirstMatch match type. This match type reports a partial match
    as soon as it is found, and other match alternatives are not tried
    (even if they could lead to a complete match). For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 19

    This happens because when matching the first branch of the alternation
    operator a partial match is found, and therefore matching stops, without
    trying the second branch. Another example:

    \snippet code/src_corelib_text_qregularexpression.cpp 20

    This shows what could seem a counterintuitive behavior of quantifiers:
    since \c{?} is greedy, then the engine tries first to continue the match
    after having matched \c{"abc"}; but then the matching reaches the end of the
    subject string, and therefore a partial match is reported. This is
    even more surprising in the following example:

    \snippet code/src_corelib_text_qregularexpression.cpp 21

    It's easy to understand this behavior if we remember that the engine
    expects the subject string to be only a substring of the whole text we're
    looking for a match into (that is, how we said before, that the engine
    assumes that there are other characters beyond the end of the subject
    string).

    Since the \c{*} quantifier is greedy, then reporting a complete match could
    be an error, because after the current subject \c{"abc"} there may be other
    occurrences of \c{"abc"}. For instance, the complete text could have been
    "abcabcX", and therefore the \e{right} match to report (in the complete
    text) would have been \c{"abcabc"}; by matching only against the leading
    \c{"abc"} we instead get a partial match.

    \section1 Error Handling

    It is possible for a QRegularExpression object to be invalid because of
    syntax errors in the pattern string. The isValid() function will return
    true if the regular expression is valid, or false otherwise:

    \snippet code/src_corelib_text_qregularexpression.cpp 22

    You can get more information about the specific error by calling the
    errorString() function; moreover, the patternErrorOffset() function
    will return the offset inside the pattern string

    \snippet code/src_corelib_text_qregularexpression.cpp 23

    If a match is attempted with an invalid QRegularExpression, then the
    returned QRegularExpressionMatch object will be invalid as well (that is,
    its \l{QRegularExpressionMatch::}{isValid()} function will return false).
    The same applies for attempting a global match.

    \section1 Unsupported Perl-compatible Regular Expressions Features

    QRegularExpression does not support all the features available in
    Perl-compatible regular expressions. The most notable one is the fact that
    duplicated names for capturing groups are not supported, and using them can
    lead to undefined behavior.

    This may change in a future version of Qt.

    \section1 Debugging Code that Uses QRegularExpression

    QRegularExpression internally uses a just in time compiler (JIT) to
    optimize the execution of the matching algorithm. The JIT makes extensive
    usage of self-modifying code, which can lead debugging tools such as
    Valgrind to crash. You must enable all checks for self-modifying code if
    you want to debug programs using QRegularExpression (for instance, Valgrind's
    \c{--smc-check} command line option). The downside of enabling such checks
    is that your program will run considerably slower.

    To avoid that, the JIT is disabled by default if you compile Qt in debug
    mode. It is possible to override the default and enable or disable the JIT
    usage (both in debug or release mode) by setting the
    \c{QT_ENABLE_REGEXP_JIT} environment variable to a non-zero or zero value
    respectively.

    \sa QRegularExpressionMatch, QRegularExpressionMatchIterator
*/

/*!
    \class QRegularExpressionMatch
    \inmodule QtCore
    \reentrant

    \brief The QRegularExpressionMatch class provides the results of a matching
    a QRegularExpression against a string.

    \since 5.0

    \ingroup tools
    \ingroup shared

    \keyword regular expression match

    A QRegularExpressionMatch object can be obtained by calling the
    QRegularExpression::match() function, or as a single result of a global
    match from a QRegularExpressionMatchIterator.

    The success or the failure of a match attempt can be inspected by calling
    the hasMatch() function. QRegularExpressionMatch also reports a successful
    partial match through the hasPartialMatch() function.

    In addition, QRegularExpressionMatch returns the substrings captured by the
    capturing groups in the pattern string. The implicit capturing group with
    index 0 captures the result of the whole match. The captured() function
    returns each substring captured, either by the capturing group's index or
    by its name:

    \snippet code/src_corelib_text_qregularexpression.cpp 29

    For each captured substring it is possible to query its starting and ending
    offsets in the subject string by calling the capturedStart() and the
    capturedEnd() function, respectively. The length of each captured
    substring is available using the capturedLength() function.

    The convenience function capturedTexts() will return \e{all} the captured
    substrings at once (including the substring matched by the entire pattern)
    in the order they have been captured by capturing groups; that is,
    \c{captured(i) == capturedTexts().at(i)}.

    You can retrieve the QRegularExpression object the subject string was
    matched against by calling the regularExpression() function; the
    match type and the match options are available as well by calling
    the matchType() and the matchOptions() respectively.

    Please refer to the QRegularExpression documentation for more information
    about the Qt regular expression classes.

    \sa QRegularExpression
*/

/*!
    \class QRegularExpressionMatchIterator
    \inmodule QtCore
    \reentrant

    \brief The QRegularExpressionMatchIterator class provides an iterator on
    the results of a global match of a QRegularExpression object against a string.

    \since 5.0

    \ingroup tools
    \ingroup shared

    \keyword regular expression iterator

    A QRegularExpressionMatchIterator object is a forward only Java-like
    iterator; it can be obtained by calling the
    QRegularExpression::globalMatch() function. A new
    QRegularExpressionMatchIterator will be positioned before the first result.
    You can then call the hasNext() function to check if there are more
    results available; if so, the next() function will return the next
    result and advance the iterator.

    Each result is a QRegularExpressionMatch object holding all the information
    for that result (including captured substrings).

    For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 30

    Moreover, QRegularExpressionMatchIterator offers a peekNext() function
    to get the next result \e{without} advancing the iterator.

    Starting with Qt 6.0, it is also possible to simply use the result of
    QRegularExpression::globalMatch in a range-based for loop, for instance
    like this:

    \snippet code/src_corelib_text_qregularexpression.cpp 34

    You can retrieve the QRegularExpression object the subject string was
    matched against by calling the regularExpression() function; the
    match type and the match options are available as well by calling
    the matchType() and the matchOptions() respectively.

    Please refer to the QRegularExpression documentation for more information
    about the Qt regular expression classes.

    \sa QRegularExpression, QRegularExpressionMatch
*/


/*!
    \enum QRegularExpression::PatternOption

    The PatternOption enum defines modifiers to the way the pattern string
    should be interpreted, and therefore the way the pattern matches against a
    subject string.

    \value NoPatternOption
        No pattern options are set.

    \value CaseInsensitiveOption
        The pattern should match against the subject string in a case
        insensitive way. This option corresponds to the /i modifier in Perl
        regular expressions.

    \value DotMatchesEverythingOption
        The dot metacharacter (\c{.}) in the pattern string is allowed to match
        any character in the subject string, including newlines (normally, the
        dot does not match newlines). This option corresponds to the \c{/s}
        modifier in Perl regular expressions.

    \value MultilineOption
        The caret (\c{^}) and the dollar (\c{$}) metacharacters in the pattern
        string are allowed to match, respectively, immediately after and
        immediately before any newline in the subject string, as well as at the
        very beginning and at the very end of the subject string. This option
        corresponds to the \c{/m} modifier in Perl regular expressions.

    \value ExtendedPatternSyntaxOption
        Any whitespace in the pattern string which is not escaped and outside a
        character class is ignored. Moreover, an unescaped sharp (\b{#})
        outside a character class causes all the following characters, until
        the first newline (included), to be ignored. This can be used to
        increase the readability of a pattern string as well as put comments
        inside regular expressions; this is particularly useful if the pattern
        string is loaded from a file or written by the user, because in C++
        code it is always possible to use the rules for string literals to put
        comments outside the pattern string. This option corresponds to the \c{/x}
        modifier in Perl regular expressions.

    \value InvertedGreedinessOption
        The greediness of the quantifiers is inverted: \c{*}, \c{+}, \c{?},
        \c{{m,n}}, etc. become lazy, while their lazy versions (\c{*?},
        \c{+?}, \c{??}, \c{{m,n}?}, etc.) become greedy. There is no equivalent
        for this option in Perl regular expressions.

    \value DontCaptureOption
        The non-named capturing groups do not capture substrings; named
        capturing groups still work as intended, as well as the implicit
        capturing group number 0 corresponding to the entire match. There is no
        equivalent for this option in Perl regular expressions.

    \value UseUnicodePropertiesOption
        The meaning of the \c{\w}, \c{\d}, etc., character classes, as well as
        the meaning of their counterparts (\c{\W}, \c{\D}, etc.), is changed
        from matching ASCII characters only to matching any character with the
        corresponding Unicode property. For instance, \c{\d} is changed to
        match any character with the Unicode Nd (decimal digit) property;
        \c{\w} to match any character with either the Unicode L (letter) or N
        (digit) property, plus underscore, and so on. This option corresponds
        to the \c{/u} modifier in Perl regular expressions.
*/

/*!
    \enum QRegularExpression::MatchType

    The MatchType enum defines the type of the match that should be attempted
    against the subject string.

    \value NormalMatch
        A normal match is done.

    \value PartialPreferCompleteMatch
        The pattern string is matched partially against the subject string. If
        a partial match is found, then it is recorded, and other matching
        alternatives are tried as usual. If a complete match is then found,
        then it's preferred to the partial match; in this case only the
        complete match is reported. If instead no complete match is found (but
        only the partial one), then the partial one is reported.

    \value PartialPreferFirstMatch
        The pattern string is matched partially against the subject string. If
        a partial match is found, then matching stops and the partial match is
        reported. In this case, other matching alternatives (potentially
        leading to a complete match) are not tried. Moreover, this match type
        assumes that the subject string only a substring of a larger text, and
        that (in this text) there are other characters beyond the end of the
        subject string. This can lead to surprising results; see the discussion
        in the \l{partial matching} section for more details.

    \value NoMatch
        No matching is done. This value is returned as the match type by a
        default constructed QRegularExpressionMatch or
        QRegularExpressionMatchIterator. Using this match type is not very
        useful for the user, as no matching ever happens. This enum value
        has been introduced in Qt 5.1.
*/

/*!
    \enum QRegularExpression::MatchOption

    \value NoMatchOption
        No match options are set.

    \value AnchoredMatchOption
        Use AnchorAtOffsetMatchOption instead.

    \value AnchorAtOffsetMatchOption
        The match is constrained to start exactly at the offset passed to
        match() in order to be successful, even if the pattern string does not
        contain any metacharacter that anchors the match at that point.
        Note that passing this option does not anchor the end of the match
        to the end of the subject; if you want to fully anchor a regular
        expression, use anchoredPattern().
        This enum value has been introduced in Qt 6.0.

    \value DontCheckSubjectStringMatchOption
        The subject string is not checked for UTF-16 validity before
        attempting a match. Use this option with extreme caution, as
        attempting to match an invalid string may crash the program and/or
        constitute a security issue. This enum value has been introduced in
        Qt 5.4.
*/

/*!
    \internal
*/
static int convertToPcreOptions(QRegularExpression::PatternOptions patternOptions)
{
    int options = 0;

    if (patternOptions & QRegularExpression::CaseInsensitiveOption)
        options |= PCRE2_CASELESS;
    if (patternOptions & QRegularExpression::DotMatchesEverythingOption)
        options |= PCRE2_DOTALL;
    if (patternOptions & QRegularExpression::MultilineOption)
        options |= PCRE2_MULTILINE;
    if (patternOptions & QRegularExpression::ExtendedPatternSyntaxOption)
        options |= PCRE2_EXTENDED;
    if (patternOptions & QRegularExpression::InvertedGreedinessOption)
        options |= PCRE2_UNGREEDY;
    if (patternOptions & QRegularExpression::DontCaptureOption)
        options |= PCRE2_NO_AUTO_CAPTURE;
    if (patternOptions & QRegularExpression::UseUnicodePropertiesOption)
        options |= PCRE2_UCP;

    return options;
}

/*!
    \internal
*/
static int convertToPcreOptions(QRegularExpression::MatchOptions matchOptions)
{
    int options = 0;

    if (matchOptions & QRegularExpression::AnchorAtOffsetMatchOption)
        options |= PCRE2_ANCHORED;
    if (matchOptions & QRegularExpression::DontCheckSubjectStringMatchOption)
        options |= PCRE2_NO_UTF_CHECK;

    return options;
}

struct QRegularExpressionPrivate : QSharedData
{
    QRegularExpressionPrivate();
    ~QRegularExpressionPrivate();
    QRegularExpressionPrivate(const QRegularExpressionPrivate &other);

    void cleanCompiledPattern();
    void compilePattern();
    void getPatternInfo();
    void optimizePattern();

    enum CheckSubjectStringOption {
        CheckSubjectString,
        DontCheckSubjectString
    };

    void doMatch(QRegularExpressionMatchPrivate *priv,
                 qsizetype offset,
                 CheckSubjectStringOption checkSubjectStringOption = CheckSubjectString,
                 const QRegularExpressionMatchPrivate *previous = nullptr) const;

    int captureIndexForName(QStringView name) const;

    // sizeof(QSharedData) == 4, so start our members with an enum
    QRegularExpression::PatternOptions patternOptions;
    QString pattern;

    // *All* of the following members are managed while holding this mutex,
    // except for isDirty which is set to true by QRegularExpression setters
    // (right after a detach happened).
    mutable QMutex mutex;

    // The PCRE code pointer is reference-counted by the QRegularExpressionPrivate
    // objects themselves; when the private is copied (i.e. a detach happened)
    // it is set to nullptr
    pcre2_code_16 *compiledPattern;
    int errorCode;
    qsizetype errorOffset;
    int capturingCount;
    bool usingCrLfNewlines;
    bool isDirty;
};

struct QRegularExpressionMatchPrivate : QSharedData
{
    QRegularExpressionMatchPrivate(const QRegularExpression &re,
                                   const QString &subjectStorage,
                                   QStringView subject,
                                   QRegularExpression::MatchType matchType,
                                   QRegularExpression::MatchOptions matchOptions);

    QRegularExpressionMatch nextMatch() const;

    const QRegularExpression regularExpression;

    // subject is what we match upon. If we've been asked to match over
    // a QString, then subjectStorage is a copy of that string
    // (so that it's kept alive by us)
    const QString subjectStorage;
    const QStringView subject;

    const QRegularExpression::MatchType matchType;
    const QRegularExpression::MatchOptions matchOptions;

    // the capturedOffsets vector contains pairs of (start, end) positions
    // for each captured substring
    QList<qsizetype> capturedOffsets;

    int capturedCount = 0;

    bool hasMatch = false;
    bool hasPartialMatch = false;
    bool isValid = false;
};

struct QRegularExpressionMatchIteratorPrivate : QSharedData
{
    QRegularExpressionMatchIteratorPrivate(const QRegularExpression &re,
                                           QRegularExpression::MatchType matchType,
                                           QRegularExpression::MatchOptions matchOptions,
                                           const QRegularExpressionMatch &next);

    bool hasNext() const;
    QRegularExpressionMatch next;
    const QRegularExpression regularExpression;
    const QRegularExpression::MatchType matchType;
    const QRegularExpression::MatchOptions matchOptions;
};

/*!
    \internal

    Used to centralize the warning about using an invalid QRegularExpression.
    In case the pattern is an illegal UTF-16 string, we can't pass print it
    (pass it to qUtf16Printable, etc.), so we need to check for that.
*/
Q_DECL_COLD_FUNCTION
void qtWarnAboutInvalidRegularExpression(const QString &pattern, const char *where)
{
    if (pattern.isValidUtf16()) {
        qWarning("%s(): called on an invalid QRegularExpression object "
                 "(pattern is '%ls')", where, qUtf16Printable(pattern));
    } else {
        qWarning("%s(): called on an invalid QRegularExpression object", where);
    }
}

/*!
    \internal
*/
QRegularExpression::QRegularExpression(QRegularExpressionPrivate &dd)
    : d(&dd)
{
}

/*!
    \internal
*/
QRegularExpressionPrivate::QRegularExpressionPrivate()
    : QSharedData(),
      patternOptions(),
      pattern(),
      mutex(),
      compiledPattern(nullptr),
      errorCode(0),
      errorOffset(-1),
      capturingCount(0),
      usingCrLfNewlines(false),
      isDirty(true)
{
}

/*!
    \internal
*/
QRegularExpressionPrivate::~QRegularExpressionPrivate()
{
    cleanCompiledPattern();
}

/*!
    \internal

    Copies the private, which means copying only the pattern and the pattern
    options. The compiledPattern pointer is NOT copied (we
    do not own it any more), and in general all the members set when
    compiling a pattern are set to default values. isDirty is set back to true
    so that the pattern has to be recompiled again.
*/
QRegularExpressionPrivate::QRegularExpressionPrivate(const QRegularExpressionPrivate &other)
    : QSharedData(other),
      patternOptions(other.patternOptions),
      pattern(other.pattern),
      mutex(),
      compiledPattern(nullptr),
      errorCode(0),
      errorOffset(-1),
      capturingCount(0),
      usingCrLfNewlines(false),
      isDirty(true)
{
}

/*!
    \internal
*/
void QRegularExpressionPrivate::cleanCompiledPattern()
{
    pcre2_code_free_16(compiledPattern);
    compiledPattern = nullptr;
    errorCode = 0;
    errorOffset = -1;
    capturingCount = 0;
    usingCrLfNewlines = false;
}

/*!
    \internal
*/
void QRegularExpressionPrivate::compilePattern()
{
    const QMutexLocker lock(&mutex);

    if (!isDirty)
        return;

    isDirty = false;
    cleanCompiledPattern();

    int options = convertToPcreOptions(patternOptions);
    options |= PCRE2_UTF;

    PCRE2_SIZE patternErrorOffset;
    compiledPattern = pcre2_compile_16(reinterpret_cast<PCRE2_SPTR16>(pattern.constData()),
                                       pattern.size(),
                                       options,
                                       &errorCode,
                                       &patternErrorOffset,
                                       nullptr);

    if (!compiledPattern) {
        errorOffset = qsizetype(patternErrorOffset);
        return;
    } else {
        // ignore whatever PCRE2 wrote into errorCode -- leave it to 0 to mean "no error"
        errorCode = 0;
    }

    optimizePattern();
    getPatternInfo();
}

/*!
    \internal
*/
void QRegularExpressionPrivate::getPatternInfo()
{
    Q_ASSERT(compiledPattern);

    pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_CAPTURECOUNT, &capturingCount);

    // detect the settings for the newline
    unsigned int patternNewlineSetting;
    if (pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_NEWLINE, &patternNewlineSetting) != 0) {
        // no option was specified in the regexp, grab PCRE build defaults
        pcre2_config_16(PCRE2_CONFIG_NEWLINE, &patternNewlineSetting);
    }

    usingCrLfNewlines = (patternNewlineSetting == PCRE2_NEWLINE_CRLF) ||
            (patternNewlineSetting == PCRE2_NEWLINE_ANY) ||
            (patternNewlineSetting == PCRE2_NEWLINE_ANYCRLF);

    unsigned int hasJOptionChanged;
    pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_JCHANGED, &hasJOptionChanged);
    if (Q_UNLIKELY(hasJOptionChanged)) {
        qWarning("QRegularExpressionPrivate::getPatternInfo(): the pattern '%ls'\n    is using the (?J) option; duplicate capturing group names are not supported by Qt",
                 qUtf16Printable(pattern));
    }
}


/*
    Simple "smartpointer" wrapper around a pcre2_jit_stack_16, to be used with
    QThreadStorage.
*/
namespace {
struct PcreJitStackFree
{
    void operator()(pcre2_jit_stack_16 *stack)
    {
        if (stack)
            pcre2_jit_stack_free_16(stack);
    }
};
Q_CONSTINIT static thread_local std::unique_ptr<pcre2_jit_stack_16, PcreJitStackFree> jitStacks;
}

/*!
    \internal
*/
static pcre2_jit_stack_16 *qtPcreCallback(void *)
{
    return jitStacks.get();
}

/*!
    \internal
*/
static bool isJitEnabled()
{
    QByteArray jitEnvironment = qgetenv("QT_ENABLE_REGEXP_JIT");
    if (!jitEnvironment.isEmpty()) {
        bool ok;
        int enableJit = jitEnvironment.toInt(&ok);
        return ok ? (enableJit != 0) : true;
    }

#ifdef QT_DEBUG
    return false;
#elif defined(Q_OS_MACOS)
    return !qt_mac_runningUnderRosetta();
#else
    return true;
#endif
}

/*!
    \internal

    The purpose of the function is to call pcre2_jit_compile_16, which
    JIT-compiles the pattern.

    It gets called when a pattern is recompiled by us (in compilePattern()),
    under mutex protection.
*/
void QRegularExpressionPrivate::optimizePattern()
{
    Q_ASSERT(compiledPattern);

    static const bool enableJit = isJitEnabled();

    if (!enableJit)
        return;

    pcre2_jit_compile_16(compiledPattern, PCRE2_JIT_COMPLETE | PCRE2_JIT_PARTIAL_SOFT | PCRE2_JIT_PARTIAL_HARD);
}

/*!
    \internal

    Returns the capturing group number for the given name. Duplicated names for
    capturing groups are not supported.
*/
int QRegularExpressionPrivate::captureIndexForName(QStringView name) const
{
    Q_ASSERT(!name.isEmpty());

    if (!compiledPattern)
        return -1;

    // See the other usages of pcre2_pattern_info_16 for more details about this
    PCRE2_SPTR16 *namedCapturingTable;
    unsigned int namedCapturingTableEntryCount;
    unsigned int namedCapturingTableEntrySize;

    pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_NAMETABLE, &namedCapturingTable);
    pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_NAMECOUNT, &namedCapturingTableEntryCount);
    pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_NAMEENTRYSIZE, &namedCapturingTableEntrySize);

    for (unsigned int i = 0; i < namedCapturingTableEntryCount; ++i) {
        const auto currentNamedCapturingTableRow =
                reinterpret_cast<const char16_t *>(namedCapturingTable) + namedCapturingTableEntrySize * i;

        if (name == (currentNamedCapturingTableRow + 1)) {
            const int index = *currentNamedCapturingTableRow;
            return index;
        }
    }

    return -1;
}

/*!
    \internal

    This is a simple wrapper for pcre2_match_16 for handling the case in which the
    JIT runs out of memory. In that case, we allocate a thread-local JIT stack
    and re-run pcre2_match_16.
*/
static int safe_pcre2_match_16(const pcre2_code_16 *code,
                               PCRE2_SPTR16 subject, qsizetype length,
                               qsizetype startOffset, int options,
                               pcre2_match_data_16 *matchData,
                               pcre2_match_context_16 *matchContext)
{
    int result = pcre2_match_16(code, subject, length,
                                startOffset, options, matchData, matchContext);

    if (result == PCRE2_ERROR_JIT_STACKLIMIT && !jitStacks) {
        // The default JIT stack size in PCRE is 32K,
        // we allocate from 32K up to 512K.
        jitStacks.reset(pcre2_jit_stack_create_16(32 * 1024, 512 * 1024, NULL));

        result = pcre2_match_16(code, subject, length,
                                startOffset, options, matchData, matchContext);
    }

    return result;
}

/*!
    \internal

    Performs a match on the subject string view held by \a priv. The
    match will be of type priv->matchType and using the options
    priv->matchOptions; the matching \a offset is relative the
    substring, and if negative, it's taken as an offset from the end of
    the substring.

    It also advances a match if a previous result is given as \a
    previous. The subject string goes a Unicode validity check if
    \a checkSubjectString is CheckSubjectString and the match options don't
    include DontCheckSubjectStringMatchOption (PCRE doesn't like illegal
    UTF-16 sequences).

    \a priv is modified to hold the results of the match.

    Advancing a match is a tricky algorithm. If the previous match matched a
    non-empty string, we just do an ordinary match at the offset position.

    If the previous match matched an empty string, then an anchored, non-empty
    match is attempted at the offset position. If that succeeds, then we got
    the next match and we can return it. Otherwise, we advance by 1 position
    (which can be one or two code units in UTF-16!) and reattempt a "normal"
    match. We also have the problem of detecting the current newline format: if
    the new advanced offset is pointing to the beginning of a CRLF sequence, we
    must advance over it.
*/
void QRegularExpressionPrivate::doMatch(QRegularExpressionMatchPrivate *priv,
                                        qsizetype offset,
                                        CheckSubjectStringOption checkSubjectStringOption,
                                        const QRegularExpressionMatchPrivate *previous) const
{
    Q_ASSERT(priv);
    Q_ASSUME(priv != previous);

    const qsizetype subjectLength = priv->subject.size();

    if (offset < 0)
        offset += subjectLength;

    if (offset < 0 || offset > subjectLength)
        return;

    if (Q_UNLIKELY(!compiledPattern)) {
        qtWarnAboutInvalidRegularExpression(pattern, "QRegularExpressionPrivate::doMatch");
        return;
    }

    // skip doing the actual matching if NoMatch type was requested
    if (priv->matchType == QRegularExpression::NoMatch) {
        priv->isValid = true;
        return;
    }

    int pcreOptions = convertToPcreOptions(priv->matchOptions);

    if (priv->matchType == QRegularExpression::PartialPreferCompleteMatch)
        pcreOptions |= PCRE2_PARTIAL_SOFT;
    else if (priv->matchType == QRegularExpression::PartialPreferFirstMatch)
        pcreOptions |= PCRE2_PARTIAL_HARD;

    if (checkSubjectStringOption == DontCheckSubjectString)
        pcreOptions |= PCRE2_NO_UTF_CHECK;

    bool previousMatchWasEmpty = false;
    if (previous && previous->hasMatch &&
            (previous->capturedOffsets.at(0) == previous->capturedOffsets.at(1))) {
        previousMatchWasEmpty = true;
    }

    pcre2_match_context_16 *matchContext = pcre2_match_context_create_16(nullptr);
    pcre2_jit_stack_assign_16(matchContext, &qtPcreCallback, nullptr);
    pcre2_match_data_16 *matchData = pcre2_match_data_create_from_pattern_16(compiledPattern, nullptr);

    // PCRE does not accept a null pointer as subject string, even if
    // its length is zero. We however allow it in input: a QStringView
    // subject may have data == nullptr. In this case, to keep PCRE
    // happy, pass a pointer to a dummy character.
    const char16_t dummySubject = 0;
    const char16_t * const subjectUtf16 = [&]()
    {
        const auto subjectUtf16 = priv->subject.utf16();
        if (subjectUtf16)
            return subjectUtf16;
        Q_ASSERT(subjectLength == 0);
        return &dummySubject;
    }();

    int result;

    if (!previousMatchWasEmpty) {
        result = safe_pcre2_match_16(compiledPattern,
                                     reinterpret_cast<PCRE2_SPTR16>(subjectUtf16), subjectLength,
                                     offset, pcreOptions,
                                     matchData, matchContext);
    } else {
        result = safe_pcre2_match_16(compiledPattern,
                                     reinterpret_cast<PCRE2_SPTR16>(subjectUtf16), subjectLength,
                                     offset, pcreOptions | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED,
                                     matchData, matchContext);

        if (result == PCRE2_ERROR_NOMATCH) {
            ++offset;

            if (usingCrLfNewlines
                    && offset < subjectLength
                    && subjectUtf16[offset - 1] == u'\r'
                    && subjectUtf16[offset] == u'\n') {
                ++offset;
            } else if (offset < subjectLength
                       && QChar::isLowSurrogate(subjectUtf16[offset])) {
                ++offset;
            }

            result = safe_pcre2_match_16(compiledPattern,
                                         reinterpret_cast<PCRE2_SPTR16>(subjectUtf16), subjectLength,
                                         offset, pcreOptions,
                                         matchData, matchContext);
        }
    }

#ifdef QREGULAREXPRESSION_DEBUG
    qDebug() << "Matching" <<  pattern << "against" << subject
             << "offset" << offset
             << priv->matchType << priv->matchOptions << previousMatchWasEmpty
             << "result" << result;
#endif

    // result == 0 means not enough space in captureOffsets; should never happen
    Q_ASSERT(result != 0);

    if (result > 0) {
        // full match
        priv->isValid = true;
        priv->hasMatch = true;
        priv->capturedCount = result;
        priv->capturedOffsets.resize(result * 2);
    } else {
        // no match, partial match or error
        priv->hasPartialMatch = (result == PCRE2_ERROR_PARTIAL);
        priv->isValid = (result == PCRE2_ERROR_NOMATCH || result == PCRE2_ERROR_PARTIAL);

        if (result == PCRE2_ERROR_PARTIAL) {
            // partial match:
            // leave the start and end capture offsets (i.e. cap(0))
            priv->capturedCount = 1;
            priv->capturedOffsets.resize(2);
        } else {
            // no match or error
            priv->capturedCount = 0;
            priv->capturedOffsets.clear();
        }
    }

    // copy the captured substrings offsets, if any
    if (priv->capturedCount) {
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_16(matchData);
        qsizetype *const capturedOffsets = priv->capturedOffsets.data();

        // We rely on the fact that capturing groups that did not
        // capture anything have offset -1, but PCRE technically
        // returns "PCRE2_UNSET". Test that out, better safe than
        // sorry...
        static_assert(qsizetype(PCRE2_UNSET) == qsizetype(-1), "Internal error: PCRE2 changed its API");

        for (int i = 0; i < priv->capturedCount * 2; ++i)
            capturedOffsets[i] = qsizetype(ovector[i]);

        // For partial matches, PCRE2 and PCRE1 differ in behavior when lookbehinds
        // are involved. PCRE2 reports the real begin of the match and the maximum
        // used lookbehind as distinct information; PCRE1 instead automatically
        // adjusted ovector[0] to include the maximum lookbehind.
        //
        // For instance, given the pattern "\bstring\b", and the subject "a str":
        // * PCRE1 reports partial, capturing " str"
        // * PCRE2 reports partial, capturing "str" with a lookbehind of 1
        //
        // To keep behavior, emulate PCRE1 here.
        // (Eventually, we could expose the lookbehind info in a future patch.)
        if (result == PCRE2_ERROR_PARTIAL) {
            unsigned int maximumLookBehind;
            pcre2_pattern_info_16(compiledPattern, PCRE2_INFO_MAXLOOKBEHIND, &maximumLookBehind);
            capturedOffsets[0] -= maximumLookBehind;
        }
    }

    pcre2_match_data_free_16(matchData);
    pcre2_match_context_free_16(matchContext);
}

/*!
    \internal
*/
QRegularExpressionMatchPrivate::QRegularExpressionMatchPrivate(const QRegularExpression &re,
                                                               const QString &subjectStorage,
                                                               QStringView subject,
                                                               QRegularExpression::MatchType matchType,
                                                               QRegularExpression::MatchOptions matchOptions)
    : regularExpression(re),
      subjectStorage(subjectStorage),
      subject(subject),
      matchType(matchType),
      matchOptions(matchOptions)
{
}

/*!
    \internal
*/
QRegularExpressionMatch QRegularExpressionMatchPrivate::nextMatch() const
{
    Q_ASSERT(isValid);
    Q_ASSERT(hasMatch || hasPartialMatch);

    auto nextPrivate = new QRegularExpressionMatchPrivate(regularExpression,
                                                          subjectStorage,
                                                          subject,
                                                          matchType,
                                                          matchOptions);

    // Note the DontCheckSubjectString passed for the check of the subject string:
    // if we're advancing a match on the same subject,
    // then that subject was already checked at least once (when this object
    // was created, or when the object that created this one was created, etc.)
    regularExpression.d->doMatch(nextPrivate,
                                 capturedOffsets.at(1),
                                 QRegularExpressionPrivate::DontCheckSubjectString,
                                 this);
    return QRegularExpressionMatch(*nextPrivate);
}

/*!
    \internal
*/
QRegularExpressionMatchIteratorPrivate::QRegularExpressionMatchIteratorPrivate(const QRegularExpression &re,
                                                                               QRegularExpression::MatchType matchType,
                                                                               QRegularExpression::MatchOptions matchOptions,
                                                                               const QRegularExpressionMatch &next)
    : next(next),
      regularExpression(re),
      matchType(matchType), matchOptions(matchOptions)
{
}

/*!
    \internal
*/
bool QRegularExpressionMatchIteratorPrivate::hasNext() const
{
    return next.isValid() && (next.hasMatch() || next.hasPartialMatch());
}

// PUBLIC API

/*!
    Constructs a QRegularExpression object with an empty pattern and no pattern
    options.

    \sa setPattern(), setPatternOptions()
*/
QRegularExpression::QRegularExpression()
    : d(new QRegularExpressionPrivate)
{
}

/*!
    Constructs a QRegularExpression object using the given \a pattern as
    pattern and the \a options as the pattern options.

    \sa setPattern(), setPatternOptions()
*/
QRegularExpression::QRegularExpression(const QString &pattern, PatternOptions options)
    : d(new QRegularExpressionPrivate)
{
    d->pattern = pattern;
    d->patternOptions = options;
}

/*!
    Constructs a QRegularExpression object as a copy of \a re.

    \sa operator=()
*/
QRegularExpression::QRegularExpression(const QRegularExpression &re) noexcept = default;

/*!
    \fn QRegularExpression::QRegularExpression(QRegularExpression &&re)

    \since 6.1

    Constructs a QRegularExpression object by moving from \a re.

    Note that a moved-from QRegularExpression can only be destroyed or
    assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.

    \sa operator=()
*/

/*!
    Destroys the QRegularExpression object.
*/
QRegularExpression::~QRegularExpression()
{
}

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QRegularExpressionPrivate)

/*!
    Assigns the regular expression \a re to this object, and returns a reference
    to the copy. Both the pattern and the pattern options are copied.
*/
QRegularExpression &QRegularExpression::operator=(const QRegularExpression &re) noexcept = default;

/*!
    \fn void QRegularExpression::swap(QRegularExpression &other)

    Swaps the regular expression \a other with this regular expression. This
    operation is very fast and never fails.
*/

/*!
    Returns the pattern string of the regular expression.

    \sa setPattern(), patternOptions()
*/
QString QRegularExpression::pattern() const
{
    return d->pattern;
}

/*!
    Sets the pattern string of the regular expression to \a pattern. The
    pattern options are left unchanged.

    \sa pattern(), setPatternOptions()
*/
void QRegularExpression::setPattern(const QString &pattern)
{
    if (d->pattern == pattern)
        return;
    d.detach();
    d->isDirty = true;
    d->pattern = pattern;
}

/*!
    Returns the pattern options for the regular expression.

    \sa setPatternOptions(), pattern()
*/
QRegularExpression::PatternOptions QRegularExpression::patternOptions() const
{
    return d->patternOptions;
}

/*!
    Sets the given \a options as the pattern options of the regular expression.
    The pattern string is left unchanged.

    \sa patternOptions(), setPattern()
*/
void QRegularExpression::setPatternOptions(PatternOptions options)
{
    if (d->patternOptions == options)
        return;
    d.detach();
    d->isDirty = true;
    d->patternOptions = options;
}

/*!
    Returns the number of capturing groups inside the pattern string,
    or -1 if the regular expression is not valid.

    \note The implicit capturing group 0 is \e{not} included in the returned number.

    \sa isValid()
*/
int QRegularExpression::captureCount() const
{
    if (!isValid()) // will compile the pattern
        return -1;
    return d->capturingCount;
}

/*!
    \since 5.1

    Returns a list of captureCount() + 1 elements, containing the names of the
    named capturing groups in the pattern string. The list is sorted such that
    the element of the list at position \c{i} is the name of the \c{i}-th
    capturing group, if it has a name, or an empty string if that capturing
    group is unnamed.

    For instance, given the regular expression

    \snippet code/src_corelib_text_qregularexpression.cpp 32

    namedCaptureGroups() will return the following list:

    \snippet code/src_corelib_text_qregularexpression.cpp 33

    which corresponds to the fact that the capturing group #0 (corresponding to
    the whole match) has no name, the capturing group #1 has name "day", the
    capturing group #2 has name "month", etc.

    If the regular expression is not valid, returns an empty list.

    \sa isValid(), QRegularExpressionMatch::captured(), QString::isEmpty()
*/
QStringList QRegularExpression::namedCaptureGroups() const
{
    if (!isValid()) // isValid() will compile the pattern
        return QStringList();

    // namedCapturingTable will point to a table of
    // namedCapturingTableEntryCount entries, each one of which
    // contains one ushort followed by the name, NUL terminated.
    // The ushort is the numerical index of the name in the pattern.
    // The length of each entry is namedCapturingTableEntrySize.
    PCRE2_SPTR16 *namedCapturingTable;
    unsigned int namedCapturingTableEntryCount;
    unsigned int namedCapturingTableEntrySize;

    pcre2_pattern_info_16(d->compiledPattern, PCRE2_INFO_NAMETABLE, &namedCapturingTable);
    pcre2_pattern_info_16(d->compiledPattern, PCRE2_INFO_NAMECOUNT, &namedCapturingTableEntryCount);
    pcre2_pattern_info_16(d->compiledPattern, PCRE2_INFO_NAMEENTRYSIZE, &namedCapturingTableEntrySize);

    // The +1 is for the implicit group #0
    QStringList result(d->capturingCount + 1);

    for (unsigned int i = 0; i < namedCapturingTableEntryCount; ++i) {
        const auto currentNamedCapturingTableRow =
                reinterpret_cast<const char16_t *>(namedCapturingTable) + namedCapturingTableEntrySize * i;

        const int index = *currentNamedCapturingTableRow;
        result[index] = QString::fromUtf16(currentNamedCapturingTableRow + 1);
    }

    return result;
}

/*!
    Returns \c true if the regular expression is a valid regular expression (that
    is, it contains no syntax errors, etc.), or false otherwise. Use
    errorString() to obtain a textual description of the error.

    \sa errorString(), patternErrorOffset()
*/
bool QRegularExpression::isValid() const
{
    d.data()->compilePattern();
    return d->compiledPattern;
}

/*!
    Returns a textual description of the error found when checking the validity
    of the regular expression, or "no error" if no error was found.

    \sa isValid(), patternErrorOffset()
*/
QString QRegularExpression::errorString() const
{
    d.data()->compilePattern();
    if (d->errorCode) {
        QString errorString;
        int errorStringLength;
        do {
            errorString.resize(errorString.size() + 64);
            errorStringLength = pcre2_get_error_message_16(d->errorCode,
                                                           reinterpret_cast<ushort *>(errorString.data()),
                                                           errorString.size());
        } while (errorStringLength < 0);
        errorString.resize(errorStringLength);

#ifdef QT_NO_TRANSLATION
        return errorString;
#else
        return QCoreApplication::translate("QRegularExpression", std::move(errorString).toLatin1().constData());
#endif
    }
#ifdef QT_NO_TRANSLATION
        return u"no error"_s;
#else
    return QCoreApplication::translate("QRegularExpression", "no error");
#endif
}

/*!
    Returns the offset, inside the pattern string, at which an error was found
    when checking the validity of the regular expression. If no error was
    found, then -1 is returned.

    \sa pattern(), isValid(), errorString()
*/
qsizetype QRegularExpression::patternErrorOffset() const
{
    d.data()->compilePattern();
    return d->errorOffset;
}

/*!
    Attempts to match the regular expression against the given \a subject
    string, starting at the position \a offset inside the subject, using a
    match of type \a matchType and honoring the given \a matchOptions.

    The returned QRegularExpressionMatch object contains the results of the
    match.

    \sa QRegularExpressionMatch, {normal matching}
*/
QRegularExpressionMatch QRegularExpression::match(const QString &subject,
                                                  qsizetype offset,
                                                  MatchType matchType,
                                                  MatchOptions matchOptions) const
{
    d.data()->compilePattern();
    auto priv = new QRegularExpressionMatchPrivate(*this,
                                                   subject,
                                                   QStringView(subject),
                                                   matchType,
                                                   matchOptions);
    d->doMatch(priv, offset);
    return QRegularExpressionMatch(*priv);
}

#if QT_DEPRECATED_SINCE(6, 8)
/*!
    \since 6.0
    \overload
    \obsolete

    Use matchView() instead.
*/
QRegularExpressionMatch QRegularExpression::match(QStringView subjectView,
                                                  qsizetype offset,
                                                  MatchType matchType,
                                                  MatchOptions matchOptions) const
{
    return matchView(subjectView, offset, matchType, matchOptions);
}
#endif // QT_DEPRECATED_SINCE(6, 8)

/*!
    \since 6.5
    \overload

    Attempts to match the regular expression against the given \a subjectView
    string view, starting at the position \a offset inside the subject, using a
    match of type \a matchType and honoring the given \a matchOptions.

    The returned QRegularExpressionMatch object contains the results of the
    match.

    \note The data referenced by \a subjectView must remain valid as long
    as there are QRegularExpressionMatch objects using it.

    \sa QRegularExpressionMatch, {normal matching}
*/
QRegularExpressionMatch QRegularExpression::matchView(QStringView subjectView,
                                                      qsizetype offset,
                                                      MatchType matchType,
                                                      MatchOptions matchOptions) const
{
    d.data()->compilePattern();
    auto priv = new QRegularExpressionMatchPrivate(*this,
                                                   QString(),
                                                   subjectView,
                                                   matchType,
                                                   matchOptions);
    d->doMatch(priv, offset);
    return QRegularExpressionMatch(*priv);
}

/*!
    Attempts to perform a global match of the regular expression against the
    given \a subject string, starting at the position \a offset inside the
    subject, using a match of type \a matchType and honoring the given \a
    matchOptions.

    The returned QRegularExpressionMatchIterator is positioned before the
    first match result (if any).

    \sa QRegularExpressionMatchIterator, {global matching}
*/
QRegularExpressionMatchIterator QRegularExpression::globalMatch(const QString &subject,
                                                                qsizetype offset,
                                                                MatchType matchType,
                                                                MatchOptions matchOptions) const
{
    QRegularExpressionMatchIteratorPrivate *priv =
            new QRegularExpressionMatchIteratorPrivate(*this,
                                                       matchType,
                                                       matchOptions,
                                                       match(subject, offset, matchType, matchOptions));

    return QRegularExpressionMatchIterator(*priv);
}

#if QT_DEPRECATED_SINCE(6, 8)
/*!
    \since 6.0
    \overload
    \obsolete

    Use globalMatchView() instead.
*/
QRegularExpressionMatchIterator QRegularExpression::globalMatch(QStringView subjectView,
                                                                qsizetype offset,
                                                                MatchType matchType,
                                                                MatchOptions matchOptions) const
{
    return globalMatchView(subjectView, offset, matchType, matchOptions);
}
#endif // QT_DEPRECATED_SINCE(6, 8)

/*!
    \since 6.5
    \overload

    Attempts to perform a global match of the regular expression against the
    given \a subjectView string view, starting at the position \a offset inside the
    subject, using a match of type \a matchType and honoring the given \a
    matchOptions.

    The returned QRegularExpressionMatchIterator is positioned before the
    first match result (if any).

    \note The data referenced by \a subjectView must remain valid as
    long as there are QRegularExpressionMatchIterator or
    QRegularExpressionMatch objects using it.

    \sa QRegularExpressionMatchIterator, {global matching}
*/
QRegularExpressionMatchIterator QRegularExpression::globalMatchView(QStringView subjectView,
                                                                    qsizetype offset,
                                                                    MatchType matchType,
                                                                    MatchOptions matchOptions) const
{
    QRegularExpressionMatchIteratorPrivate *priv =
            new QRegularExpressionMatchIteratorPrivate(*this,
                                                       matchType,
                                                       matchOptions,
                                                       matchView(subjectView, offset, matchType, matchOptions));

    return QRegularExpressionMatchIterator(*priv);
}

/*!
    \since 5.4

    Compiles the pattern immediately, including JIT compiling it (if
    the JIT is enabled) for optimization.

    \sa isValid(), {Debugging Code that Uses QRegularExpression}
*/
void QRegularExpression::optimize() const
{
    d.data()->compilePattern();
}

/*!
    Returns \c true if the regular expression is equal to \a re, or false
    otherwise. Two QRegularExpression objects are equal if they have
    the same pattern string and the same pattern options.

    \sa operator!=()
*/
bool QRegularExpression::operator==(const QRegularExpression &re) const
{
    return (d == re.d) ||
           (d->pattern == re.d->pattern && d->patternOptions == re.d->patternOptions);
}

/*!
    \fn QRegularExpression & QRegularExpression::operator=(QRegularExpression && re)

    Move-assigns the regular expression \a re to this object, and returns a
    reference to the result. Both the pattern and the pattern options are copied.

    Note that a moved-from QRegularExpression can only be destroyed or
    assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.
*/

/*!
    \fn bool QRegularExpression::operator!=(const QRegularExpression &re) const

    Returns \c true if the regular expression is different from \a re, or
    false otherwise.

    \sa operator==()
*/

/*!
    \since 5.6
    \relates QRegularExpression

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
*/
size_t qHash(const QRegularExpression &key, size_t seed) noexcept
{
    return qHashMulti(seed, key.d->pattern, key.d->patternOptions);
}

/*!
    \fn QString QRegularExpression::escape(const QString &str)
    \overload
*/

/*!
    \since 5.15

    Escapes all characters of \a str so that they no longer have any special
    meaning when used as a regular expression pattern string, and returns
    the escaped string. For instance:

    \snippet code/src_corelib_text_qregularexpression.cpp 26

    This is very convenient in order to build patterns from arbitrary strings:

    \snippet code/src_corelib_text_qregularexpression.cpp 27

    \note This function implements Perl's quotemeta algorithm and escapes with
    a backslash all characters in \a str, except for the characters in the
    \c{[A-Z]}, \c{[a-z]} and \c{[0-9]} ranges, as well as the underscore
    (\c{_}) character. The only difference with Perl is that a literal NUL
    inside \a str is escaped with the sequence \c{"\\0"} (backslash +
    \c{'0'}), instead of \c{"\\\0"} (backslash + \c{NUL}).
*/
QString QRegularExpression::escape(QStringView str)
{
    QString result;
    const qsizetype count = str.size();
    result.reserve(count * 2);

    // everything but [a-zA-Z0-9_] gets escaped,
    // cf. perldoc -f quotemeta
    for (qsizetype i = 0; i < count; ++i) {
        const QChar current = str.at(i);

        if (current == QChar::Null) {
            // unlike Perl, a literal NUL must be escaped with
            // "\\0" (backslash + 0) and not "\\\0" (backslash + NUL),
            // because pcre16_compile uses a NUL-terminated string
            result.append(u'\\');
            result.append(u'0');
        } else if ((current < u'a' || current > u'z') &&
                   (current < u'A' || current > u'Z') &&
                   (current < u'0' || current > u'9') &&
                   current != u'_') {
            result.append(u'\\');
            result.append(current);
            if (current.isHighSurrogate() && i < (count - 1))
                result.append(str.at(++i));
        } else {
            result.append(current);
        }
    }

    result.squeeze();
    return result;
}

/*!
    \since 5.12
    \fn QString QRegularExpression::wildcardToRegularExpression(const QString &pattern, WildcardConversionOptions options)
    \overload
*/

/*!
    \since 6.0
    \enum QRegularExpression::WildcardConversionOption

    The WildcardConversionOption enum defines modifiers to the way a wildcard glob
    pattern gets converted to a regular expression pattern.

    \value DefaultWildcardConversion
        No conversion options are set.

    \value UnanchoredWildcardConversion
        The conversion will not anchor the pattern. This allows for partial string matches of
        wildcard expressions.

    \value [since 6.6] NonPathWildcardConversion
        The conversion will \e{not} interpret the pattern as filepath globbing.

    \sa QRegularExpression::wildcardToRegularExpression
*/

/*!
    \since 5.15

    Returns a regular expression representation of the given glob \a pattern.

    There are two transformations possible, one that targets file path
    globbing, and another one which is more generic.

    By default, the transformation is targeting file path globbing,
    which means in particular that path separators receive special
    treatment. This implies that it is not just a basic translation
    from "*" to ".*" and similar.

    \snippet code/src_corelib_text_qregularexpression.cpp 31

    The more generic globbing transformation is available by passing
    \c NonPathWildcardConversion in the conversion \a options.

    This implementation follows closely the definition
    of wildcard for glob patterns:
    \table
    \row \li \b{c}
         \li Any character represents itself apart from those mentioned
         below. Thus \b{c} matches the character \e c.
    \row \li \b{?}
         \li Matches any single character, except for a path separator
         (in case file path globbing has been selected). It is the
         same as b{.} in full regexps.
    \row \li \b{*}
         \li Matches zero or more of any characters, except for path
         separators (in case file path globbing has been selected). It is the
         same as \b{.*} in full regexps.
    \row \li \b{[abc]}
         \li Matches one character given in the bracket.
    \row \li \b{[a-c]}
         \li Matches one character from the range given in the bracket.
    \row \li \b{[!abc]}
         \li Matches one character that is not given in the bracket. It is the
         same as \b{[^abc]} in full regexp.
    \row \li \b{[!a-c]}
         \li Matches one character that is not from the range given in the
         bracket. It is the same as \b{[^a-c]} in full regexp.
    \endtable

    \note For historical reasons, a backslash (\\) character is \e not
    an escape char in this context. In order to match one of the
    special characters, place it in square brackets (for example,
    \c{[?]}).

    More information about the implementation can be found in:
    \list
    \li \l {https://en.wikipedia.org/wiki/Glob_(programming)} {The Wikipedia Glob article}
    \li \c {man 7 glob}
    \endlist

    By default, the returned regular expression is fully anchored. In other
    words, there is no need of calling anchoredPattern() again on the
    result. To get a regular expression that is not anchored, pass
    UnanchoredWildcardConversion in the conversion \a options.

    \sa escape()
*/
QString QRegularExpression::wildcardToRegularExpression(QStringView pattern, WildcardConversionOptions options)
{
    const qsizetype wclen = pattern.size();
    QString rx;
    rx.reserve(wclen + wclen / 16);
    qsizetype i = 0;
    const QChar *wc = pattern.data();

    struct GlobSettings {
        char16_t nativePathSeparator;
        QStringView starEscape;
        QStringView questionMarkEscape;
    };

    const GlobSettings settings = [options]() {
        if (options.testFlag(NonPathWildcardConversion)) {
            // using [\d\D] to mean "match everything";
            // dot doesn't match newlines, unless in /s mode
            return GlobSettings{ u'\0', u"[\\d\\D]*", u"[\\d\\D]" };
        } else {
#ifdef Q_OS_WIN
            return GlobSettings{ u'\\', u"[^/\\\\]*", u"[^/\\\\]" };
#else
            return GlobSettings{ u'/', u"[^/]*", u"[^/]" };
#endif
        }
    }();

    while (i < wclen) {
        const QChar c = wc[i++];
        switch (c.unicode()) {
        case '*':
            rx += settings.starEscape;
            break;
        case '?':
            rx += settings.questionMarkEscape;
            break;
        // When not using filepath globbing: \ is escaped, / is itself
        // When using filepath globbing:
        // * Unix: \ gets escaped. / is itself
        // * Windows: \ and / can match each other -- they become [/\\] in regexp
        case '\\':
#ifdef Q_OS_WIN
            if (options.testFlag(NonPathWildcardConversion))
                rx += u"\\\\";
            else
                rx += u"[/\\\\]";
            break;
        case '/':
            if (options.testFlag(NonPathWildcardConversion))
                rx += u'/';
            else
                rx += u"[/\\\\]";
            break;
#endif
        case '$':
        case '(':
        case ')':
        case '+':
        case '.':
        case '^':
        case '{':
        case '|':
        case '}':
            rx += u'\\';
            rx += c;
            break;
        case '[':
            rx += c;
            // Support for the [!abc] or [!a-c] syntax
            if (i < wclen) {
                if (wc[i] == u'!') {
                    rx += u'^';
                    ++i;
                }

                if (i < wclen && wc[i] == u']')
                    rx += wc[i++];

                while (i < wclen && wc[i] != u']') {
                    if (!options.testFlag(NonPathWildcardConversion)) {
                        // The '/' appearing in a character class invalidates the
                        // regular expression parsing. It also concerns '\\' on
                        // Windows OS types.
                        if (wc[i] == u'/' || wc[i] == settings.nativePathSeparator)
                            return rx;
                    }
                    if (wc[i] == u'\\')
                        rx += u'\\';
                    rx += wc[i++];
                }
            }
            break;
        default:
            rx += c;
            break;
        }
    }

    if (!(options & UnanchoredWildcardConversion))
        rx = anchoredPattern(rx);

    return rx;
}

/*!
  \since 6.0
  Returns a regular expression of the glob pattern \a pattern. The regular expression
  will be case sensitive if \a cs is \l{Qt::CaseSensitive}, and converted according to
  \a options.

  Equivalent to
  \code
  auto reOptions = cs == Qt::CaseSensitive ? QRegularExpression::NoPatternOption :
                                             QRegularExpression::CaseInsensitiveOption;
  return QRegularExpression(wildcardToRegularExpression(str, options), reOptions);
  \endcode
*/
QRegularExpression QRegularExpression::fromWildcard(QStringView pattern, Qt::CaseSensitivity cs,
                                                    WildcardConversionOptions options)
{
    auto reOptions = cs == Qt::CaseSensitive ? QRegularExpression::NoPatternOption :
                                             QRegularExpression::CaseInsensitiveOption;
    return QRegularExpression(wildcardToRegularExpression(pattern, options), reOptions);
}

/*!
    \fn QRegularExpression::anchoredPattern(const QString &expression)
    \since 5.12
    \overload
*/

/*!
    \since 5.15

    Returns the \a expression wrapped between the \c{\A} and \c{\z} anchors to
    be used for exact matching.
*/
QString QRegularExpression::anchoredPattern(QStringView expression)
{
    return QString()
           + "\\A(?:"_L1
           + expression
           + ")\\z"_L1;
}

/*!
    \since 5.1

    Constructs a valid, empty QRegularExpressionMatch object. The regular
    expression is set to a default-constructed one; the match type to
    QRegularExpression::NoMatch and the match options to
    QRegularExpression::NoMatchOption.

    The object will report no match through the hasMatch() and the
    hasPartialMatch() member functions.
*/
QRegularExpressionMatch::QRegularExpressionMatch()
    : d(new QRegularExpressionMatchPrivate(QRegularExpression(),
                                           QString(),
                                           QStringView(),
                                           QRegularExpression::NoMatch,
                                           QRegularExpression::NoMatchOption))
{
    d->isValid = true;
}

/*!
    Destroys the match result.
*/
QRegularExpressionMatch::~QRegularExpressionMatch()
{
}

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QRegularExpressionMatchPrivate)

/*!
    Constructs a match result by copying the result of the given \a match.

    \sa operator=()
*/
QRegularExpressionMatch::QRegularExpressionMatch(const QRegularExpressionMatch &match)
    : d(match.d)
{
}

/*!
    \fn QRegularExpressionMatch::QRegularExpressionMatch(QRegularExpressionMatch &&match)

    \since 6.1

    Constructs a match result by moving the result from the given \a match.

    Note that a moved-from QRegularExpressionMatch can only be destroyed or
    assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.

    \sa operator=()
*/

/*!
    Assigns the match result \a match to this object, and returns a reference
    to the copy.
*/
QRegularExpressionMatch &QRegularExpressionMatch::operator=(const QRegularExpressionMatch &match)
{
    d = match.d;
    return *this;
}

/*!
    \fn QRegularExpressionMatch &QRegularExpressionMatch::operator=(QRegularExpressionMatch &&match)

    Move-assigns the match result \a match to this object, and returns a
    reference to the result.

    Note that a moved-from QRegularExpressionMatch can only be destroyed or
    assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.
*/

/*!
    \fn void QRegularExpressionMatch::swap(QRegularExpressionMatch &other)

    Swaps the match result \a other with this match result. This
    operation is very fast and never fails.
*/

/*!
    \internal
*/
QRegularExpressionMatch::QRegularExpressionMatch(QRegularExpressionMatchPrivate &dd)
    : d(&dd)
{
}

/*!
    Returns the QRegularExpression object whose match() function returned this
    object.

    \sa QRegularExpression::match(), matchType(), matchOptions()
*/
QRegularExpression QRegularExpressionMatch::regularExpression() const
{
    return d->regularExpression;
}


/*!
    Returns the match type that was used to get this QRegularExpressionMatch
    object, that is, the match type that was passed to
    QRegularExpression::match() or QRegularExpression::globalMatch().

    \sa QRegularExpression::match(), regularExpression(), matchOptions()
*/
QRegularExpression::MatchType QRegularExpressionMatch::matchType() const
{
    return d->matchType;
}

/*!
    Returns the match options that were used to get this
    QRegularExpressionMatch object, that is, the match options that were passed
    to QRegularExpression::match() or QRegularExpression::globalMatch().

    \sa QRegularExpression::match(), regularExpression(), matchType()
*/
QRegularExpression::MatchOptions QRegularExpressionMatch::matchOptions() const
{
    return d->matchOptions;
}

/*!
    Returns the index of the last capturing group that captured something,
    including the implicit capturing group 0. This can be used to extract all
    the substrings that were captured:

    \snippet code/src_corelib_text_qregularexpression.cpp 28

    Note that some of the capturing groups with an index less than
    lastCapturedIndex() could have not matched, and therefore captured nothing.

    If the regular expression did not match, this function returns -1.

    \sa hasCaptured(), captured(), capturedStart(), capturedEnd(), capturedLength()
*/
int QRegularExpressionMatch::lastCapturedIndex() const
{
    return d->capturedCount - 1;
}

/*!
    \fn bool QRegularExpressionMatch::hasCaptured(const QString &name) const
    \fn bool QRegularExpressionMatch::hasCaptured(QStringView name) const
    \since 6.3

    Returns true if the capturing group named \a name captured something
    in the subject string, and false otherwise (or if there is no
    capturing group called \a name).

    \note Some capturing groups in a regular expression may not have
    captured anything even if the regular expression matched. This may
    happen, for instance, if a conditional operator is used in the
    pattern:

    \snippet code/src_corelib_text_qregularexpression.cpp 36

    Similarly, a capturing group may capture a substring of length 0;
    this function will return \c{true} for such a capturing group.

    \sa captured(), hasMatch()
*/
bool QRegularExpressionMatch::hasCaptured(QStringView name) const
{
    const int nth = d->regularExpression.d->captureIndexForName(name);
    return hasCaptured(nth);
}

/*!
    \since 6.3

    Returns true if the \a nth capturing group captured something
    in the subject string, and false otherwise (or if there is no
    such capturing group).

    \note The implicit capturing group number 0 captures the substring
    matched by the entire pattern.

    \note Some capturing groups in a regular expression may not have
    captured anything even if the regular expression matched. This may
    happen, for instance, if a conditional operator is used in the
    pattern:

    \snippet code/src_corelib_text_qregularexpression.cpp 36

    Similarly, a capturing group may capture a substring of length 0;
    this function will return \c{true} for such a capturing group.

    \sa captured(), lastCapturedIndex(), hasMatch()
*/
bool QRegularExpressionMatch::hasCaptured(int nth) const
{
    if (nth < 0 || nth > lastCapturedIndex())
        return false;

    return d->capturedOffsets.at(nth * 2) != -1;
}

/*!
    Returns the substring captured by the \a nth capturing group.

    If the \a nth capturing group did not capture a string, or if there is no
    such capturing group, returns a null QString.

    \note The implicit capturing group number 0 captures the substring matched
    by the entire pattern.

    \sa capturedView(), lastCapturedIndex(), capturedStart(), capturedEnd(),
    capturedLength(), QString::isNull()
*/
QString QRegularExpressionMatch::captured(int nth) const
{
    return capturedView(nth).toString();
}

/*!
    \since 5.10

    Returns a view of the substring captured by the \a nth capturing group.

    If the \a nth capturing group did not capture a string, or if there is no
    such capturing group, returns a null QStringView.

    \note The implicit capturing group number 0 captures the substring matched
    by the entire pattern.

    \sa captured(), lastCapturedIndex(), capturedStart(), capturedEnd(),
    capturedLength(), QStringView::isNull()
*/
QStringView QRegularExpressionMatch::capturedView(int nth) const
{
    if (!hasCaptured(nth))
        return QStringView();

    qsizetype start = capturedStart(nth);

    if (start == -1) // didn't capture
        return QStringView();

    return d->subject.mid(start, capturedLength(nth));
}

/*! \fn QString QRegularExpressionMatch::captured(const QString &name) const

    Returns the substring captured by the capturing group named \a name.

    If the named capturing group \a name did not capture a string, or if
    there is no capturing group named \a name, returns a null QString.

    \sa capturedView(), capturedStart(), capturedEnd(), capturedLength(),
    QString::isNull()
*/

/*!
    \since 5.10

    Returns the substring captured by the capturing group named \a name.

    If the named capturing group \a name did not capture a string, or if
    there is no capturing group named \a name, returns a null QString.

    \sa capturedView(), capturedStart(), capturedEnd(), capturedLength(),
    QString::isNull()
*/
QString QRegularExpressionMatch::captured(QStringView name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::captured: empty capturing group name passed");
        return QString();
    }

    return capturedView(name).toString();
}

/*!
    \since 5.10

    Returns a view of the string captured by the capturing group named \a
    name.

    If the named capturing group \a name did not capture a string, or if
    there is no capturing group named \a name, returns a null QStringView.

    \sa captured(), capturedStart(), capturedEnd(), capturedLength(),
    QStringView::isNull()
*/
QStringView QRegularExpressionMatch::capturedView(QStringView name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedView: empty capturing group name passed");
        return QStringView();
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return QStringView();
    return capturedView(nth);
}

/*!
    Returns a list of all strings captured by capturing groups, in the order
    the groups themselves appear in the pattern string. The list includes the
    implicit capturing group number 0, capturing the substring matched by the
    entire pattern.
*/
QStringList QRegularExpressionMatch::capturedTexts() const
{
    QStringList texts;
    texts.reserve(d->capturedCount);
    for (int i = 0; i < d->capturedCount; ++i)
        texts << captured(i);
    return texts;
}

/*!
    Returns the offset inside the subject string corresponding to the
    starting position of the substring captured by the \a nth capturing group.
    If the \a nth capturing group did not capture a string or doesn't exist,
    returns -1.

    \sa capturedEnd(), capturedLength(), captured()
*/
qsizetype QRegularExpressionMatch::capturedStart(int nth) const
{
    if (!hasCaptured(nth))
        return -1;

    return d->capturedOffsets.at(nth * 2);
}

/*!
    Returns the length of the substring captured by the \a nth capturing group.

    \note This function returns 0 if the \a nth capturing group did not capture
    a string or doesn't exist.

    \sa capturedStart(), capturedEnd(), captured()
*/
qsizetype QRegularExpressionMatch::capturedLength(int nth) const
{
    // bound checking performed by these two functions
    return capturedEnd(nth) - capturedStart(nth);
}

/*!
    Returns the offset inside the subject string immediately after the ending
    position of the substring captured by the \a nth capturing group. If the \a
    nth capturing group did not capture a string or doesn't exist, returns -1.

    \sa capturedStart(), capturedLength(), captured()
*/
qsizetype QRegularExpressionMatch::capturedEnd(int nth) const
{
    if (!hasCaptured(nth))
        return -1;

    return d->capturedOffsets.at(nth * 2 + 1);
}

/*! \fn qsizetype QRegularExpressionMatch::capturedStart(const QString &name) const

    Returns the offset inside the subject string corresponding to the starting
    position of the substring captured by the capturing group named \a name.
    If the capturing group named \a name did not capture a string or doesn't
    exist, returns -1.

    \sa capturedEnd(), capturedLength(), captured()
*/

/*! \fn qsizetype QRegularExpressionMatch::capturedLength(const QString &name) const

    Returns the length of the substring captured by the capturing group named
    \a name.

    \note This function returns 0 if the capturing group named \a name did not
    capture a string or doesn't exist.

    \sa capturedStart(), capturedEnd(), captured()
*/

/*! \fn qsizetype QRegularExpressionMatch::capturedEnd(const QString &name) const

    Returns the offset inside the subject string immediately after the ending
    position of the substring captured by the capturing group named \a name. If
    the capturing group named \a name did not capture a string or doesn't
    exist, returns -1.

    \sa capturedStart(), capturedLength(), captured()
*/

/*!
    \since 5.10

    Returns the offset inside the subject string corresponding to the starting
    position of the substring captured by the capturing group named \a name.
    If the capturing group named \a name did not capture a string or doesn't
    exist, returns -1.

    \sa capturedEnd(), capturedLength(), captured()
*/
qsizetype QRegularExpressionMatch::capturedStart(QStringView name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedStart: empty capturing group name passed");
        return -1;
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return -1;
    return capturedStart(nth);
}

/*!
    \since 5.10

    Returns the length of the substring captured by the capturing group named
    \a name.

    \note This function returns 0 if the capturing group named \a name did not
    capture a string or doesn't exist.

    \sa capturedStart(), capturedEnd(), captured()
*/
qsizetype QRegularExpressionMatch::capturedLength(QStringView name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedLength: empty capturing group name passed");
        return 0;
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return 0;
    return capturedLength(nth);
}

/*!
    \since 5.10

    Returns the offset inside the subject string immediately after the ending
    position of the substring captured by the capturing group named \a name. If
    the capturing group named \a name did not capture a string or doesn't
    exist, returns -1.

    \sa capturedStart(), capturedLength(), captured()
*/
qsizetype QRegularExpressionMatch::capturedEnd(QStringView name) const
{
    if (name.isEmpty()) {
        qWarning("QRegularExpressionMatch::capturedEnd: empty capturing group name passed");
        return -1;
    }
    int nth = d->regularExpression.d->captureIndexForName(name);
    if (nth == -1)
        return -1;
    return capturedEnd(nth);
}

/*!
    Returns \c true if the regular expression matched against the subject string,
    or false otherwise.

    \sa QRegularExpression::match(), hasPartialMatch()
*/
bool QRegularExpressionMatch::hasMatch() const
{
    return d->hasMatch;
}

/*!
    Returns \c true if the regular expression partially matched against the
    subject string, or false otherwise.

    \note Only a match that explicitly used the one of the partial match types
    can yield a partial match. Still, if such a match succeeds totally, this
    function will return false, while hasMatch() will return true.

    \sa QRegularExpression::match(), QRegularExpression::MatchType, hasMatch()
*/
bool QRegularExpressionMatch::hasPartialMatch() const
{
    return d->hasPartialMatch;
}

/*!
    Returns \c true if the match object was obtained as a result from the
    QRegularExpression::match() function invoked on a valid QRegularExpression
    object; returns \c false if the QRegularExpression was invalid.

    \sa QRegularExpression::match(), QRegularExpression::isValid()
*/
bool QRegularExpressionMatch::isValid() const
{
    return d->isValid;
}

/*!
    \internal
*/
QRegularExpressionMatchIterator::QRegularExpressionMatchIterator(QRegularExpressionMatchIteratorPrivate &dd)
    : d(&dd)
{
}

/*!
    \since 5.1

    Constructs an empty, valid QRegularExpressionMatchIterator object. The
    regular expression is set to a default-constructed one; the match type to
    QRegularExpression::NoMatch and the match options to
    QRegularExpression::NoMatchOption.

    Invoking the hasNext() member function on the constructed object will
    return false, as the iterator is not iterating on a valid sequence of
    matches.
*/
QRegularExpressionMatchIterator::QRegularExpressionMatchIterator()
    : d(new QRegularExpressionMatchIteratorPrivate(QRegularExpression(),
                                                   QRegularExpression::NoMatch,
                                                   QRegularExpression::NoMatchOption,
                                                   QRegularExpressionMatch()))
{
}

/*!
    Destroys the QRegularExpressionMatchIterator object.
*/
QRegularExpressionMatchIterator::~QRegularExpressionMatchIterator()
{
}

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QRegularExpressionMatchIteratorPrivate)

/*!
    Constructs a QRegularExpressionMatchIterator object as a copy of \a
    iterator.

    \sa operator=()
*/
QRegularExpressionMatchIterator::QRegularExpressionMatchIterator(const QRegularExpressionMatchIterator &iterator)
    : d(iterator.d)
{
}

/*!
    \fn QRegularExpressionMatchIterator::QRegularExpressionMatchIterator(QRegularExpressionMatchIterator &&iterator)

    \since 6.1

    Constructs a QRegularExpressionMatchIterator object by moving from \a iterator.

    Note that a moved-from QRegularExpressionMatchIterator can only be destroyed
    or assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.

    \sa operator=()
*/

/*!
    Assigns the iterator \a iterator to this object, and returns a reference to
    the copy.
*/
QRegularExpressionMatchIterator &QRegularExpressionMatchIterator::operator=(const QRegularExpressionMatchIterator &iterator)
{
    d = iterator.d;
    return *this;
}

/*!
    \fn QRegularExpressionMatchIterator &QRegularExpressionMatchIterator::operator=(QRegularExpressionMatchIterator &&iterator)

    Move-assigns the \a iterator to this object, and returns a reference to the
    result.

    Note that a moved-from QRegularExpressionMatchIterator can only be destroyed
    or assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.
*/

/*!
    \fn void QRegularExpressionMatchIterator::swap(QRegularExpressionMatchIterator &other)

    Swaps the iterator \a other with this iterator object. This operation is
    very fast and never fails.
*/

/*!
    Returns \c true if the iterator object was obtained as a result from the
    QRegularExpression::globalMatch() function invoked on a valid
    QRegularExpression object; returns \c false if the QRegularExpression was
    invalid.

    \sa QRegularExpression::globalMatch(), QRegularExpression::isValid()
*/
bool QRegularExpressionMatchIterator::isValid() const
{
    return d->next.isValid();
}

/*!
    Returns \c true if there is at least one match result ahead of the iterator;
    otherwise it returns \c false.

    \sa next()
*/
bool QRegularExpressionMatchIterator::hasNext() const
{
    return d->hasNext();
}

/*!
    Returns the next match result without moving the iterator.

    \note Calling this function when the iterator is at the end of the result
    set leads to undefined results.
*/
QRegularExpressionMatch QRegularExpressionMatchIterator::peekNext() const
{
    if (!hasNext())
        qWarning("QRegularExpressionMatchIterator::peekNext() called on an iterator already at end");

    return d->next;
}

/*!
    Returns the next match result and advances the iterator by one position.

    \note Calling this function when the iterator is at the end of the result
    set leads to undefined results.
*/
QRegularExpressionMatch QRegularExpressionMatchIterator::next()
{
    if (!hasNext()) {
        qWarning("QRegularExpressionMatchIterator::next() called on an iterator already at end");
        return d.constData()->next;
    }

    d.detach();
    return std::exchange(d->next, d->next.d.constData()->nextMatch());
}

/*!
    Returns the QRegularExpression object whose globalMatch() function returned
    this object.

    \sa QRegularExpression::globalMatch(), matchType(), matchOptions()
*/
QRegularExpression QRegularExpressionMatchIterator::regularExpression() const
{
    return d->regularExpression;
}

/*!
    Returns the match type that was used to get this
    QRegularExpressionMatchIterator object, that is, the match type that was
    passed to QRegularExpression::globalMatch().

    \sa QRegularExpression::globalMatch(), regularExpression(), matchOptions()
*/
QRegularExpression::MatchType QRegularExpressionMatchIterator::matchType() const
{
    return d->matchType;
}

/*!
    Returns the match options that were used to get this
    QRegularExpressionMatchIterator object, that is, the match options that
    were passed to QRegularExpression::globalMatch().

    \sa QRegularExpression::globalMatch(), regularExpression(), matchType()
*/
QRegularExpression::MatchOptions QRegularExpressionMatchIterator::matchOptions() const
{
    return d->matchOptions;
}

/*!
  \internal
*/
QtPrivate::QRegularExpressionMatchIteratorRangeBasedForIterator begin(const QRegularExpressionMatchIterator &iterator)
{
    return QtPrivate::QRegularExpressionMatchIteratorRangeBasedForIterator(iterator);
}

/*!
  \fn QtPrivate::QRegularExpressionMatchIteratorRangeBasedForIteratorSentinel end(const QRegularExpressionMatchIterator &)
  \internal
*/

#ifndef QT_NO_DATASTREAM
/*!
    \relates QRegularExpression

    Writes the regular expression \a re to stream \a out.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QRegularExpression &re)
{
    out << re.pattern() << quint32(re.patternOptions().toInt());
    return out;
}

/*!
    \relates QRegularExpression

    Reads a regular expression from stream \a in into \a re.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QRegularExpression &re)
{
    QString pattern;
    quint32 patternOptions;
    in >> pattern >> patternOptions;
    re.setPattern(pattern);
    re.setPatternOptions(QRegularExpression::PatternOptions::fromInt(patternOptions));
    return in;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
/*!
    \relates QRegularExpression

    Writes the regular expression \a re into the debug object \a debug for
    debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QRegularExpression &re)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QRegularExpression(" << re.pattern() << ", " << re.patternOptions() << ')';
    return debug;
}

/*!
    \relates QRegularExpression

    Writes the pattern options \a patternOptions into the debug object \a debug
    for debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, QRegularExpression::PatternOptions patternOptions)
{
    QDebugStateSaver saver(debug);
    QByteArray flags;

    if (patternOptions == QRegularExpression::NoPatternOption) {
        flags = "NoPatternOption";
    } else {
        flags.reserve(200); // worst case...
        if (patternOptions & QRegularExpression::CaseInsensitiveOption)
            flags.append("CaseInsensitiveOption|");
        if (patternOptions & QRegularExpression::DotMatchesEverythingOption)
            flags.append("DotMatchesEverythingOption|");
        if (patternOptions & QRegularExpression::MultilineOption)
            flags.append("MultilineOption|");
        if (patternOptions & QRegularExpression::ExtendedPatternSyntaxOption)
            flags.append("ExtendedPatternSyntaxOption|");
        if (patternOptions & QRegularExpression::InvertedGreedinessOption)
            flags.append("InvertedGreedinessOption|");
        if (patternOptions & QRegularExpression::DontCaptureOption)
            flags.append("DontCaptureOption|");
        if (patternOptions & QRegularExpression::UseUnicodePropertiesOption)
            flags.append("UseUnicodePropertiesOption|");
        flags.chop(1);
    }

    debug.nospace() << "QRegularExpression::PatternOptions(" << flags << ')';

    return debug;
}
/*!
    \relates QRegularExpressionMatch

    Writes the match object \a match into the debug object \a debug for
    debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QRegularExpressionMatch &match)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QRegularExpressionMatch(";

    if (!match.isValid()) {
        debug << "Invalid)";
        return debug;
    }

    debug << "Valid";

    if (match.hasMatch()) {
        debug << ", has match: ";
        for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
            debug << i
                  << ":(" << match.capturedStart(i) << ", " << match.capturedEnd(i)
                  << ", " << match.captured(i) << ')';
            if (i < match.lastCapturedIndex())
                debug << ", ";
        }
    } else if (match.hasPartialMatch()) {
        debug << ", has partial match: ("
              << match.capturedStart(0) << ", "
              << match.capturedEnd(0) << ", "
              << match.captured(0) << ')';
    } else {
        debug << ", no match";
    }

    debug << ')';

    return debug;
}
#endif

// fool lupdate: make it extract those strings for translation, but don't put them
// inside Qt -- they're already inside libpcre (cf. man 3 pcreapi, pcre_compile.c).
#if 0

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
     Original API code Copyright (c) 1997-2012 University of Cambridge
         New API code Copyright (c) 2015 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

static const char *pcreCompileErrorCodes[] =
{
    QT_TRANSLATE_NOOP("QRegularExpression", "no error"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\ at end of pattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\c at end of pattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character follows \\"),
    QT_TRANSLATE_NOOP("QRegularExpression", "numbers out of order in {} quantifier"),
    QT_TRANSLATE_NOOP("QRegularExpression", "number too big in {} quantifier"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing terminating ] for character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "escape sequence is invalid in character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "range out of order in character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "quantifier does not follow a repeatable item"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unexpected repeat"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character after (? or (?-"),
    QT_TRANSLATE_NOOP("QRegularExpression", "POSIX named classes are supported only within a class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "POSIX collating elements are not supported"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing closing parenthesis"),
    QT_TRANSLATE_NOOP("QRegularExpression", "reference to non-existent subpattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "pattern passed as NULL"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognised compile-time option bit(s)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing ) after (?# comment"),
    QT_TRANSLATE_NOOP("QRegularExpression", "parentheses are too deeply nested"),
    QT_TRANSLATE_NOOP("QRegularExpression", "regular expression is too large"),
    QT_TRANSLATE_NOOP("QRegularExpression", "failed to allocate heap memory"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unmatched closing parenthesis"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: code overflow"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing closing parenthesis for condition"),
    QT_TRANSLATE_NOOP("QRegularExpression", "lookbehind assertion is not fixed length"),
    QT_TRANSLATE_NOOP("QRegularExpression", "a relative value of zero is not allowed"),
    QT_TRANSLATE_NOOP("QRegularExpression", "conditional subpattern contains more than two branches"),
    QT_TRANSLATE_NOOP("QRegularExpression", "assertion expected after (?( or (?(?C)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "digit expected after (?+ or (?-"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unknown POSIX class name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error in pcre2_study(): should not occur"),
    QT_TRANSLATE_NOOP("QRegularExpression", "this version of PCRE2 does not have Unicode support"),
    QT_TRANSLATE_NOOP("QRegularExpression", "parentheses are too deeply nested (stack check)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "character code point value in \\x{} or \\o{} is too large"),
    QT_TRANSLATE_NOOP("QRegularExpression", "lookbehind is too complicated"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\C is not allowed in a lookbehind assertion in UTF-" "16" " mode"),
    QT_TRANSLATE_NOOP("QRegularExpression", "PCRE2 does not support \\F, \\L, \\l, \\N{name}, \\U, or \\u"),
    QT_TRANSLATE_NOOP("QRegularExpression", "number after (?C is greater than 255"),
    QT_TRANSLATE_NOOP("QRegularExpression", "closing parenthesis for (?C expected"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid escape sequence in (*VERB) name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized character after (?P"),
    QT_TRANSLATE_NOOP("QRegularExpression", "syntax error in subpattern name (missing terminator?)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "two named subpatterns have the same name (PCRE2_DUPNAMES not set)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "subpattern name must start with a non-digit"),
    QT_TRANSLATE_NOOP("QRegularExpression", "this version of PCRE2 does not have support for \\P, \\p, or \\X"),
    QT_TRANSLATE_NOOP("QRegularExpression", "malformed \\P or \\p sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unknown property name after \\P or \\p"),
    QT_TRANSLATE_NOOP("QRegularExpression", "subpattern name is too long (maximum " "32" " code units)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "too many named subpatterns (maximum " "10000" ")"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid range in character class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "octal value is greater than \\377 in 8-bit non-UTF-8 mode"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: overran compiling workspace"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: previously-checked referenced subpattern not found"),
    QT_TRANSLATE_NOOP("QRegularExpression", "DEFINE subpattern contains more than one branch"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing opening brace after \\o"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unknown newline setting"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\g is not followed by a braced, angle-bracketed, or quoted name/number or by a plain number"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(?R (recursive pattern call) must be followed by a closing parenthesis"),
    QT_TRANSLATE_NOOP("QRegularExpression", "obsolete error (should not occur)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(*VERB) not recognized or malformed"),
    QT_TRANSLATE_NOOP("QRegularExpression", "subpattern number is too big"),
    QT_TRANSLATE_NOOP("QRegularExpression", "subpattern name expected"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: parsed pattern overflow"),
    QT_TRANSLATE_NOOP("QRegularExpression", "non-octal character in \\o{} (closing brace missing?)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "different names for subpatterns of the same number are not allowed"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(*MARK) must have an argument"),
    QT_TRANSLATE_NOOP("QRegularExpression", "non-hex character in \\x{} (closing brace missing?)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\c must be followed by a printable ASCII character"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\c must be followed by a letter or one of [\\]^_?"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\k is not followed by a braced, angle-bracketed, or quoted name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unknown meta code in check_lookbehinds()"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\N is not supported in a class"),
    QT_TRANSLATE_NOOP("QRegularExpression", "callout string is too long"),
    QT_TRANSLATE_NOOP("QRegularExpression", "disallowed Unicode code point (>= 0xd800 && <= 0xdfff)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "using UTF is disabled by the application"),
    QT_TRANSLATE_NOOP("QRegularExpression", "using UCP is disabled by the application"),
    QT_TRANSLATE_NOOP("QRegularExpression", "name is too long in (*MARK), (*PRUNE), (*SKIP), or (*THEN)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "character code point value in \\u.... sequence is too large"),
    QT_TRANSLATE_NOOP("QRegularExpression", "digits missing in \\x{} or \\o{} or \\N{U+}"),
    QT_TRANSLATE_NOOP("QRegularExpression", "syntax error or number too big in (?(VERSION condition"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unknown opcode in auto_possessify()"),
    QT_TRANSLATE_NOOP("QRegularExpression", "missing terminating delimiter for callout with string argument"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unrecognized string delimiter follows (?C"),
    QT_TRANSLATE_NOOP("QRegularExpression", "using \\C is disabled by the application"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(?| and/or (?J: or (?x: parentheses are too deeply nested"),
    QT_TRANSLATE_NOOP("QRegularExpression", "using \\C is disabled in this PCRE2 library"),
    QT_TRANSLATE_NOOP("QRegularExpression", "regular expression is too complicated"),
    QT_TRANSLATE_NOOP("QRegularExpression", "lookbehind assertion is too long"),
    QT_TRANSLATE_NOOP("QRegularExpression", "pattern string is longer than the limit set by the application"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: unknown code in parsed pattern"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error: bad code value in parsed_skip()"),
    QT_TRANSLATE_NOOP("QRegularExpression", "PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES is not allowed in UTF-16 mode"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid option bits with PCRE2_LITERAL"),
    QT_TRANSLATE_NOOP("QRegularExpression", "\\N{U+dddd} is supported only in Unicode (UTF) mode"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid hyphen in option setting"),
    QT_TRANSLATE_NOOP("QRegularExpression", "(*alpha_assertion) not recognized"),
    QT_TRANSLATE_NOOP("QRegularExpression", "script runs require Unicode support, which this version of PCRE2 does not have"),
    QT_TRANSLATE_NOOP("QRegularExpression", "too many capturing groups (maximum 65535)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "atomic assertion expected after (?( or (?(?C)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "no error"),
    QT_TRANSLATE_NOOP("QRegularExpression", "no match"),
    QT_TRANSLATE_NOOP("QRegularExpression", "partial match"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 1 byte missing at end"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 2 bytes missing at end"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 3 bytes missing at end"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 4 bytes missing at end"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 5 bytes missing at end"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: byte 2 top bits not 0x80"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: byte 3 top bits not 0x80"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: byte 4 top bits not 0x80"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: byte 5 top bits not 0x80"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: byte 6 top bits not 0x80"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 5-byte character is not allowed (RFC 3629)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: 6-byte character is not allowed (RFC 3629)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: code points greater than 0x10ffff are not defined"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: code points 0xd800-0xdfff are not defined"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: overlong 2-byte sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: overlong 3-byte sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: overlong 4-byte sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: overlong 5-byte sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: overlong 6-byte sequence"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: isolated byte with 0x80 bit set"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-8 error: illegal byte (0xfe or 0xff)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-16 error: missing low surrogate at end"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-16 error: invalid low surrogate"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-16 error: isolated low surrogate"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-32 error: code points 0xd800-0xdfff are not defined"),
    QT_TRANSLATE_NOOP("QRegularExpression", "UTF-32 error: code points greater than 0x10ffff are not defined"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad data value"),
    QT_TRANSLATE_NOOP("QRegularExpression", "patterns do not all use the same character tables"),
    QT_TRANSLATE_NOOP("QRegularExpression", "magic number missing"),
    QT_TRANSLATE_NOOP("QRegularExpression", "pattern compiled in wrong mode: 8/16/32-bit error"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad offset value"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad option value"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid replacement string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad offset into UTF string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "callout error code"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid data in workspace for DFA restart"),
    QT_TRANSLATE_NOOP("QRegularExpression", "too much recursion for DFA matching"),
    QT_TRANSLATE_NOOP("QRegularExpression", "backreference condition or recursion test is not supported for DFA matching"),
    QT_TRANSLATE_NOOP("QRegularExpression", "function is not supported for DFA matching"),
    QT_TRANSLATE_NOOP("QRegularExpression", "pattern contains an item that is not supported for DFA matching"),
    QT_TRANSLATE_NOOP("QRegularExpression", "workspace size exceeded in DFA matching"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error - pattern overwritten?"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad JIT option"),
    QT_TRANSLATE_NOOP("QRegularExpression", "JIT stack limit reached"),
    QT_TRANSLATE_NOOP("QRegularExpression", "match limit exceeded"),
    QT_TRANSLATE_NOOP("QRegularExpression", "no more memory"),
    QT_TRANSLATE_NOOP("QRegularExpression", "unknown substring"),
    QT_TRANSLATE_NOOP("QRegularExpression", "non-unique substring name"),
    QT_TRANSLATE_NOOP("QRegularExpression", "NULL argument passed"),
    QT_TRANSLATE_NOOP("QRegularExpression", "nested recursion at the same subject position"),
    QT_TRANSLATE_NOOP("QRegularExpression", "matching depth limit exceeded"),
    QT_TRANSLATE_NOOP("QRegularExpression", "requested value is not available"),
    QT_TRANSLATE_NOOP("QRegularExpression", "requested value is not set"),
    QT_TRANSLATE_NOOP("QRegularExpression", "offset limit set without PCRE2_USE_OFFSET_LIMIT"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad escape sequence in replacement string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "expected closing curly bracket in replacement string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad substitution in replacement string"),
    QT_TRANSLATE_NOOP("QRegularExpression", "match with end before start or start moved backwards is not supported"),
    QT_TRANSLATE_NOOP("QRegularExpression", "too many replacements (more than INT_MAX)"),
    QT_TRANSLATE_NOOP("QRegularExpression", "bad serialized data"),
    QT_TRANSLATE_NOOP("QRegularExpression", "heap limit exceeded"),
    QT_TRANSLATE_NOOP("QRegularExpression", "invalid syntax"),
    QT_TRANSLATE_NOOP("QRegularExpression", "internal error - duplicate substitution match"),
    QT_TRANSLATE_NOOP("QRegularExpression", "PCRE2_MATCH_INVALID_UTF is not supported for DFA matching")
};
#endif // #if 0

QT_END_NAMESPACE
