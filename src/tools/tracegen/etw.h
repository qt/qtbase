// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef ETW_H
#define ETW_H

struct Provider;
class QFile;

void writeEtw(QFile &device, const Provider &p);

#endif // ETW_H
