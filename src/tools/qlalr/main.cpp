// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lalr.h"
#include "dotgraph.h"
#include "parsetable.h"
#include "cppgenerator.h"
#include "recognizer.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>

#include <cstdlib>

#define QLALR_NO_DEBUG_TABLE
#define QLALR_NO_DEBUG_DOT

using namespace Qt::StringLiterals;

static void help_me ()
{
  qerr() << "Usage: qlalr [options] [input file name]" << Qt::endl
       << Qt::endl
       << "  --help, -h\t\tdisplay this help and exit" << Qt::endl
       << "  --verbose, -v\t\tverbose output" << Qt::endl
       << "  --no-debug\t\tno debug information" << Qt::endl
       << "  --no-lines\t\tno #line directives" << Qt::endl
       << "  --dot\t\t\tgenerate a graph" << Qt::endl
       << "  --qt\t\t\tadd the Qt copyright header and Qt-specific types and macros" << Qt::endl
       << "  --exit-on-warn\texit with status code 2 on warning" << Qt::endl
       << Qt::endl;
  exit (0);
}

int main (int argc, char *argv[])
{
  QCoreApplication app (argc, argv);

  bool generate_dot = false;
  bool generate_report = false;
  bool no_lines = false;
  bool debug_info = true;
  bool qt_copyright = false;
  bool warnings_are_errors = false;
  QString file_name;

  const QStringList args = app.arguments().mid(1);
  for (const QString &arg : args) {
      if (arg == "-h"_L1 || arg == "--help"_L1)
        help_me ();

      else if (arg == "-v"_L1 || arg == "--verbose"_L1)
        generate_report = true;

      else if (arg == "--dot"_L1)
        generate_dot = true;

      else if (arg == "--no-lines"_L1)
        no_lines = true;

      else if (arg == "--no-debug"_L1)
        debug_info = false;

      else if (arg == "--qt"_L1)
        qt_copyright = true;

      else if (arg == "--exit-on-warn"_L1)
        warnings_are_errors = true;

      else if (file_name.isEmpty ())
        file_name = arg;

      else
        qerr() << "*** Warning. Ignore argument `" << arg << "'" << Qt::endl;
    }

  if (file_name.isEmpty ())
    {
      help_me ();
      exit (EXIT_SUCCESS);
    }

  Grammar grammar;
  Recognizer p (&grammar, no_lines);

  if (! p.parse (file_name))
    exit (EXIT_FAILURE);

  if (grammar.rules.empty())
    {
      qerr() << "*** Fatal. No rules!" << Qt::endl;
      exit (EXIT_FAILURE);
    }

  else if (grammar.start == grammar.names.end ())
    {
      qerr() << "*** Fatal. No start symbol!" << Qt::endl;
      exit (EXIT_FAILURE);
    }

  grammar.buildExtendedGrammar ();
  grammar.buildRuleMap ();

  Automaton aut (&grammar);
  aut.build ();

  CppGenerator gen (p, grammar, aut, generate_report);
  gen.setDebugInfo (debug_info);
  gen.setCopyright (qt_copyright);
  gen.setWarningsAreErrors (warnings_are_errors);
  gen ();

  if (generate_dot)
    {
      DotGraph genDotFile (qout());
      genDotFile (&aut);
    }

  else if (generate_report)
    {
      ParseTable genParseTable (qout());
      genParseTable(&aut);
    }

  return EXIT_SUCCESS;
}

QString Recognizer::expand (const QString &text) const
{
  QString code = text;

  if (_M_grammar->start != _M_grammar->names.end ())
    {
      code = code.replace ("$start_id"_L1, QString::number (std::distance (_M_grammar->names.begin (), _M_grammar->start)));
      code = code.replace ("$start"_L1, *_M_grammar->start);
    }

  code = code.replace ("$header"_L1, _M_grammar->table_name.toLower () + "_p.h"_L1);

  code = code.replace ("$table"_L1, _M_grammar->table_name);
  code = code.replace ("$parser"_L1, _M_grammar->table_name);

  if (_M_current_rule != _M_grammar->rules.end ())
    {
      code = code.replace ("$rule_number"_L1, QString::number (std::distance (_M_grammar->rules.begin (), _M_current_rule)));
      code = code.replace ("$rule"_L1, *_M_current_rule->lhs);
    }

  return code;
}
