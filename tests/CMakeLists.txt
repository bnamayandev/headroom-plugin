juce_add_console_app(headroom_tests PRODUCT_NAME "headroom_tests" NEEDS_CURL TRUE)

target_sources(headroom_tests PRIVATE
    AllocationTracker.cpp
    TestAgent.cpp
    TestMain.cpp
    TestDsp.cpp
    TestLevels.cpp
    TestPedals.cpp
    TestOversampler.cpp
    TestResampler.cpp
    TestNam.cpp
    TestRealtime.cpp
    TestTuner.cpp)

target_compile_definitions(headroom_tests PRIVATE
    ${HEADROOM_JUCE_DEFINITIONS}
    HEADROOM_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    NAM_EXAMPLE_MODELS_DIR="${NAM_DIR}/example_models")

# No juce_recommended_warning_flags here: tests compare floats exactly on purpose
# (bit-exact pass-through, determinism). The shared sources are warning-checked in the
# plugin and CLI builds.
target_link_libraries(headroom_tests
    PRIVATE
        headroom_core
        Catch2::Catch2
    PUBLIC
        juce::juce_recommended_config_flags)

add_test(NAME headroom_tests COMMAND headroom_tests)
