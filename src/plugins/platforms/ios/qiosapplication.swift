// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import SwiftUI
import CompositorServices
import QIOSIntegrationPlugin
import RealityKit

struct QIOSSwiftApplication: App {
    @UIApplicationDelegateAdaptor private var appDelegate: QIOSApplicationDelegate

    @State private var immersionStyle:ImmersionStyle = .automatic

    var body: some SwiftUI.Scene {
        WindowGroup() {
            ImmersiveSpaceControlView()
        }

        ImmersiveSpace(id: "QIOSImmersiveSpace") {
            CompositorLayer(configuration: QIOSLayerConfiguration()) { layerRenderer in
                QIOSIntegration.instance().renderCompositorLayer(layerRenderer)

                // Handle any events in the scene.
                layerRenderer.onSpatialEvent = { eventCollection in
                    QIOSIntegration.instance().handleSpatialEvents(jsonStringFromEventCollection(eventCollection))
                }
            }
        }
        .immersionStyle(selection: .constant(immersionStyle), in: immersionStyle)
    }
}

public struct QIOSLayerConfiguration: CompositorLayerConfiguration {
    public func makeConfiguration(capabilities: LayerRenderer.Capabilities,
                                  configuration: inout LayerRenderer.Configuration) {
        if #available(visionOS 2, *) {
            QIOSIntegration.instance().configureCompositorLayer(
                capabilities as cp_layer_renderer_capabilities_t,
                configuration as cp_layer_renderer_configuration_t)
        } else {
            // Use reflection to pull out underlying C handles
            let capabilitiesMirror = Mirror(reflecting: capabilities)
            let configurationMirror = Mirror(reflecting: configuration)
            QIOSIntegration.instance().configureCompositorLayer(
                capabilitiesMirror.descendant("c_capabilities") as? cp_layer_renderer_capabilities_t,
                configurationMirror.descendant("box", "value") as? cp_layer_renderer_configuration_t
            )
        }
    }
}

public func runSwiftAppMain() {
    QIOSSwiftApplication.main()
}

public class ImmersiveState: ObservableObject {
    static let shared = ImmersiveState()
    @Published var showImmersiveSpace: Bool = false
}

struct ImmersiveSpaceControlView: View {
    @ObservedObject private var immersiveState = ImmersiveState.shared

    @Environment(\.openImmersiveSpace) var openImmersiveSpace
    @Environment(\.dismissImmersiveSpace) var dismissImmersiveSpace

    var body: some View {
        VStack {}
        .onChange(of: immersiveState.showImmersiveSpace) { _, newValue in
            Task {
                if newValue {
                    await openImmersiveSpace(id: "QIOSImmersiveSpace")
                } else {
                    await dismissImmersiveSpace()
                }
            }
        }
    }
}

public class ImmersiveSpaceManager : NSObject {
    @objc public static func openImmersiveSpace() {
        ImmersiveState.shared.showImmersiveSpace = true
    }

    @objc public static func dismissImmersiveSpace() {
        ImmersiveState.shared.showImmersiveSpace = false
    }
}

extension SpatialEventCollection.Event.Kind: Encodable {
    enum CodingKeys: String, CodingKey {
        case touch
        case directPinch
        case indirectPinch
        case pointer
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        switch self {
        case .touch:
            try container.encode("touch")
        case .directPinch:
            try container.encode("directPinch")
        case .indirectPinch:
            try container.encode("indirectPinch")
        case .pointer:
            try container.encode("pointer")
        @unknown default:
            try container.encode("unknown")
        }
    }
}
extension SpatialEventCollection.Event.Phase: Encodable {
    enum CodingKeys: String, CodingKey {
        case active
        case ending
        case cancled
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        switch self {
        case .active:
            try container.encode("active")
        case .ended:
            try container.encode("ended")
        case .cancelled:
            try container.encode("canceled")
        @unknown default:
            try container.encode("unknown")
        }
    }
}
extension SpatialEventCollection.Event.InputDevicePose: Encodable {
    enum CodingKeys: String, CodingKey {
        case altitude
        case azimuth
        case pose3D
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(altitude.radians, forKey: .altitude)
        try container.encode(azimuth.radians, forKey: .azimuth)
        try container.encode(pose3D, forKey: .pose3D)
    }
}

extension SpatialEventCollection.Event: Encodable {
    enum CodingKeys: String, CodingKey {
        case id
        case timestamp
        case kind
        case location
        case phase
        case modifierKeys
        case inputDevicePose
        case location3D
        case selectionRay
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(id.hashValue, forKey: .id)
        try container.encode(timestamp, forKey: .timestamp)
        try container.encode(kind, forKey: .kind)
        try container.encode(location, forKey: .location)
        try container.encode(phase, forKey: .phase)
        try container.encode(modifierKeys.rawValue, forKey: .modifierKeys)
        try container.encode(inputDevicePose, forKey: .inputDevicePose)
        try container.encode(location3D, forKey: .location3D)
        try container.encode(selectionRay, forKey: .selectionRay)
    }
}

extension SpatialEventCollection: Encodable {
    enum CodingKeys: String, CodingKey {
        case events
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(Array(self), forKey: .events)
    }
}

func jsonStringFromEventCollection(_ eventCollection: SpatialEventCollection) -> String {
    let encoder = JSONEncoder()
    encoder.dateEncodingStrategy = .iso8601

    do {
        let jsonData = try encoder.encode(eventCollection)
        return String(data: jsonData, encoding: .utf8) ?? "{}"
    } catch {
        print("Failed to encode event collection: \(error)")
        return "{}"
    }
}
