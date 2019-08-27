project(WOWQueueCamper)
add_executable(WOWQueueCamper main.cpp)
add_custom_command(
        TARGET WOWQueueCamper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/default.wav"
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/default.wav")