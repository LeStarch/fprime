//
// Created by Starch, Michael D (348C) on 12/7/20.
//
#include <gtest/gtest.h>
#include <Drv/Ip/TcpClientSocket.hpp>
#include <Drv/Ip/TcpServerSocket.hpp>
#include <Drv/Ip/IpSocket.hpp>
#include <Fw/Types/EightyCharString.hpp>
#include <Os/Task.hpp>
#include <Os/Log.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Drv/Ip/test/ut/PortSelector.hpp>
#include <sys/socket.h>

Os::Log logger;

const char* SERVER_GREET = "Hello, from your friendly neighborhood server";
const char* CLIENT_GREET = "Hello, I'm SUPER client";

const char* SERVER_RESP = "I AM THE SERVER UBER SERVERS";
const char* CLIENT_RESP = "Client of one";

struct ThreadControl {
    U32 iterations;
    U16 port;
};

void force_recv_timeout(Drv::IpSocket& socket) {
    // Set timeout socket option
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000; // 50ms max before test failure
    // set socket write to timeout after 1 sec
    setsockopt(socket.m_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
}

void recv_check(const char* message,  Drv::IpSocket& socket) {
    U8 buffer[100];
    U32 total = 0;
    U32 message_size = strnlen(message, sizeof(buffer)) + 1;
    while (total < message_size) {
        I32 to_get = message_size - total;
        Drv::SocketIpStatus status = socket.recv(buffer + total, to_get);
        total += to_get;
        ASSERT_EQ(status, Drv::SOCK_SUCCESS);
    }
    EXPECT_STREQ(reinterpret_cast<char*>(buffer), message);
}

void server_thread(void* arg) {
    U8 data[100];
    I32 size = sizeof(data);
    Drv::TcpServerSocket server;
    ThreadControl* control = reinterpret_cast<ThreadControl*>(arg);
    U16 port = control->port;
    ASSERT_NE(port, 0);
    server.configure("127.0.0.1", port, 0, 100);
    server.startup();
    for (U32 i = 0; i < control->iterations; i++) {
        // ***NOTE*** port must be available or this will fail
        Drv::SocketIpStatus status = server.open();
        EXPECT_EQ(status, Drv::SOCK_SUCCESS);
        if (Drv::SOCK_SUCCESS != status) {
            break;
        }
        force_recv_timeout(server); // So the test won't hang
        recv_check(CLIENT_GREET, server);
        server.send(reinterpret_cast<const U8*>(SERVER_RESP), strlen(SERVER_RESP) + 1);
        server.send(reinterpret_cast<const U8*>(SERVER_GREET), strlen(SERVER_GREET) + 1);
        recv_check(CLIENT_RESP, server);
    }
    // Last receive notices client closure and allows us to shutdown without timewait
    EXPECT_EQ(server.recv(data, size), Drv::SOCK_DISCONNECTED);
    server.shutdown();
}

void client_thread(void* arg) {
    ThreadControl* control = reinterpret_cast<ThreadControl*>(arg);
    U16 port = control->port;
    for (U32 i = 0; i < control->iterations; i++) {
        Drv::TcpClientSocket client;
        ASSERT_NE(port, 0);
        client.configure("127.0.0.1",port,0,100);
        Drv::SocketIpStatus status = client.open();
        EXPECT_EQ(status, Drv::SOCK_SUCCESS);
        if (Drv::SOCK_SUCCESS == status) {
            force_recv_timeout(client); // So the test won't hang
            client.send(reinterpret_cast<const U8*>(CLIENT_GREET), strlen(CLIENT_GREET) + 1);
            recv_check(SERVER_RESP, client);
            recv_check(SERVER_GREET, client);
            client.send(reinterpret_cast<const U8*>(CLIENT_RESP), strlen(CLIENT_RESP) + 1);
        }
        client.close();
    }

}

TEST(Nominal, TestNominalTcp) {
    ThreadControl control;
    control.iterations = 1;
    control.port =  Drv::get_free_port();
    ASSERT_NE(0, control.port);
    Os::Task t_server;
    Os::Task t_client;
    Fw::EightyCharString task_s_name("Server Task");
    t_server.start(task_s_name, 99, 255, 1024, server_thread, &control);

    Fw::EightyCharString task_c_name("Client Task");
    t_client.start(task_c_name, 98, 254, 1024, client_thread, &control);

    t_server.join(NULL);
    t_client.join(NULL);
}

TEST(Nominal, TestMulipleTcp) {
    ThreadControl control;
    control.iterations = 10;
    control.port =  Drv::get_free_port();
    ASSERT_NE(0, control.port);
    Os::Task t_server;
    Os::Task t_client;
    Fw::EightyCharString task_s_name("Server Task");
    t_server.start(task_s_name, 99, 255, 1024, server_thread, &control);

    Fw::EightyCharString task_c_name("Client Task");
    t_client.start(task_c_name, 98, 254, 1024, client_thread, &control);

    t_server.join(NULL);
    t_client.join(NULL);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
