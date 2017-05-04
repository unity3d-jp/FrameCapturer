#pragma once
#include "IUnityInterface.h"
#ifndef __cplusplus
    #include <stdbool.h>
#endif

typedef struct UnityGraphicsD3D12ResourceState UnityGraphicsD3D12ResourceState;
struct UnityGraphicsD3D12ResourceState
{
    ID3D12Resource*       resource; // Resource to barrier.
    D3D12_RESOURCE_STATES expected; // Expected resource state before this command list is executed.
    D3D12_RESOURCE_STATES current;  // State this resource will be in after this command list is executed.
};

typedef struct UnityGraphicsD3D12PhysicalVideoMemoryControlValues UnityGraphicsD3D12PhysicalVideoMemoryControlValues;
struct UnityGraphicsD3D12PhysicalVideoMemoryControlValues // all values in bytes
{
    UINT64 reservation;           // Minimum required physical memory for an application [default = 64MB].
    UINT64 systemMemoryThreshold; // If free physical video memory drops below this threshold, resources will be allocated in system memory. [default = 64MB]
    UINT64 residencyThreshold;    // Minimum free physical video memory needed to start bringing evicted resources back after shrunken video memory budget expands again. [default = 128MB]
};

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v4)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Executes a given command list on a worker thread.
    // [Optional] Declares expected and post-execution resource states.
    // Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);

    void(UNITY_INTERFACE_API * SetPhysicalVideoMemoryControlValues)(const UnityGraphicsD3D12PhysicalVideoMemoryControlValues * memInfo);

    ID3D12CommandQueue* (UNITY_INTERFACE_API * GetCommandQueue)();
};
UNITY_REGISTER_INTERFACE_GUID(0X498FFCC13EC94006ULL, 0XB18F8B0FF67778C8ULL, IUnityGraphicsD3D12v4)

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v3)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Executes a given command list on a worker thread.
    // [Optional] Declares expected and post-execution resource states.
    // Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);

    void(UNITY_INTERFACE_API * SetPhysicalVideoMemoryControlValues)(const UnityGraphicsD3D12PhysicalVideoMemoryControlValues * memInfo);
};
UNITY_REGISTER_INTERFACE_GUID(0x57C3FAFE59E5E843ULL, 0xBF4F5998474BB600ULL, IUnityGraphicsD3D12v3)

// Should only be used on the rendering/submission thread.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12v2)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Executes a given command list on a worker thread.
    // [Optional] Declares expected and post-execution resource states.
    // Returns the fence value.
    UINT64(UNITY_INTERFACE_API * ExecuteCommandList)(ID3D12GraphicsCommandList * commandList, int stateCount, UnityGraphicsD3D12ResourceState * states);
};
UNITY_REGISTER_INTERFACE_GUID(0xEC39D2F18446C745ULL, 0xB1A2626641D6B11FULL, IUnityGraphicsD3D12v2)


// Obsolete
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D12)
{
    ID3D12Device* (UNITY_INTERFACE_API * GetDevice)();
    ID3D12CommandQueue* (UNITY_INTERFACE_API * GetCommandQueue)();

    ID3D12Fence* (UNITY_INTERFACE_API * GetFrameFence)();
    // Returns the value set on the frame fence once the current frame completes or the GPU is flushed
    UINT64(UNITY_INTERFACE_API * GetNextFrameFenceValue)();

    // Returns the state a resource will be in after the last command list is executed
    bool(UNITY_INTERFACE_API * GetResourceState)(ID3D12Resource * resource, D3D12_RESOURCE_STATES * outState);
    // Specifies the state a resource will be in after a plugin command list with resource barriers is executed
    void(UNITY_INTERFACE_API * SetResourceState)(ID3D12Resource * resource, D3D12_RESOURCE_STATES state);
};
UNITY_REGISTER_INTERFACE_GUID(0xEF4CEC88A45F4C4CULL, 0xBD295B6F2A38D9DEULL, IUnityGraphicsD3D12)
