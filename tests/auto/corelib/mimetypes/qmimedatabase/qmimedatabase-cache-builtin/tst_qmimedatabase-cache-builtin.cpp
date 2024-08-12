// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../tst_qmimedatabase.cpp"

void tst_QMimeDatabase::initTestCaseInternal()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#else
    const QString mimeDirName = m_globalXdgDir + QStringLiteral("/mime");
    runUpdateMimeDatabase(mimeDirName);
    QVERIFY(QFile::exists(mimeDirName + QStringLiteral("/mime.cache")));
#endif
}

bool tst_QMimeDatabase::useCacheProvider() const
{
    return true;
}

bool tst_QMimeDatabase::useFreeDesktopOrgXml() const
{
    return false;
}
