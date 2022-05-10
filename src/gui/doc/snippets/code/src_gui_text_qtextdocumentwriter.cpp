// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QTextDocumentWriter>

namespace src_gui_text_qtextdocumentwriter {

void wrapper() {
//! [0]
        QTextDocumentWriter writer;
        writer.setFormat("odf"); // same as writer.setFormat("ODF");
//! [0]
} // wrapper

} // src_gui_text_qtextdocumentwriter
