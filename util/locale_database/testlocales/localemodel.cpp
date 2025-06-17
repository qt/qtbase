// Copyright (C) 2016 The Qt Company Ltd.
// Copyright © 2004-2023 Unicode, Inc.
// SPDX-License-Identifier: Unicode-3.0

#include "localemodel.h"

#include <QLocale>
#include <QDate>
#include <qdebug.h>

static const int g_model_cols = 6;

struct LocaleListItem
{
    int language;
    int territory;
};

// GENERATED PART STARTS HERE

/*
    This part of the file was generated on 2025-06-17 from the
    Common Locale Data Repository v47

    http://www.unicode.org/cldr/

    Do not edit this section: instead regenerate it using
    cldr2qlocalexml.py and qlocalexml2cpp.py on updated (or
    edited) CLDR data; see qtbase/util/locale_database/.
*/

const LocaleListItem g_locale_list[] = {
    {      1,     0 }, // C/AnyTerritory
    {      2,    90 }, // Abkhazian/Georgia
    {      3,    77 }, // Afar/Ethiopia
    {      3,    67 }, // Afar/Djibouti
    {      3,    74 }, // Afar/Eritrea
    {      4,   216 }, // Afrikaans/South Africa
    {      4,   162 }, // Afrikaans/Namibia
    {      5,    40 }, // Aghem/Cameroon
    {      6,    92 }, // Akan/Ghana
    {      8,    40 }, // Akoose/Cameroon
    {      9,     3 }, // Albanian/Albania
    {      9,   126 }, // Albanian/Kosovo
    {      9,   140 }, // Albanian/Macedonia
    {     11,    77 }, // Amharic/Ethiopia
    {     14,    71 }, // Arabic/Egypt
    {     14,     4 }, // Arabic/Algeria
    {     14,    19 }, // Arabic/Bahrain
    {     14,    48 }, // Arabic/Chad
    {     14,    55 }, // Arabic/Comoros
    {     14,    67 }, // Arabic/Djibouti
    {     14,    74 }, // Arabic/Eritrea
    {     14,   113 }, // Arabic/Iraq
    {     14,   116 }, // Arabic/Israel
    {     14,   122 }, // Arabic/Jordan
    {     14,   127 }, // Arabic/Kuwait
    {     14,   132 }, // Arabic/Lebanon
    {     14,   135 }, // Arabic/Libya
    {     14,   149 }, // Arabic/Mauritania
    {     14,   159 }, // Arabic/Morocco
    {     14,   176 }, // Arabic/Oman
    {     14,   180 }, // Arabic/Palestinian Territories
    {     14,   190 }, // Arabic/Qatar
    {     14,   205 }, // Arabic/Saudi Arabia
    {     14,   215 }, // Arabic/Somalia
    {     14,   219 }, // Arabic/South Sudan
    {     14,   222 }, // Arabic/Sudan
    {     14,   227 }, // Arabic/Syria
    {     14,   238 }, // Arabic/Tunisia
    {     14,   245 }, // Arabic/United Arab Emirates
    {     14,   257 }, // Arabic/Western Sahara
    {     14,   258 }, // Arabic/world
    {     14,   259 }, // Arabic/Yemen
    {     15,   220 }, // Aragonese/Spain
    {     17,    12 }, // Armenian/Armenia
    {     18,   110 }, // Assamese/India
    {     19,   220 }, // Asturian/Spain
    {     20,   230 }, // Asu/Tanzania
    {     21,   169 }, // Atsam/Nigeria
    {     25,    17 }, // Azerbaijani/Azerbaijan
    {     25,   112 }, // Azerbaijani/Iran
    {     25,   113 }, // Azerbaijani/Iraq
    {     25,   239 }, // Azerbaijani/Turkey
    {     26,    40 }, // Bafia/Cameroon
    {     28,   145 }, // Bambara/Mali
    {     30,    20 }, // Bangla/Bangladesh
    {     30,   110 }, // Bangla/India
    {     31,    40 }, // Basaa/Cameroon
    {     32,   193 }, // Bashkir/Russia
    {     33,   220 }, // Basque/Spain
    {     35,    22 }, // Belarusian/Belarus
    {     36,   260 }, // Bemba/Zambia
    {     37,   230 }, // Bena/Tanzania
    {     38,   110 }, // Bhojpuri/India
    {     40,    74 }, // Blin/Eritrea
    {     41,   110 }, // Bodo/India
    {     42,    29 }, // Bosnian/Bosnia and Herzegovina
    {     43,    84 }, // Breton/France
    {     45,    36 }, // Bulgarian/Bulgaria
    {     46,   161 }, // Burmese/Myanmar
    {     47,   107 }, // Cantonese/Hong Kong
    {     47,    50 }, // Cantonese/China
    {     47,   139 }, // Cantonese/Macao
    {     48,   220 }, // Catalan/Spain
    {     48,     6 }, // Catalan/Andorra
    {     48,    84 }, // Catalan/France
    {     48,   117 }, // Catalan/Italy
    {     49,   185 }, // Cebuano/Philippines
    {     50,   159 }, // Central Atlas Tamazight/Morocco
    {     51,   113 }, // Central Kurdish/Iraq
    {     51,   112 }, // Central Kurdish/Iran
    {     52,    20 }, // Chakma/Bangladesh
    {     52,   110 }, // Chakma/India
    {     54,   193 }, // Chechen/Russia
    {     55,   248 }, // Cherokee/United States
    {     56,   248 }, // Chickasaw/United States
    {     57,   243 }, // Chiga/Uganda
    {     58,    50 }, // Chinese/China
    {     58,   107 }, // Chinese/Hong Kong
    {     58,   139 }, // Chinese/Macao
    {     58,   143 }, // Chinese/Malaysia
    {     58,   210 }, // Chinese/Singapore
    {     58,   228 }, // Chinese/Taiwan
    {     59,   193 }, // Church/Russia
    {     60,   193 }, // Chuvash/Russia
    {     61,    91 }, // Colognian/Germany
    {     62,    71 }, // Coptic/Egypt
    {     63,   246 }, // Cornish/United Kingdom
    {     64,    84 }, // Corsican/France
    {     66,    60 }, // Croatian/Croatia
    {     66,    29 }, // Croatian/Bosnia and Herzegovina
    {     67,    64 }, // Czech/Czechia
    {     68,    65 }, // Danish/Denmark
    {     68,    95 }, // Danish/Greenland
    {     69,   144 }, // Divehi/Maldives
    {     70,   110 }, // Dogri/India
    {     71,    40 }, // Duala/Cameroon
    {     72,   165 }, // Dutch/Netherlands
    {     72,    13 }, // Dutch/Aruba
    {     72,    23 }, // Dutch/Belgium
    {     72,    44 }, // Dutch/Caribbean Netherlands
    {     72,    62 }, // Dutch/Curacao
    {     72,   211 }, // Dutch/Sint Maarten
    {     72,   223 }, // Dutch/Suriname
    {     73,    27 }, // Dzongkha/Bhutan
    {     74,   124 }, // Embu/Kenya
    {     75,   248 }, // English/United States
    {     75,     5 }, // English/American Samoa
    {     75,     8 }, // English/Anguilla
    {     75,    10 }, // English/Antigua and Barbuda
    {     75,    15 }, // English/Australia
    {     75,    16 }, // English/Austria
    {     75,    18 }, // English/Bahamas
    {     75,    21 }, // English/Barbados
    {     75,    23 }, // English/Belgium
    {     75,    24 }, // English/Belize
    {     75,    26 }, // English/Bermuda
    {     75,    30 }, // English/Botswana
    {     75,    33 }, // English/British Indian Ocean Territory
    {     75,    34 }, // English/British Virgin Islands
    {     75,    38 }, // English/Burundi
    {     75,    40 }, // English/Cameroon
    {     75,    41 }, // English/Canada
    {     75,    45 }, // English/Cayman Islands
    {     75,    51 }, // English/Christmas Island
    {     75,    53 }, // English/Cocos Islands
    {     75,    58 }, // English/Cook Islands
    {     75,    63 }, // English/Cyprus
    {     75,    64 }, // English/Czechia
    {     75,    65 }, // English/Denmark
    {     75,    66 }, // English/Diego Garcia
    {     75,    68 }, // English/Dominica
    {     75,    74 }, // English/Eritrea
    {     75,    76 }, // English/Eswatini
    {     75,    78 }, // English/Europe
    {     75,    80 }, // English/Falkland Islands
    {     75,    82 }, // English/Fiji
    {     75,    83 }, // English/Finland
    {     75,    84 }, // English/France
    {     75,    89 }, // English/Gambia
    {     75,    91 }, // English/Germany
    {     75,    92 }, // English/Ghana
    {     75,    93 }, // English/Gibraltar
    {     75,    96 }, // English/Grenada
    {     75,    98 }, // English/Guam
    {     75,   100 }, // English/Guernsey
    {     75,   103 }, // English/Guyana
    {     75,   107 }, // English/Hong Kong
    {     75,   108 }, // English/Hungary
    {     75,   110 }, // English/India
    {     75,   111 }, // English/Indonesia
    {     75,   114 }, // English/Ireland
    {     75,   115 }, // English/Isle of Man
    {     75,   116 }, // English/Israel
    {     75,   117 }, // English/Italy
    {     75,   119 }, // English/Jamaica
    {     75,   121 }, // English/Jersey
    {     75,   124 }, // English/Kenya
    {     75,   125 }, // English/Kiribati
    {     75,   133 }, // English/Lesotho
    {     75,   134 }, // English/Liberia
    {     75,   139 }, // English/Macao
    {     75,   141 }, // English/Madagascar
    {     75,   142 }, // English/Malawi
    {     75,   143 }, // English/Malaysia
    {     75,   144 }, // English/Maldives
    {     75,   146 }, // English/Malta
    {     75,   147 }, // English/Marshall Islands
    {     75,   150 }, // English/Mauritius
    {     75,   153 }, // English/Micronesia
    {     75,   158 }, // English/Montserrat
    {     75,   162 }, // English/Namibia
    {     75,   163 }, // English/Nauru
    {     75,   165 }, // English/Netherlands
    {     75,   167 }, // English/New Zealand
    {     75,   169 }, // English/Nigeria
    {     75,   171 }, // English/Niue
    {     75,   172 }, // English/Norfolk Island
    {     75,   173 }, // English/Northern Mariana Islands
    {     75,   175 }, // English/Norway
    {     75,   178 }, // English/Pakistan
    {     75,   179 }, // English/Palau
    {     75,   182 }, // English/Papua New Guinea
    {     75,   185 }, // English/Philippines
    {     75,   186 }, // English/Pitcairn
    {     75,   187 }, // English/Poland
    {     75,   188 }, // English/Portugal
    {     75,   189 }, // English/Puerto Rico
    {     75,   192 }, // English/Romania
    {     75,   194 }, // English/Rwanda
    {     75,   196 }, // English/Saint Helena
    {     75,   197 }, // English/Saint Kitts and Nevis
    {     75,   198 }, // English/Saint Lucia
    {     75,   201 }, // English/Saint Vincent and Grenadines
    {     75,   202 }, // English/Samoa
    {     75,   208 }, // English/Seychelles
    {     75,   209 }, // English/Sierra Leone
    {     75,   210 }, // English/Singapore
    {     75,   211 }, // English/Sint Maarten
    {     75,   212 }, // English/Slovakia
    {     75,   213 }, // English/Slovenia
    {     75,   214 }, // English/Solomon Islands
    {     75,   216 }, // English/South Africa
    {     75,   217 }, // English/South Georgia and South Sandwich Islands
    {     75,   219 }, // English/South Sudan
    {     75,   220 }, // English/Spain
    {     75,   222 }, // English/Sudan
    {     75,   225 }, // English/Sweden
    {     75,   226 }, // English/Switzerland
    {     75,   230 }, // English/Tanzania
    {     75,   234 }, // English/Tokelau
    {     75,   235 }, // English/Tonga
    {     75,   236 }, // English/Trinidad and Tobago
    {     75,   241 }, // English/Turks and Caicos Islands
    {     75,   242 }, // English/Tuvalu
    {     75,   243 }, // English/Uganda
    {     75,   245 }, // English/United Arab Emirates
    {     75,   246 }, // English/United Kingdom
    {     75,   247 }, // English/United States Outlying Islands
    {     75,   249 }, // English/United States Virgin Islands
    {     75,   252 }, // English/Vanuatu
    {     75,   258 }, // English/world
    {     75,   260 }, // English/Zambia
    {     75,   261 }, // English/Zimbabwe
    {     76,   193 }, // Erzya/Russia
    {     77,   258 }, // Esperanto/world
    {     78,    75 }, // Estonian/Estonia
    {     79,    92 }, // Ewe/Ghana
    {     79,   233 }, // Ewe/Togo
    {     80,    40 }, // Ewondo/Cameroon
    {     81,    81 }, // Faroese/Faroe Islands
    {     81,    65 }, // Faroese/Denmark
    {     83,   185 }, // Filipino/Philippines
    {     84,    83 }, // Finnish/Finland
    {     85,    84 }, // French/France
    {     85,     4 }, // French/Algeria
    {     85,    23 }, // French/Belgium
    {     85,    25 }, // French/Benin
    {     85,    37 }, // French/Burkina Faso
    {     85,    38 }, // French/Burundi
    {     85,    40 }, // French/Cameroon
    {     85,    41 }, // French/Canada
    {     85,    46 }, // French/Central African Republic
    {     85,    48 }, // French/Chad
    {     85,    55 }, // French/Comoros
    {     85,    56 }, // French/Congo - Brazzaville
    {     85,    57 }, // French/Congo - Kinshasa
    {     85,    67 }, // French/Djibouti
    {     85,    73 }, // French/Equatorial Guinea
    {     85,    85 }, // French/French Guiana
    {     85,    86 }, // French/French Polynesia
    {     85,    88 }, // French/Gabon
    {     85,    97 }, // French/Guadeloupe
    {     85,   102 }, // French/Guinea
    {     85,   104 }, // French/Haiti
    {     85,   118 }, // French/Ivory Coast
    {     85,   138 }, // French/Luxembourg
    {     85,   141 }, // French/Madagascar
    {     85,   145 }, // French/Mali
    {     85,   148 }, // French/Martinique
    {     85,   149 }, // French/Mauritania
    {     85,   150 }, // French/Mauritius
    {     85,   151 }, // French/Mayotte
    {     85,   155 }, // French/Monaco
    {     85,   159 }, // French/Morocco
    {     85,   166 }, // French/New Caledonia
    {     85,   170 }, // French/Niger
    {     85,   191 }, // French/Reunion
    {     85,   194 }, // French/Rwanda
    {     85,   195 }, // French/Saint Barthelemy
    {     85,   199 }, // French/Saint Martin
    {     85,   200 }, // French/Saint Pierre and Miquelon
    {     85,   206 }, // French/Senegal
    {     85,   208 }, // French/Seychelles
    {     85,   226 }, // French/Switzerland
    {     85,   227 }, // French/Syria
    {     85,   233 }, // French/Togo
    {     85,   238 }, // French/Tunisia
    {     85,   252 }, // French/Vanuatu
    {     85,   256 }, // French/Wallis and Futuna
    {     86,   117 }, // Friulian/Italy
    {     87,   206 }, // Fulah/Senegal
    {     87,    37 }, // Fulah/Burkina Faso
    {     87,    40 }, // Fulah/Cameroon
    {     87,    89 }, // Fulah/Gambia
    {     87,    92 }, // Fulah/Ghana
    {     87,   101 }, // Fulah/Guinea-Bissau
    {     87,   102 }, // Fulah/Guinea
    {     87,   134 }, // Fulah/Liberia
    {     87,   149 }, // Fulah/Mauritania
    {     87,   169 }, // Fulah/Nigeria
    {     87,   170 }, // Fulah/Niger
    {     87,   209 }, // Fulah/Sierra Leone
    {     88,   246 }, // Gaelic/United Kingdom
    {     89,    92 }, // Ga/Ghana
    {     90,   220 }, // Galician/Spain
    {     91,   243 }, // Ganda/Uganda
    {     92,    77 }, // Geez/Ethiopia
    {     92,    74 }, // Geez/Eritrea
    {     93,    90 }, // Georgian/Georgia
    {     94,    91 }, // German/Germany
    {     94,    16 }, // German/Austria
    {     94,    23 }, // German/Belgium
    {     94,   117 }, // German/Italy
    {     94,   136 }, // German/Liechtenstein
    {     94,   138 }, // German/Luxembourg
    {     94,   226 }, // German/Switzerland
    {     96,    94 }, // Greek/Greece
    {     96,    63 }, // Greek/Cyprus
    {     97,   183 }, // Guarani/Paraguay
    {     98,   110 }, // Gujarati/India
    {     99,   124 }, // Gusii/Kenya
    {    100,   104 }, // Haitian/Haiti
    {    101,   169 }, // Hausa/Nigeria
    {    101,   222 }, // Hausa/Sudan
    {    101,    92 }, // Hausa/Ghana
    {    101,   170 }, // Hausa/Niger
    {    102,   248 }, // Hawaiian/United States
    {    103,   116 }, // Hebrew/Israel
    {    105,   110 }, // Hindi/India
    {    107,   108 }, // Hungarian/Hungary
    {    108,   109 }, // Icelandic/Iceland
    {    109,   258 }, // Ido/world
    {    110,   169 }, // Igbo/Nigeria
    {    111,    83 }, // Inari Sami/Finland
    {    112,   111 }, // Indonesian/Indonesia
    {    114,   258 }, // Interlingua/world
    {    115,    75 }, // Interlingue/Estonia
    {    116,    41 }, // Inuktitut/Canada
    {    118,   114 }, // Irish/Ireland
    {    118,   246 }, // Irish/United Kingdom
    {    119,   117 }, // Italian/Italy
    {    119,   203 }, // Italian/San Marino
    {    119,   226 }, // Italian/Switzerland
    {    119,   253 }, // Italian/Vatican City
    {    120,   120 }, // Japanese/Japan
    {    121,   111 }, // Javanese/Indonesia
    {    122,   169 }, // Jju/Nigeria
    {    123,   206 }, // Jola-Fonyi/Senegal
    {    124,    43 }, // Kabuverdianu/Cape Verde
    {    125,     4 }, // Kabyle/Algeria
    {    126,    40 }, // Kako/Cameroon
    {    127,    95 }, // Kalaallisut/Greenland
    {    128,   124 }, // Kalenjin/Kenya
    {    129,   124 }, // Kamba/Kenya
    {    130,   110 }, // Kannada/India
    {    132,   110 }, // Kashmiri/India
    {    133,   123 }, // Kazakh/Kazakhstan
    {    133,    50 }, // Kazakh/China
    {    134,    40 }, // Kenyang/Cameroon
    {    135,    39 }, // Khmer/Cambodia
    {    136,    99 }, // Kiche/Guatemala
    {    137,   124 }, // Kikuyu/Kenya
    {    138,   194 }, // Kinyarwanda/Rwanda
    {    141,   110 }, // Konkani/India
    {    142,   218 }, // Korean/South Korea
    {    142,    50 }, // Korean/China
    {    142,   174 }, // Korean/North Korea
    {    144,   145 }, // Koyraboro Senni/Mali
    {    145,   145 }, // Koyra Chiini/Mali
    {    146,   134 }, // Kpelle/Liberia
    {    146,   102 }, // Kpelle/Guinea
    {    148,   239 }, // Kurdish/Turkey
    {    149,    40 }, // Kwasio/Cameroon
    {    150,   128 }, // Kyrgyz/Kyrgyzstan
    {    151,   248 }, // Lakota/United States
    {    152,   230 }, // Langi/Tanzania
    {    153,   129 }, // Lao/Laos
    {    154,   253 }, // Latin/Vatican City
    {    155,   131 }, // Latvian/Latvia
    {    158,    57 }, // Lingala/Congo - Kinshasa
    {    158,     7 }, // Lingala/Angola
    {    158,    46 }, // Lingala/Central African Republic
    {    158,    56 }, // Lingala/Congo - Brazzaville
    {    160,   137 }, // Lithuanian/Lithuania
    {    161,   258 }, // Lojban/world
    {    162,    91 }, // Lower Sorbian/Germany
    {    163,    91 }, // Low German/Germany
    {    163,   165 }, // Low German/Netherlands
    {    164,    57 }, // Luba-Katanga/Congo - Kinshasa
    {    165,   225 }, // Lule Sami/Sweden
    {    165,   175 }, // Lule Sami/Norway
    {    166,   124 }, // Luo/Kenya
    {    167,   138 }, // Luxembourgish/Luxembourg
    {    168,   124 }, // Luyia/Kenya
    {    169,   140 }, // Macedonian/Macedonia
    {    170,   230 }, // Machame/Tanzania
    {    171,   110 }, // Maithili/India
    {    172,   160 }, // Makhuwa-Meetto/Mozambique
    {    173,   230 }, // Makonde/Tanzania
    {    174,   141 }, // Malagasy/Madagascar
    {    175,   110 }, // Malayalam/India
    {    176,   143 }, // Malay/Malaysia
    {    176,    35 }, // Malay/Brunei
    {    176,   111 }, // Malay/Indonesia
    {    176,   210 }, // Malay/Singapore
    {    177,   146 }, // Maltese/Malta
    {    179,   110 }, // Manipuri/India
    {    180,   115 }, // Manx/Isle of Man
    {    181,   167 }, // Maori/New Zealand
    {    182,    49 }, // Mapuche/Chile
    {    183,   110 }, // Marathi/India
    {    185,   124 }, // Masai/Kenya
    {    185,   230 }, // Masai/Tanzania
    {    186,   112 }, // Mazanderani/Iran
    {    188,   124 }, // Meru/Kenya
    {    189,    40 }, // Meta/Cameroon
    {    190,    41 }, // Mohawk/Canada
    {    191,   156 }, // Mongolian/Mongolia
    {    191,    50 }, // Mongolian/China
    {    192,   150 }, // Morisyen/Mauritius
    {    193,    40 }, // Mundang/Cameroon
    {    194,   248 }, // Muscogee/United States
    {    195,   162 }, // Nama/Namibia
    {    197,   248 }, // Navajo/United States
    {    199,   164 }, // Nepali/Nepal
    {    199,   110 }, // Nepali/India
    {    201,    40 }, // Ngiemboon/Cameroon
    {    202,    40 }, // Ngomba/Cameroon
    {    203,   169 }, // Nigerian Pidgin/Nigeria
    {    204,   102 }, // Nko/Guinea
    {    205,   112 }, // Northern Luri/Iran
    {    205,   113 }, // Northern Luri/Iraq
    {    206,   175 }, // Northern Sami/Norway
    {    206,    83 }, // Northern Sami/Finland
    {    206,   225 }, // Northern Sami/Sweden
    {    207,   216 }, // Northern Sotho/South Africa
    {    208,   261 }, // North Ndebele/Zimbabwe
    {    209,   175 }, // Norwegian Bokmal/Norway
    {    209,   224 }, // Norwegian Bokmal/Svalbard and Jan Mayen
    {    210,   175 }, // Norwegian Nynorsk/Norway
    {    211,   219 }, // Nuer/South Sudan
    {    212,   142 }, // Nyanja/Malawi
    {    213,   243 }, // Nyankole/Uganda
    {    214,    84 }, // Occitan/France
    {    214,   220 }, // Occitan/Spain
    {    215,   110 }, // Odia/India
    {    220,    77 }, // Oromo/Ethiopia
    {    220,   124 }, // Oromo/Kenya
    {    221,   248 }, // Osage/United States
    {    222,    90 }, // Ossetic/Georgia
    {    222,   193 }, // Ossetic/Russia
    {    226,    62 }, // Papiamento/Curacao
    {    226,    13 }, // Papiamento/Aruba
    {    227,     1 }, // Pashto/Afghanistan
    {    227,   178 }, // Pashto/Pakistan
    {    228,   112 }, // Persian/Iran
    {    228,     1 }, // Persian/Afghanistan
    {    230,   187 }, // Polish/Poland
    {    231,    32 }, // Portuguese/Brazil
    {    231,     7 }, // Portuguese/Angola
    {    231,    43 }, // Portuguese/Cape Verde
    {    231,    73 }, // Portuguese/Equatorial Guinea
    {    231,   101 }, // Portuguese/Guinea-Bissau
    {    231,   138 }, // Portuguese/Luxembourg
    {    231,   139 }, // Portuguese/Macao
    {    231,   160 }, // Portuguese/Mozambique
    {    231,   188 }, // Portuguese/Portugal
    {    231,   204 }, // Portuguese/Sao Tome and Principe
    {    231,   226 }, // Portuguese/Switzerland
    {    231,   232 }, // Portuguese/Timor-Leste
    {    232,   187 }, // Prussian/Poland
    {    233,   110 }, // Punjabi/India
    {    233,   178 }, // Punjabi/Pakistan
    {    234,   184 }, // Quechua/Peru
    {    234,    28 }, // Quechua/Bolivia
    {    234,    70 }, // Quechua/Ecuador
    {    235,   192 }, // Romanian/Romania
    {    235,   154 }, // Romanian/Moldova
    {    236,   226 }, // Romansh/Switzerland
    {    237,   230 }, // Rombo/Tanzania
    {    238,    38 }, // Rundi/Burundi
    {    239,   193 }, // Russian/Russia
    {    239,    22 }, // Russian/Belarus
    {    239,   123 }, // Russian/Kazakhstan
    {    239,   128 }, // Russian/Kyrgyzstan
    {    239,   154 }, // Russian/Moldova
    {    239,   244 }, // Russian/Ukraine
    {    240,   230 }, // Rwa/Tanzania
    {    241,    74 }, // Saho/Eritrea
    {    242,   193 }, // Sakha/Russia
    {    243,   124 }, // Samburu/Kenya
    {    245,    46 }, // Sango/Central African Republic
    {    246,   230 }, // Sangu/Tanzania
    {    247,   110 }, // Sanskrit/India
    {    248,   110 }, // Santali/India
    {    249,   117 }, // Sardinian/Italy
    {    251,   160 }, // Sena/Mozambique
    {    252,   207 }, // Serbian/Serbia
    {    252,    29 }, // Serbian/Bosnia and Herzegovina
    {    252,   126 }, // Serbian/Kosovo
    {    252,   157 }, // Serbian/Montenegro
    {    253,   230 }, // Shambala/Tanzania
    {    254,   261 }, // Shona/Zimbabwe
    {    255,    50 }, // Sichuan Yi/China
    {    256,   117 }, // Sicilian/Italy
    {    257,    77 }, // Sidamo/Ethiopia
    {    258,   187 }, // Silesian/Poland
    {    259,   178 }, // Sindhi/Pakistan
    {    259,   110 }, // Sindhi/India
    {    260,   221 }, // Sinhala/Sri Lanka
    {    261,    83 }, // Skolt Sami/Finland
    {    262,   212 }, // Slovak/Slovakia
    {    263,   213 }, // Slovenian/Slovenia
    {    264,   243 }, // Soga/Uganda
    {    265,   215 }, // Somali/Somalia
    {    265,    67 }, // Somali/Djibouti
    {    265,    77 }, // Somali/Ethiopia
    {    265,   124 }, // Somali/Kenya
    {    266,   112 }, // Southern Kurdish/Iran
    {    266,   113 }, // Southern Kurdish/Iraq
    {    267,   225 }, // Southern Sami/Sweden
    {    267,   175 }, // Southern Sami/Norway
    {    268,   216 }, // Southern Sotho/South Africa
    {    268,   133 }, // Southern Sotho/Lesotho
    {    269,   216 }, // South Ndebele/South Africa
    {    270,   220 }, // Spanish/Spain
    {    270,    11 }, // Spanish/Argentina
    {    270,    24 }, // Spanish/Belize
    {    270,    28 }, // Spanish/Bolivia
    {    270,    32 }, // Spanish/Brazil
    {    270,    42 }, // Spanish/Canary Islands
    {    270,    47 }, // Spanish/Ceuta and Melilla
    {    270,    49 }, // Spanish/Chile
    {    270,    54 }, // Spanish/Colombia
    {    270,    59 }, // Spanish/Costa Rica
    {    270,    61 }, // Spanish/Cuba
    {    270,    69 }, // Spanish/Dominican Republic
    {    270,    70 }, // Spanish/Ecuador
    {    270,    72 }, // Spanish/El Salvador
    {    270,    73 }, // Spanish/Equatorial Guinea
    {    270,    99 }, // Spanish/Guatemala
    {    270,   106 }, // Spanish/Honduras
    {    270,   130 }, // Spanish/Latin America
    {    270,   152 }, // Spanish/Mexico
    {    270,   168 }, // Spanish/Nicaragua
    {    270,   181 }, // Spanish/Panama
    {    270,   183 }, // Spanish/Paraguay
    {    270,   184 }, // Spanish/Peru
    {    270,   185 }, // Spanish/Philippines
    {    270,   189 }, // Spanish/Puerto Rico
    {    270,   248 }, // Spanish/United States
    {    270,   250 }, // Spanish/Uruguay
    {    270,   254 }, // Spanish/Venezuela
    {    271,   159 }, // Standard Moroccan Tamazight/Morocco
    {    272,   111 }, // Sundanese/Indonesia
    {    273,   230 }, // Swahili/Tanzania
    {    273,    57 }, // Swahili/Congo - Kinshasa
    {    273,   124 }, // Swahili/Kenya
    {    273,   243 }, // Swahili/Uganda
    {    274,   216 }, // Swati/South Africa
    {    274,    76 }, // Swati/Eswatini
    {    275,   225 }, // Swedish/Sweden
    {    275,     2 }, // Swedish/Aland Islands
    {    275,    83 }, // Swedish/Finland
    {    276,   226 }, // Swiss German/Switzerland
    {    276,    84 }, // Swiss German/France
    {    276,   136 }, // Swiss German/Liechtenstein
    {    277,   113 }, // Syriac/Iraq
    {    277,   227 }, // Syriac/Syria
    {    278,   159 }, // Tachelhit/Morocco
    {    280,   255 }, // Tai Dam/Vietnam
    {    281,   124 }, // Taita/Kenya
    {    282,   229 }, // Tajik/Tajikistan
    {    283,   110 }, // Tamil/India
    {    283,   143 }, // Tamil/Malaysia
    {    283,   210 }, // Tamil/Singapore
    {    283,   221 }, // Tamil/Sri Lanka
    {    284,   228 }, // Taroko/Taiwan
    {    285,   170 }, // Tasawaq/Niger
    {    286,   193 }, // Tatar/Russia
    {    287,   110 }, // Telugu/India
    {    288,   243 }, // Teso/Uganda
    {    288,   124 }, // Teso/Kenya
    {    289,   231 }, // Thai/Thailand
    {    290,    50 }, // Tibetan/China
    {    290,   110 }, // Tibetan/India
    {    291,    74 }, // Tigre/Eritrea
    {    292,    77 }, // Tigrinya/Ethiopia
    {    292,    74 }, // Tigrinya/Eritrea
    {    294,   182 }, // Tok Pisin/Papua New Guinea
    {    295,   235 }, // Tongan/Tonga
    {    296,   216 }, // Tsonga/South Africa
    {    297,   216 }, // Tswana/South Africa
    {    297,    30 }, // Tswana/Botswana
    {    298,   239 }, // Turkish/Turkey
    {    298,    63 }, // Turkish/Cyprus
    {    299,   240 }, // Turkmen/Turkmenistan
    {    301,   169 }, // Tyap/Nigeria
    {    303,   244 }, // Ukrainian/Ukraine
    {    304,    91 }, // Upper Sorbian/Germany
    {    305,   178 }, // Urdu/Pakistan
    {    305,   110 }, // Urdu/India
    {    306,    50 }, // Uyghur/China
    {    307,   251 }, // Uzbek/Uzbekistan
    {    307,     1 }, // Uzbek/Afghanistan
    {    308,   134 }, // Vai/Liberia
    {    309,   216 }, // Venda/South Africa
    {    310,   255 }, // Vietnamese/Vietnam
    {    311,   258 }, // Volapuk/world
    {    312,   230 }, // Vunjo/Tanzania
    {    313,    23 }, // Walloon/Belgium
    {    314,   226 }, // Walser/Switzerland
    {    315,    15 }, // Warlpiri/Australia
    {    316,   246 }, // Welsh/United Kingdom
    {    317,   178 }, // Western Balochi/Pakistan
    {    317,     1 }, // Western Balochi/Afghanistan
    {    317,   112 }, // Western Balochi/Iran
    {    317,   176 }, // Western Balochi/Oman
    {    317,   245 }, // Western Balochi/United Arab Emirates
    {    318,   165 }, // Western Frisian/Netherlands
    {    319,    77 }, // Wolaytta/Ethiopia
    {    320,   206 }, // Wolof/Senegal
    {    321,   216 }, // Xhosa/South Africa
    {    322,    40 }, // Yangben/Cameroon
    {    323,   244 }, // Yiddish/Ukraine
    {    324,   169 }, // Yoruba/Nigeria
    {    324,    25 }, // Yoruba/Benin
    {    325,   170 }, // Zarma/Niger
    {    326,    50 }, // Zhuang/China
    {    327,   216 }, // Zulu/South Africa
    {    328,    32 }, // Kaingang/Brazil
    {    329,    32 }, // Nheengatu/Brazil
    {    329,    54 }, // Nheengatu/Colombia
    {    329,   254 }, // Nheengatu/Venezuela
    {    330,   110 }, // Haryanvi/India
    {    331,    91 }, // Northern Frisian/Germany
    {    332,   110 }, // Rajasthani/India
    {    333,   193 }, // Moksha/Russia
    {    334,   258 }, // Toki Pona/world
    {    335,   214 }, // Pijin/Solomon Islands
    {    336,   169 }, // Obolo/Nigeria
    {    337,   178 }, // Baluchi/Pakistan
    {    338,   117 }, // Ligurian/Italy
    {    339,   161 }, // Rohingya/Myanmar
    {    339,    20 }, // Rohingya/Bangladesh
    {    340,   178 }, // Torwali/Pakistan
    {    341,    25 }, // Anii/Benin
    {    342,   110 }, // Kangri/India
    {    343,   117 }, // Venetian/Italy
    {    344,   110 }, // Kuvi/India
    {    345,   251 }, // Kara-Kalpak/Uzbekistan
    {    346,    41 }, // Swampy Cree/Canada
};

