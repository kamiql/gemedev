add_executable(Flappy
        src/main.cpp
        src/scenes/menu.cpp
        src/scenes/game.cpp
        src/scenes/death.cpp
)

target_link_libraries(Flappy PRIVATE gemedev::gemedev)
target_compile_definitions(Flappy PRIVATE
        GEMEDEV_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)