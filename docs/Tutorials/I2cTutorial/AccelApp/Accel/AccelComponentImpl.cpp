// ======================================================================
// \title  AccelComponentImpl.cpp
// \author rpk
// \brief  cpp file for Accel component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <AccelApp/Accel/AccelComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"

#define addr 0x53 //define the ADXL 345 I2C address

namespace AccelApp {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  AccelComponentImpl ::
    AccelComponentImpl(
        const char *const compName
    ) : AccelComponentBase(compName)
  {

  }

  void AccelComponentImpl ::
    init(
        const NATIVE_INT_TYPE instance
    )
  {
    AccelComponentBase::init(instance);
  }
  


  AccelComponentImpl ::
    ~AccelComponentImpl(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void AccelComponentImpl ::
    SchedIn_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
    //To do: I2CTutorial
    //set up BW rate, power control, data format based on ADXL 345 datasheet
  
    Fw::Buffer BwRateSetup;
    Fw::Buffer PwrSetup;
    Fw::Buffer FormatSetup;
    unsigned char BwConfig[2];
    unsigned char PwrConfig[2];
    unsigned char FormatConfig[2];
    Drv::I2cStatus i2cStatusBwrate;
    Drv::I2cStatus i2cStatusPwr;
    Drv::I2cStatus i2cStatusFormat;


    BwConfig[0] = 0x2C; //BW register
    BwConfig[1] = 0x0B; //BW set value
    PwrConfig[0] = 0x2D; //Power register
    PwrConfig[1] = 0x08; //Power value
    FormatConfig[0] = 0x31; //Power register
    FormatConfig[1] = 0x0B; //Power value

    BwRateSetup.setSize(2); //set buffer size
    BwRateSetup.setData((U8*)BwConfig); //set data to our buffer

    PwrSetup.setSize(2); //set buffer size
    PwrSetup.setData((U8*)PwrConfig); //set data to our buffer

    FormatSetup.setSize(2); //set buffer size
    FormatSetup.setData((U8*)FormatConfig); //set data to our buffer

    //Write to ADXL345 to set up the device
    i2cStatusBwrate = I2cWrite_out(0,addr,BwRateSetup);
    i2cStatusPwr = I2cWrite_out(0,addr,PwrSetup);
    i2cStatusFormat = I2cWrite_out(0,addr,FormatSetup);

    //Write the status of the writing process to telemetry
    this->tlmWrite_I2C_BwrateStatus(i2cStatusBwrate);
    this->tlmWrite_I2C_PwrStatus(i2cStatusPwr);
    this->tlmWrite_I2C_FormatStatus(i2cStatusFormat);

    //get the sensor reading based on ADXL 345 datasheet
    Fw::Buffer accelRecv;
    Fw::Buffer accelWrite;
    unsigned char accelData[6];
    unsigned char accelDataReg[1];
    char inBuf[1];
    Drv::I2cStatus i2cstatusreadenum;
    Drv::I2cStatus i2cstatuswriteenum;
    accelDataReg[0] = 0x32;

    accelRecv.setSize(6);
    accelWrite.setSize(1);
    accelRecv.setData((U8*)accelData);
    accelWrite.setData((U8*)accelDataReg);

    //Write the registry and read the next 6 registry from 0x32
    //Can be replaced with writeread
    i2cstatuswriteenum = I2cWrite_out(0,addr,accelWrite);
    i2cstatusreadenum = I2cRead_out(0,addr,accelRecv);
    
    //Write the status of writing and reading to the telemetry
    this->tlmWrite_I2C_Write(i2cstatuswriteenum);
    this->tlmWrite_I2C_Read(i2cstatusreadenum);

    //Data format
    short x = accelData[1]<<8 | accelData[0];
    short y = accelData[3]<<8 | accelData[2];
    short z = accelData[5]<<8 | accelData[4];

    x = (float)x * 0.004*9.8;
    y = (float)y * 0.004*9.8;
    z = (float)z * 0.004*9.8;

    //Write the sensor reading to the telemetry
    this->tlmWrite_Accel_X(x);
    this->tlmWrite_Accel_Y(y);
    this->tlmWrite_Accel_Z(z);
  }

} // end namespace AccelApp
