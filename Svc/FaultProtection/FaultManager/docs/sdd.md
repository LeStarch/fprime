# Svc::FaultProtection::FaultManager

Translates incoming fault reports into a series of fault response step dispatches.

## 1. Requirements

| ID                   | Description (shall)                                                                                                                                       | Verification |
| -------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------ |
| SVC_FAULTMANAGER_001 | FaultManager shall have a synchronous fault report input port carrying a project-configured `FaultConfig.Fault` enumeration value identifying the fault.  | Unit-Test    |
| SVC_FAULTMANAGER_002 | FaultManager shall map each fault to a response and each response to an ordered list of steps per the project `FaultConfig` tables.                       | Unit-Test    |
| SVC_FAULTMANAGER_003 | FaultManager shall accept as parameters the fault response table (`FAULT_RESPONSE_TABLE`), the per-response enabled flags (`RESPONSE_TABLE`), and the per-step failure modes (`STEP_TABLE`), defaulting to the `FaultConfig` tables; the response and step definition tables are static configuration. | Unit-Test    |
| SVC_FAULTMANAGER_004 | Upon selecting a response, FaultManager shall dispatch its steps sequentially, dispatching each step after the completion of the previous step.            | Unit-Test    |
| SVC_FAULTMANAGER_005 | For each step, FaultManager shall dispatch to the step's configured port passing the response, step, and project-configured context.                       | Unit-Test    |
| SVC_FAULTMANAGER_006 | FaultManager shall validate fault, response, and step values from ports and commands against the configured enumerations, rejecting out-of-range values.  | Unit-Test    |
| SVC_FAULTMANAGER_007 | FaultManager shall require the project's `FaultConfig.Fault` enumeration to define `FAULT_RESPONSE_FAILURE` reserved for FaultManager's own fault.        | Unit-Test    |
| SVC_FAULTMANAGER_008 | If a step fails with failure mode `FAULT`, FaultManager shall cease the response and report `FAULT_RESPONSE_FAILURE`. Mode `DEFER` continues the steps and reports after the response; mode `IGNORE` continues the steps and reports nothing. | Unit-Test |
| SVC_FAULTMANAGER_009 | FaultManager shall provide a command to enable/disable each fault.                                                                                        | Unit-Test    |
| SVC_FAULTMANAGER_010 | FaultManager shall latch each reported fault until the response to it completes (or fails); further reports of a latched fault shall be ignored. A preempted fault stays latched. | Unit-Test    |
| SVC_FAULTMANAGER_011 | FaultManager shall provide a command to enable/disable each response.                                                                                      | Unit-Test    |
| SVC_FAULTMANAGER_012 | FaultManager shall provide a command to set the failure mode of each step.                                                                                 | Unit-Test    |
| SVC_FAULTMANAGER_013 | FaultManager shall wait `FaultConfig.RESPONSE_COUNTDOWN_TICKS` ticks after the first latched report, then respond to the highest-precedence latched fault. | Unit-Test    |
| SVC_FAULTMANAGER_014 | FaultManager shall ignore (take no action on) a reported fault that is disabled.                                                                           | Unit-Test    |
| SVC_FAULTMANAGER_015 | FaultManager shall skip every step of a disabled response, emitting an event per skipped step, and complete the response.                                  | Unit-Test    |
| SVC_FAULTMANAGER_016 | FaultManager shall preempt an active response when a fault of higher precedence than the fault under response is reported, cancelling the active step.    | Unit-Test    |
| SVC_FAULTMANAGER_017 | FaultManager shall not report `FAULT_RESPONSE_FAILURE` in response to a failure of the `FAULT_RESPONSE_FAILURE` response itself.                            | Unit-Test    |
| SVC_FAULTMANAGER_018 | FaultManager shall not assert (FATAL) on any port or command input: all inputs are validated and overflow is handled by dropping.                           | Unit-Test    |
| SVC_FAULTMANAGER_019 | FaultManager shall report telemetry counting faults reported, faults ignored, responses completed, and responses failed.                                   | Unit-Test    |

