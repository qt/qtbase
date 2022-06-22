// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui/private/qlocalfileapi_p.h>
#include <QTest>
#include <emscripten/val.h>

class tst_LocalFileApi : public QObject
{
    Q_OBJECT

private:
    emscripten::val makeAccept(std::vector<emscripten::val> types) {
        auto accept = emscripten::val::object();
        accept.set("application/octet-stream",
            emscripten::val::array(std::move(types)));
        return accept;
    }

    emscripten::val makeType(QString description, std::vector<emscripten::val> acceptExtensions) {
        using namespace emscripten;

        auto type = val::object();
        type.set("description", description.toStdString());

        auto accept = val::object();
        accept.set("application/octet-stream",
            val::array(std::move(acceptExtensions)));
        type.set("accept", makeAccept(std::move(acceptExtensions)));

        return type;
    }

    emscripten::val makeOpenFileOptions(bool acceptMultiple, std::vector<emscripten::val> types) {
        using namespace emscripten;

        auto webFilter = val::object();
        webFilter.set("types", val::array(std::move(types)));
        if (!types.empty())
            webFilter.set("excludeAcceptAllOption", val(true));
        webFilter.set("multiple", val(acceptMultiple));

        return webFilter;
    }

    emscripten::val makeSaveFileOptions(QString suggestedName, std::vector<emscripten::val> types) {
        using namespace emscripten;

        auto webFilter = val::object();
        webFilter.set("suggestedName", val(suggestedName.toStdString()));
        webFilter.set("types", val::array(std::move(types)));

        return webFilter;
    }

private Q_SLOTS:
    void fileExtensionFilterTransformation_data();
    void fileExtensionFilterTransformation();
    void acceptTransformation_data();
    void acceptTransformation();
    void typeTransformation_data();
    void typeTransformation();
    void openFileOptions_data();
    void openFileOptions();
    void saveFileOptions_data();
    void saveFileOptions();
};

bool valDeepEquals(emscripten::val lhs, emscripten::val rhs)
{
    auto json = emscripten::val::global("JSON");
    auto lhsAsJsonString = json.call<emscripten::val>("stringify", lhs);
    auto rhsAsJsonString = json.call<emscripten::val>("stringify", rhs);

    return lhsAsJsonString.equals(rhsAsJsonString);
}

void tst_LocalFileApi::fileExtensionFilterTransformation_data()
{
    QTest::addColumn<QString>("qtFileFilter");
    QTest::addColumn<std::optional<std::string>>("expectedWebExtensionFilter");

    QTest::newRow("PNG extension with an asterisk") << QString("*.png") << std::make_optional<std::string>(".png");
    QTest::newRow("Long extension with an asterisk") << QString("*.someotherfile") << std::make_optional<std::string>(".someotherfile");
    QTest::newRow(".dat with no asterisk") << QString(".dat") << std::make_optional<std::string>(".dat");
    QTest::newRow("Multiple asterisks") << QString("*ot*.abc") << std::optional<std::string>();
    QTest::newRow("Filename") << QString("abcd.abc") << std::optional<std::string>();
    QTest::newRow("match all") << QString("*.*") << std::optional<std::string>();
}

void tst_LocalFileApi::fileExtensionFilterTransformation()
{
    QFETCH(QString, qtFileFilter);
    QFETCH(std::optional<std::string>, expectedWebExtensionFilter);

    auto result = LocalFileApi::Type::Accept::MimeType::Extension::fromQt(qtFileFilter);
    if (expectedWebExtensionFilter) {
        QCOMPARE_EQ(expectedWebExtensionFilter, result->asVal().as<std::string>());
    } else {
        QVERIFY(!result.has_value());
    }
}

void tst_LocalFileApi::acceptTransformation_data()
{
    using namespace emscripten;

    QTest::addColumn<QString>("qtFilterList");
    QTest::addColumn<emscripten::val>("expectedWebType");

    QTest::newRow("Multiple types") << QString("*.png *.other *.txt")
        << makeAccept(std::vector<val> { val(".png"), val(".other"), val(".txt") });

    QTest::newRow("Single type") << QString("*.png")
        << makeAccept(std::vector<val> { val(".png") });

    QTest::newRow("No filter when accepts all") << QString("*.*")
        << val::undefined();

    QTest::newRow("No filter when one filter accepts all") << QString("*.* *.jpg")
        << val::undefined();

    QTest::newRow("Weird spaces") << QString("    *.jpg     *.png *.icon  ")
        << makeAccept(std::vector<val> { val(".jpg"), val(".png"), val(".icon") });
}

