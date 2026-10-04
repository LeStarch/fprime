// ======================================================================
// \title  RebootResponder.hpp
// \author mstarch
// \brief  hpp file for RebootResponder component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_RebootResponder_HPP
#define Svc_FaultProtection_RebootResponder_HPP

#include "Fw/Time/TimeInterval.hpp"
#include "Svc/FaultProtection/RebootResponder/RebootResponderComponentAc.hpp"

namespace Svc {

namespace FaultProtection {

class RebootResponder : public RebootResponderComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct RebootResponder object
    RebootResponder(const char* const compName  //!< The component name
    );

    //! Destroy RebootResponder object
    virtual ~RebootResponder();

    //! Configure the delay between announcing and performing the reboot (allows the announcement to downlink)
    void configure(const Fw::TimeInterval& delay);

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
    //! Announces, delays, and performs the reboot
    void faultResponseDispatch_handler(FwIndexType portNum,                    //!< The port number
                                       const FaultConfig::Response& response,  //!< Active fault response
                                       const FaultConfig::Step& step,          //!< Step of the active fault response
                                       const FaultConfig::Context& context     //!< Context for the step
                                       ) override;

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    Fw::TimeInterval m_delay;  //!< Delay between announcement and reboot
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
