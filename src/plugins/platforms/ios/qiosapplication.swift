// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import SwiftUI
import CompositorServices
import QIOSIntegrationPlugin
import RealityKit

struct QIOSSwiftApplication: App {
    @UIApplicationDelegateAdaptor private var appDelegate: QIOSApplicationDelegate

    var body: some SwiftUI.Scene {
        WindowGroup() {
            ImmersiveSpaceControlView()
        }

        ImmersiveSpace(id: "QIOSImmersiveSpace") {
            CompositorLayer(configuration: QIOSLayerConfiguration()) { layerRenderer in
                QIOSIntegration.instance().renderCompositorLayer(layerRenderer)
            }
        }
        // CompositorLayer immersive spaces are always full, and should not need
        // to set the immersion style, but lacking this we get a warning in the
        // console about not being able to "configure an immersive space with
        // selected style 'AutomaticImmersionStyle' since it is not in the list
        // of supported styles for this type of content: 'FullImmersionStyle'."
        .immersionStyle(selection: .constant(.full), in: .full)
    }
}

public struct QIOSLayerConfiguration: CompositorLayerConfiguration {
    public func makeConfiguration(capabilities: LayerRenderer.Capabilities,
                                  configuration: inout LayerRenderer.Configuration) {
        // Use reflection to pull out underlying C handles
        // FIXME: Use proper bridging APIs when available
        let capabilitiesMirror = Mirror(reflecting: capabilities)
        let configurationMirror = Mirror(reflecting: configuration)
        QIOSIntegration.instance().configureCompositorLayer(
            capabilitiesMirror.descendant("c_capabilities") as? cp_layer_renderer_capabilities_t,
            configurationMirror.descendant("box", "value") as? cp_layer_renderer_configuration_t
        )
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
