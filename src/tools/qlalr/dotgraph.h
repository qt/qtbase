// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DOTGRAPH_H
#define DOTGRAPH_H

#include <QtCore/qglobal.h>

QT_FORWARD_DECLARE_CLASS(QTextStream);
class Automaton;

class DotGraph
{
public:
  DotGraph (QTextStream &out);

  void operator () (Automaton *a);

private:
  QTextStream &out;
};

#endif // DOTGRAPH_H
