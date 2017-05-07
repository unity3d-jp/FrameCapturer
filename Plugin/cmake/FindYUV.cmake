set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${YUV_DIR}/lib
)

find_path(YUV_INCLUDE_DIR
    libyuv.h
    PATHS ${YUV_DIR}/include
)

find_library(YUV_LIBRARY
    NAMES libyuv
    PATHS ${LIBRARY_PATHS}
)

mark_as_advanced(YUV_INCLUDE_DIR)
mark_as_advanced(YUV_LIBRARY)

find_package_handle_standard_args("YUV"
    DEFAULT_MSG
    YUV_LIBRARY
    YUV_INCLUDE_DIR
)
