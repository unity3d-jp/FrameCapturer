typedef unsigned int8   u8;
typedef unsigned int16  i16;
typedef int32           i32;
typedef int16           f16;
typedef float           f32;

uniform u8 to_u8(uniform u8 v) { return v; }
uniform u8 to_u8(uniform i16 v) { return v & 0xff; }
uniform u8 to_u8(uniform f16 v) { return (int)(half_to_float(v) * 255.0f) & 0xff; }
uniform u8 to_u8(uniform float v) { return (int)(v * 255.0f) & 0xff; }
u8 to_u8(u8 v) { return v; }
u8 to_u8(i16 v) { return v & 0xff; }
u8 to_u8(f16 v) { return (int)(half_to_float(v) * 255.0f) & 0xff; }
u8 to_u8(float v) { return (int)(v * 255.0f) & 0xff; }

uniform i16 to_i16(uniform u8 v) { return v; }
uniform i16 to_i16(uniform i16 v) { return v; }
uniform i16 to_i16(uniform f16 v) { uniform float f = half_to_float(v); return (int)(f * 255.0f) | (signbits(f) >> 16); }
uniform i16 to_i16(uniform float v) { return (int)(v * 255.0f) | (signbits(v) >> 16); }
i16 to_i16(u8 v) { return v; }
i16 to_i16(i16 v) { return v; }
i16 to_i16(f16 v) { float f = half_to_float(v); return (int)(f * 255.0f) | (signbits(f) >> 16); }
i16 to_i16(float v) { return (int)(v * 255.0f) | (signbits(v) >> 16); }

uniform f16 to_f16(uniform u8 v) { return float_to_half((float)((int)v) / 255.0f); }
uniform f16 to_f16(uniform i16 v) { return float_to_half((float)((int)v) / 255.0f); }
uniform f16 to_f16(uniform f16 v) { return v; }
uniform f16 to_f16(uniform float v) { return float_to_half(v); }
f16 to_f16(u8 v) { return float_to_half((float)((int)v) / 255.0f); }
f16 to_f16(i16 v) { return float_to_half((float)((int)v) / 255.0f); }
f16 to_f16(f16 v) { return v; }
f16 to_f16(float v) { return float_to_half(v); }

uniform float to_f32(uniform u8 v) { return (float)((int)v) / 255.0f; }
uniform float to_f32(uniform i16 v) { return (float)v / 255.0f; }
uniform float to_f32(uniform f16 v) { return half_to_float(v); }
uniform float to_f32(uniform float v) { return v; }
float to_f32(u8 v) { return (float)((int)v) / 255.0f; }
float to_f32(i16 v) { return (float)((int)v) / 255.0f; }
float to_f32(f16 v) { return half_to_float(v); }
float to_f32(float v) { return v; }

export void ScaleU8(uniform u8 data[], uniform size_t size, uniform float scale)
{
    foreach(i=0 ... size) {
        data[i] = min((int)((float)((int)data[i]) * scale), 0xff);
    }
}
export void ScaleI16(uniform i16 data[], uniform size_t size, uniform float scale)
{
    foreach(i=0 ... size) {
        float t = (float)((int)data[i]) * scale;
        data[i] = ((int)t & 0x7fff) | (signbits(t) >> 16);
    }
}
export void ScaleI32(uniform i32 data[], uniform size_t size, uniform float scale)
{
    foreach(i=0 ... size) {
        data[i] = (int)((float)data[i] * scale);
    }
}
export void ScaleF16(uniform f16 data[], uniform size_t size, uniform float scale)
{
    foreach(i=0 ... size) {
        data[i] = float_to_half(half_to_float(data[i]) * scale);
    }
}
export void ScaleF32(uniform float data[], uniform size_t size, uniform float scale)
{
    foreach(i=0 ... size) {
        data[i] *= scale;
    }
}

export void U8ToI16(uniform f16 dst[], uniform u8 src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_i16(src[i]); }
}
export void U8ToF16(uniform f16 dst[], uniform u8 src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_f16(src[i]); }
}
export void U8ToF32(uniform float dst[], uniform u8 src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_f32(src[i]); }
}

export void F16ToU8(uniform u8 dst[], uniform f16 src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_u8(src[i]); }
}
export void F16ToI16(uniform u8 dst[], uniform f16 src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_i16(src[i]); }
}
export void F16ToF32(uniform float dst[], uniform f16 src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_f32(src[i]); }
}

export void F32ToU8(uniform u8 dst[], uniform float src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_u8(src[i]); }
}
export void F32ToI16(uniform u8 dst[], uniform float src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_i16(src[i]); }
}
export void F32ToF16(uniform f16 dst[], uniform float src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = to_f16(src[i]); }
}


