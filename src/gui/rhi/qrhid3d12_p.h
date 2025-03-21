// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHID3D12_P_H
#define QRHID3D12_P_H

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

#include "qrhi_p.h"
#include <QWindow>
#include <QBitArray>

#include <optional>
#include <array>

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <dcomp.h>

#include "D3D12MemAlloc.h"

// ID3D12Device2 and ID3D12GraphicsCommandList1 and types and enums introduced
// with those are hard requirements now. These should be declared in any
// moderately recent d3d12.h, but if it is an SDK from before Windows 10
// version 1703 then these types could be missing. In the absence of other
// options, handle this by skipping all the code and making QRhi::create() fail
// in such builds.
#ifdef __ID3D12Device2_INTERFACE_DEFINED__
#define QRHI_D3D12_AVAILABLE

// Will use ID3D12GraphicsCommandList5 as long as the d3d12.h is new enough.
// Otherwise, some features (VRS) will not be available.
#ifdef __ID3D12GraphicsCommandList5_INTERFACE_DEFINED__
#define QRHI_D3D12_CL5_AVAILABLE
using D3D12GraphicsCommandList = ID3D12GraphicsCommandList5;
#else
using D3D12GraphicsCommandList = ID3D12GraphicsCommandList1;
#endif

QT_BEGIN_NAMESPACE

static const int QD3D12_FRAMES_IN_FLIGHT = 2;

class QRhiD3D12;

struct QD3D12Descriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};

    bool isValid() const { return cpuHandle.ptr != 0; }
};

struct QD3D12ReleaseQueue;

struct QD3D12DescriptorHeap
{
    bool isValid() const { return heap && capacity; }
    bool create(ID3D12Device *device,
                quint32 descriptorCount,
                D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags);
    void createWithExisting(const QD3D12DescriptorHeap &other,
                            quint32 offsetInDescriptors,
                            quint32 descriptorCount);
    void destroy();
    void destroyWithDeferredRelease(QD3D12ReleaseQueue *releaseQueue);

    QD3D12Descriptor get(quint32 count);
    QD3D12Descriptor at(quint32 index) const;
    quint32 remainingCapacity() const { return capacity - head; }

    QD3D12Descriptor incremented(const QD3D12Descriptor &descriptor, quint32 offsetInDescriptors) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptor.cpuHandle;
        cpuHandle.ptr += offsetInDescriptors * descriptorByteSize;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptor.gpuHandle;
        if (gpuHandle.ptr)
            gpuHandle.ptr += offsetInDescriptors * descriptorByteSize;
        return { cpuHandle, gpuHandle };
    }

    ID3D12DescriptorHeap *heap = nullptr;
    quint32 capacity = 0;
    QD3D12Descriptor heapStart;
    quint32 head = 0;
    quint32 descriptorByteSize = 0;
    D3D12_DESCRIPTOR_HEAP_TYPE heapType;
    D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags;
};

struct QD3D12CpuDescriptorPool
{
    bool isValid() const { return !heaps.isEmpty(); }
    bool create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, const char *debugName = "");
    void destroy();

    QD3D12Descriptor allocate(quint32 count);
    void release(const QD3D12Descriptor &descriptor, quint32 count);

    static const int DESCRIPTORS_PER_HEAP = 256;

    struct HeapWithMap {
        QD3D12DescriptorHeap heap;
        QBitArray map;
        static HeapWithMap init(const QD3D12DescriptorHeap &heap, quint32 descriptorCount) {
            HeapWithMap result;
            result.heap = heap;
            result.map.resize(descriptorCount);
            return result;
        }
    };

    ID3D12Device *device;
    quint32 descriptorByteSize;
    QVector<HeapWithMap> heaps;
    const char *debugName;
};

struct QD3D12QueryHeap
{
    bool isValid() const { return heap && capacity; }
    bool create(ID3D12Device *device,
                quint32 queryCount,
                D3D12_QUERY_HEAP_TYPE heapType);
    void destroy();

    ID3D12QueryHeap *heap = nullptr;
    quint32 capacity = 0;
};

struct QD3D12StagingArea
{
    static const quint32 ALIGNMENT = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT; // 512 so good enough both for cb and texdata

    struct Allocation {
        quint8 *p = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;
        ID3D12Resource *buffer = nullptr;
        quint32 bufferOffset = 0;
        bool isValid() const { return p != nullptr; }
    };

    bool isValid() const { return allocation && mem.isValid(); }
    bool create(QRhiD3D12 *rhi, quint32 capacity, D3D12_HEAP_TYPE heapType);
    void destroy();
    void destroyWithDeferredRelease(QD3D12ReleaseQueue *releaseQueue);

    Allocation get(quint32 byteSize);

    quint32 remainingCapacity() const
    {
        return capacity - head;
    }

    static quint32 allocSizeForArray(quint32 size, int count = 1)
    {
        return count * ((size + ALIGNMENT - 1) & ~(ALIGNMENT - 1));
    }

    Allocation mem;
    ID3D12Resource *resource =  nullptr;
    D3D12MA::Allocation *allocation = nullptr;
    quint32 head;
    quint32 capacity;
};

struct QD3D12ObjectHandle
{
    quint32 index = 0;
    quint32 generation = 0;

    // the default, null handle is guaranteed to give ObjectPool::isValid() == false
    bool isNull() const { return index == 0 && generation == 0; }
};

inline bool operator==(const QD3D12ObjectHandle &a, const QD3D12ObjectHandle &b) noexcept
{
    return a.index == b.index && a.generation == b.generation;
}

inline bool operator!=(const QD3D12ObjectHandle &a, const QD3D12ObjectHandle &b) noexcept
{
    return !(a == b);
}

template<typename T>
struct QD3D12ObjectPool
{
    void create(const char *debugName = "")
    {
        this->debugName = debugName;
        Q_ASSERT(data.isEmpty());
        data.append(Data()); // index 0 is always invalid
    }

