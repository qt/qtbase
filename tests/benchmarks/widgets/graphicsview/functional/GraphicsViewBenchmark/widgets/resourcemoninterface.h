// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef __RESOURCEMONINTERFACE_H__
#define __RESOURCEMONINTERFACE_H__

class ResourceMonitorInterface
{
public:
    struct MemoryAllocation
    {
    int allocatedInAppThread;
    int numberOfAllocatedCellsInAppThread;
    int availableMemoryInAppThreadHeap;
    qint64 availableMemoryInSystem;
    qint64 totalMemoryInSystem;
    MemoryAllocation() :
        allocatedInAppThread(0),
        numberOfAllocatedCellsInAppThread(0),
        availableMemoryInAppThreadHeap(0),
        availableMemoryInSystem(0),
        totalMemoryInSystem(0)
        {}
    };

    struct CpuUsage
    {
    qreal systemUsage;
    qreal appTreadUsage;
    CpuUsage() :
        systemUsage(0.0),
        appTreadUsage(0.0)
        {}
    };

public:
    virtual ~ResourceMonitorInterface() {}

public:
    //for symbian, prepares the resource monitor for data capture, opens handle to ekern null
    //thread and sets initial values
    virtual bool Prepare(QString applicationThreadName) = 0;

    //functions for CPU load and memory - Call Prepare before calling these
    virtual CpuUsage CPULoad()=0;
    virtual MemoryAllocation MemoryLoad()=0;

    virtual void BeginMeasureMemoryLoad()=0;
    virtual MemoryAllocation EndMeasureMemoryLoad()=0;

    virtual void BeginMeasureCPULoad()=0;
    virtual CpuUsage EndMeasureCPULoad()=0;

};

Q_DECLARE_INTERFACE(ResourceMonitorInterface,
                     "com.trolltech.Plugin.ResourceMonitorInterface/1.0");

#endif //__RESOURCEMONINTERFACE_H__
