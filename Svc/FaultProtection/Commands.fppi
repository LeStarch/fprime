@ Set the fault response for a given fault
async command SET_FAULT_RESPONSE(
                                  faultId: FaultCfg.FaultId @< Fault ID that will trigger the given response
                                  sequencePath: string size 1024 @< Full path to sequence that will handle fault
                                ) \
  opcode 0

@ Set the fault response for a given fault
async command CLEAR_FAULT_RESPONSE(
                                    faultId: FaultCfg.FaultId @< Fault ID that will trigger the given response
                                    newResponse: Svc.ClearFault @< Response once sequence-base response is cleared
                                  ) \
  opcode 1
