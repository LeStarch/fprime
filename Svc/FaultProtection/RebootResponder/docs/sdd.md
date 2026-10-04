# Svc::FaultProtection::RebootResponder

Reboots the flight software in response to a fault response step.

## 1. Requirements

| ID                        | Description (shall)                                                                                                   | Verification |
| ------------------------- | --------------------------------------------------------------------------------------------------------------------- | ------------ |
| SVC_REBOOTRESPONDER_001   | RebootResponder shall emit an event announcing the reboot when a step is dispatched to it.                             | Unit-Test    |
| SVC_REBOOTRESPONDER_002   | RebootResponder shall delay a configurable interval before rebooting to allow the announcement to downlink.            | Unit-Test    |
| SVC_REBOOTRESPONDER_003   | RebootResponder shall perform the reboot through an overridable hook defaulting to hard process termination.           | Unit-Test    |
| SVC_REBOOTRESPONDER_004   | RebootResponder shall complete the step with failure if the reboot hook returns.                                        | Unit-Test    |
| SVC_REBOOTRESPONDER_005   | RebootResponder shall refuse cancellation of a requested reboot.                                                        | Unit-Test    |

## 2. Design

`RebootResponder` is a `passive` component implementing the `SyncResponder` interface. On dispatch it emits
`RebootRequested`, waits `m_delay`, and calls the `virtual doReboot()` hook. The default hook calls `_Exit` so the
process supervisor restarts the software; platforms with a hardware reset override `doReboot()` in a derived class.
A reboot never returns, so the step never completes; if the hook does return, the step completes with
`Fw::Success::FAILURE` so the `FaultManager` can escalate per the step's failure mode.

Cancellation is refused (`RebootCancelRefused`): a reboot is irrevocable once requested.

| Port                    | Kind         | Type                     | Description                      |
| ----------------------- | ------------ | ------------------------ | -------------------------------- |
| `faultResponseDispatch` | `sync input` | `FaultResponseDispatch`  | Step dispatch from FaultManager  |
| `faultResponseCancel`   | `sync input` | `Fw.Signal`              | Step cancellation (refused)      |
| `faultResponseComplete` | `output`     | `FaultResponseComplete`  | Step completion (failure only)   |

## 3. Configuration

`configure(delay)` sets the downlink delay. The step mapped to the reboot (e.g. `REBOOT`) is configured in the
project's `FaultConfig.StepDefinitionTable` with `dispatchPort = Port.REBOOT_RESPONDER_PORT`.