    void destroy() {
        int leakCount = 0; // will nicely destroy everything here, but warn about it if enabled
        for (Data &d : data) {
            if (d.object.has_value()) {
                leakCount += 1;
                d.object->releaseResources();
            }
        }
        data.clear();
#ifndef QT_NO_DEBUG
        // debug builds: just do it always
        static bool leakCheck = true;
#else
        // release builds: opt-in
        static bool leakCheck = qEnvironmentVariableIntValue("QT_RHI_LEAK_CHECK");
#endif
        if (leakCheck) {
            if (leakCount > 0) {
                qWarning("QD3D12ObjectPool::destroy(): Pool %p '%s' had %d unreleased objects",
                         this, debugName, leakCount);
            }
        }
    }

    bool isValid(const QD3D12ObjectHandle &handle) const
    {
        return handle.index > 0
                && handle.index < quint32(data.count())
                && handle.generation > 0
                && handle.generation == data[handle.index].generation
                && data[handle.index].object.has_value();
    }

    T lookup(const QD3D12ObjectHandle &handle) const
    {
        return isValid(handle) ? *data[handle.index].object : T();
    }

    const T *lookupRef(const QD3D12ObjectHandle &handle) const
    {
        return isValid(handle) ? &*data[handle.index].object : nullptr;
    }

    T *lookupRef(const QD3D12ObjectHandle &handle)
    {
        return isValid(handle) ? &*data[handle.index].object : nullptr;
    }

    QD3D12ObjectHandle add(const T &object)
    {
        Q_ASSERT(!data.isEmpty());
        const quint32 count = quint32(data.count());
        quint32 index = 1; // index 0 is always invalid
        for (; index < count; ++index) {
            if (!data[index].object.has_value())
                break;
        }
        if (index < count) {
            data[index].object = object;
            quint32 &generation = data[index].generation;
            generation += 1u;
            return { index, generation };
        } else {
            data.append({ object, 1 });
            return { count, 1 };
        }
    }

    void remove(const QD3D12ObjectHandle &handle)
    {
        if (T *object = lookupRef(handle)) {
            object->releaseResources();
            data[handle.index].object.reset();
        }
    }

    const char *debugName;
    struct Data {
        std::optional<T> object;
        quint32 generation = 0;
    };
    QVector<Data> data;
};

struct QD3D12Resource
{
    ID3D12Resource *resource;
    D3D12_RESOURCE_STATES state;
    D3D12_RESOURCE_DESC desc;
    D3D12MA::Allocation *allocation;
    void *cpuMapPtr;
    enum { UavUsageRead = 0x01, UavUsageWrite = 0x02 };
    int uavUsage;
    bool owns;

    // note that this assumes the allocation (if there is one) and the resource
    // are separately releaseable, see D3D12MemAlloc docs
    static QD3D12ObjectHandle addToPool(QD3D12ObjectPool<QD3D12Resource> *pool,
                                        ID3D12Resource *resource,
                                        D3D12_RESOURCE_STATES state,
                                        D3D12MA::Allocation *allocation = nullptr,
                                        void *cpuMapPtr = nullptr)
    {
        Q_ASSERT(resource);
        return pool->add({ resource, state, resource->GetDesc(), allocation, cpuMapPtr, 0, true });
    }

    // for QRhiTexture::createFrom() where the ID3D12Resource is not owned by us
    static QD3D12ObjectHandle addNonOwningToPool(QD3D12ObjectPool<QD3D12Resource> *pool,
                                                 ID3D12Resource *resource,
                                                 D3D12_RESOURCE_STATES state)
    {
        Q_ASSERT(resource);
        return pool->add({ resource, state, resource->GetDesc(), nullptr, nullptr, 0, false });
    }

    void releaseResources()
    {
        if (owns) {
            // order matters: resource first, then the allocation
            resource->Release();
            if (allocation)
                allocation->Release();
        }
    }
};

struct QD3D12Pipeline
{
    enum Type {
        Graphics,
        Compute
    };
    Type type;
    ID3D12PipelineState *pso;

    static QD3D12ObjectHandle addToPool(QD3D12ObjectPool<QD3D12Pipeline> *pool,
                                        Type type,
                                        ID3D12PipelineState *pso)
    {
        return pool->add({ type, pso });
    }

    void releaseResources()
    {
        pso->Release();
    }
};

struct QD3D12RootSignature
{
    ID3D12RootSignature *rootSig;

    static QD3D12ObjectHandle addToPool(QD3D12ObjectPool<QD3D12RootSignature> *pool,
                                        ID3D12RootSignature *rootSig)
    {
        return pool->add({ rootSig });
    }

    void releaseResources()
    {
        rootSig->Release();
    }
};

struct QD3D12ReleaseQueue
{
    void create(QD3D12ObjectPool<QD3D12Resource> *resourcePool,
                QD3D12ObjectPool<QD3D12Pipeline> *pipelinePool,
                QD3D12ObjectPool<QD3D12RootSignature> *rootSignaturePool)
    {
        this->resourcePool = resourcePool;
        this->pipelinePool = pipelinePool;
        this->rootSignaturePool = rootSignaturePool;
    }

    void deferredReleaseResource(const QD3D12ObjectHandle &handle);
    void deferredReleaseResourceWithViews(const QD3D12ObjectHandle &handle,
                                          QD3D12CpuDescriptorPool *pool,
                                          const QD3D12Descriptor &viewsStart,
                                          int viewCount);
    void deferredReleasePipeline(const QD3D12ObjectHandle &handle);
    void deferredReleaseRootSignature(const QD3D12ObjectHandle &handle);
    void deferredReleaseCallback(std::function<void(void*)> callback, void *userData);
    void deferredReleaseResourceAndAllocation(ID3D12Resource *resource,
                                              D3D12MA::Allocation *allocation);
    void deferredReleaseDescriptorHeap(ID3D12DescriptorHeap *heap);
    void deferredReleaseViews(QD3D12CpuDescriptorPool *pool,
                              const QD3D12Descriptor &viewsStart,
                              int viewCount);

