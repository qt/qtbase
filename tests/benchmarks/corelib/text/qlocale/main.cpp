/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QLocale>
#include <QTest>

class tst_QLocale : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fromString_data();
    void fromString();
    void fromTags_data();
    void fromTags();
    void fromLangScript_data();
    void fromLangScript();
    void fromLangLand_data();
    void fromLangLand();
    void fromScriptLand_data();
    void fromScriptLand();
    void fromLang_data();
    void fromLang();
    void fromScript_data();
    void fromScript();
    void fromLand_data();
    void fromLand();
    void toUpper_QLocale_1();
    void toUpper_QLocale_2();
    void toUpper_QString();
    void number_QString();
};

static QString data()
{
    return QStringLiteral("/qt-5/qtbase/tests/benchmarks/corelib/tools/qlocale");
}

// Make individual cycles O(a few) msecs, rather than tiny fractions thereof:
#define LOOP(s) for (int i = 0; i < 5000; ++i) { s; }

void tst_QLocale::fromString_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("C") << QStringLiteral("C");
#define ROW(name) QTest::newRow(name) << QStringLiteral(name)
    ROW("en-Latn-DE");
    ROW("sd-Deva-IN");
    ROW("az-Cyrl-AZ");
    ROW("az-Latn-AZ");
    ROW("bs-Cyrl-BA");
    ROW("bs-Latn-BA");
    ROW("ff-Latn-LR");
    ROW("ff-Latn-MR");
    ROW("pa-Arab-PK");
    ROW("pa-Guru-IN");
    ROW("shi-Latn-MA");
    ROW("shi-Tfng-MA");
    ROW("sr-Cyrl-BA");
    ROW("sr-Cyrl-RS");
    ROW("sr-Latn-BA");
    ROW("sr-Latn-ME");
    ROW("uz-Arab-AF");
    ROW("uz-Cyrl-UZ");
    ROW("uz-Latn-UZ");
    ROW("vai-Latn-LR");
    ROW("vai-Vaii-LR");
    ROW("yue-Hans-CN");
    ROW("yue-Hant-HK");
    ROW("zh-Hans-CN");
    ROW("zh-Hans-HK");
    ROW("zh-Hans-SG");
    ROW("zh-Hant-HK");
    ROW("zh-Hant-TW");
#undef ROW
}

void tst_QLocale::fromString()
{
    QFETCH(const QString, name);
    QBENCHMARK { LOOP(QLocale loc(name)) }
}

void tst_QLocale::fromTags_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Script>("script");
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, lang, text, land) \
        QTest::newRow(name) << QLocale::lang << QLocale::text << QLocale::land
    ROW("C", C, AnyScript, AnyTerritory);
    ROW("en-Latn-DE", English, LatinScript, Germany);
    ROW("sd-Deva-IN", Sindhi, DevanagariScript, India);
    ROW("az-Cyrl-AZ", Azerbaijani, CyrillicScript, Azerbaijan);
    ROW("az-Latn-AZ", Azerbaijani, LatinScript, Azerbaijan);
    ROW("bs-Cyrl-BA", Bosnian, CyrillicScript, BosniaAndHerzegowina);
    ROW("bs-Latn-BA", Bosnian, LatinScript, BosniaAndHerzegowina);
    ROW("ff-Latn-LR", Fulah, LatinScript, Liberia);
    ROW("ff-Latn-MR", Fulah, LatinScript, Mauritania);
    ROW("pa-Arab-PK", Punjabi, ArabicScript, Pakistan);
    ROW("pa-Guru-IN", Punjabi, GurmukhiScript, India);
    ROW("shi-Latn-MA", Tachelhit, LatinScript, Morocco);
    ROW("shi-Tfng-MA", Tachelhit, TifinaghScript, Morocco);
    ROW("sr-Cyrl-BA", Serbian, CyrillicScript, BosniaAndHerzegowina);
    ROW("sr-Cyrl-RS", Serbian, CyrillicScript, Serbia);
    ROW("sr-Latn-BA", Serbian, LatinScript, BosniaAndHerzegowina);
    ROW("sr-Latn-ME", Serbian, LatinScript, Montenegro);
    ROW("uz-Arab-AF", Uzbek, ArabicScript, Afghanistan);
    ROW("uz-Cyrl-UZ", Uzbek, CyrillicScript, Uzbekistan);
    ROW("uz-Latn-UZ", Uzbek, LatinScript, Uzbekistan);
    ROW("vai-Latn-LR", Vai, LatinScript, Liberia);
    ROW("vai-Vaii-LR", Vai, VaiScript, Liberia);
    ROW("yue-Hans-CN", Cantonese, SimplifiedHanScript, China);
    ROW("yue-Hant-HK", Cantonese, TraditionalHanScript, HongKong);
    ROW("zh-Hans-CN", Chinese, SimplifiedHanScript, China);
    ROW("zh-Hans-HK", Chinese, SimplifiedHanScript, HongKong);
    ROW("zh-Hans-SG", Chinese, SimplifiedHanScript, Singapore);
    ROW("zh-Hant-HK", Chinese, TraditionalHanScript, HongKong);
    ROW("zh-Hant-TW", Chinese, TraditionalHanScript, Taiwan);
