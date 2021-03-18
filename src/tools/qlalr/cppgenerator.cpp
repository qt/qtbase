/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QLALR module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "cppgenerator.h"

#include "lalr.h"
#include "recognizer.h"

#include <QtCore/qbitarray.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qfile.h>
#include <QtCore/qmap.h>

#include <iterator>

namespace {

void generateSeparator(int i, QTextStream &out)
{
    if (!(i % 10)) {
        if (i)
            out << ",";
        out << Qt::endl << "    ";
    } else {
        out << ", ";
    }
}

void generateList(const QVector<int> &list, QTextStream &out)
{
    for (int i = 0; i < list.size(); ++i) {
        generateSeparator(i, out);

        out << list[i];
    }
}

}

QString CppGenerator::copyrightHeader() const
{
  return QLatin1String(
    "/****************************************************************************\n"
    "**\n"
    "** Copyright (C) 2016 The Qt Company Ltd.\n"
    "** Contact: https://www.qt.io/licensing/\n"
    "**\n"
    "** This file is part of the Qt Toolkit.\n"
    "**\n"
    "** $QT_BEGIN_LICENSE:GPL-EXCEPT$\n"
    "** Commercial License Usage\n"
    "** Licensees holding valid commercial Qt licenses may use this file in\n"
    "** accordance with the commercial license agreement provided with the\n"
    "** Software or, alternatively, in accordance with the terms contained in\n"
    "** a written agreement between you and The Qt Company. For licensing terms\n"
    "** and conditions see https://www.qt.io/terms-conditions. For further\n"
    "** information use the contact form at https://www.qt.io/contact-us.\n"
    "**\n"
    "** GNU General Public License Usage\n"
    "** Alternatively, this file may be used under the terms of the GNU\n"
    "** General Public License version 3 as published by the Free Software\n"
    "** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT\n"
    "** included in the packaging of this file. Please review the following\n"
    "** information to ensure the GNU General Public License requirements will\n"
    "** be met: https://www.gnu.org/licenses/gpl-3.0.html.\n"
    "**\n"
    "** $QT_END_LICENSE$\n"
    "**\n"
    "****************************************************************************/\n"
    "\n");
}

QString CppGenerator::privateCopyrightHeader() const
{
  return QLatin1String(
    "//\n"
    "//  W A R N I N G\n"
    "//  -------------\n"
    "//\n"
    "// This file is not part of the Qt API.  It exists for the convenience\n"
    "// of other Qt classes.  This header file may change from version to\n"
    "// version without notice, or even be removed.\n"
    "//\n"
    "// We mean it.\n"
    "//\n");
}

QString CppGenerator::startIncludeGuard(const QString &fileName)
{
    const QString normalized(QString(fileName).replace(QLatin1Char('.'), QLatin1Char('_')).toUpper());

    return QString::fromLatin1("#ifndef %1\n"
                               "#define %2\n").arg(normalized, normalized);
}

QString CppGenerator::endIncludeGuard(const QString &fileName)
{
    const QString normalized(QString(fileName).replace(QLatin1Char('.'), QLatin1Char('_')).toUpper());

    return QString::fromLatin1("#endif // %1\n").arg(normalized);
}

