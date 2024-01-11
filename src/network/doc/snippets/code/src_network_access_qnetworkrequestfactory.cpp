// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
using namespace Qt::StringLiterals;

//! [0]
// Instantiate a factory somewhere suitable in the application
QNetworkRequestFactory api{{"https://example.com/v1"_L1}};

// Set bearer token
api.setBearerToken("my_token");

// Issue requests (reply handling omitted for brevity)
manager.get(api.createRequest("models"_L1)); // https://example.com/v1/models
// The conventional leading '/' for the path can be used as well
manager.get(api.createRequest("/models"_L1)); // https://example.com/v1/models
//! [0]


//! [1]
// Here the API version v2 is used as the base path:
QNetworkRequestFactory api{{"https://example.com/v2"_L1}};
// ...
manager.get(api.createRequest("models"_L1)); // https://example.com/v2/models
// Equivalent with a leading '/'
manager.get(api.createRequest("/models"_L1)); // https://example.com/v2/models
//! [1]

