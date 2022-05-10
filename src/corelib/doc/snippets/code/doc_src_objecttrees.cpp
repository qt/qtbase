// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
int main()
{
    QWidget window;
    QPushButton quit("Quit", &window);
    ...
}
//![0]


//![1]
int main()
{
    QPushButton quit("Quit");
    QWidget window;

    quit.setParent(&window);
    ...
}
//![1]
