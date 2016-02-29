typedef unsigned int8 byte;
typedef int16 half;

uniform byte to_byte(uniform byte v) { return v; }
uniform byte to_byte(uniform half v) { return (int)(half_to_float(v) * 255.0f) & 0xff; }
uniform byte to_byte(uniform float v) { return (int)(v * 255.0f) & 0xff; }
byte to_byte(byte v) { return v; }
byte to_byte(half v) { return (int)(half_to_float(v) * 255.0f) & 0xff; }
byte to_byte(float v) { return (int)(v * 255.0f) & 0xff; }

uniform half to_half(uniform byte v) { return float_to_half((float)((int)v) / 255.0f); }
uniform half to_half(uniform half v) { return v; }
uniform half to_half(uniform float v) { return float_to_half(v); }
half to_half(byte v) { return float_to_half((float)((int)v) / 255.0f); }
half to_half(half v) { return v; }
half to_half(float v) { return float_to_half(v); }

uniform float to_float(uniform byte v) { return (float)((int)v) / 255.0f; }
uniform float to_float(uniform half v) { return half_to_float(v); }
uniform float to_float(uniform float v) { return v; }
float to_float(byte v) { return (float)((int)v) / 255.0f; }
float to_float(half v) { return half_to_float(v); }
float to_float(float v) { return v; }


export void ScaleHalfArray(uniform half data[], uniform int datasize, uniform float scale)
{
    foreach(i=0 ... datasize) {
        data[i] = float_to_half(half_to_float(data[i]) * scale);
    }
}
export void ScaleFloatArray(uniform float data[], uniform int datasize, uniform float scale)
{
    foreach(i=0 ... datasize) {
        data[i] *= scale;
    }
}

export void ByteToHalf(uniform half dst[], uniform byte src[], uniform int size)
{
    foreach(i=0 ... size) {
        dst[i] = to_half(src[i]);
    }
}
export void ByteToFloat(uniform float dst[], uniform byte src[], uniform int size)
{
    foreach(i=0 ... size) {
        dst[i] = to_float(src[i]);
    }
}

export void HalfToByte(uniform byte dst[], uniform half src[], uniform int size)
{
    foreach(i=0 ... size) {
        dst[i] = to_byte(src[i]);
    }
}
export void HalfToFloat(uniform float dst[], uniform half src[], uniform int size)
{
    foreach(i=0 ... size) {
        dst[i] = to_float(src[i]);
    }
}

export void FloatToByte(uniform byte dst[], uniform float src[], uniform int size)
{
    foreach(i=0 ... size) {
        dst[i] = to_byte(src[i]);
    }
}
export void FloatToHalf(uniform half dst[], uniform float src[], uniform int size)
{
    foreach(i=0 ... size) {
        dst[i] = to_half(src[i]);
    }
}


#define Convert44(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 4) { dst[i*4 + j] = C(src[i*4 + j]); }}
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
#define Convert33(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 3) { dst[i*3 + j] = C(src[i*3 + j]); }}
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
#define Convert22(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 2) { dst[i*2 + j] = C(src[i*2 + j]); }}
#define Convert21(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 1) { dst[i*1 + j] = C(src[i*2 + j]); }}


#define Convert14(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*4 + 0] = C(src[i*2 + 0]);\
        dst[i*4 + 1] = C(0.0);\
        dst[i*4 + 2] = C(0.0);\
        dst[i*4 + 3] = C(1.0);\
    }
#define Convert13(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*3 + 0] = C(src[i*2 + 0]);\
        dst[i*3 + 1] = C(0.0);\
        dst[i*3 + 2] = C(0.0);\
    }
#define Convert12(C)\
    for(uniform int i=0; i<size; ++i) {\
        dst[i*3 + 0] = C(src[i*2 + 0]);\
        dst[i*3 + 1] = C(0.0);\
    }
#define Convert11(C) for(uniform int i=0; i<size; ++i) { foreach(j=0 ... 1) { dst[i*1 + j] = C(src[i*1 + j]); }}



export void RGBAu8ToRGBu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert43(to_byte) }
export void RGBAu8ToRGu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert42(to_byte) }
export void RGBAu8ToRu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert41(to_byte) }
export void RGBAu8ToRGBAf16(uniform half dst[], uniform byte src[], uniform int size) { Convert44(to_half) }
export void RGBAu8ToRGBf16(uniform half dst[], uniform byte src[], uniform int size) { Convert43(to_half) }
export void RGBAu8ToRGf16(uniform half dst[], uniform byte src[], uniform int size) { Convert42(to_half) }
export void RGBAu8ToRf16(uniform half dst[], uniform byte src[], uniform int size) { Convert41(to_half) }
export void RGBAu8ToRGBAf32(uniform float dst[], uniform byte src[], uniform int size) { Convert44(to_float) }
export void RGBAu8ToRGBf32(uniform float dst[], uniform byte src[], uniform int size) { Convert43(to_float) }
export void RGBAu8ToRGf32(uniform float dst[], uniform byte src[], uniform int size) { Convert42(to_float) }
export void RGBAu8ToRf32(uniform float dst[], uniform byte src[], uniform int size) { Convert41(to_float) }

