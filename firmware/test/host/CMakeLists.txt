cmake_minimum_required(VERSION 3.16)
project(temptation_box_host_tests C)

# ── Unity ─────────────────────────────────────────────────────────────────────
add_library(unity STATIC
    Unity/src/unity.c
)
target_include_directories(unity PUBLIC
    Unity/src
)

# ── Test SCHEDULE ───────────────────────────────────────────────────────────────
add_executable(test_schedule
    test_schedule.c
    ../../main/schedule/schedule.c
    ../../main/hal/hal_mock.c
)

target_include_directories(test_schedule PRIVATE
    Unity/src
    ../../main/schedule
    ../../main/hal
    ../../main
)

target_link_libraries(test_schedule PRIVATE unity)

# ── TEST CACHE ───────────────────────────────────────────────────────────────

add_executable(test_cache
    test_cache.c
    ../../main/cache/cache.c
    ../../main/hal/hal_mock.c
)
target_include_directories(test_cache PRIVATE
    Unity/src
    ../../main/cache
    ../../main/schedule
    ../../main/hal
    ../../main
)
target_link_libraries(test_cache PRIVATE unity)


# ── TEST PRESENCE ───────────────────────────────────────────────────────────────

add_executable(test_presence
    test_presence.c
    ../../main/presence/presence.c
    ../../main/hal/hal_mock.c
)
target_include_directories(test_presence PRIVATE
    Unity/src
    ../../main/presence
    ../../main/cache
    ../../main/schedule
    ../../main/hal
    ../../main
)
target_link_libraries(test_presence PRIVATE unity)

# ── TEST LOCK ───────────────────────────────────────────────────────────────

add_executable(test_lock
    test_lock.c
    ../../main/lock/lock.c
    ../../main/hal/hal_mock.c
)
target_include_directories(test_lock PRIVATE
    Unity/src
    ../../main/lock
    ../../main/presence
    ../../main/cache
    ../../main/schedule
    ../../main/hal
    ../../main
)
target_link_libraries(test_lock PRIVATE unity)