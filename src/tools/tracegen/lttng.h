// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LTTNG_H
#define LTTNG_H

struct Provider;
class QFile;

void writeLttng(QFile &device, const Provider &p);

#endif // LTTNG_H
