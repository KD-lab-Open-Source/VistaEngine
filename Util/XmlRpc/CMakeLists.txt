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
