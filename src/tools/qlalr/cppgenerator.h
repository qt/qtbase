// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:insignificant reason:build-tool

#pragma once

#include "lalr.h"
#include "compress.h"

#include <optional>

class Grammar;
class Automaton;
class Recognizer;

class CppGenerator
{
public:
    struct SecurityHeader // see QUIP-23
    {
        enum class Security {
            Insignificant,
            Significant,
            Critical,
        };

        static std::optional<SecurityHeader> parse(QStringView s);
        QByteArray print() const;

        Security score;
        QByteArray reason;
    };

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
  void setSecurityHeader(SecurityHeader header)
  {
      security = std::move(header);
  }

  void setCopyright (bool t) { copyright = t; }
  void setCopyrightText (const QString &text) { m_copyrightText = text; }
  void setEmitQtCode (bool t) { emit_qt_code = t; }

  void setWarningsAreErrors (bool e) { warnings_are_errors = e; }

private:
  void generateDecl (QTextStream &out);
  void generateImpl (QTextStream &out);

  QString debugInfoProt() const;
  QByteArray copyrightHeader() const;
  QString privateCopyrightHeader() const;

private:
  QString startIncludeGuard(const QString &fileName) const;
  QString endIncludeGuard(const QString &fileName) const;

  const Recognizer &p;
  Grammar &grammar;
  Automaton &aut;

  std::optional<SecurityHeader> security;

  bool verbose;
  int accept_state;
  int state_count;
  int terminal_count;
  int non_terminal_count;
  bool debug_info;
  bool use_pragma_once = false;
  bool copyright;
  bool emit_qt_code = false;
  QString m_copyrightText;
  bool warnings_are_errors;
  Compress compressed_action;
  Compress compressed_goto;
  QList<int> count;
  QList<int> defgoto;
};
