# Svc::FaultProtection::FatalToFault

Handles FATAL events by translating them into `FATAL_OCCURRED` fault reports, bringing FATAL handling under the
control of the fault protection system.

## 1. Requirements

| ID                    | Description (shall)                                                                                                        | Verification |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------- | ------------ |
| SVC_FATALTOFAULT_001  | FatalToFault shall report the fault `FaultConfig.Fault.FATAL_OCCURRED` on receipt of a FATAL event announcement.             | Unit-Test    |
| SVC_FATALTOFAULT_002  | FatalToFault shall not return control to the asserting thread after a FATAL.                                               | Unit-Test    |
| SVC_FATALTOFAULT_003  | FatalToFault shall invoke the fallback (abort) when a configurable delay expires with the software still running.          | Unit-Test    |
| SVC_FATALTOFAULT_004  | FatalToFault shall invoke the fallback immediately when the fault report port is unconnected.                              | Unit-Test    |
| SVC_FATALTOFAULT_005  | FatalToFault shall report every FATAL received, including FATALs raised while a fault response is already in progress.   | Unit-Test    |

## 2. Design

`FatalToFault` is a `passive` drop-in replacement for `Svc.FatalHandler`. Where `Svc.FatalHandler` terminates the
process, `FatalToFault` reports `FATAL_OCCURRED` to the `FaultManager`, whose configured response (typically
`REBOOT_RESPONSE` dispatched to the `RebootResponder`) ends the software in a controlled manner after the FATAL has
been downlinked.

The FATAL handler runs on the asserting thread. An F Prime assertion must not return into the code that failed it,
so after reporting the fault the handler parks the asserting thread with `Os::Task::delay(m_fallback_delay)`. The
fault response is expected to end the software during that delay; if the software is still running when the delay
expires (the response path is impaired, disabled, or misconfigured) the handler calls the `fallback()` hook
(default: `abort()`). The fallback is taken immediately when `faultOut` is not connected. Each FATAL is reported;
a second FATAL during the response is a duplicate latched report that the `FaultManager` ignores, and its thread is
parked in the same way.

Because the asserting thread is parked, the ordered response can only run on threads other than the asserting one.
`REBOOT_RESPONSE` therefore executes when the asserting thread is neither the `FaultManager` thread nor the rate group
driving `faultManagerRun` or `rebootResponderRun`; a FATAL raised on one of those threads (in the Ref, a synchronous
member of rate group 1 or 2) ends the software through `fallback()` after the configured delay. The fallback is the
designed terminal action for that case, not a defect, and the FATAL event itself is still downlinked since the
event path is not tick driven. Deployments wanting the ordered reboot for such FATALs drive the two run ports from a
rate group without synchronous members (see the [subtopology SDD](../../Subtopology/docs/sdd.md)).

| Port          | Kind         | Type                         | Description                                             |
| ------------- | ------------ | ---------------------------- | ------------------------------------------------------- |
| `FatalReceive`| `sync input` | `Svc.FatalEvent`             | FATAL event announcement from the event manager         |
| `faultOut`    | `output`     | `FaultProtection.FaultReport`| Fault report (`Svc.FaultProtection.Reporter` interface) |

## 3. Configuration

`configure(fallbackDelay)` sets the wall-clock delay (`Fw::TimeInterval`) before the fallback. It must exceed the
`FaultManager` countdown plus the `REBOOT` step dispatch and the `RebootResponder` delay (in wall-clock terms of the
rate groups driving them) plus the time to downlink the announcement; the default is `DEFAULT_FALLBACK_SECONDS`
seconds. `fallback()` defaults to `abort()` (a core dump where enabled) whereas `RebootResponder::doReboot()` defaults
to `_Exit`; a project overrides either hook to its platform reset. A deployment
substitutes this component for `Svc.FatalHandler` in the CdhCore subtopology by overriding
`CdhCoreFatalHandlerConfig.fpp` (see `TestDeploymentsProject/Ref/Config`) and connects `faultOut` to
`FaultManager.reportIn`. Projects with a platform reset derive from the component and override `fallback()`.
