// ======================================================================
// \title  FaultProtection/test/ut/Tester.hpp
// \author mstarch
// \brief  hpp file for FaultProtection test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef TESTER_HPP
#define TESTER_HPP

#include "GTestBase.hpp"
#include "Svc/FaultProtection/FaultProtectionComponentImpl.hpp"

namespace Svc {

class Tester : public FaultProtectionGTestBase {
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

  public:
    //! Construct object Tester
    //!
    Tester(void);

    //! Destroy object Tester
    //!
    ~Tester(void);

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    U32 invoke_fault(Project::FaultId fault);

    void test_sequence_helper(const char* response, Project::FaultId fault);

    void test_fatal_helper(Project::FaultId fault);

    void test_no_response_helper(Project::FaultId fault);

    //! Test FATAL responses
    //!
    void test_fatal_response();

    //! Test no responses
    //!
    void test_no_response();

    //! Test sequence response
    //!
    void test_sequence_response();

    //! Test clear to FATAL response
    //!
    void test_clear_to_fatal_response();

    //! Test clear to no response
    //!
    void test_clear_to_no_response();

    //! Test reset to sequence
    //!
    void test_reset_response();

    //! Test serialized response
    //!
    void test_serialized_response();

    //! Test FATAL on bad sequence
    //!
    void test_fatal_on_bad_sequence();

    //! Test FATAL on failed sequence
    //!
    void test_fatal_on_failed_sequence();

  private:
    // ----------------------------------------------------------------------
    // Handlers for typed from ports
    // ----------------------------------------------------------------------

    //! Handler for from_seqRun
    //!
    void from_seqRun_handler(const NATIVE_INT_TYPE portNum, /*!< The port number*/
                             Fw::String& filename           /*!<
                                   The sequence file
                                   */
    );

  private:
    // ----------------------------------------------------------------------
    // Helper methods
    // ----------------------------------------------------------------------

    //! Connect ports
    //!
    void connectPorts(void);

    //! Initialize components
    //!
    void initComponents(void);

  private:
    // ----------------------------------------------------------------------
    // Variables
    // ----------------------------------------------------------------------

    //! The component under test
    //!
    FaultProtectionComponentImpl component;
    bool m_force_error;
};

}  // end namespace Svc

#endif
