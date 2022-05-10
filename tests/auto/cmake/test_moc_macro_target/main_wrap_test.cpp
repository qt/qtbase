// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>

#include "mywrapobject.h"

int main(int argc, char **argv)
{
  MyWrapObject mwo;
  mwo.objectName();
  return 0;
}
