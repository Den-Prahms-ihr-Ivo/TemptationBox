cmake_minimum_required(VERSION 3.16)
project(temptation_box_host_tests C)

# ── Unity ─────────────────────────────────────────────────────────────────────
add_library(unity STATIC
    Unity/src/unity.c
)
target_include_directories(unity PUBLIC
    Unity/src
)

# ── Test runner ───────────────────────────────────────────────────────────────
add_executable(test_runner
    test_schedule.c
    ../../main/schedule/schedule.c
    ../../main/hal/hal_mock.c
)

target_include_directories(test_runner PRIVATE
    Unity/src
    ../../main/schedule
    ../../main/hal
    ../../main
)

target_link_libraries(test_runner PRIVATE unity)