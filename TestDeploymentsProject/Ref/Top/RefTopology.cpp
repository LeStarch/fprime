// ======================================================================
// \title  Topology.cpp
// \author mstarch
// \brief cpp file containing the topology instantiation code
//
// \copyright
// Copyright 2009-2022, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
// ======================================================================

// Provides access to autocoded functions
#include <Ref/Top/RefTopologyAc.hpp>

// Necessary project-specified types
#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/FileNameString.hpp>
#include <Fw/Types/MallocAllocator.hpp>

// Allows easy reference to objects in FPP/autocoder required namespaces
using namespace Ref;

// Size of the buffer each command sequencer loads sequences into
static constexpr FwSizeType SEQUENCER_BUFFER_SIZE = 5 * 1024;
// Directory holding fault response sequences (<directory>/<step name>.seq); separate from the uplink sandbox so that
// uplinked files cannot replace a fault response
// Kept short: CmdSequencer bounds "<directory>/<step>.seq" to FW_CMD_STRING_MAX_SIZE (40) characters
static const char* const FAULT_SEQUENCE_DIRECTORY = "/tmp/fp-seq";
// Interval the asserting thread is parked after a FATAL before the abort fallback; covers the response countdown
// (2 s), the reboot delay (one 0.5 Hz tick), and downlink of the announcement
static const Fw::TimeInterval FATAL_FALLBACK_DELAY(20, 0);
// Rate group 2 ticks (2 s each) between the reboot announcement and the reboot, allowing the announcement to downlink
static constexpr FwSizeType REBOOT_DELAY_TICKS = 1;

// Instantiate a malloc allocator for cmdSeq buffer allocation
Fw::MallocAllocator mallocator;

// The reference topology divides the incoming clock signal (1Hz) into sub-signals: 1Hz, 1/2Hz, and 1/4Hz and
// zero offset for all the dividers
Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{1, 0}, {2, 0}, {4, 0}}};

// Rate groups may supply a context token to each of the attached children whose purpose is set by the project. The
// reference topology sets each token to zero as these contexts are unused in this project.
Svc::ActiveRateGroup::ContextArray rateGroup1Context(0);
Svc::ActiveRateGroup::ContextArray rateGroup2Context(0);
Svc::ActiveRateGroup::ContextArray rateGroup3Context(0);

enum TopologyConstants {
    COMM_PRIORITY = 34,
};

/**
 * \brief configure/setup components in project-specific way
 *
 * This is a *helper* function which configures/sets up each component requiring project specific input. This includes
 * allocating resources, passing-in arguments, etc. This function may be inlined into the topology setup function if
 * desired, but is extracted here for clarity.
 */
void configureTopology() {
    // Rate group driver needs a divisor list
    rateGroupDriverComp.configure(rateGroupDivisorsSet);

    // Rate groups require context arrays. Empty for Reference example.
    rateGroup1Comp.configure(rateGroup1Context);
    rateGroup2Comp.configure(rateGroup2Context);
    rateGroup3Comp.configure(rateGroup3Context);

    // Command sequencer needs to allocate memory to hold contents of command sequences
    cmdSeq.allocateBuffer(0, mallocator, SEQUENCER_BUFFER_SIZE);
    fpSeq.allocateBuffer(0, mallocator, SEQUENCER_BUFFER_SIZE);

    // Fault protection: where response sequences are read from, how long a reboot waits for its announcement to
    // downlink, and how long a FATAL waits for the fault response before falling back to abort
    Svc::FaultProtection::sequenceResponder.configure(Fw::FileNameString(FAULT_SEQUENCE_DIRECTORY));
    Svc::FaultProtection::rebootResponder.configure(REBOOT_DELAY_TICKS);
    CdhCore::fatalHandler.configure(FATAL_FALLBACK_DELAY);

    // Restrict uplinked files to a sandbox directory to prevent path-traversal writes
    FileHandling::fileUplink.configure("/tmp/uplink/");
}

// Public functions for use in main program are namespaced with deployment name Ref
namespace Ref {
void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    connectComponents();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);
    if (state.hostname != nullptr && state.port != 0) {
        comDriver.configure(state.hostname, state.port);
    }
    // Project-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology();
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded parameter loading. Function provided by autocoder.
    loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);
    // Initialize socket client communication if and only if there is a valid specification
    if (state.hostname != nullptr && state.port != 0) {
        Os::TaskString name("ReceiveTask");
        comDriver.start(name, COMM_PRIORITY, Default::STACK_SIZE);
    }
}

void startRateGroups(const Fw::TimeInterval& interval) {
    // This timer drives the fundamental tick rate of the system.
    // Svc::RateGroupDriver will divide this down to the slower rate groups.
    // This call will block until the stopRateGroups() call is made.
    // For this Linux demo, that call is made from a signal handler.
    linuxTimer.startTimer(interval);
}

void stopRateGroups() {
    linuxTimer.quit();
}

void teardownTopology(const TopologyState& state) {
    // Autocoded (active component) task clean-up. Functions provided by topology autocoder.
    stopTasks(state);
    freeThreads(state);

    // Stop the comDriver component, free thread
    comDriver.stop();
    (void)comDriver.join();

    // Resource deallocation
    cmdSeq.deallocateBuffer(mallocator);
    fpSeq.deallocateBuffer(mallocator);
    tearDownComponents(state);
    deinitComponents(state);
}
}  // namespace Ref
