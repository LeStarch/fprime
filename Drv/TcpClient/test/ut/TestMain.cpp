// ----------------------------------------------------------------------
// TestMain.cpp
// ----------------------------------------------------------------------

#include "Tester.hpp"

TEST(Nominal, BasicMessaging) {
    Drv::Tester tester;
    tester.test_basic_messaging();
}

TEST(Reconnect, BasicReconnect) {
    Drv::Tester tester;
    tester.test_basic_reconnect();
}

TEST(Reconnect, AdvancedReconnect) {
    Drv::Tester tester;
    tester.test_advanced_reconnect();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
