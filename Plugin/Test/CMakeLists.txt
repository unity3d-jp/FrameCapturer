if(FC_ENABLE_TEST)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}")

    file(GLOB TEST_SOURCES *.cpp *.h)
    add_executable(Test ${TEST_SOURCES})
    add_dependencies(Test fccore)
    target_link_libraries(Test fccore pthread)
    install(TARGETS Test DESTINATION .)
endif()