void CppGenerator::operator () ()
{
  // action table...
  state_count = static_cast<int>(aut.states.size());
  terminal_count = static_cast<int>(grammar.terminals.size());
  non_terminal_count = static_cast<int>(grammar.non_terminals.size());

#define ACTION(i, j) table [(i) * terminal_count + (j)]
#define GOTO(i, j) pgoto [(i) * non_terminal_count + (j)]

  int *table = new int [state_count * terminal_count];
  ::memset (table, 0, state_count * terminal_count * sizeof (int));

  int *pgoto = new int [state_count * non_terminal_count];
  ::memset (pgoto, 0, state_count * non_terminal_count * sizeof (int));

  accept_state = -1;
  int shift_reduce_conflict_count = 0;
  int reduce_reduce_conflict_count = 0;

  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state)
    {
      int q = aut.id (state);

      for (Bundle::iterator a = state->bundle.begin (); a != state->bundle.end (); ++a)
        {
          int symbol = aut.id (a.key ());
          int r = aut.id (a.value ());

          Q_ASSERT (r < state_count);

          if (grammar.isNonTerminal (a.key ()))
            {
              Q_ASSERT(symbol >= terminal_count && symbol < static_cast<int>(grammar.names.size()));
              GOTO (q, symbol - terminal_count) = r;
            }

          else
            ACTION (q, symbol) = r;
        }

      for (ItemPointer item = state->closure.begin (); item != state->closure.end (); ++item)
        {
          if (item->dot != item->end_rhs ())
            continue;

          int r = aut.id (item->rule);

          const NameSet lookaheads = aut.lookaheads.value (item);

          if (item->rule == grammar.goal)
            accept_state = q;

          for (const Name &s : lookaheads)
            {
              int &u = ACTION (q, aut.id (s));

              if (u == 0)
                u = - r;

              else if (u < 0)
                {
                  if (verbose)
                    qout() << "*** Warning. Found a reduce/reduce conflict in state " << q << " on token ``" << s << "'' between rule "
                         << r << " and " << -u << Qt::endl;

                  ++reduce_reduce_conflict_count;

                  u = qMax (u, -r);

                  if (verbose)
                    qout() << "\tresolved using rule " << -u << Qt::endl;
                }

              else if (u > 0)
                {
                  if (item->rule->prec != grammar.names.end() && grammar.token_info.contains (s))
                    {
                      Grammar::TokenInfo info_r = grammar.token_info.value (item->rule->prec);
                      Grammar::TokenInfo info_s = grammar.token_info.value (s);

                      if (info_r.prec > info_s.prec)
                        u = -r;
                      else if (info_r.prec == info_s.prec)
                        {
                          switch (info_r.assoc) {
                          case Grammar::Left:
                            u = -r;
                            break;
                          case Grammar::Right:
                            // shift... nothing to do
                            break;
                          case Grammar::NonAssoc:
                            u = 0;
                            break;
                          } // switch
                        }
                    }

                  else
                    {
                      ++shift_reduce_conflict_count;

                      if (verbose)
                        qout() << "*** Warning. Found a shift/reduce conflict in state " << q << " on token ``" << s << "'' with rule " << r << Qt::endl;
                    }
                }
            }
        }
    }

  if (shift_reduce_conflict_count || reduce_reduce_conflict_count)
    {
      if (shift_reduce_conflict_count != grammar.expected_shift_reduce
          || reduce_reduce_conflict_count != grammar.expected_reduce_reduce)
        qerr() << "*** Conflicts: " << shift_reduce_conflict_count << " shift/reduce, " << reduce_reduce_conflict_count << " reduce/reduce" << Qt::endl;

      if (verbose)
        qout() << Qt::endl << "*** Conflicts: " << shift_reduce_conflict_count << " shift/reduce, " << reduce_reduce_conflict_count << " reduce/reduce" << Qt::endl
             << Qt::endl;
    }

  QBitArray used_rules{static_cast<int>(grammar.rules.size())};

  int q = 0;
  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state, ++q)
    {
      for (int j = 0; j < terminal_count; ++j)
        {
          int &u = ACTION (q, j);

          if (u < 0)
            used_rules.setBit (-u - 1);
        }
    }

  auto rule = grammar.rules.begin();
  for (int i = 0; i < used_rules.count (); ++i, ++rule)
    {
      if (! used_rules.testBit (i))
        {
          if (rule != grammar.goal)
            qerr() << "*** Warning: Rule ``" << *rule << "'' is useless!" << Qt::endl;
        }
    }

  q = 0;
  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state, ++q)
    {
      for (int j = 0; j < terminal_count; ++j)
        {
          int &u = ACTION (q, j);

          if (u >= 0)
            continue;

          RulePointer rule = std::next(grammar.rules.begin(), - u - 1);

          if (state->defaultReduce == rule)
            u = 0;
        }
    }

  // ... compress the goto table
  defgoto.resize (non_terminal_count);
  for (int j = 0; j < non_terminal_count; ++j)
    {
      count.fill (0, state_count);

      int &mx = defgoto [j];

      for (int i = 0; i < state_count; ++i)
        {
          int r = GOTO (i, j);

          if (! r)
            continue;

          ++count [r];

          if (count [r] > count [mx])
            mx = r;
        }
    }

  for (int i = 0; i < state_count; ++i)
    {
      for (int j = 0; j < non_terminal_count; ++j)
        {
          int &r = GOTO (i, j);

          if (r == defgoto [j])
            r = 0;
        }
    }

  compressed_action (table, state_count, terminal_count);
  compressed_goto (pgoto, state_count, non_terminal_count);

  delete[] table;
  table = 0;

  delete[] pgoto;
  pgoto = 0;

