// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "lalr.h"
#include "compress.h"

class Grammar;
class Automaton;
class Recognizer;

class CppGenerator
{
public:
  CppGenerator(const Recognizer &p, Grammar &grammar, Automaton &aut, bool verbose):
    p (p),
    grammar (grammar),
    aut (aut),
    verbose (verbose),
    debug_info (false),
    copyright (false),
    warnings_are_errors(false) {}

  void operator () ();

  bool debugInfo () const { return debug_info; }
  void setDebugInfo (bool d) { debug_info = d; }

  void setUsePragmaOnce(bool use) { use_pragma_once = use; }

  void setCopyright (bool t) { copyright = t; }

  void setWarningsAreErrors (bool e) { warnings_are_errors = e; }

private:
  void generateDecl (QTextStream &out);
  void generateImpl (QTextStream &out);

  QString debugInfoProt() const;
  QString copyrightHeader() const;
  QString privateCopyrightHeader() const;

private:
  QString startIncludeGuard(const QString &fileName) const;
  QString endIncludeGuard(const QString &fileName) const;

  const Recognizer &p;
  Grammar &grammar;
  Automaton &aut;
  bool verbose;
  int accept_state;
  int state_count;
  int terminal_count;
  int non_terminal_count;
  bool debug_info;
  bool use_pragma_once = false;
  bool copyright;
  bool warnings_are_errors;
  Compress compressed_action;
  Compress compressed_goto;
  QList<int> count;
  QList<int> defgoto;
};
