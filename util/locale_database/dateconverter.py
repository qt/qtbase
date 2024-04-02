# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

class Converter (object):
    """Conversion between CLDR and Qt datetime formats.

    Keep in sync with qlocale_mac.mm's macToQtFormat().
    The definitive source of truth is:
    https://www.unicode.org/reports/tr35/tr35-68/tr35-dates.html#Date_Field_Symbol_Table

    See convert() for explanation of the approach taken. Each method
    with a single-letter name is used to scan a prefix of a text,
    presumed to begin with that letter (or one Qt treats as equivalent
    to it) and returns a pair (Qt format, length), to use the given Qt
    format in place of text[:length]. In all cases, length must be
    positive."""

    @staticmethod
    def __is_reserved(ch):
        """Every ASCII letter is a reserved symbol in CLDR datetime formats"""
        assert len(ch) == 1, ch
        return ch.isascii() and ch.isalpha();
    @staticmethod
    def __count_first(text):
        """How many of text[0] appear at the start of text ?"""
        assert text
        return len(text) - len(text.lstrip(text[0]))
    @classmethod
    def __verbatim(cls, text):
        # Used where our format coincides with LDML's, including on length.
        n = cls.__count_first(text)
        return text[:n], n
    @classmethod
    def __treat_as(cls, mimic, text):
        # Helper for aliases
        n = cls.__count_first(text)
        return mimic * n, n

    # Please follow alphabetic order, with two cases of the same
    # letter adjacent, lower before upper.
    @classmethod
    def a(cls, text): # AM/PM indicator; use locale-appropriate case
        return 'Ap', cls.__count_first(text)

    # A: Milliseconds in day. Not supported.
    b = a # AM/PM/noon/midnight
    B = a # "Flexible day period" (e.g. "at night" / "in the day")
    # (Only zh_Hant_TW affected; zh_Hant_{HK,MO} use 'ah', mapped to
    # 'APh', so do the same here.)

    @classmethod
    def c(cls, text): # Stand-alone local day of week
        # Has length-variants for several cases Qt doesn't support, as
        # do 'e' and 'E': just map all simply to weekday, abbreviated
        # or full.
        n = cls.__count_first(text)
        return ('dddd' if n == 4 else 'ddd'), n

    # C: Input skeleton symbol
    d = __verbatim # day (of month or of week, depends on length)
    # D: Day of year. Not supported.
    e = c # Local day of week
    E = c # Just plain day of week
    # F: Day of week in month. Not supported.
    # g: Modified julian day. Not supported.
    # G: Era. Not supported.
    h = __verbatim # Hour 1-12, treat as 0-11
    H = __verbatim # Hour 0-23
    # j: Input skeleton symbol
    # J: Input skeleton symbol

    @classmethod
    def k(cls, text): # Hour 1-24, treat as 0-23
        return cls.__treat_as('H', text)
    @classmethod
    def K(cls, text): # Hour 0-11
        return cls.__treat_as('h', text)

    # l: Deprecated Chinese leap month indicator.
    @classmethod
    def L(cls, text): # Stand-alone month names: treat as plain month names.
        n = cls.__count_first(text)
        # Length five is narrow; treat same as abbreviated; anything
        # shorter matches Qt's month forms.
        return ('MMM' if n > 4 else 'M' * n), n

    m = __verbatim # Minute within the hour.
    M = L # Plain month names, possibly abbreviated, and numbers.

    @classmethod
    def O(cls, text): # Localized GMT±offset formats. Map to Z-or-UTC±HH:mm
        return 't', cls.__count_first(text)

    # q: Quarter. Not supported.
    # Q: Quarter. Not supported.

    s = __verbatim # Seconds within the minute.
    @classmethod
    def S(cls, text): # Fractional seconds. Only milliseconds supported.
        # FIXME: spec is unclear, do we need to include the leading
        # dot or not ? For now, no known locale actually exercises
        # this, so stick with what we've done on Darwin since long
        # before adding support here.
        n = cls.__count_first(text)
        return ('z' if n < 3 else 'zzz'), n

    @classmethod
    def u(cls, text): # Extended year (numeric)
        # Officially, 'u' is simply the full year number, zero-padded
        # to the length of the field. Qt's closest to that is four-digit.
        # It explicitly has no special case for two-digit year.
        return 'yyyy', cls.__count_first(text)

    # U: Cyclic Year Name. Not supported
    @classmethod
    def v(cls, text): # Generic non-location format. Map to name.
        return 'tttt', cls.__count_first(text)

    V = v # Zone ID in various forms; VV is IANA ID. Map to name.
    # w: Week of year. Not supported.
    # W: Week of month. Not supported.

    @classmethod
    def x(cls, text): # Variations on offset format.
        n = cls.__count_first(text)
        # Ignore: n == 1 may omit minutes, n > 3 may include seconds.
        return ('ttt' if n > 1 and n & 1 else 'tt'), n
    X = x # Should use Z for zero offset.

    @classmethod
    def y(cls, text): # Year number.
        n = cls.__count_first(text)
        return ('yy' if n == 2 else 'yyyy'), n
    # Y: Year for Week-of-year calendars

    z = v # Specific (i.e. distinguish standard from DST) non-location format.
    @classmethod
    def Z(cls, text): # Offset format, optionaly with GMT (Qt uses UTC) prefix.
        n = cls.__count_first(text)
        return ('tt' if n < 4 else 'ttt' if n > 4 else 't'), n

    @staticmethod
    def scanQuote(text): # Can't have ' as a method name, so handle specially
        assert text.startswith("'")
        i = text.find("'", 1) # Find the next; -1 if not present.
        i = len(text) if i < 0 else i + 1 # Include the close-quote.
        return text[:i], i

    # Now put all of those to use:
    @classmethod
    def convert(cls, text):
        """Convert a CLDR datetime format string into a Qt one.

        Presumes that the caller will ''.join() the fragments it
        yields. Each sequence of CLDR field symbols that corresponds
        to a Qt format token is converted to it; all other CLDR field
        symbols are discarded; the literals in between fields are
        preserved verbatim, except that space and hyphen separators
        immediately before a discarded field are discarded with it.

        The approach is to look at the first symbol of the remainder
        of the text, at each iteration, and use that first symbol to
        select a function that will identify how much of the text to
        consume and what to replace it with."""
        sep = ''
        while text:
            ch = text[0]
            if ch == "'":
                quoted, length = cls.scanQuote(text)
                text = text[length:]
                sep += quoted
            elif hasattr(cls, ch):
                qtform, length = getattr(cls, ch)(text)
                assert qtform and length > 0, (ch, text, qtform, length)
                text = text[length:]
                if sep:
                    yield sep
                    sep = ''
                yield qtform
            elif cls.__is_reserved(ch):
                text = text[cls.__count_first(text):]
                # Discard space or dash separator that was only there
                # for the sake of the unsupported field:
                sep = sep.rstrip(' -')
                # TODO: should we also strip [ -]* from text
                # immediately following unsupported forms ?
            else:
                sep += ch
                text = text[1:]
        if sep:
            yield sep

def convert_date(text):
    # See Converter.convert()
    return ''.join(Converter.convert(text))
