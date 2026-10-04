# Svc::FaultProtection::RebootResponder

Reboots the flight software in response to a fault response step.

## 1. Requirements

| ID                        | Description (shall)                                                                                                   | Verification |
| ------------------------- | --------------------------------------------------------------------------------------------------------------------- | ------------ |
| SVC_REBOOTRESPONDER_001   | RebootResponder shall emit an event announcing the reboot when a step is dispatched to it.                             | Unit-Test    |
| SVC_REBOOTRESPONDER_002   | RebootResponder shall reboot a configurable number of `run` ticks after the request to allow the announcement to downlink. | Unit-Test    |
| SVC_REBOOTRESPONDER_003   | RebootResponder shall perform the reboot through an overridable hook defaulting to hard process termination.           | Unit-Test    |
| SVC_REBOOTRESPONDER_004   | RebootResponder shall complete the step with failure if the reboot hook returns.                                        | Unit-Test    |
| SVC_REBOOTRESPONDER_005   | RebootResponder shall refuse cancellation of a requested reboot.                                                        | Unit-Test    |
| SVC_REBOOTRESPONDER_006   | RebootResponder shall not block the dispatching thread.                                                                | Unit-Test    |

## 2. Design

`RebootResponder` is a `passive` component implementing the `SyncResponder` interface. On dispatch it emits
`RebootRequested`, records the pending request, and returns immediately so that the `FaultManager` queue is not
blocked. Each `run` tick advances the pending request; after `m_delay_ticks` ticks the `virtual doReboot()` hook is
called. The default hook calls `_Exit` so the process supervisor restarts the software; platforms with a hardware
reset override `doReboot()` in a derived class. A reboot never returns, so the step never completes; if the hook
does return, the step completes with `Fw::Success::FAILURE` so the `FaultManager` can escalate per the step's
failure mode. A second dispatch while a request is pending is announced but does not restart the delay.

Cancellation is refused (`RebootCancelRefused`): a reboot is irrevocable once requested.

| Port                    | Kind            | Type                     | Description                      |
| ----------------------- | --------------- | ------------------------ | -------------------------------- |
| `faultResponseDispatch` | `guarded input` | `FaultResponseDispatch`  | Step dispatch from FaultManager  |
| `faultResponseCancel`   | `guarded input` | `Fw.Signal`              | Step cancellation (refused)      |
| `run`                   | `guarded input` | `Svc.Sched`              | Rate group tick driving the delay |
| `faultResponseComplete` | `output`        | `FaultResponseComplete`  | Step completion (failure only)   |

## 3. Configuration

`configure(delayTicks)` sets the number of `run` ticks between the request and the reboot. The step mapped to the
reboot (e.g. `REBOOT`) is configured in the project's `FaultConfig.StepDefinitionTable` with
`dispatchPort = Port.REBOOT_RESPONDER_PORT`; `run` must be connected to a rate group for the reboot to occur.
