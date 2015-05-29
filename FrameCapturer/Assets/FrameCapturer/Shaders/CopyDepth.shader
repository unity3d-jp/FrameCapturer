Shader "FrameCapturer/CopyDepth" {

Properties {
}

CGINCLUDE
#include "UnityCG.cginc"

sampler2D_float _CameraDepthTexture;

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
    float depth             : SV_Target0;
};

ps_out frag (v2f i)
{
    float2 t = i.spos.xy * 0.5 + 0.5;
#if UNITY_UV_STARTS_AT_TOP
    //t.y = 1.0 - t.y;
#endif

    ps_out pso;
    pso.depth = tex2D(_CameraDepthTexture, t).r;
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
