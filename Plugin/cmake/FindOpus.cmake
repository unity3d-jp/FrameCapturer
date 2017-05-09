set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${OPUS_DIR}/lib
)

find_path(OPUS_INCLUDE_DIR
    opus/opus.h
    PATHS ${OPUS_DIR}/include
)

find_library(OPUS_LIBRARY
    NAMES opus
    PATHS ${LIBRARY_PATHS}
)

mark_as_advanced(OPUS_INCLUDE_DIR)
mark_as_advanced(OPUS_LIBRARY)

find_package_handle_standard_args("OPUS"
    DEFAULT_MSG
    OPUS_LIBRARY
    OPUS_INCLUDE_DIR
)