#undef ROW
}

void tst_QLocale::fromTags()
{
    QFETCH(const QLocale::Language, language);
    QFETCH(const QLocale::Script, script);
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(language, script, territory)) }
}

void tst_QLocale::fromLangScript_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Script>("script");

#define ROW(name, lang, text) \
        QTest::newRow(name) << QLocale::lang << QLocale::text
    ROW("C", C, AnyScript);
    ROW("en-Latn", English, LatinScript);
    ROW("sd-Deva", Sindhi, DevanagariScript);
    ROW("az-Cyrl", Azerbaijani, CyrillicScript);
    ROW("az-Latn", Azerbaijani, LatinScript);
    ROW("bs-Cyrl", Bosnian, CyrillicScript);
    ROW("bs-Latn", Bosnian, LatinScript);
    ROW("ff-Latn", Fulah, LatinScript);
    ROW("pa-Arab", Punjabi, ArabicScript);
    ROW("pa-Guru", Punjabi, GurmukhiScript);
    ROW("shi-Latn", Tachelhit, LatinScript);
    ROW("shi-Tfng", Tachelhit, TifinaghScript);
    ROW("sr-Cyrl", Serbian, CyrillicScript);
    ROW("sr-Latn", Serbian, LatinScript);
    ROW("uz-Arab", Uzbek, ArabicScript);
    ROW("uz-Cyrl", Uzbek, CyrillicScript);
    ROW("uz-Latn", Uzbek, LatinScript);
    ROW("vai-Latn", Vai, LatinScript);
    ROW("vai-Vaii", Vai, VaiScript);
    ROW("yue-Hans", Cantonese, SimplifiedHanScript);
    ROW("yue-Hant", Cantonese, TraditionalHanScript);
    ROW("zh-Hans", Chinese, SimplifiedHanScript);
    ROW("zh-Hant", Chinese, TraditionalHanScript);
#undef ROW
}

void tst_QLocale::fromLangScript()
{
    QFETCH(const QLocale::Language, language);
    QFETCH(const QLocale::Script, script);
    QBENCHMARK { LOOP(QLocale loc(language, script, QLocale::AnyTerritory)) }
}

void tst_QLocale::fromLangLand_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, lang, land) \
        QTest::newRow(name) << QLocale::lang << QLocale::land
    ROW("C", C, AnyTerritory);
    ROW("en-DE", English, Germany);
    ROW("sd-IN", Sindhi, India);
    ROW("az-AZ", Azerbaijani, Azerbaijan);
    ROW("bs-BA", Bosnian, BosniaAndHerzegowina);
    ROW("ff-LR", Fulah, Liberia);
    ROW("ff-MR", Fulah, Mauritania);
    ROW("pa-PK", Punjabi, Pakistan);
    ROW("pa-IN", Punjabi, India);
    ROW("shi-MA", Tachelhit, Morocco);
    ROW("sr-BA", Serbian, BosniaAndHerzegowina);
    ROW("sr-RS", Serbian, Serbia);
    ROW("sr-ME", Serbian, Montenegro);
    ROW("uz-AF", Uzbek, Afghanistan);
    ROW("uz-UZ", Uzbek, Uzbekistan);
    ROW("vai-LR", Vai, Liberia);
    ROW("yue-CN", Cantonese, China);
    ROW("yue-HK", Cantonese, HongKong);
    ROW("zh-CN", Chinese, China);
    ROW("zh-HK", Chinese, HongKong);
    ROW("zh-SG", Chinese, Singapore);
    ROW("zh-TW", Chinese, Taiwan);
#undef ROW
}

void tst_QLocale::fromLangLand()
{
    QFETCH(const QLocale::Language, language);
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(language, territory)) }
}

