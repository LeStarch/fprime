// ======================================================================
// \title  RebootResponder.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/RebootResponder/RebootResponder.hpp"

#include <cstdlib>

#include "Fw/Logger/Logger.hpp"
#include "Os/Task.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

RebootResponder ::RebootResponder(const char* const compName) : RebootResponderComponentBase(compName), m_delay(1, 0) {}

RebootResponder ::~RebootResponder() {}

void RebootResponder ::configure(const Fw::TimeInterval& delay) {
    this->m_delay = delay;
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
    (void)Os::Task::delay(this->m_delay);
    this->doReboot();
    // A reboot never returns. Reaching this point means the platform hook declined to reboot.
    Fw::Logger::log("RebootResponder: doReboot returned without rebooting; step failed\n");
    this->faultResponseComplete_out(0, Fw::Success::FAILURE, response, step);
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
