// ======================================================================
// \title  ByteStreamDriverModel.hpp
// \author mstarch
// \brief  cpp file for ByteStreamDriverModel test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================
#include "Tester.hpp"
#include "STest/Pick/Pick.hpp"
#include <Drv/Ip/test/ut/PortSelector.hpp>

#define INSTANCE 0
#define MAX_HISTORY_SIZE 10

namespace Drv {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Tester ::Tester(void)
    : ByteStreamDriverModelGTestBase("Tester", MAX_HISTORY_SIZE),
      component("ByteStreamDriverModel"),
      m_data_buffer(m_data_storage, 0),
      m_port1(Drv::get_free_port()),
      m_port2(Drv::get_free_port()),
      m_spinner(false) {
    this->initComponents();
    this->connectPorts();
}

Tester ::~Tester(void) {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Tester ::validate_random_buffer(U8* data) {
    ASSERT_NE(m_data_buffer.getSize(), 0) << "Validating unfilled buffer";
    for (U32 i = 0; i < m_data_buffer.getSize(); i++) {
        ASSERT_EQ(m_data_buffer.getData()[i], data[i]);
    }
    m_data_buffer.setSize(0);
}

void Tester ::spin_and_clear() {
    while (not m_spinner) {
    }
    m_spinner = false;
}

void Tester::fill_random_buffer() {
    ASSERT_EQ(m_data_buffer.getSize(), 0) << "Unvalidated buffer found";
    m_data_buffer.setSize(STest::Pick::lowerUpper(1, sizeof(m_data_storage)));
    for (U32 i = 0; i < m_data_buffer.getSize(); i++) {
        m_data_buffer.getData()[i] = (U8)STest::Pick::any();
    }
}

Drv::SendStatus Tester::send_with_retry() {
    Drv::SendStatus status = SEND_RETRY;
    while (status == SEND_RETRY) {
        status = invoke_to_send(0, m_data_buffer);
    }
    return status;
}

void Tester ::transmit(Drv::UdpSocket& udpSocket) {
    U8 buffer[sizeof(m_data_storage)];
    for (U32 i = 0; i < STest::Pick::lowerUpper(1, 200); i++) {
        I32 size = sizeof(buffer);
        fill_random_buffer();
        ASSERT_EQ(SEND_OK, send_with_retry());
        ASSERT_EQ(udpSocket.recv(buffer, size), SOCK_SUCCESS);
        validate_random_buffer(buffer);
        fill_random_buffer();
        ASSERT_EQ(udpSocket.send(m_data_buffer.getData(), m_data_buffer.getSize()), SOCK_SUCCESS);
        // Wait for the return message
        spin_and_clear();
        clearFromPortHistory();
    }
}

void Tester ::test_basic_messaging(void) {
    ASSERT_NE(m_port1, 0);
    ASSERT_NE(m_port1, 0);
    Drv::UdpSocket server;
    server.configureSend("127.0.0.1", m_port1, 0, 100);
    server.configureRecv("127.0.0.1", m_port2);

    Fw::EightyCharString name("clientRead");
    this->component.configureRecv("127.0.0.1", m_port1);
    this->component.configureSend("127.0.0.1", m_port2);
    this->component.startSocketTask(name, 99, 1024);
    EXPECT_EQ(server.open(), SOCK_SUCCESS);

    transmit(server);

    this->component.stopSocketTask();
    this->component.joinSocketTask(NULL);
    server.close();  // Close client
}

void Tester ::test_advanced_reconnect(void) {
    ASSERT_NE(m_port1, 0);
    ASSERT_NE(m_port1, 0);
    Drv::UdpSocket udp;
    udp.configureSend("127.0.0.1", m_port1, 0, 100);
    udp.configureRecv("127.0.0.1", m_port2);

    Fw::EightyCharString name("clientRead");
    this->component.configureRecv("127.0.0.1", m_port1);
    this->component.configureSend("127.0.0.1", m_port2);
    this->component.startSocketTask(name, 99, 1024);
    EXPECT_EQ(udp.open(), SOCK_SUCCESS);
    // Force the server to close and reopen the client
    this->component.m_socket.close();
    Os::Task::delay((SOCKET_RETRY_INTERVAL_MS * 2) + 10);
    // Wait for it to reconnect if still failed
    transmit(udp);

    this->component.stopSocketTask();
    this->component.joinSocketTask(NULL);
    udp.close();  // Close client
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

void Tester ::from_recv_handler(const NATIVE_INT_TYPE portNum, Fw::Buffer& recvBuffer, RecvStatus recvStatus) {
    this->pushFromPortEntry_recv(recvBuffer, recvStatus);
    // Make sure we can get to unblocking the spinner
    EXPECT_EQ(m_data_buffer.getSize(), recvBuffer.getSize()) << "Invalid transmission size";
    validate_random_buffer(recvBuffer.getData());
    m_spinner = true;
}

// ----------------------------------------------------------------------
// Helper methods
// ----------------------------------------------------------------------

void Tester ::connectPorts(void) {
    // send
    this->connect_to_send(0, this->component.get_send_InputPort(0));

    // recv
    this->component.set_recv_OutputPort(0, this->get_from_recv(0));
}

void Tester ::initComponents(void) {
    this->init();
    this->component.init(INSTANCE);
}

}  // end namespace Drv
