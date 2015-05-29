Shader "FrameCapturer/CopyGBuffer" {

Properties {
}

CGINCLUDE
#include "UnityCG.cginc"

sampler2D _CameraGBufferTexture0;
sampler2D _CameraGBufferTexture1;
sampler2D _CameraGBufferTexture2;
sampler2D _LightBuffer;

struct v2f {
    float4 pos : POSITION;
    float4 spos : TEXCOORD0;
};    
    
v2f vert (appdata_img v)
{
    v2f o;
    o.pos = o.spos = v.vertex;
    return o;
}

struct ps_out
{
    half4 diffuse           : SV_Target0; // RT0: diffuse color (rgb), occlusion (a)
    half4 spec_smoothness   : SV_Target1; // RT1: spec color (rgb), smoothness (a)
    half4 normal            : SV_Target2; // RT2: normal (rgb), --unused, very low precision-- (a) 
    half4 emission          : SV_Target3; // RT3: emission (rgb), --unused-- (a)
};

ps_out frag (v2f i)
{
    float2 t = i.spos.xy * 0.5 + 0.5;
#if UNITY_UV_STARTS_AT_TOP
    t.y = 1.0 - t.y;
#endif

    ps_out pso;
    pso.diffuse         = tex2D(_CameraGBufferTexture0, t);
    pso.spec_smoothness = tex2D(_CameraGBufferTexture1, t);
    pso.normal          = tex2D(_CameraGBufferTexture2, t);
    pso.emission        = tex2D(_LightBuffer, t);
    return pso;
}
ENDCG
    
Subshader {
    Tags { "Queue" = "Overlay" }
    Pass {
        Blend Off
        Cull Off
        ZTest Off ZWrite Off
        Fog { Mode off }

        CGPROGRAM
        #pragma vertex vert
        #pragma fragment frag
        ENDCG
    }
}

Fallback off
}
