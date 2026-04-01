// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIMETAL_ICB_P_H
#define QRHIMETAL_ICB_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// Metal Shading Language compute kernels for Indirect Command Buffer encoding.
// Two variants are provided: one for uint32 index buffers and one for uint16.

static const char s_icbEncodeMsl[] = R"(
#include <metal_stdlib>
using namespace metal;

struct DrawCmd {
    uint  indexCount;
    uint  instanceCount;
    uint  firstIndex;
    int   baseVertex;
    uint  baseInstance;
};

struct ICBContainer {
    command_buffer cmdBuffer [[id(0)]];
};

kernel void encode_icb_indexed_u32(
    const device char *argsRaw         [[buffer(0)]],
    device ICBContainer *container     [[buffer(1)]],
    constant uint &drawCount           [[buffer(2)]],
    const device uint *indexBuffer     [[buffer(3)]],
    constant uint &primType            [[buffer(4)]],
    constant uint &stride              [[buffer(5)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= drawCount) return;

    const device DrawCmd &a = *(const device DrawCmd *)(argsRaw + gid * stride);
    render_command cmd(container->cmdBuffer, gid);
    if (a.indexCount > 0u) {
        cmd.draw_indexed_primitives(
            static_cast<primitive_type>(primType),
            a.indexCount,
            indexBuffer + a.firstIndex,
            a.instanceCount,
            uint(a.baseVertex),
            a.baseInstance
        );
    } else {
        cmd.reset();
    }
}

kernel void encode_icb_indexed_u16(
    const device char *argsRaw         [[buffer(0)]],
    device ICBContainer *container     [[buffer(1)]],
    constant uint &drawCount           [[buffer(2)]],
    const device ushort *indexBuffer   [[buffer(3)]],
    constant uint &primType            [[buffer(4)]],
    constant uint &stride              [[buffer(5)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= drawCount) return;

    const device DrawCmd &a = *(const device DrawCmd *)(argsRaw + gid * stride);
    render_command cmd(container->cmdBuffer, gid);
    if (a.indexCount > 0u) {
        cmd.draw_indexed_primitives(
            static_cast<primitive_type>(primType),
            a.indexCount,
            indexBuffer + a.firstIndex,
            a.instanceCount,
            uint(a.baseVertex),
            a.baseInstance
        );
    } else {
        cmd.reset();
    }
}
)";

#endif // QRHIMETAL_ICB_P_H
