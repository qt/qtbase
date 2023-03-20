// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qurl_p.h"

#include <QtCore/qstringlist.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/private/qoffsetstringarray_p.h>
#include <QtCore/private/qstringiterator_p.h>
#include <QtCore/private/qunicodetables_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// needed by the punycode encoder/decoder
static const uint base = 36;
static const uint tmin = 1;
static const uint tmax = 26;
static const uint skew = 38;
static const uint damp = 700;
static const uint initial_bias = 72;
static const uint initial_n = 128;

static constexpr qsizetype MaxDomainLabelLength = 63;

static inline uint encodeDigit(uint digit)
{
  return digit + 22 + 75 * (digit < 26);
}

static inline uint adapt(uint delta, uint numpoints, bool firsttime)
{
    delta /= (firsttime ? damp : 2);
    delta += (delta / numpoints);

    uint k = 0;
    for (; delta > ((base - tmin) * tmax) / 2; k += base)
        delta /= (base - tmin);

    return k + (((base - tmin + 1) * delta) / (delta + skew));
}

static inline void appendEncode(QString *output, uint delta, uint bias)
{
    uint qq;
    uint k;
    uint t;

    // insert the variable length delta integer.
    for (qq = delta, k = base;; k += base) {
        // stop generating digits when the threshold is
        // detected.
        t = (k <= bias) ? tmin : (k >= bias + tmax) ? tmax : k - bias;
        if (qq < t) break;

        *output += QChar(encodeDigit(t + (qq - t) % (base - t)));
        qq = (qq - t) / (base - t);
    }

    *output += QChar(encodeDigit(qq));
}

Q_AUTOTEST_EXPORT void qt_punycodeEncoder(QStringView in, QString *output)
{
    uint n = initial_n;
    uint delta = 0;
    uint bias = initial_bias;

    // Do not try to encode strings that certainly will result in output
    // that is longer than allowable domain name label length. Note that
    // non-BMP codepoints are encoded as two QChars.
    if (in.size() > MaxDomainLabelLength * 2)
        return;

    int outLen = output->size();
    output->resize(outLen + in.size());

    QChar *d = output->data() + outLen;
    bool skipped = false;
    // copy all basic code points verbatim to output.
    for (QChar c : in) {
        if (c.unicode() < 0x80)
            *d++ = c;
        else
            skipped = true;
    }

    // if there were only basic code points, just return them
    // directly; don't do any encoding.
    if (!skipped)
        return;

    output->truncate(d - output->constData());
    int copied = output->size() - outLen;

    // h and b now contain the number of basic code points in input.
    uint b = copied;
    uint h = copied;

    // if basic code points were copied, add the delimiter character.
    if (h > 0)
        *output += u'-';

    // compute the input length in Unicode code points.
    uint inputLength = 0;
    for (QStringIterator iter(in); iter.hasNext();) {
        inputLength++;

        if (iter.next(char32_t(-1)) == char32_t(-1)) {
            output->truncate(outLen);
            return; // invalid surrogate pair
        }
    }

    // while there are still unprocessed non-basic code points left in
    // the input string...
    while (h < inputLength) {
        // find the character in the input string with the lowest unprocessed value.
        uint m = std::numeric_limits<uint>::max();
        for (QStringIterator iter(in); iter.hasNext();) {
            auto c = iter.nextUnchecked();
            static_assert(std::numeric_limits<decltype(m)>::max()
                                  >= std::numeric_limits<decltype(c)>::max(),
                          "Punycode uint should be able to cover all codepoints");
            if (c >= n && c < m)
                m = c;
        }

        // delta = delta + (m - n) * (h + 1), fail on overflow
        uint tmp;
        if (qMulOverflow<uint>(m - n, h + 1, &tmp) || qAddOverflow<uint>(delta, tmp, &delta)) {
            output->truncate(outLen);
            return; // punycode_overflow
        }
        n = m;

        for (QStringIterator iter(in); iter.hasNext();) {
            auto c = iter.nextUnchecked();

            // increase delta until we reach the character processed in this iteration;
            // fail if delta overflows.
            if (c < n) {
                if (qAddOverflow<uint>(delta, 1, &delta)) {
                    output->truncate(outLen);
                    return; // punycode_overflow
                }
            }

            if (c == n) {
                appendEncode(output, delta, bias);

                bias = adapt(delta, h + 1, h == b);
                delta = 0;
                ++h;
            }
        }

        ++delta;
        ++n;
    }

    // prepend ACE prefix
    output->insert(outLen, "xn--"_L1);
    return;
}

