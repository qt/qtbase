// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// This file is not part of TinyCBOR; it was added for Qt's benefit.
#ifndef CBOR_API
#  ifdef QT_BUILD_CORE_LIB
#    define CBOR_API [[maybe_unused]] static inline
#  else
#    define CBOR_API
#  endif
#endif