    void activatePendingDeferredReleaseRequests(int frameSlot);
    void executeDeferredReleases(int frameSlot, bool forced = false);
    void releaseAll();

    struct DeferredReleaseEntry {
        enum Type {
            Resource,
            Pipeline,
            RootSignature,
            Callback,
            ResourceAndAllocation,
            DescriptorHeap,
            Views
        };
        Type type = Resource;
        std::optional<int> frameSlotToBeReleasedIn;
        QD3D12ObjectHandle handle;
        QD3D12CpuDescriptorPool *poolForViews = nullptr;
        QD3D12Descriptor viewsStart;
        int viewCount = 0;
        std::function<void(void*)> callback = nullptr;
        void *callbackUserData = nullptr;
        std::pair<ID3D12Resource *, D3D12MA::Allocation *> resourceAndAllocation = {};
        ID3D12DescriptorHeap *descriptorHeap = nullptr;
    };
    QVector<DeferredReleaseEntry> queue;
    QD3D12ObjectPool<QD3D12Resource> *resourcePool = nullptr;
    QD3D12ObjectPool<QD3D12Pipeline> *pipelinePool = nullptr;
    QD3D12ObjectPool<QD3D12RootSignature> *rootSignaturePool = nullptr;
};

struct QD3D12CommandBuffer;

struct QD3D12ResourceBarrierGenerator
{
    static const int PREALLOC = 16;

    void create(QD3D12ObjectPool<QD3D12Resource> *resourcePool)
    {
        this->resourcePool = resourcePool;
    }

    void addTransitionBarrier(const QD3D12ObjectHandle &resourceHandle, D3D12_RESOURCE_STATES stateAfter);
    void enqueueBufferedTransitionBarriers(QD3D12CommandBuffer *cbD);
    void enqueueSubresourceTransitionBarrier(QD3D12CommandBuffer *cbD,
                                             const QD3D12ObjectHandle &resourceHandle,
                                             UINT subresource,
                                             D3D12_RESOURCE_STATES stateBefore,
                                             D3D12_RESOURCE_STATES stateAfter);
    void enqueueUavBarrier(QD3D12CommandBuffer *cbD, const QD3D12ObjectHandle &resourceHandle);

    struct TransitionResourceBarrier {
        QD3D12ObjectHandle resourceHandle;
        D3D12_RESOURCE_STATES stateBefore;
        D3D12_RESOURCE_STATES stateAfter;
    };
    QVarLengthArray<TransitionResourceBarrier, PREALLOC> transitionResourceBarriers;
    QD3D12ObjectPool<QD3D12Resource> *resourcePool = nullptr;
};

struct QD3D12ShaderBytecodeCache
{
    struct Shader {
        Shader() = default;
        Shader(const QByteArray &bytecode, const QShader::NativeResourceBindingMap &rbm)
            : bytecode(bytecode), nativeResourceBindingMap(rbm)
        { }
        QByteArray bytecode;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    };

    QHash<QRhiShaderStage, Shader> data;

    void insertWithCapacityLimit(const QRhiShaderStage &key, const Shader &s);
};

struct QD3D12ShaderVisibleDescriptorHeap
{
    bool create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, quint32 perFrameDescriptorCount);
    void destroy();
    void destroyWithDeferredRelease(QD3D12ReleaseQueue *releaseQueue);

    QD3D12DescriptorHeap heap;
    QD3D12DescriptorHeap perFrameHeapSlice[QD3D12_FRAMES_IN_FLIGHT];
};

// wrap foreign struct so we can legally supply equality operators and qHash:
struct Q_D3D12_SAMPLER_DESC
{
    D3D12_SAMPLER_DESC desc;

    friend bool operator==(const Q_D3D12_SAMPLER_DESC &lhs, const Q_D3D12_SAMPLER_DESC &rhs) noexcept
    {
        return lhs.desc.Filter == rhs.desc.Filter
                && lhs.desc.AddressU == rhs.desc.AddressU
                && lhs.desc.AddressV == rhs.desc.AddressV
                && lhs.desc.AddressW == rhs.desc.AddressW
                && lhs.desc.MipLODBias == rhs.desc.MipLODBias
                && lhs.desc.MaxAnisotropy == rhs.desc.MaxAnisotropy
                && lhs.desc.ComparisonFunc == rhs.desc.ComparisonFunc
                // BorderColor is never used, skip it
                && lhs.desc.MinLOD == rhs.desc.MinLOD
                && lhs.desc.MaxLOD == rhs.desc.MaxLOD;
    }

    friend bool operator!=(const Q_D3D12_SAMPLER_DESC &lhs, const Q_D3D12_SAMPLER_DESC &rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend size_t qHash(const Q_D3D12_SAMPLER_DESC &key, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash;
        seed = hash(seed, key.desc.Filter);
        seed = hash(seed, key.desc.AddressU);
        seed = hash(seed, key.desc.AddressV);
        seed = hash(seed, key.desc.AddressW);
        seed = hash(seed, key.desc.MipLODBias);
        seed = hash(seed, key.desc.MaxAnisotropy);
        seed = hash(seed, key.desc.ComparisonFunc);
        // BorderColor is never used, skip it
        seed = hash(seed, key.desc.MinLOD);
        seed = hash(seed, key.desc.MaxLOD);
        return seed;
    }
};

struct QD3D12SamplerManager
{
    const quint32 MAX_SAMPLERS = 512;

    bool create(ID3D12Device *device);
    void destroy();

    QD3D12Descriptor getShaderVisibleDescriptor(const D3D12_SAMPLER_DESC &desc);