Q_AUTOTEST_EXPORT QString qt_punycodeDecoder(const QString &pc)
{
    uint n = initial_n;
    uint i = 0;
    uint bias = initial_bias;

    // Do not try to decode strings longer than allowable for a domain label.
    // Non-ASCII strings are not allowed here anyway, so there is no need
    // to account for surrogates.
    if (pc.size() > MaxDomainLabelLength)
        return QString();

    // strip any ACE prefix
    int start = pc.startsWith("xn--"_L1) ? 4 : 0;
    if (!start)
        return pc;

    // find the last delimiter character '-' in the input array. copy
    // all data before this delimiter directly to the output array.
    int delimiterPos = pc.lastIndexOf(u'-');
    auto output = delimiterPos < 4 ? std::u32string()
                                   : pc.mid(start, delimiterPos - start).toStdU32String();

    // if a delimiter was found, skip to the position after it;
    // otherwise start at the front of the input string. everything
    // before the delimiter is assumed to be basic code points.
    uint cnt = delimiterPos + 1;

    // loop through the rest of the input string, inserting non-basic
    // characters into output as we go.
    while (cnt < (uint) pc.size()) {
        uint oldi = i;
        uint w = 1;

        // find the next index for inserting a non-basic character.
        for (uint k = base; cnt < (uint) pc.size(); k += base) {
            // grab a character from the punycode input and find its
            // delta digit (each digit code is part of the
            // variable-length integer delta)
            uint digit = pc.at(cnt++).unicode();
            if (digit - 48 < 10) digit -= 22;
            else if (digit - 65 < 26) digit -= 65;
            else if (digit - 97 < 26) digit -= 97;
            else digit = base;

            // Fail if the code point has no digit value
            if (digit >= base)
                return QString();

            // i = i + digit * w, fail on overflow
            uint tmp;
            if (qMulOverflow<uint>(digit, w, &tmp) || qAddOverflow<uint>(i, tmp, &i))
                return QString();

            // detect threshold to stop reading delta digits
            uint t;
            if (k <= bias) t = tmin;
            else if (k >= bias + tmax) t = tmax;
            else t = k - bias;

            if (digit < t) break;

            // w = w * (base - t), fail on overflow
            if (qMulOverflow<uint>(w, base - t, &w))
                return QString();
        }

        // find new bias and calculate the next non-basic code
        // character.
        uint outputLength = static_cast<uint>(output.length());
        bias = adapt(i - oldi, outputLength + 1, oldi == 0);

        // n = n + i div (length(output) + 1), fail on overflow
        if (qAddOverflow<uint>(n, i / (outputLength + 1), &n))
            return QString();

        // allow the deltas to wrap around
        i %= (outputLength + 1);

        // if n is a basic code point then fail; this should not happen with
        // correct implementation of Punycode, but check just n case.
        if (n < initial_n) {
            // Don't use Q_ASSERT() to avoid possibility of DoS
            qWarning("Attempt to insert a basic codepoint. Unhandled overflow?");
            return QString();
        }

        // Surrogates should normally be rejected later by other IDNA code.
        // But because of Qt's use of UTF-16 to represent strings the
        // IDNA code is not able to distinguish characters represented as pairs
        // of surrogates from normal code points. This is why surrogates are
        // not allowed here.
        //
        // Allowing surrogates would lead to non-unique (after normalization)
        // encoding of strings with non-BMP characters.
        //
        // Punycode that encodes characters outside the Unicode range is also
        // invalid and is rejected here.
        if (QChar::isSurrogate(n) || n > QChar::LastValidCodePoint)
            return QString();

        // insert the character n at position i
        output.insert(i, 1, static_cast<char32_t>(n));
        ++i;
    }

    return QString::fromStdU32String(output);
}

