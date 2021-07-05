// ======================================================================
// \title  AccelComponentImpl.hpp
// \author rpk
// \brief  hpp file for Accel component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef Accel_HPP
#define Accel_HPP

#include "AccelApp/Accel/AccelComponentAc.hpp"

namespace AccelApp {

  class AccelComponentImpl :
    public AccelComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object Accel
      //!
      AccelComponentImpl(
          const char *const compName /*!< The component name*/
      );

      //! Initialize object Accel
      //!
      void init(
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      void preamble(void);
      
      //! Destroy object Accel
      //!
      ~AccelComponentImpl(void);

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for SchedIn
      //!
      void SchedIn_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          NATIVE_UINT_TYPE context /*!< The call order*/
      );


    };

} // end namespace AccelApp

#endif
