module Svc {
    port Fault(
        faultId: FaultCfg.FaultId @< Fault Id
        context: U32 @< Context
    )
}