    ID3D12Device *device = nullptr;
    QD3D12ShaderVisibleDescriptorHeap shaderVisibleSamplerHeap;
    QHash<Q_D3D12_SAMPLER_DESC, QD3D12Descriptor> gpuMap;
};

enum QD3D12Stage { VS = 0, HS, DS, GS, PS, CS };

static inline QD3D12Stage qd3d12_stage(QRhiShaderStage::Type type)
{
    switch (type) {
    case QRhiShaderStage::Vertex:
        return VS;
    case QRhiShaderStage::TessellationControl:
        return HS;
    case QRhiShaderStage::TessellationEvaluation:
        return DS;
    case QRhiShaderStage::Geometry:
        return GS;
    case QRhiShaderStage::Fragment:
        return PS;
    case QRhiShaderStage::Compute:
        return CS;
    }
    Q_UNREACHABLE_RETURN(VS);
}

static inline D3D12_SHADER_VISIBILITY qd3d12_stageToVisibility(QD3D12Stage s)
{
    switch (s) {
    case VS:
        return D3D12_SHADER_VISIBILITY_VERTEX;
    case HS:
        return D3D12_SHADER_VISIBILITY_HULL;
    case DS:
        return D3D12_SHADER_VISIBILITY_DOMAIN;
    case GS:
        return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case PS:
        return D3D12_SHADER_VISIBILITY_PIXEL;
    case CS:
        return D3D12_SHADER_VISIBILITY_ALL;
    }
    Q_UNREACHABLE_RETURN(D3D12_SHADER_VISIBILITY_ALL);
}

static inline QRhiShaderResourceBinding::StageFlag qd3d12_stageToSrb(QD3D12Stage s)
{
    switch (s) {
    case VS:
        return QRhiShaderResourceBinding::VertexStage;
    case HS:
        return QRhiShaderResourceBinding::TessellationControlStage;
    case DS:
        return QRhiShaderResourceBinding::TessellationEvaluationStage;
    case GS:
        return QRhiShaderResourceBinding::GeometryStage;
    case PS:
        return QRhiShaderResourceBinding::FragmentStage;
    case CS:
        return QRhiShaderResourceBinding::ComputeStage;
    }
    Q_UNREACHABLE_RETURN(QRhiShaderResourceBinding::VertexStage);
}

struct QD3D12ShaderStageData
{
    bool valid = false; // to allow simple arrays where unused stages are indicated by !valid
    QD3D12Stage stage = VS;
    QShader::NativeResourceBindingMap nativeResourceBindingMap;
};

struct QD3D12ShaderResourceBindings;

struct QD3D12ShaderResourceVisitor
{
    enum StorageOp { Load = 0, Store, LoadStore };

    QD3D12ShaderResourceVisitor(const QD3D12ShaderResourceBindings *srb,
                                const QD3D12ShaderStageData *stageData,
                                int stageCount)
        : srb(srb),
          stageData(stageData),
          stageCount(stageCount)
    {
    }

    std::function<void(QD3D12Stage, const QRhiShaderResourceBinding::Data::UniformBufferData &, int, int)> uniformBuffer = nullptr;
    std::function<void(QD3D12Stage, const QRhiShaderResourceBinding::TextureAndSampler &, int)> texture = nullptr;
    std::function<void(QD3D12Stage, const QRhiShaderResourceBinding::TextureAndSampler &, int)> sampler = nullptr;
    std::function<void(QD3D12Stage, const QRhiShaderResourceBinding::Data::StorageImageData &, StorageOp, int)> storageImage = nullptr;
    std::function<void(QD3D12Stage, const QRhiShaderResourceBinding::Data::StorageBufferData &, StorageOp, int)> storageBuffer = nullptr;

    void visit();

    const QD3D12ShaderResourceBindings *srb;
    const QD3D12ShaderStageData *stageData;
    int stageCount;
};

struct QD3D12Readback
{
    // common
    int frameSlot = -1;
    QRhiReadbackResult *result = nullptr;
    QD3D12StagingArea staging;
    quint32 byteSize = 0;
    // textures
    quint32 bytesPerLine = 0;
    QSize pixelSize;
    QRhiTexture::Format format = QRhiTexture::UnknownFormat;
    quint32 stagingRowPitch = 0;
};

struct QD3D12MipmapGenerator
{
    bool create(QRhiD3D12 *rhiD);
    void destroy();
    void generate(QD3D12CommandBuffer *cbD, const QD3D12ObjectHandle &textureHandle);

    QRhiD3D12 *rhiD;
    QD3D12ObjectHandle rootSigHandle;
    QD3D12ObjectHandle pipelineHandle;
};

struct QD3D12MemoryAllocator
{
    bool create(ID3D12Device *device, IDXGIAdapter1 *adapter);
    void destroy();

    HRESULT createResource(D3D12_HEAP_TYPE heapType,
                           const D3D12_RESOURCE_DESC *resourceDesc,
                           D3D12_RESOURCE_STATES initialState,
                           const D3D12_CLEAR_VALUE *optimizedClearValue,
                           D3D12MA::Allocation **maybeAllocation,
                           REFIID riidResource,
                           void **ppvResource);

    void getBudget(D3D12MA::Budget *localBudget, D3D12MA::Budget *nonLocalBudget);

    bool isUsingD3D12MA() const { return allocator != nullptr; }

    ID3D12Device *device = nullptr;
    D3D12MA::Allocator *allocator = nullptr;
};

struct QD3D12Buffer : public QRhiBuffer
{
    QD3D12Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size);
    ~QD3D12Buffer();

    void destroy() override;
    bool create() override;
    QRhiBuffer::NativeBuffer nativeBuffer() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;
    void endFullDynamicBufferUpdateForCurrentFrame() override;

    void executeHostWritesForFrameSlot(int frameSlot);

    QD3D12ObjectHandle handles[QD3D12_FRAMES_IN_FLIGHT] = {};
    struct HostWrite {
        quint32 offset;
        QRhiBufferData data;
    };
    QVarLengthArray<HostWrite, 16> pendingHostWrites[QD3D12_FRAMES_IN_FLIGHT];
    friend class QRhiD3D12;
    friend struct QD3D12CommandBuffer;
};

