add_library(XmlRpc STATIC
    XmlRpcClient.cpp
    XmlRpcDispatch.cpp
    XmlRpcServer.cpp
    XmlRpcServerConnection.cpp
    XmlRpcServerMethod.cpp
    XmlRpcSocket.cpp
    XmlRpcSource.cpp
    XmlRpcUtil.cpp
    XmlRpcValue.cpp
)

target_include_directories(XmlRpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XmlRpc PUBLIC EngineIncludes)

if(WIN32)
    # XmlRpcSocket.cpp and XmlRpcDispatch.cpp use WinSock 2, and windows.h — which the
    # root CMakeLists force-includes into every TU — brings WinSock 1 along by default:
    # the two redefine sockaddr, fd_set, every socket call. WIN32_LEAN_AND_MEAN is what
    # keeps windows.h out of the sockets business, and it is scoped to this target
    # because it also drops ole2.h and mmsystem.h, which other targets do use.
    target_compile_definitions(XmlRpc PRIVATE WIN32_LEAN_AND_MEAN)
endif()