#define Convert44(C) foreach(j=0 ... size*4) { dst[j] = C(src[j]); }
#define Convert43(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 3) { dst[i*3 + j] = C(src[i*4 + j]); }}
#define Convert42(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 2) { dst[i*2 + j] = C(src[i*4 + j]); }}
#define Convert41(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 1) { dst[i*1 + j] = C(src[i*4 + j]); }}

#define Convert34(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*4 + 0] = C(src[i*3 + 0]);\
        dst[i*4 + 1] = C(src[i*3 + 1]);\
        dst[i*4 + 2] = C(src[i*3 + 2]);\
        dst[i*4 + 3] = C(1.0);\
    }
#define Convert33(C) foreach(j=0 ... size*3) { dst[j] = C(src[j]); }
#define Convert32(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 2) { dst[i*2 + j] = C(src[i*3 + j]); }}
#define Convert31(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 1) { dst[i*1 + j] = C(src[i*3 + j]); }}


#define Convert24(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*4 + 0] = C(src[i*2 + 0]);\
        dst[i*4 + 1] = C(src[i*2 + 1]);\
        dst[i*4 + 2] = C(0.0);\
        dst[i*4 + 3] = C(1.0);\
    }
#define Convert23(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*3 + 0] = C(src[i*2 + 0]);\
        dst[i*3 + 1] = C(src[i*2 + 1]);\
        dst[i*3 + 2] = C(0.0);\
    }
#define Convert22(C) foreach(j=0 ... size*2) { dst[j] = C(src[j]); }
#define Convert21(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 1) { dst[i*1 + j] = C(src[i*2 + j]); }}


#define Convert14(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*4 + 0] = C(src[i]);\
        dst[i*4 + 1] = C(src[i]);\
        dst[i*4 + 2] = C(src[i]);\
        dst[i*4 + 3] = C(1.0);\
    }
#define Convert13(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*3 + 0] = C(src[i]);\
        dst[i*3 + 1] = C(src[i]);\
        dst[i*3 + 2] = C(src[i]);\
    }
#define Convert12(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*2 + 0] = C(src[i]);\
        dst[i*2 + 1] = C(0.0);\
    }
#define Convert11(C) foreach(i=0 ... size) { dst[i] = C(src[i]); }



export void RGBAu8ToRGBu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert43(to_u8) }
export void RGBAu8ToRGu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert42(to_u8) }
export void RGBAu8ToRu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert41(to_u8) }
export void RGBAu8ToRGBAf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert44(to_f16) }
export void RGBAu8ToRGBf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert43(to_f16) }
export void RGBAu8ToRGf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert42(to_f16) }
export void RGBAu8ToRf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert41(to_f16) }
export void RGBAu8ToRGBAf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert44(to_f32) }
export void RGBAu8ToRGBf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert43(to_f32) }
export void RGBAu8ToRGf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert42(to_f32) }
export void RGBAu8ToRf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert41(to_f32) }

export void RGBu8ToRGBAu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert34(to_u8) }
export void RGBu8ToRGu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert32(to_u8) }
export void RGBu8ToRu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert31(to_u8) }
export void RGBu8ToRGBAf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert34(to_f16) }
export void RGBu8ToRGBf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert33(to_f16) }
export void RGBu8ToRGf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert32(to_f16) }
export void RGBu8ToRf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert31(to_f16) }
export void RGBu8ToRGBAf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert34(to_f32) }
export void RGBu8ToRGBf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert33(to_f32) }
export void RGBu8ToRGf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert32(to_f32) }
export void RGBu8ToRf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert31(to_f32) }

export void RGu8ToRGBAu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert24(to_u8) }
export void RGu8ToRGBu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert23(to_u8) }
export void RGu8ToRu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert21(to_u8) }
export void RGu8ToRGBAf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert24(to_f16) }
export void RGu8ToRGBf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert23(to_f16) }
export void RGu8ToRGf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert22(to_f16) }
export void RGu8ToRf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert21(to_f16) }
export void RGu8ToRGBAf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert24(to_f32) }
export void RGu8ToRGBf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert23(to_f32) }
export void RGu8ToRGf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert22(to_f32) }
export void RGu8ToRf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert21(to_f32) }

