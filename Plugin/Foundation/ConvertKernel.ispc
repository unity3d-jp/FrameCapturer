typedef unsigned int8 byte;
typedef int16 half;

struct Ru8    { byte r;       };
struct RGu8   { byte r,g;     };
struct RGBu8  { byte r,g,b;   };
struct RGBAu8 { byte r,g,b,a; };

struct Rf16    { half r;       };
struct RGf16   { half r,g;     };
struct RGBf16  { half r,g,b;   };
struct RGBAf16 { half r,g,b,a; };

struct Rf32    { float r;       };
struct RGf32   { float r,g;     };
struct RGBf32  { float r,g,b;   };
struct RGBAf32 { float r,g,b,a; };

byte to_byte(byte v) { return v; }
byte to_byte(half v) { return (int)(half_to_float(v) * 255.0f) & 0xff; }
byte to_byte(float v) { return (int)(v * 255.0f) & 0xff; }

half to_half(byte v) { return float_to_half((float)((int)v) / 255.0f); }
half to_half(half v) { return v; }
half to_half(float v) { return float_to_half(v); }

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
