// ======================================================================
// \title  BodySensorComponentImpl.hpp
// \author pelmini
// \brief  hpp file for BodySensor component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef BodySensor_HPP
#define BodySensor_HPP

#include "DoorBell/BodySensor/BodySensorComponentAc.hpp"

namespace DoorBell {

  class BodySensorComponentImpl :
    public BodySensorComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object BodySensor
      //!
      BodySensorComponentImpl(
#if FW_OBJECT_NAMES == 1
          const char *const compName /*!< The component name*/
#else
          void
#endif
      );

      //! Initialize object BodySensor
      //!
      void init(
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      //! Destroy object BodySensor
      //!
      ~BodySensorComponentImpl(void);

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for schedIn
      //!
      void schedIn_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          NATIVE_UINT_TYPE context /*!< The call order*/
      );

      unsigned int count;


    };

} // end namespace DoorBell

#endif
