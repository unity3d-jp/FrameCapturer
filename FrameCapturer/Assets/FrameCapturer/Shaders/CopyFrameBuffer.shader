Shader "FrameCapturer/CopyFrameBuffer" {

Properties {
}

CGINCLUDE
#include "UnityCG.cginc"

sampler2D _TmpFrameBuffer;
sampler2D _RenderTarget;

sampler2D _CameraGBufferTexture0;
sampler2D _CameraGBufferTexture1;
sampler2D _CameraGBufferTexture2;
sampler2D _LightBuffer;
sampler2D_float _CameraDepthTexture;
int _InvertY;


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

float2 get_texcoord_iy(v2f i)
{
    float2 t = i.spos.xy * 0.5 + 0.5;
    t.y = 1.0 - t.y;
    return t;
}


half4 copy_framebuffer(v2f i) : SV_Target
{
    half4 r = tex2D(_TmpFrameBuffer, get_texcoord_iy(i));
    r.a = 1.0;
    return r;
}

half4 copy_rendertarget(v2f i) : SV_Target
{
    return tex2D(_RenderTarget, get_texcoord(i));
}

struct gbuffer_out
{
    half4 diffuse           : SV_Target0; // RT0: diffuse color (rgb), occlusion (a)
    half4 spec_smoothness   : SV_Target1; // RT1: spec color (rgb), smoothness (a)
    half4 normal            : SV_Target2; // RT2: normal (rgb), --unused, very low precision-- (a) 
    half4 emission          : SV_Target3; // RT3: emission (rgb), --unused-- (a)
    //float depth             : SV_Target4;
};
gbuffer_out copy_gbuffer(v2f i)
{
    float2 t = get_texcoord(i);
    gbuffer_out o;
    o.diffuse           = tex2D(_CameraGBufferTexture0, t);
    o.spec_smoothness   = tex2D(_CameraGBufferTexture1, t);
    o.normal            = tex2D(_CameraGBufferTexture2, t);
    o.emission          = tex2D(_LightBuffer, t);
    //o.depth             = tex2D(_CameraDepthTexture, t).r;
    return o;
}

float copy_depth(v2f i) : SV_Target
{
    return tex2D(_CameraDepthTexture, get_texcoord(i)).r;
}

ENDCG

Subshader {
    // Pass 0: copy_framebuffer
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_framebuffer
        ENDCG
    }

    // Pass 1: copy_rendertarget
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_rendertarget
        ENDCG
    }

    // Pass 2: copy_gbuffer
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_gbuffer
        ENDCG
    }

    // Pass 3: copy_depth
    Pass {
        Blend Off Cull Off ZTest Off ZWrite Off
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment copy_depth
        ENDCG
    }
}

Fallback off
}