struct QD3D12RenderBuffer : public QRhiRenderBuffer
{
    QD3D12RenderBuffer(QRhiImplementation *rhi,
                       Type type,
                       const QSize &pixelSize,
                       int sampleCount,
                       Flags flags,
                       QRhiTexture::Format backingFormatHint);
    ~QD3D12RenderBuffer();
    void destroy() override;
    bool create() override;
    QRhiTexture::Format backingFormat() const override;

    static const DXGI_FORMAT DS_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;

    QD3D12ObjectHandle handle;
    QD3D12Descriptor rtv;
    QD3D12Descriptor dsv;
    DXGI_FORMAT dxgiFormat;
    DXGI_SAMPLE_DESC sampleDesc;
    uint generation = 0;
    friend class QRhiD3D12;
};

struct QD3D12Texture : public QRhiTexture
{
    QD3D12Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                  int arraySize, int sampleCount, Flags flags);
    ~QD3D12Texture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;
    NativeTexture nativeTexture() override;
    void setNativeLayout(int layout) override;

    bool prepareCreate(QSize *adjustedSize = nullptr);
    bool finishCreate();

    QD3D12ObjectHandle handle;
    QD3D12Descriptor srv;
    DXGI_FORMAT dxgiFormat;
    DXGI_FORMAT srvFormat;
    DXGI_FORMAT rtFormat; // RTV/DSV/UAV
    uint mipLevelCount;
    DXGI_SAMPLE_DESC sampleDesc;
    uint generation = 0;
    friend class QRhiD3D12;
    friend struct QD3D12CommandBuffer;
};

struct QD3D12Sampler : public QRhiSampler
{
    QD3D12Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                  AddressMode u, AddressMode v, AddressMode w);
    ~QD3D12Sampler();
    void destroy() override;
    bool create() override;

    QD3D12Descriptor lookupOrCreateShaderVisibleDescriptor();

    D3D12_SAMPLER_DESC desc = {};
    QD3D12Descriptor shaderVisibleDescriptor;
};

struct QD3D12ShadingRateMap : public QRhiShadingRateMap
{
    QD3D12ShadingRateMap(QRhiImplementation *rhi);
    ~QD3D12ShadingRateMap();
    void destroy() override;
    bool createFrom(QRhiTexture *src) override;

    QD3D12ObjectHandle handle; // just copied from the texture
    friend class QRhiD3D12;
};

struct QD3D12RenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QD3D12RenderPassDescriptor(QRhiImplementation *rhi);
    ~QD3D12RenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;
    QVector<quint32> serializedFormat() const override;

    void updateSerializedFormat();

    static const int MAX_COLOR_ATTACHMENTS = 8;
    int colorAttachmentCount = 0;
    bool hasDepthStencil = false;
    int colorFormat[MAX_COLOR_ATTACHMENTS];
    int dsFormat;
    bool hasShadingRateMap = false;
    QVector<quint32> serializedFormatData;
};

struct QD3D12RenderTargetData
{
    QD3D12RenderTargetData(QRhiImplementation *) { }

    QD3D12RenderPassDescriptor *rp = nullptr;
    QSize pixelSize;
    float dpr = 1;
    int sampleCount = 1;
    int colorAttCount = 0;
    int dsAttCount = 0;
    QRhiRenderTargetAttachmentTracker::ResIdList currentResIdList;
    static const int MAX_COLOR_ATTACHMENTS = QD3D12RenderPassDescriptor::MAX_COLOR_ATTACHMENTS;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv[MAX_COLOR_ATTACHMENTS];
    D3D12_CPU_DESCRIPTOR_HANDLE dsv;
};

struct QD3D12SwapChainRenderTarget : public QRhiSwapChainRenderTarget
{
    QD3D12SwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain);
    ~QD3D12SwapChainRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QD3D12RenderTargetData d;
};

struct QD3D12TextureRenderTarget : public QRhiTextureRenderTarget
{
    QD3D12TextureRenderTarget(QRhiImplementation *rhi,
                              const QRhiTextureRenderTargetDescription &desc,
                              Flags flags);
    ~QD3D12TextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QD3D12RenderTargetData d;
    bool ownsRtv[QD3D12RenderTargetData::MAX_COLOR_ATTACHMENTS];
    QD3D12Descriptor rtv[QD3D12RenderTargetData::MAX_COLOR_ATTACHMENTS];
    bool ownsDsv = false;
    QD3D12Descriptor dsv;
    friend class QRhiD3D12;
};

struct QD3D12ShaderResourceBindings : public QRhiShaderResourceBindings
{
    QD3D12ShaderResourceBindings(QRhiImplementation *rhi);
    ~QD3D12ShaderResourceBindings();
    void destroy() override;
    bool create() override;
    void updateResources(UpdateFlags flags) override;

    QD3D12ObjectHandle createRootSignature(const QD3D12ShaderStageData *stageData, int stageCount);

    struct VisitorData {
        QVarLengthArray<D3D12_ROOT_PARAMETER1, 2> cbParams[6];

        D3D12_ROOT_PARAMETER1 srvTables[6] = {};
        QVarLengthArray<D3D12_DESCRIPTOR_RANGE1, 4> srvRanges[6];
        quint32 currentSrvRangeOffset[6] = {};

        QVarLengthArray<D3D12_ROOT_PARAMETER1, 4> samplerTables[6];
        std::array<D3D12_DESCRIPTOR_RANGE1, 16> samplerRanges[6] = {};
        int samplerRangeHeads[6] = {};

        D3D12_ROOT_PARAMETER1 uavTables[6] = {};
        QVarLengthArray<D3D12_DESCRIPTOR_RANGE1, 4> uavRanges[6];
        quint32 currentUavRangeOffset[6] = {};
    } visitorData;


