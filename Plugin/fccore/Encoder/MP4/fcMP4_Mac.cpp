#include "pch.h"
#include "fcInternal.h"

#ifdef __APPLE__
// todo: use AVFoundation

fcIMP4Context* fcMP4OSCreateContextImpl(fcMP4Config &conf, fcIGraphicsDevice *dev, const char *path)
{
    return nullptr;
}

bool fcMP4OSIsSupportedImpl()
{
    return false;
}

#endif // __APPLE__
