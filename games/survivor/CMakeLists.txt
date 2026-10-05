add_executable(Survivor
        src/main.cpp
        src/scenes/ui/settings.cpp
        src/scenes/ui/menu.cpp
        src/scenes/game/game.cpp
        src/logic/Tiles.cpp
)

target_link_libraries(Survivor PRIVATE gemedev::gemedev)
target_compile_definitions(Survivor PRIVATE
        GEMEDEV_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)