#undef ACTION
#undef GOTO

  if (! grammar.merged_output.isEmpty())
    {
      QFile f(grammar.merged_output);
      if (! f.open (QFile::WriteOnly))
        {
          fprintf (stderr, "*** cannot create %s\n", qPrintable(grammar.merged_output));
          return;
        }

      QTextStream out (&f);

      // copyright headers must come first, otherwise the headers tests will fail
      if (copyright)
        {
          out << copyrightHeader()
              << privateCopyrightHeader()
              << Qt::endl;
        }

      out << "// This file was generated by qlalr - DO NOT EDIT!\n";

      out << startIncludeGuard(grammar.merged_output) << Qt::endl;

      if (copyright) {
          out << "#if defined(ERROR)" << Qt::endl
              << "#  undef ERROR" << Qt::endl
              << "#endif" << Qt::endl << Qt::endl;
      }

      generateDecl (out);
      generateImpl (out);
      out << p.decls();
      out << p.impls();
      out << Qt::endl;

      out << endIncludeGuard(grammar.merged_output) << Qt::endl;

      return;
    }

  // default behaviour
  QString declFileName = grammar.table_name.toLower () + QLatin1String("_p.h");
  QString bitsFileName = grammar.table_name.toLower () + QLatin1String(".cpp");

  { // decls...
    QFile f (declFileName);
    f.open (QFile::WriteOnly);
    QTextStream out (&f);

    QString prot = declFileName.toUpper ().replace (QLatin1Char ('.'), QLatin1Char ('_'));

    // copyright headers must come first, otherwise the headers tests will fail
    if (copyright)
      {
        out << copyrightHeader()
            << privateCopyrightHeader()
            << Qt::endl;
      }

    out << "// This file was generated by qlalr - DO NOT EDIT!\n";

    out << "#ifndef " << prot << Qt::endl
        << "#define " << prot << Qt::endl
        << Qt::endl;

    if (copyright) {
        out << "#include <QtCore/qglobal.h>" << Qt::endl << Qt::endl;
        out << "QT_BEGIN_NAMESPACE" << Qt::endl << Qt::endl;
    }
    generateDecl (out);
    if (copyright)
        out << "QT_END_NAMESPACE" << Qt::endl;

    out << "#endif // " << prot << Qt::endl << Qt::endl;
  } // end decls

  { // bits...
    QFile f (bitsFileName);
    f.open (QFile::WriteOnly);
    QTextStream out (&f);

    // copyright headers must come first, otherwise the headers tests will fail
    if (copyright)
      out << copyrightHeader();

    out << "// This file was generated by qlalr - DO NOT EDIT!\n";

    out << "#include \"" << declFileName << "\"" << Qt::endl << Qt::endl;
    if (copyright)
        out << "QT_BEGIN_NAMESPACE" << Qt::endl << Qt::endl;
    generateImpl(out);
    if (copyright)
        out << "QT_END_NAMESPACE" << Qt::endl;

  } // end bits

  if (! grammar.decl_file_name.isEmpty ())
    {
      QFile f (grammar.decl_file_name);
      f.open (QFile::WriteOnly);
      QTextStream out (&f);
      out << p.decls();
    }

  if (! grammar.impl_file_name.isEmpty ())
    {
      QFile f (grammar.impl_file_name);
      f.open (QFile::WriteOnly);
      QTextStream out (&f);
      out << p.impls();
    }
}