static constexpr auto idn_whitelist = qOffsetStringArray(
    "ac", "ar", "asia", "at",
    "biz", "br",
    "cat", "ch", "cl", "cn", "com",
    "de", "dk",
    "es",
    "fi",
    "gr",
    "hu",
    "il", "info", "io", "is", "ir",
    "jp",
    "kr",
    "li", "lt", "lu", "lv",
    "museum",
    "name", "net", "no", "nu", "nz",
    "org",
    "pl", "pr",
    "se", "sh",
    "tel", "th", "tm", "tw",
    "ua",
    "vn",
    "xn--fiqs8s",               // China
    "xn--fiqz9s",               // China
    "xn--fzc2c9e2c",            // Sri Lanka
    "xn--j6w193g",              // Hong Kong
    "xn--kprw13d",              // Taiwan
    "xn--kpry57d",              // Taiwan
    "xn--mgba3a4f16a",          // Iran
    "xn--mgba3a4fra",           // Iran
    "xn--mgbaam7a8h",           // UAE
    "xn--mgbayh7gpa",           // Jordan
    "xn--mgberp4a5d4ar",        // Saudi Arabia
    "xn--ogbpf8fl",             // Syria
    "xn--p1ai",                 // Russian Federation
    "xn--wgbh1c",               // Egypt
    "xn--wgbl6a",               // Qatar
    "xn--xkc2al3hye2a"          // Sri Lanka
);

Q_CONSTINIT static QStringList *user_idn_whitelist = nullptr;

static bool lessThan(const QChar *a, int l, const char *c)
{
    const auto *uc = reinterpret_cast<const char16_t *>(a);
    const char16_t *e = uc + l;

    if (!c || *c == 0)
        return false;

    while (*c) {
        if (uc == e || *uc != static_cast<unsigned char>(*c))
            break;
        ++uc;
        ++c;
    }
    return uc == e ? *c : (*uc < static_cast<unsigned char>(*c));
}

static bool equal(const QChar *a, int l, const char *b)
{
    while (l && a->unicode() && *b) {
        if (*a != QLatin1Char(*b))
            return false;
        ++a;
        ++b;
        --l;
    }
    return l == 0;
}

static bool qt_is_idn_enabled(QStringView aceDomain)
{
    auto idx = aceDomain.lastIndexOf(u'.');
    if (idx == -1)
        return false;

    auto tldString = aceDomain.mid(idx + 1);
    const auto len = tldString.size();

    const QChar *tld = tldString.constData();

    if (user_idn_whitelist)
        return user_idn_whitelist->contains(tldString);

    int l = 0;
    int r = idn_whitelist.count() - 1;
    int i = (l + r + 1) / 2;

    while (r != l) {
        if (lessThan(tld, len, idn_whitelist.at(i)))
            r = i - 1;
        else
            l = i;
        i = (l + r + 1) / 2;
    }
    return equal(tld, len, idn_whitelist.at(i));
}

template<typename C>
static inline bool isValidInNormalizedAsciiLabel(C c)
{
    return c == u'-' || c == u'_' || (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'z');
}

template<typename C>
static inline bool isValidInNormalizedAsciiName(C c)
{
    return isValidInNormalizedAsciiLabel(c) || c == u'.';
}

