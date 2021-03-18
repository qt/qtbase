/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
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

#ifndef LALR_H
#define LALR_H

#include <QtCore/qset.h>
#include <QtCore/qstack.h>
#include <QtCore/qmap.h>
#include <QtCore/qlinkedlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qpair.h>

#include <algorithm>
#include <functional>
#include <set>

class Rule;
class State;
class Grammar;
class Item;
class State;
class Arrow;
class Automaton;


// names
typedef std::list<QString>::iterator Name;
typedef std::list<Name> NameList;
typedef std::set<Name> NameSet;

// items
typedef std::list<Item> ItemList;
typedef ItemList::iterator ItemPointer;

// rules
typedef std::list<Rule> debug_infot;
typedef debug_infot::iterator RulePointer;
typedef QMultiMap<Name, RulePointer> RuleMap;

// states
typedef std::list<State> StateList;
typedef StateList::iterator StatePointer;

// arrows
typedef QMap<Name, StatePointer> Bundle;

class Rule
{
public:
  void clear ()
  {
    lhs = Name ();
    rhs.clear ();
    prec = Name ();
  }

public:
  Name lhs;
  NameList rhs;
  Name prec;
};

class Lookback
{
public:
  Lookback (StatePointer s, Name n):
    state (s), nt (n) {}

  inline bool operator == (const Lookback &other) const
  { return state == other.state && nt == other.nt; }

  inline bool operator != (const Lookback &other) const
  { return state != other.state || nt != other.nt; }

  bool operator < (const Lookback &other) const;

public:
  StatePointer state;
  Name nt;
};

class Item
{
public:
  inline NameList::iterator begin_rhs () const
  { return rule->rhs.begin (); }

  inline NameList::iterator end_rhs () const
  { return rule->rhs.end (); }

  inline bool operator == (const Item &other) const
  { return rule == other.rule && dot == other.dot; }

  inline bool operator != (const Item &other) const
  { return rule != other.rule || dot != other.dot; }

  inline bool isReduceItem () const
  { return dot == rule->rhs.end (); }

  Item next () const;

public:
  RulePointer rule;
  NameList::iterator dot;
};

class State
{
public:
  State (Grammar *grammar);

  inline bool operator == (const State &other) const
  { return kernel == other.kernel; }

  inline bool operator != (const State &other) const
  { return kernel != other.kernel; }

  QPair<ItemPointer, bool> insert (const Item &item);
  QPair<ItemPointer, bool> insertClosure (const Item &item);

public: // attributes
  ItemList kernel;
  ItemList closure;
  Bundle bundle;
  QMap<Name, NameSet> reads;
  QMap<Name, NameSet> follows;
  RulePointer defaultReduce;
};

/////////////////////////////////////////////////////////////
// digraph
/////////////////////////////////////////////////////////////
template <typename _Tp>
class Node
{
public:
  typedef std::set<Node<_Tp> > Repository;
  typedef typename Repository::iterator iterator;
  typedef typename std::list<iterator>::iterator edge_iterator;

public:
  static iterator get (_Tp data);

  QPair<edge_iterator, bool> insertEdge (iterator other) const;

  inline edge_iterator begin () const
  { return outs.begin (); }

  inline edge_iterator end () const
  { return outs.end (); }

  inline bool operator == (const Node<_Tp> &other) const
  { return data == other.data; }

  inline bool operator != (const Node<_Tp> &other) const
  { return data != other.data; }

  inline bool operator < (const Node<_Tp> &other) const
  { return data < other.data; }

  static inline iterator begin_nodes ()
  { return repository ().begin (); }

  static inline iterator end_nodes ()
  { return repository ().end (); }

  static Repository &repository ()
  {
    static Repository r;
    return r;
  }

public: // attributes
  mutable bool root;
  mutable int dfn;
  mutable _Tp data;
  mutable std::list<iterator> outs;

protected:
  inline Node () {}

  inline Node (_Tp d):
    root (true), dfn (0), data (d) {}
};

template <typename _Tp>
typename Node<_Tp>::iterator Node<_Tp>::get (_Tp data)
{
  Node<_Tp> tmp (data);
  iterator it = repository ().find (tmp);

  if (it != repository ().end ())
    return it;

  return repository ().insert (tmp).first;
}

