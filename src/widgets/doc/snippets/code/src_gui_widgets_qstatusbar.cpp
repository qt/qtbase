// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
statusBar()->addWidget(new MyReadWriteIndication);
//! [0]

//! [1]
statusBar()->showMessage(tr("Ready"));
//! [1]

//! [2]
statusBar()->showMessage(tr("Ready"), 2000);
//! [2]