export void RGBu8ToRGBAu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert34(to_byte) }
export void RGBu8ToRGu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert32(to_byte) }
export void RGBu8ToRu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert31(to_byte) }
export void RGBu8ToRGBAf16(uniform half dst[], uniform byte src[], uniform int size) { Convert34(to_half) }
export void RGBu8ToRGBf16(uniform half dst[], uniform byte src[], uniform int size) { Convert33(to_half) }
export void RGBu8ToRGf16(uniform half dst[], uniform byte src[], uniform int size) { Convert32(to_half) }
export void RGBu8ToRf16(uniform half dst[], uniform byte src[], uniform int size) { Convert31(to_half) }
export void RGBu8ToRGBAf32(uniform float dst[], uniform byte src[], uniform int size) { Convert34(to_float) }
export void RGBu8ToRGBf32(uniform float dst[], uniform byte src[], uniform int size) { Convert33(to_float) }
export void RGBu8ToRGf32(uniform float dst[], uniform byte src[], uniform int size) { Convert32(to_float) }
export void RGBu8ToRf32(uniform float dst[], uniform byte src[], uniform int size) { Convert31(to_float) }

export void RGu8ToRGBAu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert24(to_byte) }
export void RGu8ToRGBu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert23(to_byte) }
export void RGu8ToRu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert21(to_byte) }
export void RGu8ToRGBAf16(uniform half dst[], uniform byte src[], uniform int size) { Convert24(to_half) }
export void RGu8ToRGBf16(uniform half dst[], uniform byte src[], uniform int size) { Convert23(to_half) }
export void RGu8ToRGf16(uniform half dst[], uniform byte src[], uniform int size) { Convert22(to_half) }
export void RGu8ToRf16(uniform half dst[], uniform byte src[], uniform int size) { Convert21(to_half) }
export void RGu8ToRGBAf32(uniform float dst[], uniform byte src[], uniform int size) { Convert24(to_float) }
export void RGu8ToRGBf32(uniform float dst[], uniform byte src[], uniform int size) { Convert23(to_float) }
export void RGu8ToRGf32(uniform float dst[], uniform byte src[], uniform int size) { Convert22(to_float) }
export void RGu8ToRf32(uniform float dst[], uniform byte src[], uniform int size) { Convert21(to_float) }

export void Ru8ToRGBAu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert14(to_byte) }
export void Ru8ToRGBu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert13(to_byte) }
export void Ru8ToRGu8(uniform byte dst[], uniform byte src[], uniform int size) { Convert12(to_byte) }
export void Ru8ToRGBAf16(uniform half dst[], uniform byte src[], uniform int size) { Convert14(to_half) }
export void Ru8ToRGBf16(uniform half dst[], uniform byte src[], uniform int size) { Convert13(to_half) }
export void Ru8ToRGf16(uniform half dst[], uniform byte src[], uniform int size) { Convert12(to_half) }
export void Ru8ToRf16(uniform half dst[], uniform byte src[], uniform int size) { Convert11(to_half) }
export void Ru8ToRGBAf32(uniform float dst[], uniform byte src[], uniform int size) { Convert14(to_float) }
export void Ru8ToRGBf32(uniform float dst[], uniform byte src[], uniform int size) { Convert13(to_float) }
export void Ru8ToRGf32(uniform float dst[], uniform byte src[], uniform int size) { Convert12(to_float) }
export void Ru8ToRf32(uniform float dst[], uniform byte src[], uniform int size) { Convert11(to_float) }


export void RGBAf16ToRGBAu8(uniform byte dst[], uniform half src[], uniform int size) { Convert44(to_byte) }
export void RGBAf16ToRGBu8(uniform byte dst[], uniform half src[], uniform int size) { Convert43(to_byte) }
export void RGBAf16ToRGu8(uniform byte dst[], uniform half src[], uniform int size) { Convert42(to_byte) }
export void RGBAf16ToRu8(uniform byte dst[], uniform half src[], uniform int size) { Convert41(to_byte) }
export void RGBAf16ToRGBf16(uniform half dst[], uniform half src[], uniform int size) { Convert43(to_half) }
export void RGBAf16ToRGf16(uniform half dst[], uniform half src[], uniform int size) { Convert42(to_half) }
export void RGBAf16ToRf16(uniform half dst[], uniform half src[], uniform int size) { Convert41(to_half) }
export void RGBAf16ToRGBAf32(uniform float dst[], uniform half src[], uniform int size) { Convert44(to_float) }
export void RGBAf16ToRGBf32(uniform float dst[], uniform half src[], uniform int size) { Convert43(to_float) }
export void RGBAf16ToRGf32(uniform float dst[], uniform half src[], uniform int size) { Convert42(to_float) }
export void RGBAf16ToRf32(uniform float dst[], uniform half src[], uniform int size) { Convert41(to_float) }

