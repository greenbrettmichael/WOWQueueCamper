cmake_minimum_required(VERSION 3.4)
project(WOWQueueCamper)
find_package(Tesseract CONFIG REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)
add_executable(WOWQueueCamper main.cpp)
target_link_libraries(WOWQueueCamper PRIVATE libtesseract Boost::program_options)
set_target_properties(WOWQueueCamper PROPERTIES CXX_STANDARD 17)
add_custom_command(
        TARGET WOWQueueCamper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/default.wav"
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/default.wav")
add_custom_command(
        TARGET WOWQueueCamper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/eng.traineddata"
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/tessdata/eng.traineddata")