// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CTF_H
#define CTF_H

struct Provider;
class QFile;

void writeCtf(QFile &device, const Provider &p);

#endif // CTF_H