/*
    Map domain name according to algorithm in UTS #46, 4.1

    Returns empty string if there are disallowed characters in the input.

    Sets resultIsAscii if the result is known for sure to be all ASCII.
*/
static QString mapDomainName(const QString &in, QUrl::AceProcessingOptions options,
                             bool *resultIsAscii)
{
    *resultIsAscii = true;

    // Check if the input is already normalized ASCII first and can be returned as is.
    int i = 0;
    for (auto c : in) {
        if (c.unicode() >= 0x80 || !isValidInNormalizedAsciiName(c))
            break;
        i++;
    }

    if (i == in.size())
        return in;

    QString result;
    result.reserve(in.size());
    result.append(in.constData(), i);
    bool allAscii = true;

    for (QStringIterator iter(QStringView(in).sliced(i)); iter.hasNext();) {
        char32_t uc = iter.next();

        // Fast path for ASCII-only inputs
        if (Q_LIKELY(uc < 0x80)) {
            if (uc >= U'A' && uc <= U'Z')
                uc |= 0x20; // lower-case it

            if (!isValidInNormalizedAsciiName(uc))
                return {};

            result.append(static_cast<char16_t>(uc));
            continue;
        }
        allAscii = false;

        QUnicodeTables::IdnaStatus status = QUnicodeTables::idnaStatus(uc);

        if (status == QUnicodeTables::IdnaStatus::Deviation)
            status = options.testFlag(QUrl::AceTransitionalProcessing)
                    ? QUnicodeTables::IdnaStatus::Mapped
                    : QUnicodeTables::IdnaStatus::Valid;

        switch (status) {
        case QUnicodeTables::IdnaStatus::Ignored:
            continue;
        case QUnicodeTables::IdnaStatus::Valid:
            for (auto c : QChar::fromUcs4(uc))
                result.append(c);
            break;
        case QUnicodeTables::IdnaStatus::Mapped:
            result.append(QUnicodeTables::idnaMapping(uc));
            break;
        case QUnicodeTables::IdnaStatus::Disallowed:
            return {};
        default:
            Q_UNREACHABLE();
        }
    }

    *resultIsAscii = allAscii;
    return result;
}

/*
    Check the rules for an ASCII label.

    Check the size restriction and that the label does not start or end with dashes.

    The label should be nonempty.
*/
static bool validateAsciiLabel(QStringView label)
{
    if (label.size() > MaxDomainLabelLength)
        return false;

    if (label.first() == u'-' || label.last() == u'-')
        return false;

    return std::all_of(label.begin(), label.end(), isValidInNormalizedAsciiLabel<QChar>);
}

namespace {

class DomainValidityChecker
{
    bool domainNameIsBidi = false;
    bool hadBidiErrors = false;

    static constexpr char32_t ZWNJ = U'\u200C';
    static constexpr char32_t ZWJ = U'\u200D';

public:
    DomainValidityChecker() { }
    bool checkLabel(const QString &label, QUrl::AceProcessingOptions options);

private:
    static bool checkContextJRules(QStringView label);
    static bool checkBidiRules(QStringView label);
};

} // anonymous namespace

/*
    Check CONTEXTJ rules according to RFC 5892, appendix A.1 & A.2.

    Rule Set for U+200C (ZWNJ):

      False;

      If Canonical_Combining_Class(Before(cp)) .eq.  Virama Then True;

      If RegExpMatch((Joining_Type:{L,D})(Joining_Type:T)*\u200C

         (Joining_Type:T)*(Joining_Type:{R,D})) Then True;

    Rule Set for U+200D (ZWJ):

      False;

      If Canonical_Combining_Class(Before(cp)) .eq.  Virama Then True;

*/
bool DomainValidityChecker::checkContextJRules(QStringView label)
{
    constexpr unsigned char CombiningClassVirama = 9;

    enum class State {
        Initial,
        LD_T, // L,D with possible following T*
        ZWNJ_T, // ZWNJ with possible following T*
    };
    State regexpState = State::Initial;
    bool previousIsVirama = false;

    for (QStringIterator iter(label); iter.hasNext();) {
        auto ch = iter.next();

        if (ch == ZWJ) {
            if (!previousIsVirama)
                return false;
            regexpState = State::Initial;
        } else if (ch == ZWNJ) {
            if (!previousIsVirama && regexpState != State::LD_T)
                return false;
            regexpState = previousIsVirama ? State::Initial : State::ZWNJ_T;
        } else {
            switch (QChar::joiningType(ch)) {
            case QChar::Joining_Left:
                if (regexpState == State::ZWNJ_T)
                    return false;
                regexpState = State::LD_T;
                break;
            case QChar::Joining_Right:
                regexpState = State::Initial;
                break;
            case QChar::Joining_Dual:
                regexpState = State::LD_T;
                break;
            case QChar::Joining_Transparent:
                break;
            default:
                regexpState = State::Initial;
                break;
            }
        }

        previousIsVirama = QChar::combiningClass(ch) == CombiningClassVirama;
    }

    return regexpState != State::ZWNJ_T;
}

