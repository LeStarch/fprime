// ====================================================================== 
// \title  RadioWrapperImpl.hpp
// \author lestarch
// \brief  hpp file for RadioWrapper component implementation class
// ====================================================================== 

#ifndef RadioWrapper_HPP
#define RadioWrapper_HPP

#include "examples/Arduino/RadioWrapper/RadioWrapperComponentAc.hpp"
#include "examples/Arduino/RFM69/RFM69.h"

#define NODEID_GROUND 0
#define NODEID_REMOTE 1
#define RADIO_RETRIES 0

namespace Arduino {

  class RadioWrapperComponentImpl :
    public RadioWrapperComponentBase
  {

    public:
      const static NATIVE_UINT_TYPE SERIAL_BUFFER_SIZE = 100; //115200 / 10 / 10 + 1;
      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object RadioWrapper
      //!
      RadioWrapperComponentImpl(
#if FW_OBJECT_NAMES == 1
          const char *const compName /*!< The component name*/
#endif
      );

      //! Initialize object RadioWrapper
      //!
      void init(
          const NATIVE_INT_TYPE instance /*!< The instance number*/
      );

      //! Destroy object RadioWrapper
      //!
      ~RadioWrapperComponentImpl(void);

    PRIVATE:
      //! Read the actual data
      void read_data(Fw::Buffer &fwBuffer);
      //! Write the actual data
      void write_data(Fw::Buffer &fwBuffer);
      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for write
      //!
      void write_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          Fw::Buffer &fwBuffer 
      );

      //! Handler implementation for readPoll
      //!
      void readPoll_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          Fw::Buffer &fwBuffer 
      );

      //! Handler implementation for schedIn
      //!
      void schedIn_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          NATIVE_UINT_TYPE context /*!< The call order*/
      );
#ifdef ARDUINO
      RFM69 m_radio;
#else
      //! Data store for self-generated serial data
      U8 m_data[61]; // Max data in 1/10th second at 115200
#endif
      //! Buffer to wrap
      Fw::Buffer m_local_buffer;
    };

} // end namespace Arduino

#endif
