// ======================================================================
// \title  FatalToFault.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component implementation class
// ======================================================================

#include "Svc/FaultProtection/FatalToFault/FatalToFault.hpp"

#include <cstdlib>

#include "Fw/Logger/Logger.hpp"
#include "Os/Task.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FatalToFault ::FatalToFault(const char* const compName)
    : FatalToFaultComponentBase(compName), m_fallback_delay(DEFAULT_FALLBACK_SECONDS, 0) {}

FatalToFault ::~FatalToFault() {}

void FatalToFault ::configure(const Fw::TimeInterval& fallbackDelay) {
    this->m_fallback_delay = fallbackDelay;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FatalToFault ::FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) {
    // Events cannot be relied upon here: the FATAL may have originated in the event path. Log directly.
    Fw::Logger::log("FATAL event 0x%" PRI_FwEventIdType " received: reporting fault FATAL_OCCURRED\n", Id);
    if (this->isConnected_faultOut_OutputPort(0)) {
        this->faultOut_out(0, FaultConfig::Fault::FATAL_OCCURRED);
        // The asserting thread must not resume: park it while the fault response (typically a reboot) runs
        (void)Os::Task::delay(this->m_fallback_delay);
        Fw::Logger::log("FATAL: fault response did not end the software within %" PRIu32 ".%06" PRIu32
                        " s; invoking fallback\n",
                        this->m_fallback_delay.getSeconds(), this->m_fallback_delay.getUSeconds());
    } else {
        Fw::Logger::log("FATAL: faultOut is not connected; invoking fallback immediately\n");
    }
    this->fallback();
}

// ----------------------------------------------------------------------
// Fallback
// ----------------------------------------------------------------------

void FatalToFault ::fallback() {
    abort();
}

}  // namespace FaultProtection

}  // namespace Svc
