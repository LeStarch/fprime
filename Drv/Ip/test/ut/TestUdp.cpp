//
// Created by Starch, Michael D (348C) on 12/7/20.
//
#include <gtest/gtest.h>
#include <Drv/Ip/UdpSocket.hpp>
#include <Drv/Ip/IpSocket.hpp>
#include <Fw/Types/EightyCharString.hpp>
#include <Os/Task.hpp>
#include <Os/Log.hpp>
#include <Fw/Logger/Logger.hpp>
#include <sys/socket.h>
#include <Drv/Ip/test/ut/PortSelector.hpp>

Os::Log logger;

const char* SERVER_GREET = "Hello, from your friendly neighborhood not-a-server";
const char* CLIENT_GREET = "Hello, I'm SUPER client";

const char* SERVER_RESP = "I AM THE not-a-SERVER UBER SERVERS";
const char* CLIENT_RESP = "Client of one";

struct ThreadControl {
    U32 iterations;
    U32 client_number;
    U16 port1;
    U16 port2;
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

void client_thread(void* arg) {
    ThreadControl* control = reinterpret_cast<ThreadControl*>(arg);
    U16 local_port = (control->client_number == 1) ? control->port1 : control->port2;
    U16 remote_port = (control->client_number == 1) ? control->port2 : control->port1;


    for (U32 i = 0; i < control->iterations; i++) {
        Drv::UdpSocket client;
        ASSERT_EQ(client.configureSend("127.0.0.1", remote_port,0,100), Drv::SOCK_SUCCESS);
        ASSERT_EQ(client.configureRecv("127.0.0.1", local_port),Drv::SOCK_SUCCESS);
        Drv::SocketIpStatus status = client.open();
        EXPECT_EQ(status, Drv::SOCK_SUCCESS);
        if (Drv::SOCK_SUCCESS == status) {
            force_recv_timeout(client); // So the test won't hang
            // Send first message modulated by client number
            const char * message = (control->client_number == 1) ? CLIENT_GREET : SERVER_GREET;
            size_t message_size =  (control->client_number == 1) ? (strlen(CLIENT_GREET) + 1) : (strlen(SERVER_GREET) + 1);
            const char * look_for = (control->client_number == 1) ? SERVER_GREET : CLIENT_GREET;

            Os::Task::delay(10); // Wait a touch trying to yield CPU such that we both send at nearly the same time
            client.send(reinterpret_cast<const U8*>(message), message_size);
            // Recv first message modulated by client number
            recv_check(look_for, client);
            Os::Task::delay(55); // Wait a touch before closing
        }
        client.close();
    }

}

TEST(Nominal, TestNominalUdp) {
    ThreadControl control1;
    control1.iterations = 1;
    control1.client_number = 1;
    U16 port1 = Drv::get_free_port(true);
    ASSERT_NE(port1, 0);
    U16 port2 = Drv::get_free_port(true);
    ASSERT_NE(port2, 0);
    control1.port1 = port1;
    control1.port2 = port2;

    ThreadControl control2;
    control2.iterations = 1;
    control2.client_number = 2;
    control2.port1 = port1;
    control2.port2 = port2;
    ASSERT_EQ(control1.iterations, control2.iterations);
    ASSERT_NE(control1.client_number, control2.client_number);

    Os::Task t_client1;
    Os::Task t_client2;
    Fw::EightyCharString task_s_name("Client 1 Task");

    t_client1.start(task_s_name, 99, 255, 1024, client_thread, &control1);

    Fw::EightyCharString task_c_name("Client 2 Task");
    t_client2.start(task_c_name, 98, 254, 1024, client_thread, &control2);

    t_client1.join(NULL);
    t_client2.join(NULL);
}

TEST(Nominal, TestMulipleUdp) {
    ThreadControl control1;
    control1.iterations = 10;
    control1.client_number = 1;
    U16 port1 = Drv::get_free_port(true);
    ASSERT_NE(port1, 0);
    U16 port2 = Drv::get_free_port(true);
    ASSERT_NE(port2, 0);
    control1.port1 = port1;
    control1.port2 = port2;

    ThreadControl control2;
    control2.iterations = 10;
    control2.client_number = 2;
    control2.port1 = port1;
    control2.port2 = port2;
    ASSERT_EQ(control1.iterations, control2.iterations);
    ASSERT_NE(control1.client_number, control2.client_number);

    Os::Task t_client1;
    Os::Task t_client2;
    Fw::EightyCharString task_s_name("Client 1 Task");

    t_client1.start(task_s_name, 99, 255, 1024, client_thread, &control1);

    Fw::EightyCharString task_c_name("Client 2 Task");
    t_client2.start(task_c_name, 98, 254, 1024, client_thread, &control2);

    t_client1.join(NULL);
    t_client2.join(NULL);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}