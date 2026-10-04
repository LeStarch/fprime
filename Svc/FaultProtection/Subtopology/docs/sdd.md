# Svc::FaultProtection::Subtopology

The fault protection subtopology instantiates the `FaultManager` with the two framework responders and exposes the
ports a deployment connects to put fault management in service.

## 1. Instances

| Instance            | Component                            | Base ID offset | Kind                                   |
| ------------------- | ------------------------------------ | -------------- | -------------------------------------- |
| `faultManager`      | `Svc.FaultProtection.FaultManager`   | `+0x4000`      | active (queue, stack, priority from `FaultProtectionSubtopologyConfig.fpp`) |
| `sequenceResponder` | `Svc.FaultProtection.SequenceResponder` | `+0x2000`   | passive                                |
| `rebootResponder`   | `Svc.FaultProtection.RebootResponder` | `+0x3000`     | passive                                |

Offsets are relative to `Svc.FaultProtection.BASE_ID` (`FaultProtectionSubtopologyConfig.fpp`, `0x0F000000` by
default). The `Responders` connection graph wires `faultManager.stepDispatchOut`/`stepCancelOut` at
`FaultConfig.Port.SEQUENCE_RESPONDER_PORT` and `REBOOT_RESPONDER_PORT` to the respective responder and both
`faultResponseComplete` ports back to `faultManager.stepCompletionIn`.

## 2. Exposed ports

| Port                | Direction | Type                          | Connect to                                                      |
| ------------------- | --------- | ----------------------------- | --------------------------------------------------------------- |
| `reportIn`          | input     | `FaultProtection.FaultReport` | the `faultOut` port of every fault reporter (`Reporter` interface), including the FATAL handler |
| `faultManagerRun`   | input     | `Svc.Sched`                   | a rate group; one call is one FaultManager tick (countdown, step timeouts, latch audit) |
| `rebootResponderRun`| input     | `Svc.Sched`                   | a rate group; one call is one tick of the reboot delay          |
| `seqRunOut`         | output    | `Svc.CmdSeqIn`                | `seqRunIn` of a `Svc.CmdSequencer` dedicated to fault responses |
| `seqCancelOut`      | output    | `Svc.CmdSeqCancel`            | `seqCancelIn` of that sequencer                                 |
| `seqDoneIn`         | input     | `Fw.CmdResponse`              | `seqDone` of that sequencer                                     |

`FaultConfig.RESPONSE_COUNTDOWN_TICKS`, the `timeoutTicks` of the step table, and the `RebootResponder` delay are
counted in calls of the respective run port: choose the rate groups accordingly.

The rate groups driving `faultManagerRun` and `rebootResponderRun` must stay alive while a fault response runs. In
particular, `FatalToFault` parks the asserting thread; a FATAL raised by a synchronous member of the rate group that
drives either port stalls the ordered reboot, which then ends through `FatalToFault`'s fallback instead (see the
[FatalToFault SDD](../../FatalToFault/docs/sdd.md)). Prefer a rate group whose other members are active (queued)
components, or a dedicated one.

## 3. Deployment wiring

1. Instantiate the subtopology and connect the ports above (the Ref deployment: `TestDeploymentsProject/Ref/Top/topology.fpp`).
2. Give fault responses their own `Svc.CmdSequencer` instance (Ref: `fpSeq`) so that ground sequences cannot block
   a response, and call `SequenceResponder::configure(<directory>)` with the directory holding the compiled
   `<step name>.seq` files (Ref: `RefTopology.cpp`).
3. Route FATALs into fault management by overriding `CdhCoreFatalHandlerConfig.fpp` to instantiate
   `Svc.FaultProtection.FatalToFault` as CdhCore's `fatalHandler` and connecting `CdhCore.fatalHandler.faultOut ->
   Svc.FaultProtection.Subtopology.reportIn` (Ref: `TestDeploymentsProject/Ref/Config`).
4. Provide the deployment's `FaultConfig.fpp` (faults, responses, steps, and tables) as a configuration override.