export void Ru8ToRGBAu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert14(to_u8) }
export void Ru8ToRGBu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert13(to_u8) }
export void Ru8ToRGu8(uniform u8 dst[], uniform u8 src[], uniform size_t size) { Convert12(to_u8) }
export void Ru8ToRGBAf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert14(to_f16) }
export void Ru8ToRGBf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert13(to_f16) }
export void Ru8ToRGf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert12(to_f16) }
export void Ru8ToRf16(uniform f16 dst[], uniform u8 src[], uniform size_t size) { Convert11(to_f16) }
export void Ru8ToRGBAf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert14(to_f32) }
export void Ru8ToRGBf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert13(to_f32) }
export void Ru8ToRGf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert12(to_f32) }
export void Ru8ToRf32(uniform float dst[], uniform u8 src[], uniform size_t size) { Convert11(to_f32) }


export void RGBAf16ToRGBAu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert44(to_u8) }
export void RGBAf16ToRGBu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert43(to_u8) }
export void RGBAf16ToRGu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert42(to_u8) }
export void RGBAf16ToRu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert41(to_u8) }
export void RGBAf16ToRGBAi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert44(to_i16) }
export void RGBAf16ToRGBi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert43(to_i16) }
export void RGBAf16ToRGi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert42(to_i16) }
export void RGBAf16ToRi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert41(to_i16) }
export void RGBAf16ToRGBf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert43(to_f16) }
export void RGBAf16ToRGf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert42(to_f16) }
export void RGBAf16ToRf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert41(to_f16) }
export void RGBAf16ToRGBAf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert44(to_f32) }
export void RGBAf16ToRGBf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert43(to_f32) }
export void RGBAf16ToRGf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert42(to_f32) }
export void RGBAf16ToRf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert41(to_f32) }

export void RGBf16ToRGBAu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert34(to_u8) }
export void RGBf16ToRGBu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert33(to_u8) }
export void RGBf16ToRGu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert32(to_u8) }
export void RGBf16ToRu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert31(to_u8) }
export void RGBf16ToRGBAi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert34(to_i16) }
export void RGBf16ToRGBi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert33(to_i16) }
export void RGBf16ToRGi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert32(to_i16) }
export void RGBf16ToRi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert31(to_i16) }
export void RGBf16ToRGBAf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert34(to_f16) }
export void RGBf16ToRGf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert32(to_f16) }
export void RGBf16ToRf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert31(to_f16) }
export void RGBf16ToRGBAf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert34(to_f32) }
export void RGBf16ToRGBf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert33(to_f32) }
export void RGBf16ToRGf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert32(to_f32) }
export void RGBf16ToRf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert31(to_f32) }

export void RGf16ToRGBAu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert24(to_u8) }
export void RGf16ToRGBu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert23(to_u8) }
export void RGf16ToRGu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert22(to_u8) }
export void RGf16ToRu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert21(to_u8) }
export void RGf16ToRGBAi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert24(to_i16) }
export void RGf16ToRGBi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert23(to_i16) }
export void RGf16ToRGi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert22(to_i16) }
export void RGf16ToRi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert21(to_i16) }
export void RGf16ToRGBAf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert24(to_f16) }
export void RGf16ToRGBf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert23(to_f16) }
export void RGf16ToRf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert21(to_f16) }
export void RGf16ToRGBAf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert24(to_f32) }
export void RGf16ToRGBf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert23(to_f32) }
export void RGf16ToRGf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert22(to_f32) }
export void RGf16ToRf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert21(to_f32) }

export void Rf16ToRGBAu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert14(to_u8) }
export void Rf16ToRGBu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert13(to_u8) }
export void Rf16ToRGu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert12(to_u8) }
export void Rf16ToRu8(uniform u8 dst[], uniform f16 src[], uniform size_t size) { Convert11(to_u8) }
export void Rf16ToRGBAi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert14(to_i16) }
export void Rf16ToRGBi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert13(to_i16) }
export void Rf16ToRGi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert12(to_i16) }
export void Rf16ToRi16(uniform i16 dst[], uniform f16 src[], uniform size_t size) { Convert11(to_i16) }
export void Rf16ToRGBAf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert14(to_f16) }
export void Rf16ToRGBf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert13(to_f16) }
export void Rf16ToRGf16(uniform f16 dst[], uniform f16 src[], uniform size_t size) { Convert12(to_f16) }
export void Rf16ToRGBAf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert14(to_f32) }
export void Rf16ToRGBf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert13(to_f32) }
export void Rf16ToRGf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert12(to_f32) }
export void Rf16ToRf32(uniform float dst[], uniform f16 src[], uniform size_t size) { Convert11(to_f32) }