    void visitUniformBuffer(QD3D12Stage s,
                            const QRhiShaderResourceBinding::Data::UniformBufferData &d,
                            int shaderRegister,
                            int binding);
    void visitTexture(QD3D12Stage s,
                      const QRhiShaderResourceBinding::TextureAndSampler &d,
                      int shaderRegister);
    void visitSampler(QD3D12Stage s,
                      const QRhiShaderResourceBinding::TextureAndSampler &d,
                      int shaderRegister);
    void visitStorageBuffer(QD3D12Stage s,
                            const QRhiShaderResourceBinding::Data::StorageBufferData &d,
                            QD3D12ShaderResourceVisitor::StorageOp op,
                            int shaderRegister);
    void visitStorageImage(QD3D12Stage s,
                           const QRhiShaderResourceBinding::Data::StorageImageData &d,
                           QD3D12ShaderResourceVisitor::StorageOp op,
                           int shaderRegister);

    bool hasDynamicOffset = false;
    uint generation = 0;

    friend class QRhiD3D12;
    friend struct QD3D12ShaderResourceVisitor;
};

struct QD3D12GraphicsPipeline : public QRhiGraphicsPipeline
{
    QD3D12GraphicsPipeline(QRhiImplementation *rhi);
    ~QD3D12GraphicsPipeline();
    void destroy() override;
    bool create() override;

    QD3D12ObjectHandle handle;
    QD3D12ObjectHandle rootSigHandle;
    std::array<QD3D12ShaderStageData, 5> stageData;
    D3D12_PRIMITIVE_TOPOLOGY topology;
    UINT viewInstanceMask = 0;
    uint generation = 0;
    friend class QRhiD3D12;
};

struct QD3D12ComputePipeline : public QRhiComputePipeline
{
    QD3D12ComputePipeline(QRhiImplementation *rhi);
    ~QD3D12ComputePipeline();
    void destroy() override;
    bool create() override;

    QD3D12ObjectHandle handle;
    QD3D12ObjectHandle rootSigHandle;
    QD3D12ShaderStageData stageData;
    uint generation = 0;
    friend class QRhiD3D12;
};

struct QD3D12CommandBuffer : public QRhiCommandBuffer
{
    QD3D12CommandBuffer(QRhiImplementation *rhi);
    ~QD3D12CommandBuffer();
    void destroy() override;

    const QRhiNativeHandles *nativeHandles();

    D3D12GraphicsCommandList *cmdList = nullptr; // not owned
    QRhiD3D12CommandBufferNativeHandles nativeHandlesStruct;

    enum PassType {
        NoPass,
        RenderPass,
        ComputePass
    };

    void resetState()
    {
        recordingPass = NoPass;
        currentTarget = nullptr;

        resetPerPassState();
    }

    void resetPerPassState()
    {
        currentGraphicsPipeline = nullptr;
        currentComputePipeline = nullptr;
        currentPipelineGeneration = 0;
        currentGraphicsSrb = nullptr;
        currentComputeSrb = nullptr;
        currentSrbGeneration = 0;
        currentIndexBuffer = {};
        currentIndexOffset = 0;
        currentIndexFormat = DXGI_FORMAT_R16_UINT;
        currentVertexBuffers = {};
        currentVertexOffsets = {};
        hasShadingRateSet = false;
        hasShadingRateMapSet = false;
    }

    // per-frame
    PassType recordingPass;
    QRhiRenderTarget *currentTarget;

    // per-pass
    QD3D12GraphicsPipeline *currentGraphicsPipeline;
    QD3D12ComputePipeline *currentComputePipeline;
    uint currentPipelineGeneration;
    QRhiShaderResourceBindings *currentGraphicsSrb;
    QRhiShaderResourceBindings *currentComputeSrb;
    uint currentSrbGeneration;
    QD3D12ObjectHandle currentIndexBuffer;
    quint32 currentIndexOffset;
    DXGI_FORMAT currentIndexFormat;
    std::array<QD3D12ObjectHandle, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> currentVertexBuffers;
    std::array<quint32, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> currentVertexOffsets;
    bool hasShadingRateSet;
    bool hasShadingRateMapSet;

    // global
    double lastGpuTime = 0;

    // per-setShaderResources
    struct VisitorData {
        QVarLengthArray<std::pair<QD3D12ObjectHandle, quint32>, 4> cbufs[6];
        QVarLengthArray<QD3D12Descriptor, 8> srvs[6];
        QVarLengthArray<QD3D12Descriptor, 8> samplers[6];
        QVarLengthArray<std::pair<QD3D12ObjectHandle, D3D12_UNORDERED_ACCESS_VIEW_DESC>, 4> uavs[6];
    } visitorData;

    void visitUniformBuffer(QD3D12Stage s,
                            const QRhiShaderResourceBinding::Data::UniformBufferData &d,
                            int shaderRegister,
                            int binding,
                            int dynamicOffsetCount,
                            const QRhiCommandBuffer::DynamicOffset *dynamicOffsets);
    void visitTexture(QD3D12Stage s,
                      const QRhiShaderResourceBinding::TextureAndSampler &d,
                      int shaderRegister);
    void visitSampler(QD3D12Stage s,
                      const QRhiShaderResourceBinding::TextureAndSampler &d,
                      int shaderRegister);
    void visitStorageBuffer(QD3D12Stage s,
                            const QRhiShaderResourceBinding::Data::StorageBufferData &d,
                            QD3D12ShaderResourceVisitor::StorageOp op,
                            int shaderRegister);
    void visitStorageImage(QD3D12Stage s,
                           const QRhiShaderResourceBinding::Data::StorageImageData &d,
                           QD3D12ShaderResourceVisitor::StorageOp op,
                           int shaderRegister);
};

struct QD3D12SwapChain : public QRhiSwapChain
{
    QD3D12SwapChain(QRhiImplementation *rhi);
    ~QD3D12SwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;
    QRhiRenderTarget *currentFrameRenderTarget(StereoTargetBuffer targetBuffer) override;

