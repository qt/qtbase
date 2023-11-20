// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QRestReply *reply = manager->get(request);
QObject::connect(reply, &QRestReply::finished, this, &MyClass::handleFinished);
//! [0]


//! [1]
// With lambda
manager->get(request, this, [this](QRestReply *reply) {
    if (reply->isSuccess()) {
        // ...
    }
});
// With member function
manager->get(request, this, &MyClass::handleFinished);
//! [1]


//! [2]
QJsonObject myJson;
// ...
manager->post(request, myJson, this, [this](QRestReply *reply) {
    if (!reply->isSuccess()) {
        // ...
    }
    if (std::optional<QJsonObject> json = reply->json()) {
        // use *json
    }
});
//! [2]


//! [3]
manager->get(request, this, [this](QRestReply *reply) {
    if (!reply->isSuccess())
        // handle error
    if (std::optional<QJsonObject> json = reply->json())
        // use *json
});
//! [3]


//! [4]
manager->get(request, myData, this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [4]


//! [5]
manager->post(request, myData, this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [5]


//! [6]
manager->put(request, myData, this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [6]


//! [7]
manager->head(request, this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [7]


//! [8]
manager->deleteResource(request, this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [8]


//! [9]
manager->sendCustomRequest(request, "MYMETHOD",  myData,  this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [9]


//! [10]
manager->patch(request, myData, this, [this](QRestReply *reply) {
    if (reply->isSuccess())
        // ...
});
//! [10]
