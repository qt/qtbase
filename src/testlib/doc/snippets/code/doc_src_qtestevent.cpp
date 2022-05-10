// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QTest>
#include <QLineEdit>

#include "doc_src_qtestevent.h"

void TestGui::testGui_data()
{
QWidget *myParent = nullptr;
//! [0]
QTestEventList events;
events.addKeyClick('a');
events.addKeyClick(Qt::Key_Backspace);
events.addDelay(200);
QLineEdit *lineEdit = new QLineEdit(myParent);
// ...
events.simulate(lineEdit);
events.simulate(lineEdit);
//! [0]
}
