// ======================================================================
// \title  Fw/FPrimeBasicTypes.h
// \author mstarch
// \brief  header file for basic types used in F Prime
//
// \copyright
// Copyright 2025, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// FPrime defines a number of basic types, platform configurable types,
// and project configurable types. This file provides a single header
// users can import for using these types.
//
// This header is intended to be C-compatible.
//
// ======================================================================
#ifndef FPRIME_BASIC_TYPES_H
#define FPRIME_BASIC_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
#include <Fw/Types/BasicTypes.h>
#include <Platform/PlatformTypes.h>
#include <base_config/FpConfig.h>

// ----------------------------------------------------------------------
// Type aliases defined by FPP
// ----------------------------------------------------------------------

#include <base_config/FwAssertArgTypeAliasAc.h>
#include <base_config/FwIndexTypeAliasAc.h>
#include <base_config/FwQueuePriorityTypeAliasAc.h>
#include <base_config/FwSignedSizeTypeAliasAc.h>
#include <base_config/FwSizeTypeAliasAc.h>
#include <base_config/FwTaskIdTypeAliasAc.h>
#include <base_config/FwTaskPriorityTypeAliasAc.h>

#include <base_config/FwChanIdTypeAliasAc.h>
#include <base_config/FwDpIdTypeAliasAc.h>
#include <base_config/FwDpPriorityTypeAliasAc.h>
#include <base_config/FwEnumStoreTypeAliasAc.h>
#include <base_config/FwEventIdTypeAliasAc.h>
#include <base_config/FwIdTypeAliasAc.h>
#include <base_config/FwOpcodeTypeAliasAc.h>
#include <base_config/FwPacketDescriptorTypeAliasAc.h>
#include <base_config/FwPrmIdTypeAliasAc.h>
#include <base_config/FwSizeStoreTypeAliasAc.h>
#include <base_config/FwSizeTypeAliasAc.h>
#include <base_config/FwTimeBaseStoreTypeAliasAc.h>
#include <base_config/FwTimeContextStoreTypeAliasAc.h>
#include <base_config/FwTlmPacketizeIdTypeAliasAc.h>
#include <base_config/FwTraceIdTypeAliasAc.h>

// Backwards naming compatibility.
typedef FwSizeStoreType FwBuffSizeType;
#define PRI_FwBuffSizeType PRI_FwSizeStoreType  // NO_CODESONAR  LANG.PREPROC.MACROSTART/END

#ifdef __cplusplus
}
#endif
#endif  // FPRIME_BASIC_TYPES_H
