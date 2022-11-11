// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compress.h"

#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>

#include <algorithm>
#include <iterator>
#include <iostream>

#define QLALR_NO_CHECK_SORTED_TABLE

struct _Fit
{
  inline bool operator () (int a, int b) const
  {
    return a == 0 || b == 0 || a == b;
  }
};

struct _PerfectMatch
{
  inline bool operator () (int a, int b) const
  { return a == b; }
};

struct _GenerateCheck
{
    QList<int>::const_iterator iterator;
    int initial;

    _GenerateCheck(QList<int>::const_iterator it, int i) : iterator(it), initial(i) { }

    inline int operator()()
    {
        int check = initial++;
        return *iterator++ ? check : -1;
  }
};

class UncompressedRow
{
public:
  typedef const int *const_iterator;
  typedef const int *iterator;

public:
  inline UncompressedRow ():
    _M_index (0),
    _M_begin (nullptr),
    _M_end (nullptr),
    _M_beginNonZeros (nullptr),
    _M_endNonZeros (nullptr) {}

  inline UncompressedRow (int index, const_iterator begin, const_iterator end)
  { assign (index, begin, end); }

  inline int index () const { return _M_index; }
  inline const_iterator begin () const { return _M_begin; }
  inline const_iterator end () const { return _M_end; }

  inline void assign (int index, const_iterator begin, const_iterator end)
  {
    _M_index = index;
    _M_begin = begin;
    _M_end = end;

    _M_beginNonZeros = _M_begin;
    _M_endNonZeros = _M_end;

    for (_M_beginNonZeros = _M_begin; _M_beginNonZeros != _M_end && ! _M_beginNonZeros [0]; ++_M_beginNonZeros)
      /*continue*/ ;

#if 0
    for (_M_endNonZeros = _M_end; _M_endNonZeros != _M_beginNonZeros && ! _M_endNonZeros [-1]; --_M_endNonZeros)
      /*continue*/ ;
#endif
  }

  inline int at (int index) const
  { return _M_begin [index]; }

  inline bool isEmpty () const
  { return _M_begin == _M_end; }

  inline int size () const
  { return _M_end - _M_begin; }

  inline int nonZeroElements () const
  { return _M_endNonZeros - _M_beginNonZeros; }

  inline int count (int value) const
  { return std::count (begin (), end (), value); }

  inline const_iterator beginNonZeros () const
  { return _M_beginNonZeros; }

  inline const_iterator endNonZeros () const
  { return _M_endNonZeros; }

private:
  int _M_index;
  const_iterator _M_begin;
  const_iterator _M_end;
  const_iterator _M_beginNonZeros;
  const_iterator _M_endNonZeros;
};

struct _SortUncompressedRow
{
  inline bool operator () (const UncompressedRow &a, const UncompressedRow &b) const
  { return a.count (0) > b.count (0); }
};

Compress::Compress ()
{
}

void Compress::operator () (int *table, int row_count, int column_count)
{
  index.clear ();
  info.clear ();
  check.clear ();

  QList<UncompressedRow> sortedTable(row_count);

  for (int i = 0; i < row_count; ++i)
    {
      int *begin = &table [i * column_count];
      int *end = begin + column_count;

      sortedTable [i].assign (i, begin, end);
    }

  std::sort (sortedTable.begin (), sortedTable.end (), _SortUncompressedRow ());

#ifndef QLALR_NO_CHECK_SORTED_TABLE
  int previous_zeros = INT_MAX;

  for (const UncompressedRow &row : std::as_const(sortedTable))
    {
      int zeros = row.count (0);

      Q_ASSERT (zeros <= previous_zeros);
      zeros = previous_zeros;
      qDebug () << "OK!";
    }
#endif

  index.fill (-999999, row_count);

  for (const UncompressedRow &row : std::as_const(sortedTable))
    {
      int first_token = std::distance (row.begin (), row.beginNonZeros ());
      QList<int>::iterator pos = info.begin();

      while (pos != info.end ())
        {
          if (pos == info.begin ())
            {
              // try to find a perfect match
              QList<int>::iterator pm = std::search(pos, info.end(), row.beginNonZeros(),
                                                    row.endNonZeros(), _PerfectMatch());

              if (pm != info.end ())
                {
                  pos = pm;
                  break;
                }
            }

          pos = std::search (pos, info.end (), row.beginNonZeros (), row.endNonZeros (), _Fit ());

          if (pos == info.end ())
            break;

          int idx = std::distance (info.begin (), pos) - first_token;
          bool conflict = false;

          for (int j = 0; ! conflict && j < row.size (); ++j)
            {
              if (row.at (j) == 0)
                conflict |= idx + j >= 0 && check [idx + j] == j;

              else
                conflict |= check [idx + j] == j;
            }

          if (! conflict)
            break;

          ++pos;
        }

      if (pos == info.end ())
        {
          int size = info.size ();

          info.resize (info.size () + row.nonZeroElements ());
          check.resize (info.size ());

          std::fill (check.begin () + size, check.end (), -1);
          pos = info.begin () + size;
        }

      int offset = std::distance (info.begin (), pos);
      index [row.index ()] = offset - first_token;

      for (const int *it = row.beginNonZeros (); it != row.endNonZeros (); ++it, ++pos)
        {
          if (*it)
            *pos = *it;
        }

      int i = row.index ();

      for (int j = 0; j < row.size (); ++j)
        {
          if (row.at (j) == 0)
            continue;

          check [index [i] + j] = j;
        }
    }

#if 0
  for (const UncompressedRow &row : std::as_const(sortedTable))
    {
      int i = row.index ();
      Q_ASSERT (i < sortedTable.size ());

      for (int j = 0; j < row.size (); ++j)
        {
          if (row.at (j) == 0)
            {
              Q_ASSERT (index [i] + j < 0 || check [index [i] + j] != j);
              continue;
            }

          Q_ASSERT ( info [index [i] + j] == row.at (j));
          Q_ASSERT (check [index [i] + j] == j);
        }
    }
#endif
}
