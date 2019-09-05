// ======================================================================
// \title  LinuxI2cDriverComponentImpl.cpp
// \author tcanham
// \brief  cpp file for LinuxI2cDriver component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <Drv/LinuxI2cDriver/LinuxI2cDriverComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include "Fw/Types/Assert.hpp"

#define DEBUG_PRINT 0

namespace Drv {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  LinuxI2cDriverComponentImpl ::
#if FW_OBJECT_NAMES == 1
    LinuxI2cDriverComponentImpl(
        const char *const compName
    ) :
      LinuxI2cDriverComponentBase(compName)
#else
    LinuxI2cDriverComponentImpl(void)
#endif
	,m_fd(-1)
  {

  }

  void LinuxI2cDriverComponentImpl ::
    init(
        const NATIVE_INT_TYPE instance
    )
  {
    LinuxI2cDriverComponentBase::init(instance);
  }

  LinuxI2cDriverComponentImpl ::
    ~LinuxI2cDriverComponentImpl(void)
  {

  }

  void LinuxI2cDriverComponentImpl::open(const char* device) {
  }


  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  // Note this this port handler is guarded, so we can make the ioctl call

  void LinuxI2cDriverComponentImpl ::
    write_handler(
        const NATIVE_INT_TYPE portNum,
        U32 addr,
        Fw::Buffer &serBuffer
    )
  {
  }
} // end namespace Drv