    QSize surfacePixelSize() override;
    bool isFormatSupported(Format f) override;
    QRhiSwapChainHdrInfo hdrInfo() override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool createOrResize() override;

    void releaseBuffers();
    void waitCommandCompletionForFrameSlot(int frameSlot);
    void addCommandCompletionSignalForCurrentFrameSlot();
    void chooseFormats();

    QWindow *window = nullptr;
    IDXGISwapChain1 *sourceSwapChain1 = nullptr;
    IDXGISwapChain3 *swapChain = nullptr;
    QSize pixelSize;
    UINT swapInterval = 1;
    UINT swapChainFlags = 0;
    BOOL stereo = false;
    DXGI_FORMAT colorFormat;
    DXGI_FORMAT srgbAdjustedColorFormat;
    DXGI_COLOR_SPACE_TYPE hdrColorSpace;
    IDCompositionTarget *dcompTarget = nullptr;
    IDCompositionVisual *dcompVisual = nullptr;
    static const UINT BUFFER_COUNT = 3;
    QD3D12ObjectHandle colorBuffers[BUFFER_COUNT];
    QD3D12Descriptor rtvs[BUFFER_COUNT];
    QD3D12Descriptor rtvsRight[BUFFER_COUNT];
    DXGI_SAMPLE_DESC sampleDesc;
    QD3D12ObjectHandle msaaBuffers[BUFFER_COUNT];
    QD3D12Descriptor msaaRtvs[BUFFER_COUNT];
    QD3D12RenderBuffer *ds = nullptr;
    UINT currentBackBufferIndex = 0;
    QD3D12SwapChainRenderTarget rtWrapper;
    QD3D12SwapChainRenderTarget rtWrapperRight;
    QD3D12CommandBuffer cbWrapper;
    HANDLE frameLatencyWaitableObject = nullptr;

    struct FrameResources {
        ID3D12Fence *fence = nullptr;
        HANDLE fenceEvent = nullptr;
        UINT64 fenceCounter = 0;
        D3D12GraphicsCommandList *cmdList = nullptr;
    } frameRes[QD3D12_FRAMES_IN_FLIGHT];

    int currentFrameSlot = 0; // index in frameRes
    int lastFrameLatencyWaitSlot = -1;
};

template<typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
struct alignas(void*) QD3D12PipelineStateSubObject
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = Type;
    T object = {};
};

class QRhiD3D12 : public QRhiImplementation
{
public:
    // 16MB * QD3D12_FRAMES_IN_FLIGHT; buffer and texture upload staging data that
    // gets no space from this will get their own temporary staging areas.
    static const quint32 SMALL_STAGING_AREA_BYTES_PER_FRAME = 16 * 1024 * 1024;

    static const quint32 SHADER_VISIBLE_CBV_SRV_UAV_HEAP_PER_FRAME_START_SIZE = 16384;

    QRhiD3D12(QRhiD3D12InitParams *params, QRhiD3D12NativeHandles *importDevice = nullptr);

    bool create(QRhi::Flags flags) override;
    void destroy() override;

    QRhiGraphicsPipeline *createGraphicsPipeline() override;
    QRhiComputePipeline *createComputePipeline() override;
    QRhiShaderResourceBindings *createShaderResourceBindings() override;
    QRhiBuffer *createBuffer(QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usage,
                             quint32 size) override;
    QRhiRenderBuffer *createRenderBuffer(QRhiRenderBuffer::Type type,
                                         const QSize &pixelSize,
                                         int sampleCount,
                                         QRhiRenderBuffer::Flags flags,
                                         QRhiTexture::Format backingFormatHint) override;
    QRhiTexture *createTexture(QRhiTexture::Format format,
                               const QSize &pixelSize,
                               int depth,
                               int arraySize,
                               int sampleCount,
                               QRhiTexture::Flags flags) override;
    QRhiSampler *createSampler(QRhiSampler::Filter magFilter,
                               QRhiSampler::Filter minFilter,
                               QRhiSampler::Filter mipmapMode,
                               QRhiSampler:: AddressMode u,
                               QRhiSampler::AddressMode v,
                               QRhiSampler::AddressMode w) override;

