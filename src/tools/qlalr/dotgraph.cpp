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

#include "dotgraph.h"

#include "lalr.h"

#include <QtCore/qtextstream.h>

DotGraph::DotGraph(QTextStream &o):
  out (o)
{
}

void DotGraph::operator () (Automaton *aut)
{
  Grammar *g = aut->_M_grammar;

  out << "digraph {" << Qt::endl << Qt::endl;

  out << "subgraph Includes {" << Qt::endl;
  for (Automaton::IncludesGraph::iterator incl = Automaton::IncludesGraph::begin_nodes ();
       incl != Automaton::IncludesGraph::end_nodes (); ++incl)
    {
      for (Automaton::IncludesGraph::edge_iterator edge = incl->begin (); edge != incl->end (); ++edge)
        {
          out << "\t\"(" << aut->id (incl->data.state) << ", " << incl->data.nt << ")\"";
          out << "\t->\t";
          out << "\"(" << aut->id ((*edge)->data.state) << ", " << (*edge)->data.nt << ")\"\t";
          out << "[label=\"" << incl->data.state->follows [incl->data.nt] << "\"]";
          out << Qt::endl;
        }
    }
  out << "}" << Qt::endl << Qt::endl;


  out << "subgraph LRA {" << Qt::endl;
  //out << "node [shape=record];" << Qt::endl << Qt::endl;

  for (StatePointer q = aut->states.begin (); q != aut->states.end (); ++q)
    {
      int state = aut->id (q);

      out << "\t" << state << "\t[shape=record,label=\"{";

      out << "<0> State " << state;

      int index = 1;
      for (ItemPointer item = q->kernel.begin (); item != q->kernel.end (); ++item)
        out << "| <" << index++ << "> " << *item;

      out << "}\"]" << Qt::endl;

      for (Bundle::iterator a = q->bundle.begin (); a != q->bundle.end (); ++a)
        {
          const char *clr = g->isTerminal (a.key ()) ? "blue" : "red";
          out << "\t" << state << "\t->\t" << aut->id (*a) << "\t[color=\"" << clr << "\",label=\"" << a.key () << "\"]" << Qt::endl;
        }
      out << Qt::endl;
    }

  out << "}" << Qt::endl;
  out << Qt::endl << Qt::endl << "}" << Qt::endl;
}
