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
// One variant for non-indexed draws, and two for indexed draws, differing only
// in the index buffer type.
//
// The number of commands to encode is min(maxDrawCount, countBuf[0]). Callers
// that have no device-side count pass a buffer holding 0xFFFFFFFF. The count is
// also written out as an MTLIndirectCommandBufferExecutionRange, so that
// executeCommandsInBuffer:indirectBuffer:indirectBufferOffset: only walks the
// commands that were actually encoded.

static const char s_icbEncodeMsl[] = R"(
#include <metal_stdlib>
using namespace metal;

struct DrawCmd {
    uint  vertexCount;
    uint  instanceCount;
    uint  firstVertex;
    uint  firstInstance;
};

struct DrawIndexedCmd {
    uint  indexCount;
    uint  instanceCount;
    uint  firstIndex;
    int   baseVertex;
    uint  baseInstance;
};

struct ICBContainer {
    command_buffer cmdBuffer [[id(0)]];
};

// Matches MTLIndirectCommandBufferExecutionRange.
struct ExecutionRange {
    uint location;
    uint length;
};

static uint prologue(uint gid, uint maxDrawCount, const device uint *countBuf,
                     device ExecutionRange *range)
{
    const uint count = min(maxDrawCount, countBuf[0]);
    if (gid == 0) {
        range->location = 0u;
        range->length = count;
    }
    return count;
}

template <typename IndexT>
static void encodeIndexed(const device char *argsRaw,
                          device ICBContainer *container,
                          uint maxDrawCount,
                          const device IndexT *indexBuffer,
                          uint primType,
                          uint stride,
                          const device uint *countBuf,
                          device ExecutionRange *range,
                          uint gid)
{
    const uint count = prologue(gid, maxDrawCount, countBuf, range);
    if (gid >= maxDrawCount)
        return;

    const device DrawIndexedCmd &a = *(const device DrawIndexedCmd *)(argsRaw + gid * stride);
    render_command cmd(container->cmdBuffer, gid);
    if (gid < count && a.indexCount > 0u) {
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

kernel void encode_icb(
    const device char *argsRaw         [[buffer(0)]],
    device ICBContainer *container     [[buffer(1)]],
    constant uint &maxDrawCount        [[buffer(2)]],
    constant uint &primType            [[buffer(4)]],
    constant uint &stride              [[buffer(5)]],
    const device uint *countBuf        [[buffer(6)]],
    device ExecutionRange *range       [[buffer(7)]],
    uint gid [[thread_position_in_grid]])
{
    const uint count = prologue(gid, maxDrawCount, countBuf, range);
    if (gid >= maxDrawCount)
        return;

    const device DrawCmd &a = *(const device DrawCmd *)(argsRaw + gid * stride);
    render_command cmd(container->cmdBuffer, gid);
    if (gid < count && a.vertexCount > 0u) {
        cmd.draw_primitives(
            static_cast<primitive_type>(primType),
            a.firstVertex,
            a.vertexCount,
            a.instanceCount,
            a.firstInstance
        );
    } else {
        cmd.reset();
    }
}

kernel void encode_icb_indexed_u32(
    const device char *argsRaw         [[buffer(0)]],
    device ICBContainer *container     [[buffer(1)]],
    constant uint &maxDrawCount        [[buffer(2)]],
    const device uint *indexBuffer     [[buffer(3)]],
    constant uint &primType            [[buffer(4)]],
    constant uint &stride              [[buffer(5)]],
    const device uint *countBuf        [[buffer(6)]],
    device ExecutionRange *range       [[buffer(7)]],
    uint gid [[thread_position_in_grid]])
{
    encodeIndexed(argsRaw, container, maxDrawCount, indexBuffer, primType, stride,
                  countBuf, range, gid);
}

kernel void encode_icb_indexed_u16(
    const device char *argsRaw         [[buffer(0)]],
    device ICBContainer *container     [[buffer(1)]],
    constant uint &maxDrawCount        [[buffer(2)]],
    const device ushort *indexBuffer   [[buffer(3)]],
    constant uint &primType            [[buffer(4)]],
    constant uint &stride              [[buffer(5)]],
    const device uint *countBuf        [[buffer(6)]],
    device ExecutionRange *range       [[buffer(7)]],
    uint gid [[thread_position_in_grid]])
{
    encodeIndexed(argsRaw, container, maxDrawCount, indexBuffer, primType, stride,
                  countBuf, range, gid);
}
)";

#endif // QRHIMETAL_ICB_P_H
