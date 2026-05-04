cmake_minimum_required(VERSION 3.16)
project(EndfieldSkillDetector CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED)

add_executable(skill_detector
    main.cpp
    skill_detector.cpp
)

target_include_directories(skill_detector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(skill_detector PRIVATE ${OpenCV_LIBS})

# Windows：自动复制 OpenCV DLL
if(WIN32)
    add_custom_command(TARGET skill_detector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:skill_detector>
            $<TARGET_FILE_DIR:skill_detector>
        COMMAND_EXPAND_LISTS
    )
endif()
