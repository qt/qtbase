/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QLALR module of the Qt Toolkit.
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

#ifndef CPPGENERATOR_H
#define CPPGENERATOR_H

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
    copyright (false) {}

  void operator () ();

  bool debugInfo () const { return debug_info; }
  void setDebugInfo (bool d) { debug_info = d; }

  void setCopyright (bool t) { copyright = t; }

private:
  void generateDecl (QTextStream &out);
  void generateImpl (QTextStream &out);

  QString debugInfoProt() const;
  QString copyrightHeader() const;
  QString privateCopyrightHeader() const;

private:
  static QString startIncludeGuard(const QString &fileName);
  static QString endIncludeGuard(const QString &fileName);

  const Recognizer &p;
  Grammar &grammar;
  Automaton &aut;
  bool verbose;
  int accept_state;
  int state_count;
  int terminal_count;
  int non_terminal_count;
  bool debug_info;
  bool copyright;
  Compress compressed_action;
  Compress compressed_goto;
  QVector<int> count;
  QVector<int> defgoto;
};

#endif // CPPGENERATOR_H