QString CppGenerator::debugInfoProt() const
{
    QString prot = QLatin1String("QLALR_NO_");
    prot += grammar.table_name.toUpper();
    prot += QLatin1String("_DEBUG_INFO");
    return prot;
}

void CppGenerator::generateDecl (QTextStream &out)
{
  out << "class " << grammar.table_name << Qt::endl
      << "{" << Qt::endl
      << "public:" << Qt::endl
      << "    enum VariousConstants {" << Qt::endl;

  for (const Name &t : qAsConst(grammar.terminals))
    {
      QString name = *t;
      int value = std::distance (grammar.names.begin (), t);

      if (name == QLatin1String ("$end"))
        name = QLatin1String ("EOF_SYMBOL");

      else if (name == QLatin1String ("$accept"))
        name = QLatin1String ("ACCEPT_SYMBOL");

      else
        name.prepend (grammar.token_prefix);

      out << "        " << name << " = " << value << "," << Qt::endl;
    }

  out << Qt::endl
      << "        ACCEPT_STATE = " << accept_state << "," << Qt::endl
      << "        RULE_COUNT = " << grammar.rules.size () << "," << Qt::endl
      << "        STATE_COUNT = " << state_count << "," << Qt::endl
      << "        TERMINAL_COUNT = " << terminal_count << "," << Qt::endl
      << "        NON_TERMINAL_COUNT = " << non_terminal_count << "," << Qt::endl
      << Qt::endl
      << "        GOTO_INDEX_OFFSET = " << compressed_action.index.size () << "," << Qt::endl
      << "        GOTO_INFO_OFFSET = " << compressed_action.info.size () << "," << Qt::endl
      << "        GOTO_CHECK_OFFSET = " << compressed_action.check.size () << Qt::endl
      << "    };" << Qt::endl
      << Qt::endl
      << "    static const char *const     spell[];" << Qt::endl
      << "    static const short             lhs[];" << Qt::endl
      << "    static const short             rhs[];" << Qt::endl;

  if (debug_info)
    {
      QString prot = debugInfoProt();

      out << Qt::endl << "#ifndef " << prot << Qt::endl
          << "    static const int     rule_index[];" << Qt::endl
          << "    static const int      rule_info[];" << Qt::endl
          << "#endif // " << prot << Qt::endl << Qt::endl;
    }

  out << "    static const short    goto_default[];" << Qt::endl
      << "    static const short  action_default[];" << Qt::endl
      << "    static const short    action_index[];" << Qt::endl
      << "    static const short     action_info[];" << Qt::endl
      << "    static const short    action_check[];" << Qt::endl
      << Qt::endl
      << "    static inline int nt_action (int state, int nt)" << Qt::endl
      << "    {" << Qt::endl
      << "        const int yyn = action_index [GOTO_INDEX_OFFSET + state] + nt;" << Qt::endl
      << "        if (yyn < 0 || action_check [GOTO_CHECK_OFFSET + yyn] != nt)" << Qt::endl
      << "            return goto_default [nt];" << Qt::endl
      << Qt::endl
      << "        return action_info [GOTO_INFO_OFFSET + yyn];" << Qt::endl
      << "    }" << Qt::endl
      << Qt::endl
      << "    static inline int t_action (int state, int token)" << Qt::endl
      << "    {" << Qt::endl
      << "        const int yyn = action_index [state] + token;" << Qt::endl
      << Qt::endl
      << "        if (yyn < 0 || action_check [yyn] != token)" << Qt::endl
      << "            return - action_default [state];" << Qt::endl
      << Qt::endl
      << "        return action_info [yyn];" << Qt::endl
      << "    }" << Qt::endl
      << "};" << Qt::endl
      << Qt::endl
      << Qt::endl;
}

