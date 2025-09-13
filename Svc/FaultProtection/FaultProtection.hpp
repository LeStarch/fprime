// ======================================================================
// \title  FaultProtection.hpp
// \author mstarch
// \brief  hpp file for FaultProtection component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef FaultProtection_HPP
#define FaultProtection_HPP

#include "Os/Mutex.hpp"
#include "Svc/FaultProtection/FaultProtectionComponentAc.hpp"

namespace Svc {

class FaultProtection : public FaultProtectionComponentBase {
  public:
    constexpr static const char* const NO_SEQUENCE = "";
    constexpr static const char* const NO_RESPONSE = "** RESPONSE DISABLED **";

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object FaultProtection
    //!
    FaultProtection(const char* const compName /*!< The component name*/
    );

    //! Destroy object FaultProtection
    //!
    ~FaultProtection(void);

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for seqDone
    //!
    void seqDone_handler(const FwIndexType portNum,      /*!< The port number*/
                         FwOpcodeType opCode,            /*!< Command Op Code*/
                         U32 cmdSeq,                     /*!< Command Sequence*/
                         const Fw::CmdResponse& response /*!< The command response argument*/
                         ) override;

    //! Handler implementation for fault
    //!
    void fault_handler(const FwIndexType portNum, /*!< The port number*/
                       const FaultCfg::FaultId& identifier,
                       U32 context) override;

    //! Helper converting fault to table index
    //!
    U32 fault_to_index(FaultCfg::FaultId fault);

    // ----------------------------------------------------------------------
    // Command handler implementations
    // ----------------------------------------------------------------------

    //! Implementation for SET_FAULT_RESPONSE command handler
    //! Set the fault response for a given fault
    void SET_FAULT_RESPONSE_cmdHandler(
        const FwOpcodeType opCode,           /*!< The opcode*/
        const U32 cmdSeq,                    /*!< The command sequence number*/
        FaultCfg::FaultId faultId,           /*!< Fault ID that will trigger the given response*/
        const Fw::CmdStringArg& sequencePath /*!< Full path to sequence that will handle fault*/
        ) override;

    //! Implementation for CLEAR_FAULT_RESPONSE command handler
    //! Set the fault response for a given fault
    void CLEAR_FAULT_RESPONSE_cmdHandler(
        const FwOpcodeType opCode,  /*!< The opcode*/
        const U32 cmdSeq,           /*!< The command sequence number*/
        FaultCfg::FaultId faultId,  /*!< Fault ID that will trigger the given response*/
        Svc::ClearFault newResponse /*!< Response once sequence-base response is cleared*/
        ) override;

    Os::Mutex m_lock;
    Fw::String& m_current_sequence;
    Fw::String m_response_table[FaultCfg::FaultId::NUM_CONSTANTS];
    bool m_running;
};

}  // end namespace Svc

#endif
