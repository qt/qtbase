RWBuffer<uint> uav;
cbuffer ConstantBuffer { uint zero; }

[numthreads(256, 1, 1)]
void killDeviceByTimingOut(uint3 id: SV_DispatchThreadID)
{
    while (zero == 0)
        uav[id.x] = zero;
}
