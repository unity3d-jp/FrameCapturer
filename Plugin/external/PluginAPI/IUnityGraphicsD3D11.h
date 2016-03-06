#pragma once
#include "IUnityInterface.h"

struct RenderSurfaceBase;
typedef struct RenderSurfaceBase* UnityRenderBuffer;

// Should only be used on the rendering thread unless noted otherwise.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D11)
{
	ID3D11Device* (UNITY_INTERFACE_API * GetDevice)();

	ID3D11Resource* (UNITY_INTERFACE_API * TextureFromRenderBuffer)(UnityRenderBuffer buffer);
};
UNITY_REGISTER_INTERFACE_GUID(0xAAB37EF87A87D748ULL,0xBF76967F07EFB177ULL,IUnityGraphicsD3D11)
