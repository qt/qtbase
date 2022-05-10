// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parsetable.h"

#include "lalr.h"

#include <QtCore/qtextstream.h>

ParseTable::ParseTable (QTextStream &o):
  out (o)
{
}

void ParseTable::operator () (Automaton *aut)
{
  Grammar *g = aut->_M_grammar;

  int rindex = 1;
  for (RulePointer rule = g->rules.begin (); rule != g->rules.end (); ++rule)
    out << rindex++ << ")\t" << *rule << Qt::endl;
  out << Qt::endl << Qt::endl;

  int index = 0;
  for (StatePointer state = aut->states.begin (); state != aut->states.end (); ++state)
    {
      out << "state " << index++ << Qt::endl << Qt::endl;

      for (ItemPointer item = state->kernel.begin (); item != state->kernel.end (); ++item)
        {
          out << " *  " << *item;

          if (item->dot == item->end_rhs ())
            out << " " << aut->lookaheads [item];

          out << Qt::endl;
        }

      bool first = true;
      for (Bundle::iterator arrow = state->bundle.begin (); arrow != state->bundle.end (); ++arrow)
        {
          if (! g->isTerminal (arrow.key ()))
            continue;

          if (first)
            out << Qt::endl;

          first = false;

          out << "    " << *arrow.key () << " shift, and go to state " << std::distance (aut->states.begin (), *arrow) << Qt::endl;
        }

      first = true;
      for (ItemPointer item = state->closure.begin (); item != state->closure.end (); ++item)
        {
          if (item->dot != item->end_rhs () || item->rule == state->defaultReduce)
            continue;

          if (first)
            out << Qt::endl;

          first = false;

          const auto lookaheads = aut->lookaheads.value(item);
          for (const Name &la : lookaheads)
            out << "    " << *la << " reduce using rule " << aut->id (item->rule) << " (" << *item->rule->lhs << ")" << Qt::endl;
        }

      first = true;
      for (Bundle::iterator arrow = state->bundle.begin (); arrow != state->bundle.end (); ++arrow)
        {
          if (! g->isNonTerminal (arrow.key ()))
            continue;

          if (first)
            out << Qt::endl;

          first = false;

          out << "    " << *arrow.key () << " go to state " << std::distance (aut->states.begin (), *arrow) << Qt::endl;
        }

      if (state->defaultReduce != g->rules.end ())
        {
          out << Qt::endl
              << "    $default reduce using rule " << aut->id (state->defaultReduce) << " (" << *state->defaultReduce->lhs << ")" << Qt::endl;
        }

      out << Qt::endl;
    }
}
