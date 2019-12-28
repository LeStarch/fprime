// ====================================================================== 
// \title  RadioWrapperImpl.cpp
// \author lestarch
// \brief  cpp file for RadioWrapper component implementation class
// ====================================================================== 


#include <examples/Arduino/RadioWrapper/RadioWrapper.hpp>
#include "Fw/Types/BasicTypes.hpp"

namespace Arduino {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction 
  // ----------------------------------------------------------------------

  RadioWrapperComponentImpl ::
    RadioWrapperComponentImpl(
#if FW_OBJECT_NAMES == 1
        const char *const compName
#endif
    ) :
      RadioWrapperComponentBase(compName),
#ifdef ARDUINO
	  m_radio(15, RF69_IRQ_PIN, true),
      m_local_buffer(0xfeedfeed, 0xdeeddeed, reinterpret_cast<POINTER_CAST>(m_radio.DATA), RF69_MAX_DATA_LEN)
#else
  	  m_local_buffer(0xfeedfeed, 0xdeeddeed, reinterpret_cast<POINTER_CAST>(m_data), sizeof(m_data))
#endif
  {

  }

  RadioWrapperComponentImpl ::
    ~RadioWrapperComponentImpl(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void RadioWrapperComponentImpl ::
    write_handler(
        const NATIVE_INT_TYPE portNum,
        Fw::Buffer &fwBuffer
    )
  {
      write_data(fwBuffer);
  }

  void RadioWrapperComponentImpl ::
    readPoll_handler(
        const NATIVE_INT_TYPE portNum,
        Fw::Buffer &fwBuffer
    )
  {
      read_data(fwBuffer);
  }

  void RadioWrapperComponentImpl ::
    schedIn_handler(
        const NATIVE_INT_TYPE portNum, /*!< The port number*/
        NATIVE_UINT_TYPE context /*!< The call order*/
    )
  {
      m_local_buffer.setsize(SERIAL_BUFFER_SIZE);
      read_data(m_local_buffer);
      if (m_local_buffer.getsize() > 0) {
          readCallback_out(0, m_local_buffer);
      }
  }

} // end namespace Svc
