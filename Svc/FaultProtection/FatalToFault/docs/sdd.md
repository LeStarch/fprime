# Svc::FaultProtection::FatalToFault

Handles FATAL events by translating them into `FATAL_OCCURRED` fault reports, bringing FATAL handling under the
control of the fault protection system.

## 1. Requirements

| ID                    | Description (shall)                                                                                                        | Verification |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------- | ------------ |
| SVC_FATALTOFAULT_001  | FatalToFault shall report the fault `FaultConfig.Fault.FATAL_OCCURRED` on receipt of a FATAL event announcement.             | Unit-Test    |
| SVC_FATALTOFAULT_002  | FatalToFault shall arm a fallback countdown of a configurable number of `run` ticks on receipt of the first FATAL.           | Unit-Test    |
| SVC_FATALTOFAULT_003  | FatalToFault shall invoke the fallback (abort) when the countdown expires with the software still running.                   | Unit-Test    |
| SVC_FATALTOFAULT_004  | FatalToFault shall invoke the fallback immediately when the fault report port is unconnected.                              | Unit-Test    |
| SVC_FATALTOFAULT_005  | FatalToFault shall forward every FATAL as a report and shall not restart the countdown on repeated FATALs.                 | Unit-Test    |

## 2. Design

`FatalToFault` is a `passive` drop-in replacement for `Svc.FatalHandler`. Where `Svc.FatalHandler` terminates the
process, `FatalToFault` reports `FATAL_OCCURRED` to the `FaultManager`, whose configured response (typically
`REBOOT_RESPONSE` dispatched to the `RebootResponder`) ends the software in a controlled manner after the FATAL has
been downlinked. Because the fault response path might itself be impaired, a safety net is retained: the `run` port
counts down a configurable number of ticks after the first FATAL and calls the `fallback()` hook (default: `abort()`)
if the software is still running. The fallback is also taken immediately when `faultOut` is not connected.

The FATAL handler is called on the asserting thread; the handler returns after reporting so that the asserting
component is not blocked. The countdown runs on the rate group thread.

| Port          | Kind         | Type                         | Description                                             |
| ------------- | ------------ | ---------------------------- | ------------------------------------------------------- |
| `FatalReceive`| `sync input` | `Svc.FatalEvent`             | FATAL event announcement from the event manager         |
| `run`         | `sync input` | `Svc.Sched`                  | Fallback countdown tick; no fallback if unconnected     |
| `faultOut`    | `output`     | `FaultProtection.FaultReport`| Fault report (`Svc.FaultProtection.Reporter` interface) |

## 3. Configuration

`configure(fallbackTicks)` sets the countdown length (default `DEFAULT_FALLBACK_TICKS`). A deployment substitutes
this component for `Svc.FatalHandler` in the CdhCore subtopology by overriding `CdhCoreFatalHandlerConfig.fpp` (see
`TestDeploymentsProject/Ref/Config`), connects `run` to a rate group, and connects `faultOut` to
`FaultManager.reportIn`. Projects with a platform reset derive from the component and override `fallback()`.
