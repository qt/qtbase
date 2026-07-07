// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

// Swift overlay for QtCore. This adds Swifty API on top of the imported C++
// module without modifying any Qt headers, and ships as a textual
// .swiftinterface next to the module's Clang module map. The underlying QtCore
// module is imported implicitly (built with -import-underlying-module).
//
// Bodies are marked @_alwaysEmitIntoClient so they are emitted into the client
// from the textual interface, which means the overlay needs no companion binary
// and can reach the consumer through the framework's -F search path alone.

// The QtCore API notes hide the raw C qVersion() as __qVersion(); expose it as
// an idiomatic qtVersion() returning a Swift String.
@_alwaysEmitIntoClient
public func qtVersion() -> String {
    String(cString: __qVersion())
}