// GENERATED PART ENDS HERE

static const int g_locale_list_count = std::size(g_locale_list);

LocaleModel::LocaleModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_data_list.append(1234.5678);
    m_data_list.append(QDate::currentDate());
    m_data_list.append(QDate::currentDate());
    m_data_list.append(QTime::currentTime());
    m_data_list.append(QTime::currentTime());
}

QVariant LocaleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole)
        || index.column() >= g_model_cols
        || index.row() >= g_locale_list_count + 2)
        return QVariant();

    QVariant data;
    if (index.column() < g_model_cols - 1)
        data = m_data_list.at(index.column());

    if (index.row() == 0) {
        if (role == Qt::ToolTipRole)
            return QVariant();
        switch (index.column()) {
            case 0:
                return data.toDouble();
            case 1:
                return data.toDate();
            case 2:
                return data.toDate();
            case 3:
                return data.toTime();
            case 4:
                return data.toTime();
            case 5:
                return QVariant();
            default:
                break;
        }
    } else {
        QLocale locale;
        if (index.row() == 1) {
            locale = QLocale::system();
        } else {
            LocaleListItem item = g_locale_list[index.row() - 2];
            locale = QLocale((QLocale::Language)item.language, (QLocale::Territory)item.territory);
        }

        switch (index.column()) {
            case 0:
                if (role == Qt::ToolTipRole)
                    return QVariant();
                return locale.toString(data.toDouble());
            case 1:
                if (role == Qt::ToolTipRole)
                    return locale.dateFormat(QLocale::LongFormat);
                return locale.toString(data.toDate(), QLocale::LongFormat);
            case 2:
                if (role == Qt::ToolTipRole)
                    return locale.dateFormat(QLocale::ShortFormat);
                return locale.toString(data.toDate(), QLocale::ShortFormat);
            case 3:
                if (role == Qt::ToolTipRole)
                    return locale.timeFormat(QLocale::LongFormat);
                return locale.toString(data.toTime(), QLocale::LongFormat);
            case 4:
                if (role == Qt::ToolTipRole)
                    return locale.timeFormat(QLocale::ShortFormat);
                return locale.toString(data.toTime(), QLocale::ShortFormat);
            case 5:
                if (role == Qt::ToolTipRole)
                    return QVariant();
                return locale.name();
            default:
                break;
        }
    }

    return QVariant();
}

