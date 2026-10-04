// ======================================================================
// \title  RebootResponder.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/RebootResponder/RebootResponder.hpp"

#include <cstdlib>

#include "Fw/Logger/Logger.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

RebootResponder ::RebootResponder(const char* const compName)
    : RebootResponderComponentBase(compName),
      m_delay_ticks(DEFAULT_REBOOT_DELAY_TICKS),
      m_ticks_since_request(0),
      m_pending(false),
      m_response(),
      m_step(FaultConfig::Step::SKIP) {}

RebootResponder ::~RebootResponder() {}

void RebootResponder ::configure(FwSizeType delayTicks) {
    this->m_delay_ticks = delayTicks;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void RebootResponder ::faultResponseCancel_handler(FwIndexType portNum) {
    this->log_WARNING_LO_RebootCancelRefused();
}

void RebootResponder ::faultResponseDispatch_handler(FwIndexType portNum,
                                                     const FaultConfig::Response& response,
                                                     const FaultConfig::Step& step,
                                                     const FaultConfig::Context& context) {
    this->log_WARNING_HI_RebootRequested(response, step);
    Fw::Logger::log("RebootResponder: hard reboot requested by response %d step %d\n", response.e, step.e);
    // A reboot already pending is not restarted: the earliest request sets the deadline
    if (not this->m_pending) {
        this->m_pending = true;
        this->m_ticks_since_request = 0;
        this->m_response = response;
        this->m_step = step;
    }
}

void RebootResponder ::run_handler(FwIndexType portNum, U32 context) {
    if (not this->m_pending) {
        return;
    }
    this->m_ticks_since_request++;
    if (this->m_ticks_since_request < this->m_delay_ticks) {
        return;
    }
    this->m_pending = false;
    this->doReboot();
    // A reboot never returns. Reaching this point means the platform hook declined to reboot.
    Fw::Logger::log("RebootResponder: doReboot returned without rebooting; step failed\n");
    this->faultResponseComplete_out(0, Fw::Success::FAILURE, this->m_response, this->m_step);
}

// ----------------------------------------------------------------------
// Reboot hook
// ----------------------------------------------------------------------

void RebootResponder ::doReboot() {
    // Hard termination: no destructors, no flushing. The process supervisor is expected to restart the software.
    _Exit(EXIT_FAILURE);
}

}  // namespace FaultProtection

}  // namespace Svc
