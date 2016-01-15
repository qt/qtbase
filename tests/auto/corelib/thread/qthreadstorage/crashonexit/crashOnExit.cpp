/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtCore/QCoreApplication>
#include <QtCore/QThreadStorage>

class Class
{
public:
    ~Class()
    {
        // trigger creation of a new QThreadStorage, after the previous QThreadStorage from main() was destructed
        static QThreadStorage<int *> threadstorage;
        threadstorage.setLocalData(new int);
        threadstorage.setLocalData(new int);
    }
};

int main()
{
    // instantiate the class that will use QThreadStorage from its destructor, it's destructor will be run last
    static Class instance;
    // instantiate QThreadStorage, it's destructor (and the global destructors for QThreadStorages internals) will run first
    static QThreadStorage<int *> threadstorage;
    threadstorage.setLocalData(new int);
    threadstorage.setLocalData(new int);
}
