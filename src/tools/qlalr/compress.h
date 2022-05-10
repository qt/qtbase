// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef COMPRESS_H
#define COMPRESS_H

#include <QtCore/qlist.h>

class Compress
{
public:
  Compress ();

  void operator () (int *table, int row_count, int column_count);

public:
    QList<int> index;
    QList<int> info;
    QList<int> check;
};

#endif // COMPRESS_H
