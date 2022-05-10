// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    void myfun1(QAnyStringView sv);        // preferred
    void myfun2(const QAnyStringView &sv); // compiles and works, but slower
//! [0]

//! [2]
    auto sv1 = QAnyStringView{std::begin(array), std::end(array) - 1}; // using C++11 std::begin()/std::end()
    auto sv2 = QAnyStringView(array, std::size(array) - 1); // using C++17 std::size()
//! [2]