void tst_LocalFileApi::acceptTransformation()
{
    QFETCH(QString, qtFilterList);
    QFETCH(emscripten::val, expectedWebType);

    auto result = LocalFileApi::Type::Accept::fromQt(qtFilterList);
    if (!expectedWebType.isUndefined()) {
        QVERIFY(valDeepEquals(result->asVal(), expectedWebType));
    } else {
        QVERIFY(!result.has_value());
    }
}

void tst_LocalFileApi::typeTransformation_data()
{
    using namespace emscripten;

    QTest::addColumn<QString>("qtFilterList");
    QTest::addColumn<emscripten::val>("expectedWebType");

    QTest::newRow("With description") << QString("Text files (*.txt)")
        << makeType("Text files", std::vector<val> { val(".txt") });

    QTest::newRow("No description") << QString("*.jpg")
        << makeType("", std::vector<val> { val(".jpg") });
}

void tst_LocalFileApi::typeTransformation()
{
    QFETCH(QString, qtFilterList);
    QFETCH(emscripten::val, expectedWebType);

    auto result = LocalFileApi::Type::fromQt(qtFilterList);
    if (!expectedWebType.isUndefined()) {
        QVERIFY(valDeepEquals(result->asVal(), expectedWebType));
    } else {
        QVERIFY(!result.has_value());
    }
}

void tst_LocalFileApi::openFileOptions_data()
{
    using namespace emscripten;

    QTest::addColumn<QStringList>("qtFilterList");
    QTest::addColumn<bool>("multiple");
    QTest::addColumn<emscripten::val>("expectedWebType");

    QTest::newRow("Multiple files") << QStringList({"Text files (*.txt)", "Images (*.jpg *.png)", "*.bat"})
        << true
        << makeOpenFileOptions(true, { makeType("Text files", { val(".txt")}), makeType("Images", { val(".jpg"), val(".png")}), makeType("", { val(".bat")})});
    QTest::newRow("Single file") << QStringList({"Text files (*.txt)", "Images (*.jpg *.png)", "*.bat"})
        << false
        << makeOpenFileOptions(false, { makeType("Text files", { val(".txt")}), makeType("Images", { val(".jpg"), val(".png")}), makeType("", { val(".bat")})});
}

void tst_LocalFileApi::openFileOptions()
{
    QFETCH(QStringList, qtFilterList);
    QFETCH(bool, multiple);
    QFETCH(emscripten::val, expectedWebType);

    auto result = LocalFileApi::makeOpenFileOptions(qtFilterList, multiple);
    QVERIFY(valDeepEquals(result, expectedWebType));
}

void tst_LocalFileApi::saveFileOptions_data()
{
    using namespace emscripten;

    QTest::addColumn<QStringList>("qtFilterList");
    QTest::addColumn<QString>("suggestedName");
    QTest::addColumn<emscripten::val>("expectedWebType");

    QTest::newRow("Multiple files") << QStringList({"Text files (*.txt)", "Images (*.jpg *.png)", "*.bat"})
        << "someName1"
        << makeSaveFileOptions("someName1", { makeType("Text files", { val(".txt")}), makeType("Images", { val(".jpg"), val(".png")}), makeType("", { val(".bat")})});
    QTest::newRow("Single file") << QStringList({"Text files (*.txt)", "Images (*.jpg *.png)", "*.bat"})
        << "some name 2"
        << makeSaveFileOptions("some name 2", { makeType("Text files", { val(".txt")}), makeType("Images", { val(".jpg"), val(".png")}), makeType("", { val(".bat")})});
}

void tst_LocalFileApi::saveFileOptions()
{
    QFETCH(QStringList, qtFilterList);
    QFETCH(QString, suggestedName);
    QFETCH(emscripten::val, expectedWebType);

    auto result = LocalFileApi::makeSaveFileOptions(qtFilterList, suggestedName.toStdString());
    QVERIFY(valDeepEquals(result, expectedWebType));
}

QTEST_APPLESS_MAIN(tst_LocalFileApi)
#include "tst_localfileapi.moc"
