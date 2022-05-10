// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QStringList list = myIndex.toStringList();
QStringList::Iterator it = list.begin();
while(it != list.end()) {
    myProcessing(*it);
    ++it;
}
//! [0]
