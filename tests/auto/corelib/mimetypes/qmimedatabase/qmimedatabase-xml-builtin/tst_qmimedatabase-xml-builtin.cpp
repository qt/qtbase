// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../tst_qmimedatabase.cpp"

void tst_QMimeDatabase::initTestCaseInternal()
{
    qputenv("QT_NO_MIME_CACHE", "1");
}

bool tst_QMimeDatabase::useCacheProvider() const
{
    return false;
}

bool tst_QMimeDatabase::useFreeDesktopOrgXml() const
{
    return false;
}
