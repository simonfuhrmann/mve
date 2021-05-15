set(MVE_MVE_TEST_SOURCES
        bundle
        camera
        image
        image_color
        image_drawing
        image_io
        image_tools
        mesh_info
        mesh_io
        mesh_tools
        triangle_mesh
        view
        )

foreach(testname IN LISTS MVE_MVE_TEST_SOURCES)
    set(test_target smvs_test_mve_${testname})
    add_executable(${test_target} gtest_${testname}.cc)
    target_link_libraries(${test_target} GTest::GTest GTest::Main mve_mve)

    add_test(NAME ${test_target} COMMAND ${test_target})
    unset(test_target)
endforeach()