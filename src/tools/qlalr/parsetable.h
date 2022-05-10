// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PARSETABLE_H
#define PARSETABLE_H

#include <QtCore/qglobal.h>

QT_FORWARD_DECLARE_CLASS(QTextStream);
class Automaton;

class ParseTable
{
public:
  ParseTable (QTextStream &out);

  void operator () (Automaton *a);

private:
  QTextStream &out;
};

#endif // PARSETABLE_H
