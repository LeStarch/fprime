// ======================================================================
// \title  SequenceResponder.hpp
// \author mstarch
// \brief  hpp file for SequenceResponder component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_SequenceResponder_HPP
#define Svc_FaultProtection_SequenceResponder_HPP

#include "Fw/Types/FileNameString.hpp"
#include "Os/Mutex.hpp"
#include "Svc/FaultProtection/SequenceResponder/SequenceResponderComponentAc.hpp"

namespace Svc {

namespace FaultProtection {

class SequenceResponder final : public SequenceResponderComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct SequenceResponder object
    SequenceResponder(const char* const compName  //!< The component name
    );

    //! Destroy SequenceResponder object
    ~SequenceResponder();

    //! Configure the directory containing the step sequences (`<directory>/<step name>.seq`). Asserts when the
    //! directory does not fit in a file name string.
    void configure(const Fw::StringBase& directory);

    //! Build the sequence file name for a step. Returns false when the name does not fit in `fileName`.
    bool sequenceFileName(const FaultConfig::Step& step, Fw::FileNameString& fileName) const;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for faultResponseCancel
    //!
    //! Cancels the running step's sequence
    void faultResponseCancel_handler(FwIndexType portNum  //!< The port number
                                     ) override;

    //! Handler implementation for faultResponseDispatch
    //!
    //! Runs the step's sequence on the connected sequencer
    void faultResponseDispatch_handler(FwIndexType portNum,                    //!< The port number
                                       const FaultConfig::Response& response,  //!< Active fault response
                                       const FaultConfig::Step& step,          //!< Step of the active fault response
                                       const FaultConfig::Context& context     //!< Context for the step
                                       ) override;

    //! Handler implementation for seqDoneIn
    //!
    //! Forwards sequence completion as step completion
    void seqDoneIn_handler(FwIndexType portNum,             //!< The port number
                           FwOpcodeType opCode,             //!< Command Op Code
                           U32 cmdSeq,                      //!< Command Sequence
                           const Fw::CmdResponse& response  //!< The command response argument
                           ) override;

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    Fw::FileNameString m_directory;           //!< Directory containing step sequences
    bool m_active;                            //!< A step's sequence is running
    FaultConfig::Response m_active_response;  //!< Response of the running step
    FaultConfig::Step m_active_step;          //!< Running step
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
