// ======================================================================
// \title  RebootResponder.hpp
// \author mstarch
// \brief  hpp file for RebootResponder component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_RebootResponder_HPP
#define Svc_FaultProtection_RebootResponder_HPP

#include "Svc/FaultProtection/RebootResponder/RebootResponderComponentAc.hpp"

namespace Svc {

namespace FaultProtection {

class RebootResponder : public RebootResponderComponentBase {
  public:
    //! Default number of `run` ticks between the reboot announcement and the reboot
    static constexpr FwSizeType DEFAULT_REBOOT_DELAY_TICKS = 1;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct RebootResponder object
    RebootResponder(const char* const compName  //!< The component name
    );

    //! Destroy RebootResponder object
    virtual ~RebootResponder();

    //! Configure the number of `run` ticks between announcing and performing the reboot (allows the announcement to
    //! downlink)
    void configure(FwSizeType delayTicks);

  protected:
    //! Perform the hard reboot. The default terminates the process immediately; override for platform resets.
    //! If this returns, the step is reported as failed.
    virtual void doReboot();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for faultResponseCancel
    //!
    //! Reboots are irrevocable: the request is refused
    void faultResponseCancel_handler(FwIndexType portNum  //!< The port number
                                     ) override;

    //! Handler implementation for faultResponseDispatch
    //!
    //! Announces the reboot and arms the delay
    void faultResponseDispatch_handler(FwIndexType portNum,                    //!< The port number
                                       const FaultConfig::Response& response,  //!< Active fault response
                                       const FaultConfig::Step& step,          //!< Step of the active fault response
                                       const FaultConfig::Context& context     //!< Context for the step
                                       ) override;

    //! Handler implementation for run
    //!
    //! Performs the reboot once the delay has elapsed
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    FwSizeType m_delay_ticks;          //!< Ticks between announcement and reboot
    FwSizeType m_ticks_since_request;  //!< Ticks elapsed since the reboot was requested
    bool m_pending;                    //!< A reboot has been requested and awaits its delay
    FaultConfig::Response m_response;  //!< Response that requested the reboot
    FaultConfig::Step m_step;          //!< Step that requested the reboot
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
