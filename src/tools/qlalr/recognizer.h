// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include "grammar_p.h"

#include "lalr.h"

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>

#include <cstdlib>

class Recognizer: protected grammar
{
public:
  Recognizer (Grammar *grammar, bool no_lines);
  ~Recognizer();

  bool parse (const QString &input_file = QString ());

  inline QString decls () const { return _M_decls; }
  inline QString impls () const { return _M_impls; }

protected:
  inline void reallocateStack ();

  inline QString &sym (int index)
  { return sym_stack [tos + index - 1]; }

protected: // scanner
  int nextToken();

  inline void inp ()
  {
    if (_M_currentChar != _M_lastChar)
      {
        ch = *_M_currentChar++;

        if (ch == u'\n')
          ++_M_line;
      }
    else
      ch = QChar();
  }

  QString expand (const QString &text) const;

protected:
  // recognizer
  int tos;
  int stack_size;
  QList<QString> sym_stack;
  int *state_stack;

  QString _M_contents;
  QString::const_iterator _M_firstChar;
  QString::const_iterator _M_lastChar;
  QString::const_iterator _M_currentChar;

  // scanner
  QChar ch;
  int _M_line;
  int _M_action_line;
  Grammar *_M_grammar;
  RulePointer _M_current_rule;
  QString _M_input_file;

  QString _M_decls;
  QString _M_impls;
  QString _M_current_value;
  bool _M_no_lines;
};

#endif // RECOGNIZER_H