export void RGBf16ToRGBAu8(uniform byte dst[], uniform half src[], uniform int size) { Convert34(to_byte) }
export void RGBf16ToRGBu8(uniform byte dst[], uniform half src[], uniform int size) { Convert33(to_byte) }
export void RGBf16ToRGu8(uniform byte dst[], uniform half src[], uniform int size) { Convert32(to_byte) }
export void RGBf16ToRu8(uniform byte dst[], uniform half src[], uniform int size) { Convert31(to_byte) }
export void RGBf16ToRGBAf16(uniform half dst[], uniform half src[], uniform int size) { Convert34(to_half) }
export void RGBf16ToRGf16(uniform half dst[], uniform half src[], uniform int size) { Convert32(to_half) }
export void RGBf16ToRf16(uniform half dst[], uniform half src[], uniform int size) { Convert31(to_half) }
export void RGBf16ToRGBAf32(uniform float dst[], uniform half src[], uniform int size) { Convert34(to_float) }
export void RGBf16ToRGBf32(uniform float dst[], uniform half src[], uniform int size) { Convert33(to_float) }
export void RGBf16ToRGf32(uniform float dst[], uniform half src[], uniform int size) { Convert32(to_float) }
export void RGBf16ToRf32(uniform float dst[], uniform half src[], uniform int size) { Convert31(to_float) }

export void RGf16ToRGBAu8(uniform byte dst[], uniform half src[], uniform int size) { Convert24(to_byte) }
export void RGf16ToRGBu8(uniform byte dst[], uniform half src[], uniform int size) { Convert23(to_byte) }
export void RGf16ToRGu8(uniform byte dst[], uniform half src[], uniform int size) { Convert22(to_byte) }
export void RGf16ToRu8(uniform byte dst[], uniform half src[], uniform int size) { Convert21(to_byte) }
export void RGf16ToRGBAf16(uniform half dst[], uniform half src[], uniform int size) { Convert24(to_half) }
export void RGf16ToRGBf16(uniform half dst[], uniform half src[], uniform int size) { Convert23(to_half) }
export void RGf16ToRf16(uniform half dst[], uniform half src[], uniform int size) { Convert21(to_half) }
export void RGf16ToRGBAf32(uniform float dst[], uniform half src[], uniform int size) { Convert24(to_float) }
export void RGf16ToRGBf32(uniform float dst[], uniform half src[], uniform int size) { Convert23(to_float) }
export void RGf16ToRGf32(uniform float dst[], uniform half src[], uniform int size) { Convert22(to_float) }
export void RGf16ToRf32(uniform float dst[], uniform half src[], uniform int size) { Convert21(to_float) }

export void Rf16ToRGBAu8(uniform byte dst[], uniform half src[], uniform int size) { Convert14(to_byte) }
export void Rf16ToRGBu8(uniform byte dst[], uniform half src[], uniform int size) { Convert13(to_byte) }
export void Rf16ToRGu8(uniform byte dst[], uniform half src[], uniform int size) { Convert12(to_byte) }
export void Rf16ToRu8(uniform byte dst[], uniform half src[], uniform int size) { Convert11(to_byte) }
export void Rf16ToRGBAf16(uniform half dst[], uniform half src[], uniform int size) { Convert14(to_half) }
export void Rf16ToRGBf16(uniform half dst[], uniform half src[], uniform int size) { Convert13(to_half) }
export void Rf16ToRGf16(uniform half dst[], uniform half src[], uniform int size) { Convert12(to_half) }
export void Rf16ToRGBAf32(uniform float dst[], uniform half src[], uniform int size) { Convert14(to_float) }
export void Rf16ToRGBf32(uniform float dst[], uniform half src[], uniform int size) { Convert13(to_float) }
export void Rf16ToRGf32(uniform float dst[], uniform half src[], uniform int size) { Convert12(to_float) }
export void Rf16ToRf32(uniform float dst[], uniform half src[], uniform int size) { Convert11(to_float) }


