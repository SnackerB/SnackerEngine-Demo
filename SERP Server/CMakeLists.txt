cmake_minimum_required(VERSION 3.15)
set (CMAKE_CXX_STANDARD 23)
project(SERPServer)
add_compile_definitions(_LINUX)

ADD_EXECUTABLE( SERPServer
    main.cpp    
    Client.cpp
    Server.cpp)

target_include_directories(SERPServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../SnackerEngine)
target_link_libraries(SERPServer
    ${CMAKE_SOURCE_DIR}/../../SnackerEngine/Utility/libUtility.a
    ${CMAKE_SOURCE_DIR}/../../SnackerEngine/Math/libMath.a
    ${CMAKE_SOURCE_DIR}/../../SnackerEngine/Network/libNetwork.a)

ADD_EXECUTABLE( startSERPServer
    startServer.cpp    
    Client.cpp
    Server.cpp)

target_include_directories(startSERPServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../SnackerEngine)
target_link_libraries(startSERPServer
    ${CMAKE_SOURCE_DIR}/../../SnackerEngine/Utility/libUtility.a
    ${CMAKE_SOURCE_DIR}/../../SnackerEngine/Math/libMath.a
    ${CMAKE_SOURCE_DIR}/../../SnackerEngine/Network/libNetwork.a)

ADD_EXECUTABLE( terminateSERPServer
    terminateServer.cpp )