QVariant LocaleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return QLatin1String("Double");
            case 1:
                return QLatin1String("Long Date");
            case 2:
                return QLatin1String("Short Date");
            case 3:
                return QLatin1String("Long Time");
            case 4:
                return QLatin1String("Short Time");
            case 5:
                return QLatin1String("Name");
            default:
                break;
        }
    } else {
        if (section >= g_locale_list_count + 2)
            return QVariant();
        if (section == 0) {
            return QLatin1String("Input");
        } else if (section == 1) {
            return QLatin1String("System");
        } else {
            LocaleListItem item = g_locale_list[section - 2];
            return QLocale::languageToString((QLocale::Language)item.language)
                    + QLatin1Char('/')
                    + QLocale::territoryToString((QLocale::Territory)item.territory);
        }
    }

    return QVariant();
}

QModelIndex LocaleModel::index(int row, int column,
                    const QModelIndex &parent) const
{
    if (parent.isValid()
        || row >= g_locale_list_count + 2
        || column >= g_model_cols)
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex LocaleModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

int LocaleModel::columnCount(const QModelIndex&) const
{
    return g_model_cols;
}

int LocaleModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return g_locale_list_count + 2;
}

Qt::ItemFlags LocaleModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    if (index.row() == 0 && index.column() == g_model_cols - 1)
        return {};
    if (index.row() == 0)
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    return QAbstractItemModel::flags(index);
}

bool LocaleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()
        || index.row() != 0
        || index.column() >= g_model_cols - 1
        || role != Qt::EditRole
        || m_data_list.at(index.column()).typeId() != value.typeId())
        return false;

    m_data_list[index.column()] = value;
    emit dataChanged(createIndex(1, index.column()),
            createIndex(g_locale_list_count, index.column()));

    return true;
}
