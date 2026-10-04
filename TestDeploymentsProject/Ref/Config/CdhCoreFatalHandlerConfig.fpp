module CdhCore {

    @ The Ref deployment handles FATALs as faults: FATAL_OCCURRED is reported to the FaultManager
    instance fatalHandler: Svc.FaultProtection.FatalToFault base id CdhCoreConfig.BASE_ID + 0x07000

}
