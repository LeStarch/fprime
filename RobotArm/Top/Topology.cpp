#include <Components.hpp>

#include <Fw/Types/Assert.hpp>
#include <Os/Task.hpp>
#include <Os/Log.hpp>
#include <Os/File.hpp>
#include <Fw/Types/MallocAllocator.hpp>

enum {
    DOWNLINK_PACKET_SIZE = 500,
    DOWNLINK_BUFFER_STORE_SIZE = 2500,
    DOWNLINK_BUFFER_QUEUE_SIZE = 5,
    UPLINK_BUFFER_STORE_SIZE = 3000,
    UPLINK_BUFFER_QUEUE_SIZE = 30
};

// Component instances

// Rate Group Dividers for 1Hz

static NATIVE_INT_TYPE rgDivs[3] = {1,1,1};
Svc::RateGroupDriverImpl rateGroupDriverComp("RGDRV",rgDivs,FW_NUM_ARRAY_ELEMENTS(rgDivs));

static NATIVE_UINT_TYPE rg1HzContext[10] = {0,};

Svc::ActiveRateGroupImpl rateGroup1HzComp("RG1Hz",rg1HzContext,FW_NUM_ARRAY_ELEMENTS(rg1HzContext));

// Command Components
Svc::GroundInterfaceComponentImpl groundIf("GNDIF");
Drv::SocketIpDriverComponentImpl socketIpDriver("SocketIpDriver");

#if FW_ENABLE_TEXT_LOGGING
Svc::ConsoleTextLoggerImpl textLogger("TLOG");
#endif

Svc::ActiveLoggerImpl eventLogger("ELOG");

Svc::LinuxTimeImpl linuxTime("LTIME");

Svc::LinuxTimerComponentImpl linuxTimer("LTIMER");

Svc::TlmChanImpl chanTlm("TLM");

Svc::CommandDispatcherImpl cmdDisp("CMDDISP");

Svc::PrmDbImpl prmDb("PRM","PrmDb.dat");

Svc::FileUplink fileUplink("fileUplink");

Svc::FileDownlink fileDownlink ("fileDownlink", DOWNLINK_PACKET_SIZE);

Svc::BufferManager fileDownlinkBufferManager("fileDownlinkBufferManager", DOWNLINK_BUFFER_STORE_SIZE, DOWNLINK_BUFFER_QUEUE_SIZE);

Svc::BufferManager fileUplinkBufferManager("fileUplinkBufferManager", UPLINK_BUFFER_STORE_SIZE, UPLINK_BUFFER_QUEUE_SIZE);

Svc::HealthImpl health("health");

// This needs to be statically allocated
Fw::MallocAllocator seqMallocator;

Svc::CmdSequencerComponentImpl cmdSeq("CMDSEQ");

Svc::AssertFatalAdapterComponentImpl fatalAdapter("fatalAdapter");

Svc::FatalHandlerComponentImpl fatalHandler("fatalHandler");

// i2c driver
Drv::LinuxI2cDriverComponentImpl i2cDrv("i2cDrv");

// servo controllers
RobotArm::PcaServoComponentImpl clawServo("clawServo");
RobotArm::PcaServoComponentImpl baseServo("baseServo");
RobotArm::PcaServoComponentImpl armHeightServo("armServo");
RobotArm::PcaServoComponentImpl armLengthServo("clawTiltServo");

// arm demo component
RobotArm::ArmDemoComponentImpl armDemo("armDemo");

void constructApp(int port_number, char* hostname) {

    // Initialize rate group driver
    rateGroupDriverComp.init();

    // Initialize the rate groups
    rateGroup1HzComp.init(10,1);
    
#if FW_ENABLE_TEXT_LOGGING
    textLogger.init();
#endif

    eventLogger.init(10,0);

    linuxTime.init(0);

    linuxTimer.init(0);

    chanTlm.init(10,0);

    cmdDisp.init(20,0);

    cmdSeq.init(10,0);
    cmdSeq.allocateBuffer(0,seqMallocator,5*1024);

    groundIf.init(0);
    socketIpDriver.init(0);
    
    prmDb.init(10,0);
    fileUplink.init(30, 0);
    fileDownlink.init(30, 0);
    fileUplinkBufferManager.init(0);
    fileDownlinkBufferManager.init(1);
    health.init(25,0);


    fatalAdapter.init(0);
    fatalHandler.init(0);

    i2cDrv.init(0);

    clawServo.init(10,0);
    baseServo.init(10,0);
    armHeightServo.init(10,0);
    armLengthServo.init(10,0);

    armDemo.init(10,0);

    constructRobotArmArchitecture();

    /* Register commands */
    cmdSeq.regCommands();
    cmdDisp.regCommands();
    eventLogger.regCommands();
    prmDb.regCommands();
    fileDownlink.regCommands();
    health.regCommands();

    armDemo.regCommands();

    clawServo.regCommands();
    baseServo.regCommands();
    armHeightServo.regCommands();
    armLengthServo.regCommands();

    // open i2c driver
    i2cDrv.open("/dev/i2c-1");

    // configure servos

    // set servo instance for each servo
    clawServo.configure(0,0x40);
    baseServo.configure(1,0x40);
    armHeightServo.configure(2,0x40);
    armLengthServo.configure(3,0x40);
    // configure card with one of the servos
    clawServo.configChip();

    cmdSeq.setTimeout(10);
    // read parameters
    prmDb.readParamFile();

    // set health ping entries

    // This list has to match the connections in RPITopologyAppAi.xml

    Svc::HealthImpl::PingEntry pingEntries[] = {
        {3,5,rateGroup1HzComp.getObjName()}, // 0
        {3,5,cmdDisp.getObjName()}, // 1
        {3,5,cmdSeq.getObjName()}, // 2
        {3,5,chanTlm.getObjName()}, // 3
        {3,5,eventLogger.getObjName()}, // 4
        {3,5,prmDb.getObjName()}, // 5
        {3,5,fileDownlink.getObjName()}, // 6
        {3,5,fileUplink.getObjName()}, // 7
    };

    // register ping table
    health.setPingEntries(pingEntries,FW_NUM_ARRAY_ELEMENTS(pingEntries),0x123);


    // start rate groups
    rateGroup1HzComp.start(0, 119,10 * 1024);
    // start dispatcher
    cmdDisp.start(0,101,10*1024);
    // start sequencer
    cmdSeq.start(0,100,10*1024);
    // start telemetry
    eventLogger.start(0,98,10*1024);
    chanTlm.start(0,97,10*1024);
    
    prmDb.start(0,96,10*1024);

    fileDownlink.start(0, 100, 10*1024);
    fileUplink.start(0, 100, 10*1024);

    armDemo.start(0, 100, 10*1024);

    clawServo.start(0, 100, 10*1024);
    baseServo.start(0, 100, 10*1024);
    armHeightServo.start(0, 100, 10*1024);
    armLengthServo.start(0, 100, 10*1024);

    // Initialize socket server
    if (hostname != NULL && port_number != 0) {
        socketIpDriver.startSocketTask(100, 10 * 1024, hostname, port_number);
    }

}

void exitTasks(void) {
    linuxTimer.quit();
    rateGroup1HzComp.exit();
    cmdDisp.exit();
    eventLogger.exit();
    chanTlm.exit();
    cmdSeq.exit();
    prmDb.exit();
    fileUplink.exit();
    fileDownlink.exit();

    clawServo.exit();
    baseServo.exit();
    armHeightServo.exit();
    armLengthServo.exit();

    armDemo.exit();
}

