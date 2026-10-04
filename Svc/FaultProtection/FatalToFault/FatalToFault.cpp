// ======================================================================
// \title  FatalToFault.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component implementation class
// ======================================================================

#include "Svc/FaultProtection/FatalToFault/FatalToFault.hpp"

#include <cstdlib>

#include "Fw/Logger/Logger.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FatalToFault ::FatalToFault(const char* const compName)
    : FatalToFaultComponentBase(compName),
      m_fallback_ticks(DEFAULT_FALLBACK_TICKS),
      m_armed(false),
      m_ticks_since_fatal(0) {}

FatalToFault ::~FatalToFault() {}

void FatalToFault ::configure(FwSizeType fallbackTicks) {
    this->m_fallback_ticks = fallbackTicks;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FatalToFault ::FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) {
    // Events cannot be relied upon here: the FATAL may have originated in the event path. Log directly.
    Fw::Logger::log("FATAL event 0x%" PRI_FwEventIdType " received: reporting fault FATAL_OCCURRED\n", Id);
    if (this->isConnected_faultOut_OutputPort(0)) {
        this->faultOut_out(0, FaultConfig::Fault::FATAL_OCCURRED);
        this->m_armed = true;
    } else {
        Fw::Logger::log("FATAL: faultOut is not connected; invoking fallback immediately\n");
        this->fallback();
    }
}

void FatalToFault ::run_handler(FwIndexType portNum, U32 context) {
    if (not this->m_armed) {
        return;
    }
    this->m_ticks_since_fatal++;
    if (this->m_ticks_since_fatal >= this->m_fallback_ticks) {
        Fw::Logger::log("FATAL: fault response did not end the software within %" PRI_FwSizeType
                        " ticks; invoking fallback\n",
                        this->m_fallback_ticks);
        this->fallback();
    }
}

// ----------------------------------------------------------------------
// Fallback
// ----------------------------------------------------------------------

void FatalToFault ::fallback() {
    abort();
}

}  // namespace FaultProtection

}  // namespace Svc
