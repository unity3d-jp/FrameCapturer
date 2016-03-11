#include "TestCommon.h"


void FAACSelfBuildTest()
{
    printf("FAACSelfBuildTest Begin\n");

    if (Unzip(".", "FAAC_SelfBuild.zip", [](const char *path) {
        printf("unzip: %s\n", path);
    }) > 0)
    {
#ifdef _M_IX86
        Execute("FAAC_SelfBuild\\build_x86.bat");
#elif _M_X64
        Execute("FAAC_SelfBuild\\build_x86_64.bat");
#endif
    }

    printf("FAACSelfBuildTest End\n");
}