/*
    Check if the label conforms to BiDi rule of RFC 5893.

    1.  The first character must be a character with Bidi property L, R,
        or AL.  If it has the R or AL property, it is an RTL label; if it
        has the L property, it is an LTR label.

    2.  In an RTL label, only characters with the Bidi properties R, AL,
        AN, EN, ES, CS, ET, ON, BN, or NSM are allowed.

    3.  In an RTL label, the end of the label must be a character with
        Bidi property R, AL, EN, or AN, followed by zero or more
        characters with Bidi property NSM.

    4.  In an RTL label, if an EN is present, no AN may be present, and
        vice versa.

    5.  In an LTR label, only characters with the Bidi properties L, EN,
        ES, CS, ET, ON, BN, or NSM are allowed.

    6.  In an LTR label, the end of the label must be a character with
        Bidi property L or EN, followed by zero or more characters with
        Bidi property NSM.
*/
bool DomainValidityChecker::checkBidiRules(QStringView label)
{
    if (label.isEmpty())
        return true;

    QStringIterator iter(label);
    Q_ASSERT(iter.hasNext());

    char32_t ch = iter.next();
    bool labelIsRTL = false;

    switch (QChar::direction(ch)) {
    case QChar::DirL:
        break;
    case QChar::DirR:
    case QChar::DirAL:
        labelIsRTL = true;
        break;
    default:
        return false;
    }

    bool tailOk = true;
    bool labelHasEN = false;
    bool labelHasAN = false;

    while (iter.hasNext()) {
        ch = iter.next();

        switch (QChar::direction(ch)) {
        case QChar::DirR:
        case QChar::DirAL:
            if (!labelIsRTL)
                return false;
            tailOk = true;
            break;

        case QChar::DirL:
            if (labelIsRTL)
                return false;
            tailOk = true;
            break;

        case QChar::DirES:
        case QChar::DirCS:
        case QChar::DirET:
        case QChar::DirON:
        case QChar::DirBN:
            tailOk = false;
            break;

        case QChar::DirNSM:
            break;

        case QChar::DirAN:
            if (labelIsRTL) {
                if (labelHasEN)
                    return false;
                labelHasAN = true;
                tailOk = true;
            } else {
                return false;
            }
            break;

        case QChar::DirEN:
            if (labelIsRTL) {
                if (labelHasAN)
                    return false;
                labelHasEN = true;
            }
            tailOk = true;
            break;

        default:
            return false;
        }
    }

    return tailOk;
}

