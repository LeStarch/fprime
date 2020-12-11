####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding diles
# MOD_DEPS: (optional) module dependencies
#
####
set(SOURCE_FILES
	"${CMAKE_CURRENT_LIST_DIR}/IpSocket.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/TcpClientSocket.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/TcpServerSocket.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/UdpSocket.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/SocketReadTask.cpp"
)
set(MOD_DEPS Os)

register_fprime_module()

add_library(PortSelector STATIC "${CMAKE_CURRENT_LIST_DIR}/test/ut/PortSelector.cpp")

set(UT_MOD_DEPS
	PortSelector
)

### UTs ###
set(UT_SOURCE_FILES
	"${CMAKE_CURRENT_LIST_DIR}/test/ut/TestTcp.cpp"
)
register_fprime_ut("Drv_Ip_Tcp_test")
set(UT_SOURCE_FILES
	"${CMAKE_CURRENT_LIST_DIR}/test/ut/TestUdp.cpp"
)

set(UT_MOD_DEPS
	PortSelector
)
register_fprime_ut("Drv_Ip_Udp_test")

# Special registration