export void RGBAf32ToRGBAu8(uniform byte dst[], uniform float src[], uniform int size) { Convert44(to_byte) }
export void RGBAf32ToRGBu8(uniform byte dst[], uniform float src[], uniform int size) { Convert43(to_byte) }
export void RGBAf32ToRGu8(uniform byte dst[], uniform float src[], uniform int size) { Convert42(to_byte) }
export void RGBAf32ToRu8(uniform byte dst[], uniform float src[], uniform int size) { Convert41(to_byte) }
export void RGBAf32ToRGBAf16(uniform half dst[], uniform float src[], uniform int size) { Convert44(to_half) }
export void RGBAf32ToRGBf16(uniform half dst[], uniform float src[], uniform int size) { Convert43(to_half) }
export void RGBAf32ToRGf16(uniform half dst[], uniform float src[], uniform int size) { Convert42(to_half) }
export void RGBAf32ToRf16(uniform half dst[], uniform float src[], uniform int size) { Convert41(to_half) }
export void RGBAf32ToRGBf32(uniform float dst[], uniform float src[], uniform int size) { Convert43(to_float) }
export void RGBAf32ToRGf32(uniform float dst[], uniform float src[], uniform int size) { Convert42(to_float) }
export void RGBAf32ToRf32(uniform float dst[], uniform float src[], uniform int size) { Convert41(to_float) }

export void RGBf32ToRGBAu8(uniform byte dst[], uniform float src[], uniform int size) { Convert34(to_byte) }
export void RGBf32ToRGBu8(uniform byte dst[], uniform float src[], uniform int size) { Convert33(to_byte) }
export void RGBf32ToRGu8(uniform byte dst[], uniform float src[], uniform int size) { Convert32(to_byte) }
export void RGBf32ToRu8(uniform byte dst[], uniform float src[], uniform int size) { Convert31(to_byte) }
export void RGBf32ToRGBAf16(uniform half dst[], uniform float src[], uniform int size) { Convert34(to_half) }
export void RGBf32ToRGBf16(uniform half dst[], uniform float src[], uniform int size) { Convert33(to_half) }
export void RGBf32ToRGf16(uniform half dst[], uniform float src[], uniform int size) { Convert32(to_half) }
export void RGBf32ToRf16(uniform half dst[], uniform float src[], uniform int size) { Convert31(to_half) }
export void RGBf32ToRGBAf32(uniform float dst[], uniform float src[], uniform int size) { Convert34(to_float) }
export void RGBf32ToRGf32(uniform float dst[], uniform float src[], uniform int size) { Convert32(to_float) }
export void RGBf32ToRf32(uniform float dst[], uniform float src[], uniform int size) { Convert31(to_float) }

export void RGf32ToRGBAu8(uniform byte dst[], uniform float src[], uniform int size) { Convert24(to_byte) }
export void RGf32ToRGBu8(uniform byte dst[], uniform float src[], uniform int size) { Convert23(to_byte) }
export void RGf32ToRGu8(uniform byte dst[], uniform float src[], uniform int size) { Convert22(to_byte) }
export void RGf32ToRu8(uniform byte dst[], uniform float src[], uniform int size) { Convert21(to_byte) }
export void RGf32ToRGBAf16(uniform half dst[], uniform float src[], uniform int size) { Convert24(to_half) }
export void RGf32ToRGBf16(uniform half dst[], uniform float src[], uniform int size) { Convert23(to_half) }
export void RGf32ToRGf16(uniform half dst[], uniform float src[], uniform int size) { Convert22(to_half) }
export void RGf32ToRf16(uniform half dst[], uniform float src[], uniform int size) { Convert21(to_half) }
export void RGf32ToRGBAf32(uniform float dst[], uniform float src[], uniform int size) { Convert24(to_float) }
export void RGf32ToRGBf32(uniform float dst[], uniform float src[], uniform int size) { Convert23(to_float) }
export void RGf32ToRf32(uniform float dst[], uniform float src[], uniform int size) { Convert21(to_float) }

export void Rf32ToRGBAu8(uniform byte dst[], uniform float src[], uniform int size) { Convert14(to_byte) }
export void Rf32ToRGBu8(uniform byte dst[], uniform float src[], uniform int size) { Convert13(to_byte) }
export void Rf32ToRGu8(uniform byte dst[], uniform float src[], uniform int size) { Convert12(to_byte) }
export void Rf32ToRu8(uniform byte dst[], uniform float src[], uniform int size) { Convert11(to_byte) }
export void Rf32ToRGBAf16(uniform half dst[], uniform float src[], uniform int size) { Convert14(to_half) }
export void Rf32ToRGBf16(uniform half dst[], uniform float src[], uniform int size) { Convert13(to_half) }
export void Rf32ToRGf16(uniform half dst[], uniform float src[], uniform int size) { Convert12(to_half) }
export void Rf32ToRf16(uniform half dst[], uniform float src[], uniform int size) { Convert11(to_half) }
export void Rf32ToRGBAf32(uniform float dst[], uniform float src[], uniform int size) { Convert14(to_float) }
export void Rf32ToRGBf32(uniform float dst[], uniform float src[], uniform int size) { Convert13(to_float) }
export void Rf32ToRGf32(uniform float dst[], uniform float src[], uniform int size) { Convert12(to_float) }