export void RGBAf32ToRGBAu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert44(to_u8) }
export void RGBAf32ToRGBu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert43(to_u8) }
export void RGBAf32ToRGu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert42(to_u8) }
export void RGBAf32ToRu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert41(to_u8) }
export void RGBAf32ToRGBAi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert44(to_i16) }
export void RGBAf32ToRGBi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert43(to_i16) }
export void RGBAf32ToRGi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert42(to_i16) }
export void RGBAf32ToRi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert41(to_i16) }
export void RGBAf32ToRGBAf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert44(to_f16) }
export void RGBAf32ToRGBf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert43(to_f16) }
export void RGBAf32ToRGf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert42(to_f16) }
export void RGBAf32ToRf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert41(to_f16) }
export void RGBAf32ToRGBf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert43(to_f32) }
export void RGBAf32ToRGf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert42(to_f32) }
export void RGBAf32ToRf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert41(to_f32) }

export void RGBf32ToRGBAu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert34(to_u8) }
export void RGBf32ToRGBu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert33(to_u8) }
export void RGBf32ToRGu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert32(to_u8) }
export void RGBf32ToRu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert31(to_u8) }
export void RGBf32ToRGBAi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert34(to_i16) }
export void RGBf32ToRGBi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert33(to_i16) }
export void RGBf32ToRGi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert32(to_i16) }
export void RGBf32ToRi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert31(to_i16) }
export void RGBf32ToRGBAf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert34(to_f16) }
export void RGBf32ToRGBf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert33(to_f16) }
export void RGBf32ToRGf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert32(to_f16) }
export void RGBf32ToRf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert31(to_f16) }
export void RGBf32ToRGBAf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert34(to_f32) }
export void RGBf32ToRGf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert32(to_f32) }
export void RGBf32ToRf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert31(to_f32) }

export void RGf32ToRGBAu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert24(to_u8) }
export void RGf32ToRGBu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert23(to_u8) }
export void RGf32ToRGu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert22(to_u8) }
export void RGf32ToRu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert21(to_u8) }
export void RGf32ToRGBAi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert24(to_i16) }
export void RGf32ToRGBi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert23(to_i16) }
export void RGf32ToRGi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert22(to_i16) }
export void RGf32ToRi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert21(to_i16) }
export void RGf32ToRGBAf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert24(to_f16) }
export void RGf32ToRGBf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert23(to_f16) }
export void RGf32ToRGf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert22(to_f16) }
export void RGf32ToRf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert21(to_f16) }
export void RGf32ToRGBAf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert24(to_f32) }
export void RGf32ToRGBf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert23(to_f32) }
export void RGf32ToRf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert21(to_f32) }

export void Rf32ToRGBAu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert14(to_u8) }
export void Rf32ToRGBu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert13(to_u8) }
export void Rf32ToRGu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert12(to_u8) }
export void Rf32ToRu8(uniform u8 dst[], uniform float src[], uniform size_t size) { Convert11(to_u8) }
export void Rf32ToRGBAi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert14(to_i16) }
export void Rf32ToRGBi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert13(to_i16) }
export void Rf32ToRGi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert12(to_i16) }
export void Rf32ToRi16(uniform i16 dst[], uniform float src[], uniform size_t size) { Convert11(to_i16) }
export void Rf32ToRGBAf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert14(to_f16) }
export void Rf32ToRGBf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert13(to_f16) }
export void Rf32ToRGf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert12(to_f16) }
export void Rf32ToRf16(uniform f16 dst[], uniform float src[], uniform size_t size) { Convert11(to_f16) }
export void Rf32ToRGBAf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert14(to_f32) }
export void Rf32ToRGBf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert13(to_f32) }
export void Rf32ToRGf32(uniform float dst[], uniform float src[], uniform size_t size) { Convert12(to_f32) }



export void F32ToI8Samples(uniform int8 dst[], uniform const float src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = src[i] * 128.0f; }
}
export void F32ToI16Samples(uniform int16 dst[], uniform const float src[], uniform size_t size)
{
    foreach(i=0 ... size) { dst[i] = src[i] * 32768.0f; }
}
export void F32ToI24Samples(uniform unsigned int8 dst[], uniform const float src[], uniform size_t size)
{
    foreach(i=0 ... size) {
        int32 v = src[i] * 8388608.0f;
        dst[i*3 + 0] = (v & 0x0000FF) >> 0;
        dst[i*3 + 1] = (v & 0x00FF00) >> 8;
        dst[i*3 + 2] = (v & 0xFF0000) >> 16;
    }
}
