add_executable(CookieClicker src/main.cpp)
target_link_libraries(CookieClicker PRIVATE gemedev::gemedev)
target_compile_definitions(CookieClicker PRIVATE GEMEDEV_ASSET_DIR="assets")
