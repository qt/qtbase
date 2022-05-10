// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_PrintDataTagsWithGlobalTags: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase_data() const;
    void initTestCase() const;

    void a_data() const;
    void a() const;

    void b() const;

    void c_data() const;
    void c() const;
};

void tst_PrintDataTagsWithGlobalTags::initTestCase_data() const
{
    QTest::addColumn<int>("f");
    QTest::addColumn<int>("g");

    QTest::newRow("global data tag 1 ") << 1 << 2;
    QTest::newRow("global data tag 2") << 1 << 2;
}

void tst_PrintDataTagsWithGlobalTags::initTestCase() const
{
}

void tst_PrintDataTagsWithGlobalTags::a_data() const
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");

    QTest::newRow("data tag a1 ") << 1 << 2;
    QTest::newRow("data tag a2") << 1 << 2;
}

void tst_PrintDataTagsWithGlobalTags::a() const
{
}

void tst_PrintDataTagsWithGlobalTags::b() const
{
}

void tst_PrintDataTagsWithGlobalTags::c_data() const
{
    QTest::addColumn<int>("x");

    QTest::newRow("data tag c1") << 1;
    QTest::newRow("data tag c2") << 1;
    QTest::newRow("data tag c3") << 1;
}

void tst_PrintDataTagsWithGlobalTags::c() const
{
}

QTEST_MAIN_WRAPPER(tst_PrintDataTagsWithGlobalTags,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-datatags");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_printdatatagswithglobaltags.moc"