/*
    Check if the given label is valid according to UTS #46 validity criteria.

    NFC check can be skipped if the label was transformed to NFC before calling
    this function (as optimization).

    The domain name is considered invalid if this function returns false at least
    once.

    1. The label must be in Unicode Normalization Form NFC.
    2. If CheckHyphens, the label must not contain a U+002D HYPHEN-MINUS character
       in both the third and fourth positions.
    3. If CheckHyphens, the label must neither begin nor end with a U+002D HYPHEN-MINUS character.
    4. The label must not contain a U+002E ( . ) FULL STOP.
    5. The label must not begin with a combining mark, that is: General_Category=Mark.
    6. Each code point in the label must only have certain status values according to Section 5,
       IDNA Mapping Table:
        1. For Transitional Processing, each value must be valid.
        2. For Nontransitional Processing, each value must be either valid or deviation.
    7. If CheckJoiners, the label must satisfy the ContextJ rules from Appendix A, in The Unicode
       Code Points and Internationalized Domain Names for Applications (IDNA).
    8. If CheckBidi, and if the domain name is a  Bidi domain name, then the label must satisfy
       all six of the numbered conditions in RFC 5893, Section 2.

    NOTE: Don't use QStringView for label, so that call to QString::normalized() can avoid
          memory allocation when there is nothing to normalize.
*/
bool DomainValidityChecker::checkLabel(const QString &label, QUrl::AceProcessingOptions options)
{
    if (label.isEmpty())
        return true;

    if (label != label.normalized(QString::NormalizationForm_C))
        return false;

    if (label.size() >= 4) {
        // This assumes that the first two characters are in BMP, but that's ok
        // because non-BMP characters are unlikely to be used for specifying
        // future extensions.
        if (label[2] == u'-' && label[3] == u'-')
            return false;
    }

    if (label.startsWith(u'-') || label.endsWith(u'-'))
        return false;

    if (label.contains(u'.'))
        return false;

    QStringIterator iter(label);
    auto c = iter.next();

    if (QChar::isMark(c))
        return false;

    // As optimization, CONTEXTJ rules check can be skipped if no
    // ZWJ/ZWNJ characters were found during the first pass.
    bool hasJoiners = false;

    for (;;) {
        hasJoiners = hasJoiners || c == ZWNJ || c == ZWJ;

        if (!domainNameIsBidi) {
            switch (QChar::direction(c)) {
            case QChar::DirR:
            case QChar::DirAL:
            case QChar::DirAN:
                domainNameIsBidi = true;
                if (hadBidiErrors)
                    return false;
                break;
            default:
                break;
            }
        }

        switch (QUnicodeTables::idnaStatus(c)) {
        case QUnicodeTables::IdnaStatus::Valid:
            break;
        case QUnicodeTables::IdnaStatus::Deviation:
            if (options.testFlag(QUrl::AceTransitionalProcessing))
                return false;
            break;
        default:
            return false;
        }

        if (!iter.hasNext())
            break;
        c = iter.next();
    }

    if (hasJoiners && !checkContextJRules(label))
        return false;

    hadBidiErrors = hadBidiErrors || !checkBidiRules(label);

    if (domainNameIsBidi && hadBidiErrors)
        return false;

    return true;
}

static QString convertToAscii(const QString &normalizedDomain, AceLeadingDot dot)
{
    qsizetype lastIdx = 0;
    QString aceForm; // this variable is here for caching
    QString aceResult;

    while (true) {
        auto idx = normalizedDomain.indexOf(u'.', lastIdx);
        if (idx == -1)
            idx = normalizedDomain.size();

        const auto labelLength = idx - lastIdx;
        if (labelLength == 0) {
            if (idx == normalizedDomain.size())
                break;
            if (dot == ForbidLeadingDot || idx > 0)
                return {}; // two delimiters in a row -- empty label not allowed
        } else {
            const auto label = QStringView(normalizedDomain).sliced(lastIdx, labelLength);
            aceForm.clear();
            qt_punycodeEncoder(label, &aceForm);
            if (aceForm.isEmpty())
                return {};

            aceResult.append(aceForm);
        }

        if (idx == normalizedDomain.size())
            break;

        lastIdx = idx + 1;
        aceResult += u'.';
    }

    return aceResult;
}

static bool checkAsciiDomainName(const QString &normalizedDomain, AceLeadingDot dot,
                                 bool *usesPunycode)
{
    qsizetype lastIdx = 0;
    bool hasPunycode = false;
    *usesPunycode = false;

    while (lastIdx < normalizedDomain.size()) {
        auto idx = normalizedDomain.indexOf(u'.', lastIdx);
        if (idx == -1)
            idx = normalizedDomain.size();

        const auto labelLength = idx - lastIdx;
        if (labelLength == 0) {
            if (idx == normalizedDomain.size())
                break;
            if (dot == ForbidLeadingDot || idx > 0)
                return false; // two delimiters in a row -- empty label not allowed
        } else {
            const auto label = QStringView(normalizedDomain).sliced(lastIdx, labelLength);
            if (!validateAsciiLabel(label))
                return false;

            hasPunycode = hasPunycode || label.startsWith("xn--"_L1);
        }

        lastIdx = idx + 1;
    }

    *usesPunycode = hasPunycode;
    return true;
}

