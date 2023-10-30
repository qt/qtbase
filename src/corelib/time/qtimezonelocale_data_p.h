// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: Unicode-3.0

#ifndef QTIMEZONELOCALE_DATA_P_H
#define QTIMEZONELOCALE_DATA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

// Only qtimezonelocale.cpp should #include this (after other things it needs),
// and even that only when feature icu is disabled.
#include "qtimezonelocale_p.h"

QT_REQUIRE_CONFIG(timezone_locale);
#if QT_CONFIG(icu)
#  error "This file should only be needed (or seen) when ICU is not in use"
#endif

QT_BEGIN_NAMESPACE

#ifdef QT_CLDR_ZONE_DEBUG
#  define LOCALE_TAGS(lang, script, land) lang, script, land,
#else
#  define LOCALE_TAGS(lang, script, land)
#endif

namespace QtTimeZoneLocale {

/*
    Locale-specific data for timezone naming.
    https://www.unicode.org/reports/tr35/tr35-68/tr35-dates.html#Time_Zone_Names

    Available data, per locale, from LocaleScanner.timeZoneNames():

        formats: {
            'hour': ('+HH:mm', '-HH:mm'),
            'GMT': 'GMT%0',
            'region': ("%0 Time", std, dst),
            'fallback': "%1 (%0)" }
        zones: { ianaid: {
                'exemplarCity': name,
                'long': (gen, std, dst), 'short': (gen, std, dst) } }
        metazones: { meta: {'long': (gen, std, dst), 'short': (gen, std, dst) } }

    Mapped to C++ data-structures below (defined in QTZL_p.h) and used by
    QTZL.cpp, in conjunction with QTZP_data_p.h's locale-independent data
    conditioned on timezone_locale.
*/

// GENERATED PART STARTS HERE

// sorted by locale index, then iana name
static constexpr LocaleZoneExemplar localeZoneExemplarTable[] = {
    // locInd, ianaInd, xcty{ind, sz}
    // 47770 entries @ 9 bytes per entry (maybe padded to 10 or 12 bytes)
    { 674, 0, 0, 0, }, // dummy
};

// sorted by locale index, then iana name
static constexpr LocaleZoneNames localeZoneNameTable[] = {
    // locInd, ianaInd, (lngGen, srtGen, lngStd, srtStd, lngDst, srtDst){ind, sz}
    // 781 entries @ 28 bytes per entry
    { 674, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, // dummy
};

// sort by locale index, meta key
static constexpr LocaleMetaZoneLongNames localeMetaZoneLongNameTable[] = {
    // locInd, metaKey, (generic, standard, DST){ind, sz}
    // 25257 entries @ 19 bytes per entry (maybe padded to 20)
    { 674, 0, 0, 0, 0, 0, 0, 0, }, // dummy
};

// sort by locale index, meta key
static constexpr LocaleMetaZoneShortNames localeMetaZoneShortNameTable[] = {
    // locInd, metaKey, (generic, standard, DST){ind, sz}
    // 1416 entries @ 13 bytes per entry (maybe padded to 14 or 16)
    { 674, 0, 0, 0, 0, 0, 0, 0, }, // dummy
};

// In locale-index order (see ../qlocale_data_p.h):
static constexpr LocaleZoneData localeZoneData[] = {
    // LOCALE_TAGS(lng,scp,ter) xct1st, zn1st, ml1st, ms1st, (+hr, -hr, gmt, flbk, rgen, rstd, rdst){ind,sz}
    // 674 entries @ (6+) 75 bytes (maybe padded to (6+ or 8+) 76 bytes)
    { LOCALE_TAGS(  1,  0,  0)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // C/AnyScript/AnyTerritory
    { LOCALE_TAGS(  2, 27, 90)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Abkhazian/Cyrillic/Georgia
    { LOCALE_TAGS(  3, 66, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Afar/Latin/Ethiopia
    { LOCALE_TAGS(  3, 66, 67)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Afar/Latin/Djibouti
    { LOCALE_TAGS(  3, 66, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Afar/Latin/Eritrea
    { LOCALE_TAGS(  4, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Afrikaans/Latin/South Africa
    { LOCALE_TAGS(  4, 66,162)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Afrikaans/Latin/Namibia
    { LOCALE_TAGS(  5, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Aghem/Latin/Cameroon
    { LOCALE_TAGS(  6, 66, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Akan/Latin/Ghana
    { LOCALE_TAGS(  8, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Akoose/Latin/Cameroon
    { LOCALE_TAGS(  9, 66,  3)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Albanian/Latin/Albania
    { LOCALE_TAGS(  9, 66,126)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Albanian/Latin/Kosovo
    { LOCALE_TAGS(  9, 66,140)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Albanian/Latin/Macedonia
    { LOCALE_TAGS( 11, 33, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Amharic/Ethiopic/Ethiopia
    { LOCALE_TAGS( 14,  4, 71)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Egypt
    { LOCALE_TAGS( 14,  4,  4)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Algeria
    { LOCALE_TAGS( 14,  4, 19)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Bahrain
    { LOCALE_TAGS( 14,  4, 48)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Chad
    { LOCALE_TAGS( 14,  4, 55)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Comoros
    { LOCALE_TAGS( 14,  4, 67)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Djibouti
    { LOCALE_TAGS( 14,  4, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Eritrea
    { LOCALE_TAGS( 14,  4,113)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Iraq
    { LOCALE_TAGS( 14,  4,116)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Israel
    { LOCALE_TAGS( 14,  4,122)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Jordan
    { LOCALE_TAGS( 14,  4,127)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Kuwait
    { LOCALE_TAGS( 14,  4,132)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Lebanon
    { LOCALE_TAGS( 14,  4,135)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Libya
    { LOCALE_TAGS( 14,  4,149)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Mauritania
    { LOCALE_TAGS( 14,  4,159)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Morocco
    { LOCALE_TAGS( 14,  4,176)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Oman
    { LOCALE_TAGS( 14,  4,180)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Palestinian Territories
    { LOCALE_TAGS( 14,  4,190)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Qatar
    { LOCALE_TAGS( 14,  4,205)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Saudi Arabia
    { LOCALE_TAGS( 14,  4,215)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Somalia
    { LOCALE_TAGS( 14,  4,219)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/South Sudan
    { LOCALE_TAGS( 14,  4,222)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Sudan
    { LOCALE_TAGS( 14,  4,227)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Syria
    { LOCALE_TAGS( 14,  4,238)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Tunisia
    { LOCALE_TAGS( 14,  4,245)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/United Arab Emirates
    { LOCALE_TAGS( 14,  4,257)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Western Sahara
    { LOCALE_TAGS( 14,  4,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/world
    { LOCALE_TAGS( 14,  4,259)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Arabic/Arabic/Yemen
    { LOCALE_TAGS( 15, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Aragonese/Latin/Spain
    { LOCALE_TAGS( 17,  5, 12)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Armenian/Armenian/Armenia
    { LOCALE_TAGS( 18,  9,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Assamese/Bangla/India
    { LOCALE_TAGS( 19, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Asturian/Latin/Spain
    { LOCALE_TAGS( 20, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Asu/Latin/Tanzania
    { LOCALE_TAGS( 21, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Atsam/Latin/Nigeria
    { LOCALE_TAGS( 25, 66, 17)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Azerbaijani/Latin/Azerbaijan
    { LOCALE_TAGS( 25,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Azerbaijani/Arabic/Iran
    { LOCALE_TAGS( 25,  4,113)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Azerbaijani/Arabic/Iraq
    { LOCALE_TAGS( 25,  4,239)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Azerbaijani/Arabic/Turkey
    { LOCALE_TAGS( 25, 27, 17)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Azerbaijani/Cyrillic/Azerbaijan
    { LOCALE_TAGS( 26, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bafia/Latin/Cameroon
    { LOCALE_TAGS( 28, 66,145)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bambara/Latin/Mali
    { LOCALE_TAGS( 28, 90,145)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bambara/Nko/Mali
    { LOCALE_TAGS( 30,  9, 20)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bangla/Bangla/Bangladesh
    { LOCALE_TAGS( 30,  9,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bangla/Bangla/India
    { LOCALE_TAGS( 31, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Basaa/Latin/Cameroon
    { LOCALE_TAGS( 32, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bashkir/Cyrillic/Russia
    { LOCALE_TAGS( 33, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Basque/Latin/Spain
    { LOCALE_TAGS( 35, 27, 22)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Belarusian/Cyrillic/Belarus
    { LOCALE_TAGS( 36, 66,260)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bemba/Latin/Zambia
    { LOCALE_TAGS( 37, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bena/Latin/Tanzania
    { LOCALE_TAGS( 38, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bhojpuri/Devanagari/India
    { LOCALE_TAGS( 40, 33, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Blin/Ethiopic/Eritrea
    { LOCALE_TAGS( 41, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bodo/Devanagari/India
    { LOCALE_TAGS( 42, 66, 29)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bosnian/Latin/Bosnia and Herzegovina
    { LOCALE_TAGS( 42, 27, 29)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bosnian/Cyrillic/Bosnia and Herzegovina
    { LOCALE_TAGS( 43, 66, 84)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Breton/Latin/France
    { LOCALE_TAGS( 45, 27, 36)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Bulgarian/Cyrillic/Bulgaria
    { LOCALE_TAGS( 46, 86,161)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Burmese/Myanmar/Myanmar
    { LOCALE_TAGS( 47,137,107)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Cantonese/Traditional Han/Hong Kong
    { LOCALE_TAGS( 47,118, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Cantonese/Simplified Han/China
    { LOCALE_TAGS( 48, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Catalan/Latin/Spain
    { LOCALE_TAGS( 48, 66,  6)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Catalan/Latin/Andorra
    { LOCALE_TAGS( 48, 66, 84)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Catalan/Latin/France
    { LOCALE_TAGS( 48, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Catalan/Latin/Italy
    { LOCALE_TAGS( 49, 66,185)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Cebuano/Latin/Philippines
    { LOCALE_TAGS( 50, 66,159)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Central Atlas Tamazight/Latin/Morocco
    { LOCALE_TAGS( 51,  4,113)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Central Kurdish/Arabic/Iraq
    { LOCALE_TAGS( 51,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Central Kurdish/Arabic/Iran
    { LOCALE_TAGS( 52, 21, 20)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chakma/Chakma/Bangladesh
    { LOCALE_TAGS( 52, 21,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chakma/Chakma/India
    { LOCALE_TAGS( 54, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chechen/Cyrillic/Russia
    { LOCALE_TAGS( 55, 23,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Cherokee/Cherokee/United States
    { LOCALE_TAGS( 56, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chickasaw/Latin/United States
    { LOCALE_TAGS( 57, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chiga/Latin/Uganda
    { LOCALE_TAGS( 58,118, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Simplified Han/China
    { LOCALE_TAGS( 58,118,107)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Simplified Han/Hong Kong
    { LOCALE_TAGS( 58,118,139)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Simplified Han/Macao
    { LOCALE_TAGS( 58,118,210)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Simplified Han/Singapore
    { LOCALE_TAGS( 58,137,107)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Traditional Han/Hong Kong
    { LOCALE_TAGS( 58,137,139)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Traditional Han/Macao
    { LOCALE_TAGS( 58,137,228)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chinese/Traditional Han/Taiwan
    { LOCALE_TAGS( 59, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Church/Cyrillic/Russia
    { LOCALE_TAGS( 60, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Chuvash/Cyrillic/Russia
    { LOCALE_TAGS( 61, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Colognian/Latin/Germany
    { LOCALE_TAGS( 63, 66,246)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Cornish/Latin/United Kingdom
    { LOCALE_TAGS( 64, 66, 84)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Corsican/Latin/France
    { LOCALE_TAGS( 66, 66, 60)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Croatian/Latin/Croatia
    { LOCALE_TAGS( 66, 66, 29)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Croatian/Latin/Bosnia and Herzegovina
    { LOCALE_TAGS( 67, 66, 64)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Czech/Latin/Czechia
    { LOCALE_TAGS( 68, 66, 65)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Danish/Latin/Denmark
    { LOCALE_TAGS( 68, 66, 95)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Danish/Latin/Greenland
    { LOCALE_TAGS( 69,132,144)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Divehi/Thaana/Maldives
    { LOCALE_TAGS( 70, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dogri/Devanagari/India
    { LOCALE_TAGS( 71, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Duala/Latin/Cameroon
    { LOCALE_TAGS( 72, 66,165)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Netherlands
    { LOCALE_TAGS( 72, 66, 13)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Aruba
    { LOCALE_TAGS( 72, 66, 23)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Belgium
    { LOCALE_TAGS( 72, 66, 44)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Caribbean Netherlands
    { LOCALE_TAGS( 72, 66, 62)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Curacao
    { LOCALE_TAGS( 72, 66,211)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Sint Maarten
    { LOCALE_TAGS( 72, 66,223)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dutch/Latin/Suriname
    { LOCALE_TAGS( 73,134, 27)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Dzongkha/Tibetan/Bhutan
    { LOCALE_TAGS( 74, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Embu/Latin/Kenya
    { LOCALE_TAGS( 75, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/United States
    { LOCALE_TAGS( 75, 28,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Deseret/United States
    { LOCALE_TAGS( 75, 66,  5)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/American Samoa
    { LOCALE_TAGS( 75, 66,  8)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Anguilla
    { LOCALE_TAGS( 75, 66, 10)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Antigua and Barbuda
    { LOCALE_TAGS( 75, 66, 15)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Australia
    { LOCALE_TAGS( 75, 66, 16)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Austria
    { LOCALE_TAGS( 75, 66, 18)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Bahamas
    { LOCALE_TAGS( 75, 66, 21)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Barbados
    { LOCALE_TAGS( 75, 66, 23)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Belgium
    { LOCALE_TAGS( 75, 66, 24)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Belize
    { LOCALE_TAGS( 75, 66, 26)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Bermuda
    { LOCALE_TAGS( 75, 66, 30)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Botswana
    { LOCALE_TAGS( 75, 66, 33)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/British Indian Ocean Territory
    { LOCALE_TAGS( 75, 66, 34)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/British Virgin Islands
    { LOCALE_TAGS( 75, 66, 38)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Burundi
    { LOCALE_TAGS( 75, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Cameroon
    { LOCALE_TAGS( 75, 66, 41)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Canada
    { LOCALE_TAGS( 75, 66, 45)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Cayman Islands
    { LOCALE_TAGS( 75, 66, 51)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Christmas Island
    { LOCALE_TAGS( 75, 66, 53)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Cocos Islands
    { LOCALE_TAGS( 75, 66, 58)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Cook Islands
    { LOCALE_TAGS( 75, 66, 63)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Cyprus
    { LOCALE_TAGS( 75, 66, 65)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Denmark
    { LOCALE_TAGS( 75, 66, 66)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Diego Garcia
    { LOCALE_TAGS( 75, 66, 68)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Dominica
    { LOCALE_TAGS( 75, 66, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Eritrea
    { LOCALE_TAGS( 75, 66, 76)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Eswatini
    { LOCALE_TAGS( 75, 66, 78)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Europe
    { LOCALE_TAGS( 75, 66, 80)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Falkland Islands
    { LOCALE_TAGS( 75, 66, 82)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Fiji
    { LOCALE_TAGS( 75, 66, 83)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Finland
    { LOCALE_TAGS( 75, 66, 89)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Gambia
    { LOCALE_TAGS( 75, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Germany
    { LOCALE_TAGS( 75, 66, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Ghana
    { LOCALE_TAGS( 75, 66, 93)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Gibraltar
    { LOCALE_TAGS( 75, 66, 96)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Grenada
    { LOCALE_TAGS( 75, 66, 98)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Guam
    { LOCALE_TAGS( 75, 66,100)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Guernsey
    { LOCALE_TAGS( 75, 66,103)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Guyana
    { LOCALE_TAGS( 75, 66,107)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Hong Kong
    { LOCALE_TAGS( 75, 66,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/India
    { LOCALE_TAGS( 75, 66,111)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Indonesia
    { LOCALE_TAGS( 75, 66,114)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Ireland
    { LOCALE_TAGS( 75, 66,115)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Isle of Man
    { LOCALE_TAGS( 75, 66,116)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Israel
    { LOCALE_TAGS( 75, 66,119)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Jamaica
    { LOCALE_TAGS( 75, 66,121)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Jersey
    { LOCALE_TAGS( 75, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Kenya
    { LOCALE_TAGS( 75, 66,125)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Kiribati
    { LOCALE_TAGS( 75, 66,133)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Lesotho
    { LOCALE_TAGS( 75, 66,134)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Liberia
    { LOCALE_TAGS( 75, 66,139)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Macao
    { LOCALE_TAGS( 75, 66,141)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Madagascar
    { LOCALE_TAGS( 75, 66,142)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Malawi
    { LOCALE_TAGS( 75, 66,143)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Malaysia
    { LOCALE_TAGS( 75, 66,144)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Maldives
    { LOCALE_TAGS( 75, 66,146)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Malta
    { LOCALE_TAGS( 75, 66,147)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Marshall Islands
    { LOCALE_TAGS( 75, 66,150)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Mauritius
    { LOCALE_TAGS( 75, 66,153)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Micronesia
    { LOCALE_TAGS( 75, 66,158)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Montserrat
    { LOCALE_TAGS( 75, 66,162)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Namibia
    { LOCALE_TAGS( 75, 66,163)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Nauru
    { LOCALE_TAGS( 75, 66,165)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Netherlands
    { LOCALE_TAGS( 75, 66,167)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/New Zealand
    { LOCALE_TAGS( 75, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Nigeria
    { LOCALE_TAGS( 75, 66,171)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Niue
    { LOCALE_TAGS( 75, 66,172)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Norfolk Island
    { LOCALE_TAGS( 75, 66,173)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Northern Mariana Islands
    { LOCALE_TAGS( 75, 66,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Pakistan
    { LOCALE_TAGS( 75, 66,179)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Palau
    { LOCALE_TAGS( 75, 66,182)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Papua New Guinea
    { LOCALE_TAGS( 75, 66,185)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Philippines
    { LOCALE_TAGS( 75, 66,186)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Pitcairn
    { LOCALE_TAGS( 75, 66,189)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Puerto Rico
    { LOCALE_TAGS( 75, 66,194)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Rwanda
    { LOCALE_TAGS( 75, 66,196)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Saint Helena
    { LOCALE_TAGS( 75, 66,197)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Saint Kitts and Nevis
    { LOCALE_TAGS( 75, 66,198)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Saint Lucia
    { LOCALE_TAGS( 75, 66,201)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Saint Vincent and Grenadines
    { LOCALE_TAGS( 75, 66,202)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Samoa
    { LOCALE_TAGS( 75, 66,208)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Seychelles
    { LOCALE_TAGS( 75, 66,209)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Sierra Leone
    { LOCALE_TAGS( 75, 66,210)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Singapore
    { LOCALE_TAGS( 75, 66,211)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Sint Maarten
    { LOCALE_TAGS( 75, 66,213)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Slovenia
    { LOCALE_TAGS( 75, 66,214)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Solomon Islands
    { LOCALE_TAGS( 75, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/South Africa
    { LOCALE_TAGS( 75, 66,219)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/South Sudan
    { LOCALE_TAGS( 75, 66,222)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Sudan
    { LOCALE_TAGS( 75, 66,225)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Sweden
    { LOCALE_TAGS( 75, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Switzerland
    { LOCALE_TAGS( 75, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Tanzania
    { LOCALE_TAGS( 75, 66,234)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Tokelau
    { LOCALE_TAGS( 75, 66,235)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Tonga
    { LOCALE_TAGS( 75, 66,236)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Trinidad and Tobago
    { LOCALE_TAGS( 75, 66,241)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Turks and Caicos Islands
    { LOCALE_TAGS( 75, 66,242)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Tuvalu
    { LOCALE_TAGS( 75, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Uganda
    { LOCALE_TAGS( 75, 66,245)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/United Arab Emirates
    { LOCALE_TAGS( 75, 66,246)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/United Kingdom
    { LOCALE_TAGS( 75, 66,247)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/United States Outlying Islands
    { LOCALE_TAGS( 75, 66,249)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/United States Virgin Islands
    { LOCALE_TAGS( 75, 66,252)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Vanuatu
    { LOCALE_TAGS( 75, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/world
    { LOCALE_TAGS( 75, 66,260)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Zambia
    { LOCALE_TAGS( 75, 66,261)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Latin/Zimbabwe
    { LOCALE_TAGS( 75,115,246)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // English/Shavian/United Kingdom
    { LOCALE_TAGS( 76, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Erzya/Cyrillic/Russia
    { LOCALE_TAGS( 77, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Esperanto/Latin/world
    { LOCALE_TAGS( 78, 66, 75)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Estonian/Latin/Estonia
    { LOCALE_TAGS( 79, 66, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ewe/Latin/Ghana
    { LOCALE_TAGS( 79, 66,233)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ewe/Latin/Togo
    { LOCALE_TAGS( 80, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ewondo/Latin/Cameroon
    { LOCALE_TAGS( 81, 66, 81)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Faroese/Latin/Faroe Islands
    { LOCALE_TAGS( 81, 66, 65)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Faroese/Latin/Denmark
    { LOCALE_TAGS( 83, 66,185)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Filipino/Latin/Philippines
    { LOCALE_TAGS( 84, 66, 83)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Finnish/Latin/Finland
    { LOCALE_TAGS( 85, 66, 84)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/France
    { LOCALE_TAGS( 85, 66,  4)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Algeria
    { LOCALE_TAGS( 85, 66, 23)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Belgium
    { LOCALE_TAGS( 85, 66, 25)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Benin
    { LOCALE_TAGS( 85, 66, 37)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Burkina Faso
    { LOCALE_TAGS( 85, 66, 38)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Burundi
    { LOCALE_TAGS( 85, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Cameroon
    { LOCALE_TAGS( 85, 66, 41)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Canada
    { LOCALE_TAGS( 85, 66, 46)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Central African Republic
    { LOCALE_TAGS( 85, 66, 48)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Chad
    { LOCALE_TAGS( 85, 66, 55)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Comoros
    { LOCALE_TAGS( 85, 66, 56)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Congo - Brazzaville
    { LOCALE_TAGS( 85, 66, 57)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Congo - Kinshasa
    { LOCALE_TAGS( 85, 66, 67)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Djibouti
    { LOCALE_TAGS( 85, 66, 73)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Equatorial Guinea
    { LOCALE_TAGS( 85, 66, 85)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/French Guiana
    { LOCALE_TAGS( 85, 66, 86)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/French Polynesia
    { LOCALE_TAGS( 85, 66, 88)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Gabon
    { LOCALE_TAGS( 85, 66, 97)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Guadeloupe
    { LOCALE_TAGS( 85, 66,102)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Guinea
    { LOCALE_TAGS( 85, 66,104)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Haiti
    { LOCALE_TAGS( 85, 66,118)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Ivory Coast
    { LOCALE_TAGS( 85, 66,138)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Luxembourg
    { LOCALE_TAGS( 85, 66,141)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Madagascar
    { LOCALE_TAGS( 85, 66,145)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Mali
    { LOCALE_TAGS( 85, 66,148)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Martinique
    { LOCALE_TAGS( 85, 66,149)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Mauritania
    { LOCALE_TAGS( 85, 66,150)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Mauritius
    { LOCALE_TAGS( 85, 66,151)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Mayotte
    { LOCALE_TAGS( 85, 66,155)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Monaco
    { LOCALE_TAGS( 85, 66,159)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Morocco
    { LOCALE_TAGS( 85, 66,166)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/New Caledonia
    { LOCALE_TAGS( 85, 66,170)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Niger
    { LOCALE_TAGS( 85, 66,191)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Reunion
    { LOCALE_TAGS( 85, 66,194)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Rwanda
    { LOCALE_TAGS( 85, 66,195)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Saint Barthelemy
    { LOCALE_TAGS( 85, 66,199)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Saint Martin
    { LOCALE_TAGS( 85, 66,200)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Saint Pierre and Miquelon
    { LOCALE_TAGS( 85, 66,206)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Senegal
    { LOCALE_TAGS( 85, 66,208)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Seychelles
    { LOCALE_TAGS( 85, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Switzerland
    { LOCALE_TAGS( 85, 66,227)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Syria
    { LOCALE_TAGS( 85, 66,233)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Togo
    { LOCALE_TAGS( 85, 66,238)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Tunisia
    { LOCALE_TAGS( 85, 66,252)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Vanuatu
    { LOCALE_TAGS( 85, 66,256)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // French/Latin/Wallis and Futuna
    { LOCALE_TAGS( 86, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Friulian/Latin/Italy
    { LOCALE_TAGS( 87, 66,206)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Senegal
    { LOCALE_TAGS( 87,  1, 37)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Burkina Faso
    { LOCALE_TAGS( 87,  1, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Cameroon
    { LOCALE_TAGS( 87,  1, 89)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Gambia
    { LOCALE_TAGS( 87,  1, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Ghana
    { LOCALE_TAGS( 87,  1,101)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Guinea-Bissau
    { LOCALE_TAGS( 87,  1,102)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Guinea
    { LOCALE_TAGS( 87,  1,134)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Liberia
    { LOCALE_TAGS( 87,  1,149)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Mauritania
    { LOCALE_TAGS( 87,  1,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Nigeria
    { LOCALE_TAGS( 87,  1,170)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Niger
    { LOCALE_TAGS( 87,  1,206)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Senegal
    { LOCALE_TAGS( 87,  1,209)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Adlam/Sierra Leone
    { LOCALE_TAGS( 87, 66, 37)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Burkina Faso
    { LOCALE_TAGS( 87, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Cameroon
    { LOCALE_TAGS( 87, 66, 89)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Gambia
    { LOCALE_TAGS( 87, 66, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Ghana
    { LOCALE_TAGS( 87, 66,101)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Guinea-Bissau
    { LOCALE_TAGS( 87, 66,102)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Guinea
    { LOCALE_TAGS( 87, 66,134)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Liberia
    { LOCALE_TAGS( 87, 66,149)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Mauritania
    { LOCALE_TAGS( 87, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Nigeria
    { LOCALE_TAGS( 87, 66,170)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Niger
    { LOCALE_TAGS( 87, 66,209)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Fulah/Latin/Sierra Leone
    { LOCALE_TAGS( 88, 66,246)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Gaelic/Latin/United Kingdom
    { LOCALE_TAGS( 89, 66, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ga/Latin/Ghana
    { LOCALE_TAGS( 90, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Galician/Latin/Spain
    { LOCALE_TAGS( 91, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ganda/Latin/Uganda
    { LOCALE_TAGS( 92, 33, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Geez/Ethiopic/Ethiopia
    { LOCALE_TAGS( 92, 33, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Geez/Ethiopic/Eritrea
    { LOCALE_TAGS( 93, 35, 90)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Georgian/Georgian/Georgia
    { LOCALE_TAGS( 94, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Germany
    { LOCALE_TAGS( 94, 66, 16)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Austria
    { LOCALE_TAGS( 94, 66, 23)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Belgium
    { LOCALE_TAGS( 94, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Italy
    { LOCALE_TAGS( 94, 66,136)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Liechtenstein
    { LOCALE_TAGS( 94, 66,138)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Luxembourg
    { LOCALE_TAGS( 94, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // German/Latin/Switzerland
    { LOCALE_TAGS( 96, 39, 94)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Greek/Greek/Greece
    { LOCALE_TAGS( 96, 39, 63)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Greek/Greek/Cyprus
    { LOCALE_TAGS( 97, 66,183)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Guarani/Latin/Paraguay
    { LOCALE_TAGS( 98, 40,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Gujarati/Gujarati/India
    { LOCALE_TAGS( 99, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Gusii/Latin/Kenya
    { LOCALE_TAGS(101, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hausa/Latin/Nigeria
    { LOCALE_TAGS(101,  4,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hausa/Arabic/Nigeria
    { LOCALE_TAGS(101,  4,222)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hausa/Arabic/Sudan
    { LOCALE_TAGS(101, 66, 92)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hausa/Latin/Ghana
    { LOCALE_TAGS(101, 66,170)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hausa/Latin/Niger
    { LOCALE_TAGS(102, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hawaiian/Latin/United States
    { LOCALE_TAGS(103, 47,116)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hebrew/Hebrew/Israel
    { LOCALE_TAGS(105, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hindi/Devanagari/India
    { LOCALE_TAGS(105, 66,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hindi/Latin/India
    { LOCALE_TAGS(107, 66,108)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Hungarian/Latin/Hungary
    { LOCALE_TAGS(108, 66,109)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Icelandic/Latin/Iceland
    { LOCALE_TAGS(109, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ido/Latin/world
    { LOCALE_TAGS(110, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Igbo/Latin/Nigeria
    { LOCALE_TAGS(111, 66, 83)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Inari Sami/Latin/Finland
    { LOCALE_TAGS(112, 66,111)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Indonesian/Latin/Indonesia
    { LOCALE_TAGS(114, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Interlingua/Latin/world
    { LOCALE_TAGS(115, 66, 75)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Interlingue/Latin/Estonia
    { LOCALE_TAGS(116, 18, 41)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Inuktitut/Canadian Aboriginal/Canada
    { LOCALE_TAGS(116, 66, 41)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Inuktitut/Latin/Canada
    { LOCALE_TAGS(118, 66,114)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Irish/Latin/Ireland
    { LOCALE_TAGS(118, 66,246)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Irish/Latin/United Kingdom
    { LOCALE_TAGS(119, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Italian/Latin/Italy
    { LOCALE_TAGS(119, 66,203)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Italian/Latin/San Marino
    { LOCALE_TAGS(119, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Italian/Latin/Switzerland
    { LOCALE_TAGS(119, 66,253)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Italian/Latin/Vatican City
    { LOCALE_TAGS(120, 53,120)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Japanese/Japanese/Japan
    { LOCALE_TAGS(121, 66,111)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Javanese/Latin/Indonesia
    { LOCALE_TAGS(122, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Jju/Latin/Nigeria
    { LOCALE_TAGS(123, 66,206)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Jola-Fonyi/Latin/Senegal
    { LOCALE_TAGS(124, 66, 43)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kabuverdianu/Latin/Cape Verde
    { LOCALE_TAGS(125, 66,  4)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kabyle/Latin/Algeria
    { LOCALE_TAGS(126, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kako/Latin/Cameroon
    { LOCALE_TAGS(127, 66, 95)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kalaallisut/Latin/Greenland
    { LOCALE_TAGS(128, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kalenjin/Latin/Kenya
    { LOCALE_TAGS(129, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kamba/Latin/Kenya
    { LOCALE_TAGS(130, 56,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kannada/Kannada/India
    { LOCALE_TAGS(132,  4,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kashmiri/Arabic/India
    { LOCALE_TAGS(132, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kashmiri/Devanagari/India
    { LOCALE_TAGS(133, 27,123)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kazakh/Cyrillic/Kazakhstan
    { LOCALE_TAGS(134, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kenyang/Latin/Cameroon
    { LOCALE_TAGS(135, 60, 39)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Khmer/Khmer/Cambodia
    { LOCALE_TAGS(136, 66, 99)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kiche/Latin/Guatemala
    { LOCALE_TAGS(137, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kikuyu/Latin/Kenya
    { LOCALE_TAGS(138, 66,194)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kinyarwanda/Latin/Rwanda
    { LOCALE_TAGS(141, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Konkani/Devanagari/India
    { LOCALE_TAGS(142, 63,218)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Korean/Korean/South Korea
    { LOCALE_TAGS(142, 63, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Korean/Korean/China
    { LOCALE_TAGS(142, 63,174)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Korean/Korean/North Korea
    { LOCALE_TAGS(144, 66,145)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Koyraboro Senni/Latin/Mali
    { LOCALE_TAGS(145, 66,145)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Koyra Chiini/Latin/Mali
    { LOCALE_TAGS(146, 66,134)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kpelle/Latin/Liberia
    { LOCALE_TAGS(146, 66,102)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kpelle/Latin/Guinea
    { LOCALE_TAGS(148, 66,239)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kurdish/Latin/Turkey
    { LOCALE_TAGS(149, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kwasio/Latin/Cameroon
    { LOCALE_TAGS(150, 27,128)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kyrgyz/Cyrillic/Kyrgyzstan
    { LOCALE_TAGS(151, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lakota/Latin/United States
    { LOCALE_TAGS(152, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Langi/Latin/Tanzania
    { LOCALE_TAGS(153, 65,129)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lao/Lao/Laos
    { LOCALE_TAGS(154, 66,253)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Latin/Latin/Vatican City
    { LOCALE_TAGS(155, 66,131)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Latvian/Latin/Latvia
    { LOCALE_TAGS(158, 66, 57)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lingala/Latin/Congo - Kinshasa
    { LOCALE_TAGS(158, 66,  7)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lingala/Latin/Angola
    { LOCALE_TAGS(158, 66, 46)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lingala/Latin/Central African Republic
    { LOCALE_TAGS(158, 66, 56)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lingala/Latin/Congo - Brazzaville
    { LOCALE_TAGS(160, 66,137)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lithuanian/Latin/Lithuania
    { LOCALE_TAGS(161, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lojban/Latin/world
    { LOCALE_TAGS(162, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lower Sorbian/Latin/Germany
    { LOCALE_TAGS(163, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Low German/Latin/Germany
    { LOCALE_TAGS(163, 66,165)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Low German/Latin/Netherlands
    { LOCALE_TAGS(164, 66, 57)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Luba-Katanga/Latin/Congo - Kinshasa
    { LOCALE_TAGS(165, 66,225)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lule Sami/Latin/Sweden
    { LOCALE_TAGS(165, 66,175)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Lule Sami/Latin/Norway
    { LOCALE_TAGS(166, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Luo/Latin/Kenya
    { LOCALE_TAGS(167, 66,138)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Luxembourgish/Latin/Luxembourg
    { LOCALE_TAGS(168, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Luyia/Latin/Kenya
    { LOCALE_TAGS(169, 27,140)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Macedonian/Cyrillic/Macedonia
    { LOCALE_TAGS(170, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Machame/Latin/Tanzania
    { LOCALE_TAGS(171, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Maithili/Devanagari/India
    { LOCALE_TAGS(172, 66,160)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Makhuwa-Meetto/Latin/Mozambique
    { LOCALE_TAGS(173, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Makonde/Latin/Tanzania
    { LOCALE_TAGS(174, 66,141)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malagasy/Latin/Madagascar
    { LOCALE_TAGS(175, 74,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malayalam/Malayalam/India
    { LOCALE_TAGS(176, 66,143)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malay/Latin/Malaysia
    { LOCALE_TAGS(176,  4, 35)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malay/Arabic/Brunei
    { LOCALE_TAGS(176,  4,143)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malay/Arabic/Malaysia
    { LOCALE_TAGS(176, 66, 35)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malay/Latin/Brunei
    { LOCALE_TAGS(176, 66,111)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malay/Latin/Indonesia
    { LOCALE_TAGS(176, 66,210)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Malay/Latin/Singapore
    { LOCALE_TAGS(177, 66,146)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Maltese/Latin/Malta
    { LOCALE_TAGS(179,  9,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Manipuri/Bangla/India
    { LOCALE_TAGS(179, 78,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Manipuri/Meitei Mayek/India
    { LOCALE_TAGS(180, 66,115)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Manx/Latin/Isle of Man
    { LOCALE_TAGS(181, 66,167)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Maori/Latin/New Zealand
    { LOCALE_TAGS(182, 66, 49)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mapuche/Latin/Chile
    { LOCALE_TAGS(183, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Marathi/Devanagari/India
    { LOCALE_TAGS(185, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Masai/Latin/Kenya
    { LOCALE_TAGS(185, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Masai/Latin/Tanzania
    { LOCALE_TAGS(186,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mazanderani/Arabic/Iran
    { LOCALE_TAGS(188, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Meru/Latin/Kenya
    { LOCALE_TAGS(189, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Meta/Latin/Cameroon
    { LOCALE_TAGS(190, 66, 41)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mohawk/Latin/Canada
    { LOCALE_TAGS(191, 27,156)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mongolian/Cyrillic/Mongolia
    { LOCALE_TAGS(191, 83, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mongolian/Mongolian/China
    { LOCALE_TAGS(191, 83,156)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mongolian/Mongolian/Mongolia
    { LOCALE_TAGS(192, 66,150)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Morisyen/Latin/Mauritius
    { LOCALE_TAGS(193, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Mundang/Latin/Cameroon
    { LOCALE_TAGS(194, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Muscogee/Latin/United States
    { LOCALE_TAGS(195, 66,162)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nama/Latin/Namibia
    { LOCALE_TAGS(197, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Navajo/Latin/United States
    { LOCALE_TAGS(199, 29,164)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nepali/Devanagari/Nepal
    { LOCALE_TAGS(199, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nepali/Devanagari/India
    { LOCALE_TAGS(201, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ngiemboon/Latin/Cameroon
    { LOCALE_TAGS(202, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ngomba/Latin/Cameroon
    { LOCALE_TAGS(203, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nigerian Pidgin/Latin/Nigeria
    { LOCALE_TAGS(204, 90,102)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nko/Nko/Guinea
    { LOCALE_TAGS(205,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Luri/Arabic/Iran
    { LOCALE_TAGS(205,  4,113)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Luri/Arabic/Iraq
    { LOCALE_TAGS(206, 66,175)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Sami/Latin/Norway
    { LOCALE_TAGS(206, 66, 83)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Sami/Latin/Finland
    { LOCALE_TAGS(206, 66,225)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Sami/Latin/Sweden
    { LOCALE_TAGS(207, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Sotho/Latin/South Africa
    { LOCALE_TAGS(208, 66,261)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // North Ndebele/Latin/Zimbabwe
    { LOCALE_TAGS(209, 66,175)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Norwegian Bokmal/Latin/Norway
    { LOCALE_TAGS(209, 66,224)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Norwegian Bokmal/Latin/Svalbard and Jan Mayen
    { LOCALE_TAGS(210, 66,175)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Norwegian Nynorsk/Latin/Norway
    { LOCALE_TAGS(211, 66,219)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nuer/Latin/South Sudan
    { LOCALE_TAGS(212, 66,142)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nyanja/Latin/Malawi
    { LOCALE_TAGS(213, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nyankole/Latin/Uganda
    { LOCALE_TAGS(214, 66, 84)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Occitan/Latin/France
    { LOCALE_TAGS(214, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Occitan/Latin/Spain
    { LOCALE_TAGS(215, 91,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Odia/Odia/India
    { LOCALE_TAGS(220, 66, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Oromo/Latin/Ethiopia
    { LOCALE_TAGS(220, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Oromo/Latin/Kenya
    { LOCALE_TAGS(221,101,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Osage/Osage/United States
    { LOCALE_TAGS(222, 27, 90)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ossetic/Cyrillic/Georgia
    { LOCALE_TAGS(222, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ossetic/Cyrillic/Russia
    { LOCALE_TAGS(226, 66, 62)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Papiamento/Latin/Curacao
    { LOCALE_TAGS(226, 66, 13)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Papiamento/Latin/Aruba
    { LOCALE_TAGS(227,  4,  1)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Pashto/Arabic/Afghanistan
    { LOCALE_TAGS(227,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Pashto/Arabic/Pakistan
    { LOCALE_TAGS(228,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Persian/Arabic/Iran
    { LOCALE_TAGS(228,  4,  1)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Persian/Arabic/Afghanistan
    { LOCALE_TAGS(230, 66,187)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Polish/Latin/Poland
    { LOCALE_TAGS(231, 66, 32)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Brazil
    { LOCALE_TAGS(231, 66,  7)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Angola
    { LOCALE_TAGS(231, 66, 43)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Cape Verde
    { LOCALE_TAGS(231, 66, 73)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Equatorial Guinea
    { LOCALE_TAGS(231, 66,101)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Guinea-Bissau
    { LOCALE_TAGS(231, 66,138)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Luxembourg
    { LOCALE_TAGS(231, 66,139)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Macao
    { LOCALE_TAGS(231, 66,160)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Mozambique
    { LOCALE_TAGS(231, 66,188)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Portugal
    { LOCALE_TAGS(231, 66,204)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Sao Tome and Principe
    { LOCALE_TAGS(231, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Switzerland
    { LOCALE_TAGS(231, 66,232)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Portuguese/Latin/Timor-Leste
    { LOCALE_TAGS(232, 66,187)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Prussian/Latin/Poland
    { LOCALE_TAGS(233, 41,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Punjabi/Gurmukhi/India
    { LOCALE_TAGS(233,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Punjabi/Arabic/Pakistan
    { LOCALE_TAGS(234, 66,184)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Quechua/Latin/Peru
    { LOCALE_TAGS(234, 66, 28)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Quechua/Latin/Bolivia
    { LOCALE_TAGS(234, 66, 70)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Quechua/Latin/Ecuador
    { LOCALE_TAGS(235, 66,192)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Romanian/Latin/Romania
    { LOCALE_TAGS(235, 66,154)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Romanian/Latin/Moldova
    { LOCALE_TAGS(236, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Romansh/Latin/Switzerland
    { LOCALE_TAGS(237, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Rombo/Latin/Tanzania
    { LOCALE_TAGS(238, 66, 38)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Rundi/Latin/Burundi
    { LOCALE_TAGS(239, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Russian/Cyrillic/Russia
    { LOCALE_TAGS(239, 27, 22)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Russian/Cyrillic/Belarus
    { LOCALE_TAGS(239, 27,123)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Russian/Cyrillic/Kazakhstan
    { LOCALE_TAGS(239, 27,128)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Russian/Cyrillic/Kyrgyzstan
    { LOCALE_TAGS(239, 27,154)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Russian/Cyrillic/Moldova
    { LOCALE_TAGS(239, 27,244)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Russian/Cyrillic/Ukraine
    { LOCALE_TAGS(240, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Rwa/Latin/Tanzania
    { LOCALE_TAGS(241, 66, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Saho/Latin/Eritrea
    { LOCALE_TAGS(242, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sakha/Cyrillic/Russia
    { LOCALE_TAGS(243, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Samburu/Latin/Kenya
    { LOCALE_TAGS(245, 66, 46)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sango/Latin/Central African Republic
    { LOCALE_TAGS(246, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sangu/Latin/Tanzania
    { LOCALE_TAGS(247, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sanskrit/Devanagari/India
    { LOCALE_TAGS(248, 93,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Santali/Ol Chiki/India
    { LOCALE_TAGS(248, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Santali/Devanagari/India
    { LOCALE_TAGS(249, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sardinian/Latin/Italy
    { LOCALE_TAGS(251, 66,160)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sena/Latin/Mozambique
    { LOCALE_TAGS(252, 27,207)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Cyrillic/Serbia
    { LOCALE_TAGS(252, 27, 29)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Cyrillic/Bosnia and Herzegovina
    { LOCALE_TAGS(252, 27,126)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Cyrillic/Kosovo
    { LOCALE_TAGS(252, 27,157)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Cyrillic/Montenegro
    { LOCALE_TAGS(252, 66, 29)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Latin/Bosnia and Herzegovina
    { LOCALE_TAGS(252, 66,126)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Latin/Kosovo
    { LOCALE_TAGS(252, 66,157)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Latin/Montenegro
    { LOCALE_TAGS(252, 66,207)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Serbian/Latin/Serbia
    { LOCALE_TAGS(253, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Shambala/Latin/Tanzania
    { LOCALE_TAGS(254, 66,261)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Shona/Latin/Zimbabwe
    { LOCALE_TAGS(255,141, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sichuan Yi/Yi/China
    { LOCALE_TAGS(256, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sicilian/Latin/Italy
    { LOCALE_TAGS(257, 66, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sidamo/Latin/Ethiopia
    { LOCALE_TAGS(258, 66,187)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Silesian/Latin/Poland
    { LOCALE_TAGS(259,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sindhi/Arabic/Pakistan
    { LOCALE_TAGS(259, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sindhi/Devanagari/India
    { LOCALE_TAGS(260,119,221)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sinhala/Sinhala/Sri Lanka
    { LOCALE_TAGS(261, 66, 83)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Skolt Sami/Latin/Finland
    { LOCALE_TAGS(262, 66,212)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Slovak/Latin/Slovakia
    { LOCALE_TAGS(263, 66,213)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Slovenian/Latin/Slovenia
    { LOCALE_TAGS(264, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Soga/Latin/Uganda
    { LOCALE_TAGS(265, 66,215)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Somali/Latin/Somalia
    { LOCALE_TAGS(265, 66, 67)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Somali/Latin/Djibouti
    { LOCALE_TAGS(265, 66, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Somali/Latin/Ethiopia
    { LOCALE_TAGS(265, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Somali/Latin/Kenya
    { LOCALE_TAGS(266,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Southern Kurdish/Arabic/Iran
    { LOCALE_TAGS(266,  4,113)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Southern Kurdish/Arabic/Iraq
    { LOCALE_TAGS(267, 66,225)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Southern Sami/Latin/Sweden
    { LOCALE_TAGS(267, 66,175)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Southern Sami/Latin/Norway
    { LOCALE_TAGS(268, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Southern Sotho/Latin/South Africa
    { LOCALE_TAGS(268, 66,133)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Southern Sotho/Latin/Lesotho
    { LOCALE_TAGS(269, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // South Ndebele/Latin/South Africa
    { LOCALE_TAGS(270, 66,220)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Spain
    { LOCALE_TAGS(270, 66, 11)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Argentina
    { LOCALE_TAGS(270, 66, 24)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Belize
    { LOCALE_TAGS(270, 66, 28)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Bolivia
    { LOCALE_TAGS(270, 66, 32)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Brazil
    { LOCALE_TAGS(270, 66, 42)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Canary Islands
    { LOCALE_TAGS(270, 66, 47)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Ceuta and Melilla
    { LOCALE_TAGS(270, 66, 49)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Chile
    { LOCALE_TAGS(270, 66, 54)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Colombia
    { LOCALE_TAGS(270, 66, 59)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Costa Rica
    { LOCALE_TAGS(270, 66, 61)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Cuba
    { LOCALE_TAGS(270, 66, 69)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Dominican Republic
    { LOCALE_TAGS(270, 66, 70)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Ecuador
    { LOCALE_TAGS(270, 66, 72)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/El Salvador
    { LOCALE_TAGS(270, 66, 73)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Equatorial Guinea
    { LOCALE_TAGS(270, 66, 99)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Guatemala
    { LOCALE_TAGS(270, 66,106)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Honduras
    { LOCALE_TAGS(270, 66,130)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Latin America
    { LOCALE_TAGS(270, 66,152)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Mexico
    { LOCALE_TAGS(270, 66,168)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Nicaragua
    { LOCALE_TAGS(270, 66,181)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Panama
    { LOCALE_TAGS(270, 66,183)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Paraguay
    { LOCALE_TAGS(270, 66,184)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Peru
    { LOCALE_TAGS(270, 66,185)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Philippines
    { LOCALE_TAGS(270, 66,189)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Puerto Rico
    { LOCALE_TAGS(270, 66,248)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/United States
    { LOCALE_TAGS(270, 66,250)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Uruguay
    { LOCALE_TAGS(270, 66,254)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Spanish/Latin/Venezuela
    { LOCALE_TAGS(271,135,159)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Standard Moroccan Tamazight/Tifinagh/Morocco
    { LOCALE_TAGS(272, 66,111)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Sundanese/Latin/Indonesia
    { LOCALE_TAGS(273, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swahili/Latin/Tanzania
    { LOCALE_TAGS(273, 66, 57)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swahili/Latin/Congo - Kinshasa
    { LOCALE_TAGS(273, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swahili/Latin/Kenya
    { LOCALE_TAGS(273, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swahili/Latin/Uganda
    { LOCALE_TAGS(274, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swati/Latin/South Africa
    { LOCALE_TAGS(274, 66, 76)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swati/Latin/Eswatini
    { LOCALE_TAGS(275, 66,225)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swedish/Latin/Sweden
    { LOCALE_TAGS(275, 66,  2)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swedish/Latin/Aland Islands
    { LOCALE_TAGS(275, 66, 83)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swedish/Latin/Finland
    { LOCALE_TAGS(276, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swiss German/Latin/Switzerland
    { LOCALE_TAGS(276, 66, 84)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swiss German/Latin/France
    { LOCALE_TAGS(276, 66,136)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Swiss German/Latin/Liechtenstein
    { LOCALE_TAGS(277,123,113)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Syriac/Syriac/Iraq
    { LOCALE_TAGS(277,123,227)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Syriac/Syriac/Syria
    { LOCALE_TAGS(278,135,159)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tachelhit/Tifinagh/Morocco
    { LOCALE_TAGS(278, 66,159)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tachelhit/Latin/Morocco
    { LOCALE_TAGS(280,127,255)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tai Dam/Tai Viet/Vietnam
    { LOCALE_TAGS(281, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Taita/Latin/Kenya
    { LOCALE_TAGS(282, 27,229)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tajik/Cyrillic/Tajikistan
    { LOCALE_TAGS(283,129,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tamil/Tamil/India
    { LOCALE_TAGS(283,129,143)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tamil/Tamil/Malaysia
    { LOCALE_TAGS(283,129,210)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tamil/Tamil/Singapore
    { LOCALE_TAGS(283,129,221)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tamil/Tamil/Sri Lanka
    { LOCALE_TAGS(284, 66,228)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Taroko/Latin/Taiwan
    { LOCALE_TAGS(285, 66,170)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tasawaq/Latin/Niger
    { LOCALE_TAGS(286, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tatar/Cyrillic/Russia
    { LOCALE_TAGS(287,131,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Telugu/Telugu/India
    { LOCALE_TAGS(288, 66,243)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Teso/Latin/Uganda
    { LOCALE_TAGS(288, 66,124)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Teso/Latin/Kenya
    { LOCALE_TAGS(289,133,231)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Thai/Thai/Thailand
    { LOCALE_TAGS(290,134, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tibetan/Tibetan/China
    { LOCALE_TAGS(290,134,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tibetan/Tibetan/India
    { LOCALE_TAGS(291, 33, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tigre/Ethiopic/Eritrea
    { LOCALE_TAGS(292, 33, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tigrinya/Ethiopic/Ethiopia
    { LOCALE_TAGS(292, 33, 74)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tigrinya/Ethiopic/Eritrea
    { LOCALE_TAGS(294, 66,182)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tok Pisin/Latin/Papua New Guinea
    { LOCALE_TAGS(295, 66,235)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tongan/Latin/Tonga
    { LOCALE_TAGS(296, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tsonga/Latin/South Africa
    { LOCALE_TAGS(297, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tswana/Latin/South Africa
    { LOCALE_TAGS(297, 66, 30)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tswana/Latin/Botswana
    { LOCALE_TAGS(298, 66,239)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Turkish/Latin/Turkey
    { LOCALE_TAGS(298, 66, 63)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Turkish/Latin/Cyprus
    { LOCALE_TAGS(299, 66,240)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Turkmen/Latin/Turkmenistan
    { LOCALE_TAGS(301, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Tyap/Latin/Nigeria
    { LOCALE_TAGS(303, 27,244)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ukrainian/Cyrillic/Ukraine
    { LOCALE_TAGS(304, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Upper Sorbian/Latin/Germany
    { LOCALE_TAGS(305,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Urdu/Arabic/Pakistan
    { LOCALE_TAGS(305,  4,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Urdu/Arabic/India
    { LOCALE_TAGS(306,  4, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Uyghur/Arabic/China
    { LOCALE_TAGS(307, 66,251)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Uzbek/Latin/Uzbekistan
    { LOCALE_TAGS(307,  4,  1)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Uzbek/Arabic/Afghanistan
    { LOCALE_TAGS(307, 27,251)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Uzbek/Cyrillic/Uzbekistan
    { LOCALE_TAGS(308,139,134)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Vai/Vai/Liberia
    { LOCALE_TAGS(308, 66,134)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Vai/Latin/Liberia
    { LOCALE_TAGS(309, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Venda/Latin/South Africa
    { LOCALE_TAGS(310, 66,255)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Vietnamese/Latin/Vietnam
    { LOCALE_TAGS(311, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Volapuk/Latin/world
    { LOCALE_TAGS(312, 66,230)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Vunjo/Latin/Tanzania
    { LOCALE_TAGS(313, 66, 23)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Walloon/Latin/Belgium
    { LOCALE_TAGS(314, 66,226)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Walser/Latin/Switzerland
    { LOCALE_TAGS(315, 66, 15)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Warlpiri/Latin/Australia
    { LOCALE_TAGS(316, 66,246)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Welsh/Latin/United Kingdom
    { LOCALE_TAGS(317,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Western Balochi/Arabic/Pakistan
    { LOCALE_TAGS(317,  4,  1)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Western Balochi/Arabic/Afghanistan
    { LOCALE_TAGS(317,  4,112)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Western Balochi/Arabic/Iran
    { LOCALE_TAGS(317,  4,176)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Western Balochi/Arabic/Oman
    { LOCALE_TAGS(317,  4,245)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Western Balochi/Arabic/United Arab Emirates
    { LOCALE_TAGS(318, 66,165)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Western Frisian/Latin/Netherlands
    { LOCALE_TAGS(319, 33, 77)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Wolaytta/Ethiopic/Ethiopia
    { LOCALE_TAGS(320, 66,206)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Wolof/Latin/Senegal
    { LOCALE_TAGS(321, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Xhosa/Latin/South Africa
    { LOCALE_TAGS(322, 66, 40)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Yangben/Latin/Cameroon
    { LOCALE_TAGS(323, 47,244)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Yiddish/Hebrew/Ukraine
    { LOCALE_TAGS(324, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Yoruba/Latin/Nigeria
    { LOCALE_TAGS(324, 66, 25)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Yoruba/Latin/Benin
    { LOCALE_TAGS(325, 66,170)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Zarma/Latin/Niger
    { LOCALE_TAGS(326, 66, 50)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Zhuang/Latin/China
    { LOCALE_TAGS(327, 66,216)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Zulu/Latin/South Africa
    { LOCALE_TAGS(328, 66, 32)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kaingang/Latin/Brazil
    { LOCALE_TAGS(329, 66, 32)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nheengatu/Latin/Brazil
    { LOCALE_TAGS(329, 66, 54)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nheengatu/Latin/Colombia
    { LOCALE_TAGS(329, 66,254)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Nheengatu/Latin/Venezuela
    { LOCALE_TAGS(330, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Haryanvi/Devanagari/India
    { LOCALE_TAGS(331, 66, 91)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Northern Frisian/Latin/Germany
    { LOCALE_TAGS(332, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Rajasthani/Devanagari/India
    { LOCALE_TAGS(333, 27,193)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Moksha/Cyrillic/Russia
    { LOCALE_TAGS(334, 66,258)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Toki Pona/Latin/world
    { LOCALE_TAGS(335, 66,214)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Pijin/Latin/Solomon Islands
    { LOCALE_TAGS(336, 66,169)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Obolo/Latin/Nigeria
    { LOCALE_TAGS(337,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Baluchi/Arabic/Pakistan
    { LOCALE_TAGS(337, 66,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Baluchi/Latin/Pakistan
    { LOCALE_TAGS(338, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Ligurian/Latin/Italy
    { LOCALE_TAGS(339,142,161)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Rohingya/Hanifi/Myanmar
    { LOCALE_TAGS(339,142, 20)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Rohingya/Hanifi/Bangladesh
    { LOCALE_TAGS(340,  4,178)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Torwali/Arabic/Pakistan
    { LOCALE_TAGS(341, 66, 25)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Anii/Latin/Benin
    { LOCALE_TAGS(342, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kangri/Devanagari/India
    { LOCALE_TAGS(343, 66,117)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Venetian/Latin/Italy
    { LOCALE_TAGS(344, 66,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kuvi/Latin/India
    { LOCALE_TAGS(344, 29,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kuvi/Devanagari/India
    { LOCALE_TAGS(344, 91,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kuvi/Odia/India
    { LOCALE_TAGS(344,131,110)     0,     0,     0,     0,    0,    7,    0,    0,    0,    0,    0,  6,  6,  5,  0,  0,  0,  0, }, // Kuvi/Telugu/India
    { LOCALE_TAGS(  0,  0,  0)     0,     0,     0,     0,   14,   14,    6,    0,    0,    0,    0,  0,  0,  0,  0,  0,  0,  0, } // Terminal row
};

static constexpr char16_t hourFormatTable[] = {
    // +HH:mm, -HH:mm (stubs)
    0x2b, 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0x0,
    0x2d, 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0x0 // 88 char16_t @ 2 bytes
};

static constexpr char16_t gmtFormatTable[] = {
    // UTC%0 (stub)
    0x55, 0x54, 0x43, 0x25, 0x30, 0x0 // 271 char16_t @ 2 bytes
};

static constexpr char16_t regionFormatTable[] = {
    0x0 // 6066 char16_t @ 2 bytes
};

static constexpr char16_t fallbackFormatTable[] = {
    0x0 // 49 char16_t @ 2 bytes
};

static constexpr char16_t exemplarCityTable[] = {
    0x0 // 214634 char16_t @ 2 bytes
};

static constexpr char16_t shortZoneNameTable[] = {
    0x0 // 159 char16_t @ 2 bytes
};

static constexpr char16_t longZoneNameTable[] = {
    0x0 // 9552 char16_t @ 2 bytes
};

static constexpr char16_t shortMetaZoneNameTable[] = {
    0x0 // 844 char16_t @ 2 bytes
};

static constexpr char16_t longMetaZoneNameTable[] = {
    0x0 // 910242 char16_t @ 2 bytes
};
// Total: 3457942 bytes, c 3.3 MiB
// Debug binary qtimezonelocale.cpp.o: 8246776 bytes
// Release binary qtimezonelocale.cpp.o: 2464064 bytes

// GENERATED PART ENDS HERE

} // QtTimeZoneLocale

#undef LOCALE_TAGS

QT_END_NAMESPACE

#endif // QTIMEZONELOCALE_DATA_P_H
