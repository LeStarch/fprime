// ======================================================================
// \title  FatalToFault.hpp
// \author mstarch
// \brief  hpp file for FatalToFault component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_FatalToFault_HPP
#define Svc_FaultProtection_FatalToFault_HPP

#include <atomic>

#include "Svc/FaultProtection/FatalToFault/FatalToFaultComponentAc.hpp"

namespace Svc {

namespace FaultProtection {

class FatalToFault : public FatalToFaultComponentBase {
  public:
    //! Default number of `run` ticks between the first FATAL and the fallback
    static constexpr FwSizeType DEFAULT_FALLBACK_TICKS = 10;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FatalToFault object
    FatalToFault(const char* const compName  //!< The component name
    );

    //! Destroy FatalToFault object
    virtual ~FatalToFault();

    //! Configure the number of `run` ticks to wait after a FATAL before invoking the fallback
    void configure(FwSizeType fallbackTicks);

  protected:
    //! Fallback invoked when the fault response has not ended the software within the configured ticks.
    //! The default aborts the process; override for platform-specific behavior.
    virtual void fallback();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for FatalReceive
    //!
    //! Reports FATAL_OCCURRED and arms the fallback countdown
    void FatalReceive_handler(FwIndexType portNum,  //!< The port number
                              FwEventIdType Id      //!< The ID of the FATAL event
                              ) override;

    //! Handler implementation for run
    //!
    //! Counts down to the fallback once armed
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    FwSizeType m_fallback_ticks;     //!< Configured ticks between FATAL and fallback
    std::atomic<bool> m_armed;       //!< A FATAL has been received (set from the FATAL thread, read on the tick)
    FwSizeType m_ticks_since_fatal;  //!< Ticks elapsed since arming
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
