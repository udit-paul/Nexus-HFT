include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(nexus_tests
    test_order_book.cpp
    test_matching_rules.cpp
    test_spsc_queue.cpp
    test_risk_engine.cpp
    test_oms_state_machine.cpp
    test_quant_signals.cpp
    test_avellaneda_stoikov.cpp
    test_wal_recovery.cpp
    ../src/engine/order_book.cpp
    ../src/engine/matching_engine.cpp
    ../src/concurrency/cpu_affinity.cpp
    ../src/risk/risk_engine.cpp
    ../src/oms/portfolio.cpp
    ../src/quant/micro_price.cpp
    ../src/quant/market_maker.cpp
    ../src/network/tcp_gateway.cpp
    ../src/network/mcast_publisher.cpp
    ../src/persistence/mmap_wal.cpp
)

target_include_directories(nexus_tests PRIVATE ../include)
target_link_libraries(nexus_tests gtest_main)
if (UNIX AND NOT APPLE)
    target_link_libraries(nexus_tests pthread)
endif()

include(GoogleTest)
gtest_discover_tests(nexus_tests)
