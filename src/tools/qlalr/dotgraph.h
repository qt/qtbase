/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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
