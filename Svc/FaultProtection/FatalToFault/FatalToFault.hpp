// ======================================================================
// \title  FatalToFault.hpp
// \author mstarch
// \brief  hpp file for FatalToFault component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_FatalToFault_HPP
#define Svc_FaultProtection_FatalToFault_HPP

#include "Fw/Time/TimeInterval.hpp"
#include "Svc/FaultProtection/FatalToFault/FatalToFaultComponentAc.hpp"

namespace Svc {

namespace FaultProtection {

class FatalToFault : public FatalToFaultComponentBase {
  public:
    //! Default interval between the FATAL report and the fallback (seconds)
    static constexpr U32 DEFAULT_FALLBACK_SECONDS = 10;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FatalToFault object
    FatalToFault(const char* const compName  //!< The component name
    );

    //! Destroy FatalToFault object
    virtual ~FatalToFault();

    //! Configure the interval the asserting thread is parked after reporting a FATAL before invoking the fallback
    void configure(const Fw::TimeInterval& fallbackDelay);

  protected:
    //! Fallback invoked when the fault response has not ended the software within the configured interval.
    //! The default aborts the process; override for platform-specific behavior.
    virtual void fallback();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for FatalReceive
    //!
    //! Reports FATAL_OCCURRED, parks the calling thread for the fallback interval, then invokes the fallback
    void FatalReceive_handler(FwIndexType portNum,  //!< The port number
                              FwEventIdType Id      //!< The ID of the FATAL event
                              ) override;

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    Fw::TimeInterval m_fallback_delay;  //!< Interval between the FATAL report and the fallback
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