static QString convertToUnicode(const QString &asciiDomain, QUrl::AceProcessingOptions options)
{
    QString result;
    result.reserve(asciiDomain.size());
    qsizetype lastIdx = 0;

    DomainValidityChecker checker;

    while (true) {
        auto idx = asciiDomain.indexOf(u'.', lastIdx);
        if (idx == -1)
            idx = asciiDomain.size();

        const auto labelLength = idx - lastIdx;
        if (labelLength == 0) {
            if (idx == asciiDomain.size())
                break;
        } else {
            const auto label = asciiDomain.sliced(lastIdx, labelLength);
            const auto unicodeLabel = qt_punycodeDecoder(label);

            if (unicodeLabel.isEmpty())
                return asciiDomain;

            if (!checker.checkLabel(unicodeLabel, options))
                return asciiDomain;

            result.append(unicodeLabel);
        }

        if (idx == asciiDomain.size())
            break;

        lastIdx = idx + 1;
        result += u'.';
    }
    return result;
}

QString qt_ACE_do(const QString &domain, AceOperation op, AceLeadingDot dot,
                  QUrl::AceProcessingOptions options)
{
    if (domain.isEmpty())
        return {};

    bool mappedToAscii;
    const QString mapped = mapDomainName(domain, options, &mappedToAscii);
    const QString normalized =
            mappedToAscii ? mapped : mapped.normalized(QString::NormalizationForm_C);

    if (normalized.isEmpty())
        return {};

    bool needsCoversionToUnicode;
    const QString aceResult = mappedToAscii ? normalized : convertToAscii(normalized, dot);
    if (aceResult.isEmpty() || !checkAsciiDomainName(aceResult, dot, &needsCoversionToUnicode))
        return {};

    if (op == ToAceOnly || !needsCoversionToUnicode
        || (!options.testFlag(QUrl::IgnoreIDNWhitelist) && !qt_is_idn_enabled(aceResult))) {
        return aceResult;
    }

    return convertToUnicode(aceResult, options);
}

/*!
    \since 4.2

    Returns the current whitelist of top-level domains that are allowed
    to have non-ASCII characters in their compositions.

    See setIdnWhitelist() for the rationale of this list.

    \sa AceProcessingOption
*/
QStringList QUrl::idnWhitelist()
{
    if (user_idn_whitelist)
        return *user_idn_whitelist;
    static const QStringList list = [] {
        QStringList list;
        list.reserve(idn_whitelist.count());
        int i = 0;
        while (i < idn_whitelist.count()) {
            list << QLatin1StringView(idn_whitelist.at(i));
            ++i;
        }
        return list;
    }();
    return list;
}

/*!
    \since 4.2

    Sets the whitelist of Top-Level Domains (TLDs) that are allowed to have
    non-ASCII characters in domains to the value of \a list.

    Note that if you call this function, you need to do so \e before
    you start any threads that might access idnWhitelist().

    Qt comes with a default list that contains the Internet top-level domains
    that have published support for Internationalized Domain Names (IDNs)
    and rules to guarantee that no deception can happen between similarly-looking
    characters (such as the Latin lowercase letter \c 'a' and the Cyrillic
    equivalent, which in most fonts are visually identical).

    This list is periodically maintained, as registrars publish new rules.

    This function is provided for those who need to manipulate the list, in
    order to add or remove a TLD. It is not recommended to change its value
    for purposes other than testing, as it may expose users to security risks.
*/
void QUrl::setIdnWhitelist(const QStringList &list)
{
    if (!user_idn_whitelist)
        user_idn_whitelist = new QStringList;
    *user_idn_whitelist = list;
}

QT_END_NAMESPACE