template <typename _Tp>
QPair<typename std::list<typename Node<_Tp>::iterator>::iterator, bool> Node<_Tp>::insertEdge(typename Node<_Tp>::iterator other) const
{
  edge_iterator it = std::find (outs.begin (), outs.end (), other);

  if (it != outs.end ())
    return qMakePair (it, false);

  other->root = false;
  return qMakePair (outs.insert (outs.end (), other), true);
}

/////////////////////////////////////////////////////////////
// Grammar
/////////////////////////////////////////////////////////////
class Grammar
{
public:
  Grammar ();

  Name intern (const QString &id);
  Name intern (const char *id) { return intern(QString::fromUtf8(id)); }

  inline bool isTerminal (Name name) const
  { return terminals.find (name) != terminals.end (); }

  inline bool isNonTerminal (Name name) const
  { return non_terminals.find (name) != non_terminals.end (); }

  void buildRuleMap ();
  void buildExtendedGrammar ();

public:
  QString merged_output;
  QString table_name;
  QString decl_file_name;
  QString impl_file_name;
  QString token_prefix;
  std::list<QString> names;
  Name start;
  NameSet terminals;
  NameSet non_terminals;
  QMap<Name, QString> spells;
  debug_infot rules;
  RuleMap rule_map;
  RulePointer goal;
  Name tk_end;
  Name accept_symbol;
  NameSet declared_lhs;
  int expected_shift_reduce;
  int expected_reduce_reduce;

  enum Assoc {
    NonAssoc,
    Left,
    Right
  };

  struct TokenInfo {
    Assoc assoc;
    int prec;
  };

  QMap<Name, TokenInfo> token_info;
  Assoc current_assoc;
  int current_prec;
};

class Read
{
public:
  inline Read () {}

  inline Read (StatePointer s, Name n):
    state (s), nt (n) {}

  inline bool operator == (const Read &other) const
  { return state == other.state && nt == other.nt; }

  inline bool operator != (const Read &other) const
  { return state != other.state || nt != other.nt; }

  bool operator < (const Read &other) const;

public:
  StatePointer state;
  Name nt;
};

class Include
{
public:
  inline Include () {}

  inline Include (StatePointer s, Name n):
    state (s), nt (n) {}

  inline bool operator == (const Include &other) const
  { return state == other.state && nt == other.nt; }

  inline bool operator != (const Include &other) const
  { return state != other.state || nt != other.nt; }

  bool operator < (const Include &other) const;

public:
  StatePointer state;
  Name nt;
};

class Automaton
{
public:
  Automaton (Grammar *g);

  QPair<StatePointer, bool> internState (const State &state);

  typedef Node<Read> ReadsGraph;
  typedef ReadsGraph::iterator ReadNode;

  typedef Node<Include> IncludesGraph;
  typedef IncludesGraph::iterator IncludeNode;

  void build ();
  void buildNullables ();

  void buildLookbackSets ();

  void buildDirectReads ();
  void buildReadsDigraph ();
  void buildReads ();
  void visitReadNode (ReadNode node);

  void buildIncludesAndFollows ();
  void buildIncludesDigraph ();
  void visitIncludeNode (IncludeNode node);

  void buildLookaheads ();

  void buildDefaultReduceActions ();

  void closure (StatePointer state);

  int id (RulePointer rule);
  int id (StatePointer state);
  int id (Name name);

  void dump (QTextStream &out, IncludeNode incl);
  void dump (QTextStream &out, ReadNode rd);
  void dump (QTextStream &out, const Lookback &lp);

public: // ### private
  Grammar *_M_grammar;
  StateList states;
  StatePointer start;
  NameSet nullables;
  QMultiMap<ItemPointer, Lookback> lookbacks;
  QMap<ItemPointer, NameSet> lookaheads;

private:
  QStack<ReadsGraph::iterator> _M_reads_stack;
  int _M_reads_dfn;

  QStack<IncludesGraph::iterator> _M_includes_stack;
  int _M_includes_dfn;
};

namespace std {
bool operator < (Name a, Name b);
bool operator < (StatePointer a, StatePointer b);
bool operator < (ItemPointer a, ItemPointer b);
}

QTextStream &operator << (QTextStream &out, const Name &n);
QTextStream &operator << (QTextStream &out, const Rule &r);
QTextStream &operator << (QTextStream &out, const Item &item);
QTextStream &operator << (QTextStream &out, const NameSet &ns);

QT_BEGIN_NAMESPACE
QTextStream &qerr();
QTextStream &qout();
QT_END_NAMESPACE

#endif // LALR_H
