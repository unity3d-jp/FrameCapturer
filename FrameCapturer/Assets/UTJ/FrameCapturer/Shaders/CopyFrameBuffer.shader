Shader "Hidden/UTJ/FrameCapturer/CopyFrameBuffer" {

CGINCLUDE
#include "UnityCG.cginc"
#pragma multi_compile ___ UNITY_HDR_ON
#pragma multi_compile ___ OFFSCREEN

sampler2D _TmpFrameBuffer;
sampler2D _CameraGBufferTexture0;
sampler2D _CameraGBufferTexture1;
sampler2D _CameraGBufferTexture2;
sampler2D _CameraGBufferTexture3;
sampler2D_float _CameraDepthTexture;
sampler2D_half _CameraMotionVectorsTexture;
sampler2D _TmpRenderTarget;
float4 _ClearColor;

struct v2f {
    float4 pos : POSITION;
    float4 spos : TEXCOORD0;
};

v2f vert(appdata_img v)
{
    v2f o;
    o.pos = o.spos = v.vertex;
    return o;
}


float2 get_texcoord(v2f i)
{
    float2 t = i.spos.xy * 0.5 + 0.5;
    return t;
}

float2 get_texcoord_gb(v2f i)
{
    float2 t = i.spos.xy * 0.5 + 0.5;
#if !defined(UNITY_UV_STARTS_AT_TOP)
    t.y = 1.0 - t.y;
#endif
    return t;
}


// framebuffer
struct framebuffer_out
{
    half4 color         : SV_Target0;
    half4 alpha         : SV_Target1;
};
framebuffer_out copy_framebuffer(v2f I)
{
    float2 t = get_texcoord(I);
#if !defined(OFFSCREEN) || !defined(UNITY_UV_STARTS_AT_TOP)
    t.y = 1.0 - t.y;
#endif
    half4 c = tex2D(_TmpFrameBuffer, t);

    framebuffer_out O;
    O.color = half4(c.rgb, 1.0);
    O.alpha = half4(c.aaa, 1.0);
    return O;
}

// render target (for offscreen-recorder)
half4 copy_rendertarget(v2f I) : SV_Target
{
    half4 O = tex2D(_TmpRenderTarget, get_texcoord_gb(I));
    return O;
}


// gbuffer
struct gbuffer_out
{
    half4 albedo        : SV_Target0;
    half4 occlusion     : SV_Target1;
    half4 specular      : SV_Target2;
    half4 smoothness    : SV_Target3;
    half4 normal        : SV_Target4;
    half4 emission      : SV_Target5;
    half4 depth         : SV_Target6;
};
gbuffer_out copy_gbuffer(v2f I)
{
    float2 t = get_texcoord_gb(I);
    half4 ao = tex2D(_CameraGBufferTexture0, t);
    half4 ss = tex2D(_CameraGBufferTexture1, t);
    half4 normal = tex2D(_CameraGBufferTexture2, t);
    half4 emission = tex2D(_CameraGBufferTexture3, t);
    half depth = tex2D(_CameraDepthTexture, get_texcoord_gb(I));
#if defined(UNITY_REVERSED_Z)
    depth = 1.0 - depth;
#endif

    gbuffer_out O;
    O.albedo = half4(ao.rgb, 1.0);
    O.occlusion = half4(ao.aaa, 1.0);
    O.specular = half4(ss.rgb, 1.0);
    O.smoothness = half4(ss.aaa, 1.0);
    O.normal = half4(normal.rgb, 1.0);
    O.emission = emission;
#ifndef UNITY_HDR_ON
    O.emission.rgb = -log2(O.emission.rgb);
#endif
    O.depth = half4(depth.rrr, 1.0);
    return O;
}


// clear
half4 clear(v2f I) : SV_Target
{
    return _ClearColor;
}

// velocity
half4 copy_velocity(v2f I) : SV_Target
{
    float2 t = get_texcoord_gb(I);
    half2 velocity = tex2D(_CameraMotionVectorsTexture, t).rg;
    return half4(velocity, 1.0, 1.0);
}

ENDCG

Subshader {
    // Pass 0: framebuffer
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_framebuffer
        ENDCG
    }

    // Pass 1: render target
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_rendertarget
        ENDCG
    }

    // Pass 2: gbuffer
    Pass{
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma exclude_renderers d3d9
        #pragma vertex vert
        #pragma fragment copy_gbuffer
        ENDCG
    }

    // Pass 3: clear
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment clear
        ENDCG
    }

    // Pass 4: velocity
    Pass{
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_velocity
        ENDCG
    }
}

Fallback off
}
