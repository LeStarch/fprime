module Svc {

  active component FaultProtection {

    @ FPP from XML: original path was Svc/FaultProtection/Events.xml
    include "Events.fppi"

    @ FPP from XML: original path was Svc/FaultProtection/Commands.xml
    include "Commands.fppi"

    command reg port CmdReg

    command resp port CmdStatus

    command recv port CmdDisp

    event port Log

    text event port LogText

    time get port Time

    output port seqRun: Svc.CmdSeqIn

    sync input port seqDone: Fw.CmdResponse

    async input port fault: Svc.Fault

  }

}