    QRhiTextureRenderTarget *createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                       QRhiTextureRenderTarget::Flags flags) override;

    QRhiShadingRateMap *createShadingRateMap() override;

    QRhiSwapChain *createSwapChain() override;
    QRhi::FrameOpResult beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags) override;
    QRhi::FrameOpResult endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags) override;
    QRhi::FrameOpResult beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags) override;
    QRhi::FrameOpResult endOffscreenFrame(QRhi::EndFrameFlags flags) override;
    QRhi::FrameOpResult finish() override;

    void resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;

    void beginPass(QRhiCommandBuffer *cb,
                   QRhiRenderTarget *rt,
                   const QColor &colorClearValue,
                   const QRhiDepthStencilClearValue &depthStencilClearValue,
                   QRhiResourceUpdateBatch *resourceUpdates,
                   QRhiCommandBuffer::BeginPassFlags flags) override;
    void endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;

    void setGraphicsPipeline(QRhiCommandBuffer *cb,
                             QRhiGraphicsPipeline *ps) override;

    void setShaderResources(QRhiCommandBuffer *cb,
                            QRhiShaderResourceBindings *srb,
                            int dynamicOffsetCount,
                            const QRhiCommandBuffer::DynamicOffset *dynamicOffsets) override;

    void setVertexInput(QRhiCommandBuffer *cb,
                        int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                        QRhiBuffer *indexBuf, quint32 indexOffset,
                        QRhiCommandBuffer::IndexFormat indexFormat) override;

    void setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport) override;
    void setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor) override;
    void setBlendConstants(QRhiCommandBuffer *cb, const QColor &c) override;
    void setStencilRef(QRhiCommandBuffer *cb, quint32 refValue) override;
    void setShadingRate(QRhiCommandBuffer *cb, const QSize &coarsePixelSize) override;

    void draw(QRhiCommandBuffer *cb, quint32 vertexCount,
              quint32 instanceCount, quint32 firstVertex, quint32 firstInstance) override;

    void drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                     quint32 instanceCount, quint32 firstIndex,
                     qint32 vertexOffset, quint32 firstInstance) override;

    void debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name) override;
    void debugMarkEnd(QRhiCommandBuffer *cb) override;
    void debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg) override;

    void beginComputePass(QRhiCommandBuffer *cb,
                          QRhiResourceUpdateBatch *resourceUpdates,
                          QRhiCommandBuffer::BeginPassFlags flags) override;
    void endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;
    void setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps) override;
    void dispatch(QRhiCommandBuffer *cb, int x, int y, int z) override;

    const QRhiNativeHandles *nativeHandles(QRhiCommandBuffer *cb) override;
    void beginExternal(QRhiCommandBuffer *cb) override;
    void endExternal(QRhiCommandBuffer *cb) override;
    double lastCompletedGpuTime(QRhiCommandBuffer *cb) override;

    QList<int> supportedSampleCounts() const override;
    QList<QSize> supportedShadingRates(int sampleCount) const override;
    int ubufAlignment() const override;
    bool isYUpInFramebuffer() const override;
    bool isYUpInNDC() const override;
    bool isClipDepthZeroToOne() const override;
    QMatrix4x4 clipSpaceCorrMatrix() const override;
    bool isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const override;
    bool isFeatureSupported(QRhi::Feature feature) const override;
    int resourceLimit(QRhi::ResourceLimit limit) const override;
    const QRhiNativeHandles *nativeHandles() override;
    QRhiDriverInfo driverInfo() const override;
    QRhiStats statistics() override;
    bool makeThreadLocalNativeContextCurrent() override;
    void setQueueSubmitParams(QRhiNativeHandles *params) override;
    void releaseCachedResources() override;
    bool isDeviceLost() const override;

    QByteArray pipelineCacheData() override;
    void setPipelineCacheData(const QByteArray &data) override;

    void waitGpu();
    DXGI_SAMPLE_DESC effectiveSampleDesc(int sampleCount, DXGI_FORMAT format) const;
    bool ensureDirectCompositionDevice();
    bool startCommandListForCurrentFrameSlot(D3D12GraphicsCommandList **cmdList);
    void enqueueResourceUpdates(QD3D12CommandBuffer *cbD, QRhiResourceUpdateBatch *resourceUpdates);
    void finishActiveReadbacks(bool forced = false);
    bool ensureShaderVisibleDescriptorHeapCapacity(QD3D12ShaderVisibleDescriptorHeap *h,
                                                   D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                   int frameSlot,
                                                   quint32 neededDescriptorCount,
                                                   bool *gotNew);
    void bindShaderVisibleHeaps(QD3D12CommandBuffer *cbD);

    bool debugLayer = false;
    UINT maxFrameLatency = 2; // 1-3, use 2 to keep CPU-GPU parallelism while reducing lag compared to tripple buffering
    ID3D12Device2 *dev = nullptr;
    D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL(0);
    LUID adapterLuid = {};
    bool importedDevice = false;
    bool importedCommandQueue = false;
    QRhi::Flags rhiFlags;
    IDXGIFactory2 *dxgiFactory = nullptr;
    bool supportsAllowTearing = false;
    IDXGIAdapter1 *activeAdapter = nullptr;
    QRhiDriverInfo driverInfoStruct;
    QRhiD3D12NativeHandles nativeHandlesStruct;
    bool deviceLost = false;
    ID3D12CommandQueue *cmdQueue = nullptr;
    ID3D12Fence *fullFence = nullptr;
    HANDLE fullFenceEvent = nullptr;
    UINT64 fullFenceCounter = 0;
    ID3D12CommandAllocator *cmdAllocators[QD3D12_FRAMES_IN_FLIGHT] = {};
    QD3D12MemoryAllocator vma;
    QD3D12CpuDescriptorPool rtvPool;
    QD3D12CpuDescriptorPool dsvPool;
    QD3D12CpuDescriptorPool cbvSrvUavPool;
    QD3D12ObjectPool<QD3D12Resource> resourcePool;
    QD3D12ObjectPool<QD3D12Pipeline> pipelinePool;
    QD3D12ObjectPool<QD3D12RootSignature> rootSignaturePool;
    QD3D12ReleaseQueue releaseQueue;
    QD3D12ResourceBarrierGenerator barrierGen;
    QD3D12SamplerManager samplerMgr;
    QD3D12MipmapGenerator mipmapGen;
    QD3D12StagingArea smallStagingAreas[QD3D12_FRAMES_IN_FLIGHT];
    QD3D12ShaderVisibleDescriptorHeap shaderVisibleCbvSrvUavHeap;
    UINT64 timestampTicksPerSecond = 0;
    QD3D12QueryHeap timestampQueryHeap;
    QD3D12StagingArea timestampReadbackArea;
    IDCompositionDevice *dcompDevice = nullptr;
    QD3D12SwapChain *currentSwapChain = nullptr;
    QSet<QD3D12SwapChain *> swapchains;
    QD3D12ShaderBytecodeCache shaderBytecodeCache;
    QVarLengthArray<QD3D12Readback, 4> activeReadbacks;
    bool offscreenActive = false;
    QD3D12CommandBuffer *offscreenCb[QD3D12_FRAMES_IN_FLIGHT] = {};
    UINT shadingRateImageTileSize = 0;

    struct {
        bool multiView = false;
        bool textureViewFormat = false;
        bool vrs = false;
        bool vrsMap = false;
        bool vrsAdditionalRates = false;
    } caps;
};

QT_END_NAMESPACE

#endif // __ID3D12Device2_INTERFACE_DEFINED__

#endif