void CppGenerator::generateImpl (QTextStream &out)
{
  int idx = 0;

  out << "const char *const " << grammar.table_name << "::spell [] = {";
  idx = 0;

  QMap<Name, int> name_ids;
  bool first_nt = true;

  for (Name t = grammar.names.begin (); t != grammar.names.end (); ++t, ++idx)
    {
      bool terminal = grammar.isTerminal (t);

      if (! (debug_info || terminal))
        break;

      name_ids.insert (t, idx);

      generateSeparator(idx, out);

      if (terminal)
        {
          QString spell = grammar.spells.value (t);

          if (spell.isEmpty ())
            out << "0";
          else
            out << "\"" << spell << "\"";
        }
      else
        {
          if (first_nt)
            {
              first_nt = false;
              QString prot = debugInfoProt();
              out << Qt::endl << "#ifndef " << prot << Qt::endl;
            }
          out << "\"" << *t << "\"";
        }
    }

  if (debug_info)
    out << Qt::endl << "#endif // " << debugInfoProt() << Qt::endl;

  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  out << "const short " << grammar.table_name << "::lhs [] = {";
  idx = 0;
  for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
    {
      generateSeparator(idx, out);

      out << aut.id (rule->lhs);
    }
  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  out << "const short " << grammar.table_name << "::rhs [] = {";
  idx = 0;
  for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
    {
      generateSeparator(idx, out);

      out << rule->rhs.size ();
    }
  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  if (debug_info)
    {
      QString prot = debugInfoProt();

      out << Qt::endl << "#ifndef " << prot << Qt::endl;
      out << "const int " << grammar.table_name << "::rule_info [] = {";
      idx = 0;
      for (auto rule = grammar.rules.cbegin (); rule != grammar.rules.cend (); ++rule, ++idx)
        {
          generateSeparator(idx, out);

          out << name_ids.value(rule->lhs);

          for (const Name &n : rule->rhs)
            out << ", " << name_ids.value (n);
        }
      out << Qt::endl << "};" << Qt::endl << Qt::endl;

      out << "const int " << grammar.table_name << "::rule_index [] = {";
      idx = 0;
      size_t offset = 0;
      for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
        {
          generateSeparator(idx, out);

          out << offset;
          offset += rule->rhs.size () + 1;
        }
      out << Qt::endl << "};" << Qt::endl
          << "#endif // " << prot << Qt::endl << Qt::endl;
    }

  out << "const short " << grammar.table_name << "::action_default [] = {";
  idx = 0;
  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state, ++idx)
    {
      generateSeparator(idx, out);

      if (state->defaultReduce != grammar.rules.end ())
        out << aut.id (state->defaultReduce);
      else
        out << "0";
    }
  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  out << "const short " << grammar.table_name << "::goto_default [] = {";
  generateList(defgoto, out);
  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  out << "const short " << grammar.table_name << "::action_index [] = {";
  generateList(compressed_action.index, out);
  out << "," << Qt::endl;
  generateList(compressed_goto.index, out);
  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  out << "const short " << grammar.table_name << "::action_info [] = {";
  generateList(compressed_action.info, out);
  out << "," << Qt::endl;
  generateList(compressed_goto.info, out);
  out << Qt::endl << "};" << Qt::endl << Qt::endl;

  out << "const short " << grammar.table_name << "::action_check [] = {";
  generateList(compressed_action.check, out);
  out << "," << Qt::endl;
  generateList(compressed_goto.check, out);
  out << Qt::endl << "};" << Qt::endl << Qt::endl;
}
