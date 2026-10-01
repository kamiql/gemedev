add_library(gemedev STATIC
    src/core/Application.cpp
    src/platform/GlfwWindow.cpp
    src/platform/GlfwInput.cpp
    src/graphics/Assets.cpp
    src/graphics/Renderer2D.cpp
    src/scene/Scene.cpp
    src/scene/SceneManager.cpp
    src/scene/HitTesting.cpp
    src/animation/Animation.cpp
    src/ui/UI.cpp
    src/audio/Audio.cpp
    src/persistence/Save.cpp
)
add_library(gemedev::gemedev ALIAS gemedev)
target_include_directories(gemedev PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${STB_INCLUDE_DIR}" "${MINIAUDIO_INCLUDE_DIR}")
target_link_libraries(gemedev PUBLIC nlohmann_json::nlohmann_json
    PRIVATE OpenGL::GL ${GEMEDEV_GLFW_TARGET} GLEW::GLEW Freetype::Freetype)
if(WIN32)
    target_link_libraries(gemedev PRIVATE ole32 winmm)
elseif(APPLE)
    target_link_libraries(gemedev PRIVATE "-framework CoreAudio" "-framework AudioToolbox" "-framework AudioUnit" "-framework Foundation")
elseif(UNIX)
    target_link_libraries(gemedev PRIVATE m dl pthread)
endif()
target_compile_features(gemedev PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(gemedev PRIVATE /W4)
else()
    target_compile_options(gemedev PRIVATE -Wall -Wextra -Wpedantic)
endif()
