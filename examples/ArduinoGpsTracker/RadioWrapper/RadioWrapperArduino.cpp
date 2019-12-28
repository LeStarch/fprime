// ====================================================================== 
// \title  RadioWrapperImpl.cpp
// \author lestarch
// \brief  cpp file for RadioWrapper component implementation class
// ====================================================================== 


#include <examples/ArduinoGpsTracker/RadioWrapper/RadioWrapper.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include <Arduino.h>
#include <Os/Log.hpp>

namespace Arduino {

  void RadioWrapperComponentImpl ::
    init(
        const NATIVE_INT_TYPE instance
    )
  {
      RadioWrapperComponentBase::init(instance);
      m_radio.initialize(RF69_433MHZ, NODEID_REMOTE);
      //m_radio.setHighPower();
  }


  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void RadioWrapperComponentImpl ::
    write_data(
        Fw::Buffer &fwBuffer
    )
  {
	  m_radio.sendWithRetry(NODEID_GROUND, reinterpret_cast<U8*>(fwBuffer.getdata()), fwBuffer.getsize(), 0);
  }

  void RadioWrapperComponentImpl ::
    read_data(Fw::Buffer& fwBuffer)
  {
	  if (m_radio.receiveDone()) {
	  }
      fwBuffer.setsize(m_radio.DATALEN);
  }
} // end namespace Svc