## 2. Design

![Component Block Diagram](./bdd.svg)

`Svc.FaultProtection.FaultManager` is an `active` component driven by a `run` port on a rate group. Fault reports
arrive synchronously on `reportIn` and are latched in an atomic per-fault flag, so a report is never lost to queue
state. Each `run` tick advances the `FaultManagerStateMachine`:

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> CHECK_REPORT: Tick
    CHECK_REPORT --> COUNTDOWN: [hasReport]
    CHECK_REPORT --> IDLE: [!hasReport]
    COUNTDOWN --> COUNTDOWN: Tick [!countdownExpired]
    COUNTDOWN --> RESPONSE: Tick [countdownExpired] / selectResponse, dispatchStep
    RESPONSE --> RESPONSE: StepSuccessful / StepDeferredFailure [!responseDone] / dispatchStep
    RESPONSE --> CHECK_REPORT: StepSuccessful [responseDone] / completeResponse
    RESPONSE --> CHECK_REPORT: StepFailed / completeResponse (FAULT_RESPONSE_FAILURE latched)
    RESPONSE --> CHECK_REPORT: Preempt / cancel active step, completeResponse
    RESPONSE --> RESPONSE: Tick / tickStep (step timeout)
```

1. **IDLE**: each tick enters `CHECK_REPORT`, which scans for a latched report in precedence order. A completed
   response also returns through `CHECK_REPORT`, so a report latched during the response starts its countdown
   without waiting for another tick.
2. **COUNTDOWN**: `FaultConfig.RESPONSE_COUNTDOWN_TICKS` ticks allow further reports to accumulate; with a value of
   0 this state is skipped and the response starts on the tick that detects the report.
3. **RESPONSE**: the enabled fault of highest precedence is selected and its response's steps are dispatched in
   order on `stepDispatchOut[<step port>]`. A completion on `stepCompletionIn` for the active step advances to the
   next step. The latches are cleared when the response completes: on success, the latch of every enabled fault
   mapped to the response; on failure, the latch of the triggering fault only; never on preemption. The steps of a
   disabled response are skipped with `StepSkipped`; a `SKIP` step ends the response.

Reports are latched synchronously in `reportIn` (an atomic per-fault flag); the `handleReport` message announcing a
report on the component's thread (`FaultReported`/`FaultIgnored`/`FaultDisabled`, telemetry, immediate preemption) is
dropped when the queue is full. Every tick therefore re-evaluates the latches themselves (`auditLatches`): the report
of a disabled fault is discarded and a latched fault that outranks the fault under response preempts it, so a dropped
message costs at most one diagnostic and one tick of preemption latency; the report is never lost.
4. A failed step is handled per its `FailureMode`: `IGNORE` continues; `DEFER` continues and fails the response at
   its end; `FAULT` fails the response immediately. A failed response reports `FAULT_RESPONSE_FAILURE` internally,
   unless the failed response was itself the response to `FAULT_RESPONSE_FAILURE`.
5. A report of a fault with higher precedence than the fault under response preempts the response: `stepCancelOut`
   is sent to the active step's port and `ResponsePreempted` is emitted. The preempted fault stays latched and is
   responded to after the preempting fault.

Completions for a step other than the active one are logged (`UnexpectedStepCompleted`) and ignored. Unconnected
step ports are treated as step failures (`StepPortUnconnected`). A step whose `timeoutTicks` is nonzero must
complete within that many `run` ticks of its dispatch; otherwise the step is canceled (`stepCancelOut`), logged
(`StepTimedOut`), and failed per its failure mode. When the response to `FAULT_RESPONSE_FAILURE` itself fails there
is nothing further to escalate to within the engine: `EscalationExhausted` is emitted and the `virtual
escalationExhausted()` hook is called so that a project may take a terminal action (the framework default is a
no-op, relying on the deployment to map `FAULT_RESPONSE_FAILURE` to a reboot).

`FaultReported` is bounded by latching (one per fault per response) and is not throttled. The events for reports
that are not acted on (`FaultIgnored`, `FaultDisabled`, `FaultInvalid`) are throttled, and the throttle is cleared on
every `run` tick: a reporter flapping faster than the rate group is bounded to a few events per tick without being
silenced for the remainder of the mission.

### 2.1 Ports

| Name               | Kind                  | Type                      | Description                                                      |
| ------------------ | --------------------- | ------------------------- | ---------------------------------------------------------------- |
| `run`              | `async input` (drop)  | `Svc.Sched`               | Rate group tick driving the state machine                        |
| `reportIn`         | `sync input`          | `FaultReport`             | Fault report; latched synchronously                              |
| `stepDispatchOut`  | `output [NUM_PORTS]`  | `FaultResponseDispatch`   | Step dispatch, one port per `FaultConfig.Port`                   |
| `stepCancelOut`    | `output [NUM_PORTS]`  | `Fw.Signal`               | Step cancellation, one port per `FaultConfig.Port`               |
| `stepCompletionIn` | `async input` (drop)  | `FaultResponseComplete`   | Step completion status from a responder                          |

### 2.2 Commands

| Name                       | Description                                                                 |
| -------------------------- | --------------------------------------------------------------------------- |
| `SET_FAULT_ENABLED`        | Enable/disable response to a fault (updates `FAULT_RESPONSE_TABLE`)          |
| `SET_RESPONSE_ENABLED`     | Enable/disable a response (updates `RESPONSE_TABLE`)                      |
| `UPDATE_STEP_FAILURE_MODE` | Set the failure mode of a step (updates `STEP_TABLE`)                        |

Out-of-range enumeration values (e.g. `NUM_FAULTS`) respond `VALIDATION_ERROR` with `InvalidFaultArgument`,
`InvalidResponseArgument`, or `InvalidStepArgument` carrying the rejected value.

### 2.3 Events

| Name                       | Severity      | Emitted when                                                                  |
| -------------------------- | ------------- | ----------------------------------------------------------------------------- |
| `FaultReported`            | activity high | A report is accepted and latched                                              |
| `FaultIgnored`             | warning low   | A report of an already latched fault (throttled, cleared each tick)           |
| `FaultDisabled`            | warning low   | A report of a disabled fault (throttled, cleared each tick)                   |
| `FaultInvalid`             | warning high  | A report with an unconfigured fault id (throttled, cleared each tick)         |
| `ResponseStarted`          | activity high | A response is selected for a fault                                            |
| `ResponseCompleted`        | activity high | All steps of a response completed                                             |
| `ResponseFailed`           | warning high  | A response ended in failure (`FAULT` or deferred step failure)               |
| `ResponsePreempted`        | warning low   | A higher-precedence report canceled the active response                       |
| `EscalationExhausted`      | warning high  | The response to `FAULT_RESPONSE_FAILURE` itself failed                        |
| `StepStarted`              | activity low  | A step is dispatched                                                          |
| `StepCompleted`            | activity low  | A step completed successfully                                                 |
| `StepFailed`               | warning high  | A step completed with failure, with the failure mode applied                  |
| `StepTimedOut`             | warning high  | A step did not complete within its `timeoutTicks`                             |
| `StepPortUnconnected`      | warning high  | A step's dispatch port is unconnected (step fails)                            |
| `UnexpectedStepCompleted`  | warning high  | A completion arrived for a step other than the active one                     |
| `StepCancel`               | activity high | A cancel was sent to the active step's port                                   |
| `StepSkipped`              | activity low  | A step of a disabled response was walked without dispatch                     |
| `FaultEnabledSet`          | activity high | `SET_FAULT_ENABLED` accepted                                                  |
| `ResponseEnabledSet`       | activity high | `SET_RESPONSE_ENABLED` accepted                                               |
| `StepFailureModeSet`       | activity high | `UPDATE_STEP_FAILURE_MODE` accepted                                           |
| `InvalidFaultArgument`     | warning low   | A command carried an out-of-range fault                                       |
| `InvalidResponseArgument`  | warning low   | A command carried an out-of-range response                                    |
| `InvalidStepArgument`      | warning low   | A command carried an out-of-range step                                        |

### 2.4 Telemetry

| Name                 | Description                       |
| -------------------- | --------------------------------- |
| `FaultsReported`     | Reports accepted and latched      |
| `FaultsIgnored`      | Reports ignored (latched/disabled/invalid) |
| `ResponsesCompleted` | Responses completed successfully  |
| `ResponsesFailed`    | Responses completed with failure  |

### 2.5 Parameters

| Name                   | Type                 | Description                                                    |
| ---------------------- | -------------------- | -------------------------------------------------------------- |
| `FAULT_RESPONSE_TABLE` | `FaultResponseTable` | Precedence, response, and enabled flag per fault               |
| `RESPONSE_TABLE`       | `ResponsesEnabled`   | Enabled flag per response                                      |
| `STEP_TABLE`           | `StepFailureModes`   | Failure mode per step                                          |

The parameters are external: the component is its own parameter delegate so that the tables default to the
`FaultConfig` constants when the parameter database holds no valid value. The commands in 2.2 update the cached
tables and the deployment's `*_PRM_SAVE` commands persist them.

## 3. Configuration

`Svc.FaultProtection.FaultManager` is project-configurable through the `FaultConfig` FPP module, which the project
overrides in its configuration directory (see `Svc/FaultProtection/FaultConfig/FaultConfig.fpp` for the framework
default and `TestDeploymentsProject/Ref/Config/FaultConfig.fpp` for an example override).

| Item                        | Description                                                                              |
| --------------------------- | ---------------------------------------------------------------------------------------- |
| `FAULT_RESPONSE_STEP_COUNT` | Steps per response; unused steps are `SKIP`                                              |
| `RESPONSE_COUNTDOWN_TICKS`  | Ticks between the first report and the response                                          |
| `Fault`                     | Faults; `FATAL_OCCURRED` and `FAULT_RESPONSE_FAILURE` are required, `NUM_FAULTS` last     |
| `Response`                  | Responses; `NUM_RESPONSES` last                                                          |
| `Step`                      | Steps; `NUM_STEPS` penultimate and `SKIP` last                                           |
| `Port`                      | Dispatch ports; `NUM_PORTS` last                                                         |
| `Context`                   | Project-defined structure forwarded with each dispatch                                   |
| `FaultResponseTable`        | One `FaultResponseEntry` (precedence, response, enabled) per fault                       |
| `ResponseDefinitionTable`   | One `ResponseDefinitionEntry` (steps) per response                                       |
| `StepDefinitionTable`       | One `StepDefinitionEntry` (failure mode, port, timeout ticks, context) per step except `SKIP` |

The tables are loaded at construction and may be overridden by the `FAULT_RESPONSE_TABLE`, `RESPONSE_TABLE`, and
`STEP_TABLE` parameters when those are valid in the parameter database. The parameters declare defaults so that they
are always valid and their `PRM_SAVE` commands are accepted before any `PRM_SET`; a save always persists the active
table (for `STEP_TABLE`, the `FaultConfig` step definitions' failure modes until a valid table is loaded or set), so
`SET_RESPONSE_ENABLED` and `UPDATE_STEP_FAILURE_MODE` changes can be persisted directly. The `Svc.FaultProtection.Subtopology`
instantiates the FaultManager with a `SequenceResponder` and a `RebootResponder` and exposes `reportIn`,
`faultManagerRun`, `rebootResponderRun`, and the sequencer ports (`seqRunOut`, `seqCancelOut`, `seqDoneIn`) for the
deployment to connect; see [the subtopology SDD](../../Subtopology/docs/sdd.md).
