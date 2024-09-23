cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(MyLib STATIC 
    src/model.h
    src/model.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/tagged.h
    src/collision_detector.h
    src/collision_detector.cpp
    src/geom.h
)

target_link_libraries(MyLib PUBLIC CONAN_PKG::boost)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/json_support.cpp
	src/json_support.h
	src/logging_request_handler.h
	src/request_handler_api.h
	src/request_handler_static.h
	src/content_type.h
	src/players_tokens.h
	src/players.h
	src/player.h
	src/request_handler_game.h
	src/ticker.h
	src/extra_data.h
	src/model_serialization.h
	src/model_serialization.cpp
	src/application.h
	src/serializator.h
	src/serializator.cpp
	src/connection_pool.h
	src/connection_pool.cpp
	src/database.h
	src/database.cpp
	src/tagged_uuid.h
	src/tagged_uuid.cpp
	src/application.cpp
)

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
    tests/collision-detector-tests.cpp
)

target_link_libraries(game_server PRIVATE Threads::Threads)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE MyLib)
target_link_libraries(game_server PRIVATE CONAN_PKG::libpqxx)

target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost CONAN_PKG::libpqxx Threads::Threads MyLib)