void tst_QLocale::fromScriptLand_data()
{
    QTest::addColumn<QLocale::Script>("script");
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, text, land) \
        QTest::newRow(name) << QLocale::text << QLocale::land
    ROW("Any", AnyScript, AnyTerritory);
    ROW("Latn-DE", LatinScript, Germany);
    ROW("Deva-IN", DevanagariScript, India);
    ROW("Cyrl-AZ", CyrillicScript, Azerbaijan);
    ROW("Latn-AZ", LatinScript, Azerbaijan);
    ROW("Cyrl-BA", CyrillicScript, BosniaAndHerzegowina);
    ROW("Latn-BA", LatinScript, BosniaAndHerzegowina);
    ROW("Latn-LR", LatinScript, Liberia);
    ROW("Latn-MR", LatinScript, Mauritania);
    ROW("Arab-PK", ArabicScript, Pakistan);
    ROW("Guru-IN", GurmukhiScript, India);
    ROW("Latn-MA", LatinScript, Morocco);
    ROW("Tfng-MA", TifinaghScript, Morocco);
    ROW("Cyrl-BA", CyrillicScript, BosniaAndHerzegowina);
    ROW("Cyrl-RS", CyrillicScript, Serbia);
    ROW("Latn-BA", LatinScript, BosniaAndHerzegowina);
    ROW("Latn-ME", LatinScript, Montenegro);
    ROW("Arab-AF", ArabicScript, Afghanistan);
    ROW("Cyrl-UZ", CyrillicScript, Uzbekistan);
    ROW("Latn-UZ", LatinScript, Uzbekistan);
    ROW("Latn-LR", LatinScript, Liberia);
    ROW("Vaii-LR", VaiScript, Liberia);
    ROW("Hans-CN", SimplifiedHanScript, China);
    ROW("Hant-HK", TraditionalHanScript, HongKong);
    ROW("Hans-CN", SimplifiedHanScript, China);
    ROW("Hans-HK", SimplifiedHanScript, HongKong);
    ROW("Hans-SG", SimplifiedHanScript, Singapore);
    ROW("Hant-HK", TraditionalHanScript, HongKong);
    ROW("Hant-TW", TraditionalHanScript, Taiwan);
#undef ROW
}

void tst_QLocale::fromScriptLand()
{
    QFETCH(const QLocale::Script, script);
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(QLocale::AnyLanguage, script, territory)) }
}

void tst_QLocale::fromLang_data()
{
    QTest::addColumn<QLocale::Language>("language");

#define ROW(name, lang) \
        QTest::newRow(name) << QLocale::lang
    ROW("C", C);
    ROW("en", English);
    ROW("sd", Sindhi);
    ROW("az", Azerbaijani);
    ROW("bs", Bosnian);
    ROW("ff", Fulah);
    ROW("pa", Punjabi);
    ROW("shi", Tachelhit);
    ROW("sr", Serbian);
    ROW("uz", Uzbek);
    ROW("vai", Vai);
    ROW("yue", Cantonese);
    ROW("zh", Chinese);
#undef ROW
}

void tst_QLocale::fromLang()
{
    QFETCH(const QLocale::Language, language);
    QBENCHMARK { LOOP(QLocale loc(language)) }
}

void tst_QLocale::fromScript_data()
{
    QTest::addColumn<QLocale::Script>("script");

#define ROW(name, text) \
        QTest::newRow(name) << QLocale::text
    ROW("Any", AnyScript);
    ROW("Latn", LatinScript);
    ROW("Deva", DevanagariScript);
    ROW("Cyrl", CyrillicScript);
    ROW("Arab", ArabicScript);
    ROW("Guru", GurmukhiScript);
    ROW("Tfng", TifinaghScript);
    ROW("Vaii", VaiScript);
    ROW("Hans", SimplifiedHanScript);
    ROW("Hant", TraditionalHanScript);
#undef ROW
}

void tst_QLocale::fromScript()
{
    QFETCH(const QLocale::Script, script);
    QBENCHMARK { LOOP(QLocale loc(QLocale::AnyLanguage, script, QLocale::AnyTerritory)) }
}

void tst_QLocale::fromLand_data()
{
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, land) \
        QTest::newRow(name) << QLocale::land
    ROW("Any", AnyTerritory);
    ROW("DE", Germany);
    ROW("IN", India);
    ROW("AZ", Azerbaijan);
    ROW("BA", BosniaAndHerzegowina);
    ROW("LR", Liberia);
    ROW("MR", Mauritania);
    ROW("PK", Pakistan);
    ROW("MA", Morocco);
    ROW("RS", Serbia);
    ROW("ME", Montenegro);
    ROW("AF", Afghanistan);
    ROW("UZ", Uzbekistan);
    ROW("CN", China);
    ROW("HK", HongKong);
    ROW("SG", Singapore);
    ROW("TW", Taiwan);
#undef ROW
}

void tst_QLocale::fromLand()
{
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(QLocale::AnyLanguage, territory)) }
}

void tst_QLocale::toUpper_QLocale_1()
{
    QString s = data();
    QBENCHMARK { LOOP(QString t(QLocale().toUpper(s))) }
}

void tst_QLocale::toUpper_QLocale_2()
{
    QString s = data();
    QLocale l;
    QBENCHMARK { LOOP(QString t(l.toUpper(s))) }
}

void tst_QLocale::toUpper_QString()
{
    QString s = data();
    QBENCHMARK { LOOP(QString t(s.toUpper())) }
}

void tst_QLocale::number_QString()
{
    QString s;
    QBENCHMARK {
        s = QString::number(12345678);
    }
}

QTEST_MAIN(tst_QLocale)

#include "main.moc"
