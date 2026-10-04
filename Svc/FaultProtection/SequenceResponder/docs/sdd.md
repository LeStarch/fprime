# Svc::FaultProtection::SequenceResponder

Responds to a fault response step by running a command sequence.

## 1. Requirements

| ID                          | Description (shall)                                                                                                       | Verification |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------- | ------------ |
| SVC_SEQUENCERESPONDER_001   | SequenceResponder shall request the sequence `<directory>/<step name>.seq` from the sequencer when a step is dispatched.  | Unit-Test    |
| SVC_SEQUENCERESPONDER_002   | SequenceResponder shall complete the step with success when the sequence completes with `OK` and failure otherwise.       | Unit-Test    |
| SVC_SEQUENCERESPONDER_003   | SequenceResponder shall run at most one sequence at a time, completing a dispatch received while busy with failure.       | Unit-Test    |
| SVC_SEQUENCERESPONDER_004   | SequenceResponder shall cancel the running sequence on a cancel request and shall not complete the cancelled step.        | Unit-Test    |
| SVC_SEQUENCERESPONDER_005   | SequenceResponder shall complete the step with failure when the sequencer port is unconnected.                            | Unit-Test    |
| SVC_SEQUENCERESPONDER_006   | SequenceResponder shall log and ignore sequence completions received with no step in progress.                             | Unit-Test    |
| SVC_SEQUENCERESPONDER_007   | SequenceResponder shall complete the step with failure when the sequence file name does not fit a `Fw::FileNameString`.  | Unit-Test    |

## 2. Design

`SequenceResponder` is a `passive` component implementing the `SyncResponder` interface. A dispatched step is
translated into a sequence run request on `seqRunOut` (`Svc.CmdSeqIn`) for the file `<directory>/<step name>.seq`.
The sequencer's completion arrives on `seqDoneIn` (`Fw.CmdResponse`) and is forwarded to the `FaultManager` as the
step completion. A cancel request forwards to `seqCancelOut` (`Svc.CmdSeqCancel`) and clears the active step first
so the sequencer's subsequent completion is reported as `UnexpectedSequenceDone` rather than as a step result.

Dispatch, cancel, and completion arrive on different threads, so all three input ports are `guarded`: the
component's mutex serializes the handlers. The file name is built with `Fw::FileNameString::format`; when
`FW_SERIALIZABLE_TO_STRING` is disabled the numeric step value is used in place of the step name. A name that does
not fit is reported with `SequenceFileNameTooLong` and the step fails.

| Port                    | Kind         | Type                     | Description                             |
| ----------------------- | ------------ | ------------------------ | --------------------------------------- |
| `faultResponseDispatch` | `guarded input` | `FaultResponseDispatch` | Step dispatch from FaultManager         |
| `faultResponseCancel`   | `guarded input` | `Fw.Signal`             | Step cancellation                       |
| `faultResponseComplete` | `output`     | `FaultResponseComplete`  | Step completion                         |
| `seqRunOut`             | `output`     | `Svc.CmdSeqIn`           | Sequence run request                    |
| `seqCancelOut`          | `output`     | `Svc.CmdSeqCancel`       | Sequence cancel request                 |
| `seqDoneIn`             | `guarded input` | `Fw.CmdResponse`        | Sequence completion from the sequencer  |

### Events

| Name                       | Severity      | Emitted when                                                            |
| -------------------------- | ------------- | ----------------------------------------------------------------------- |
| `SequenceStarted`          | activity high | A step is dispatched and the sequence file is requested                 |
| `SequenceCompleted`        | activity high | The sequencer completed the sequence with `OK`                          |
| `SequenceFailed`           | warning high  | The sequencer completed the sequence with any other status              |
| `SequenceCanceled`         | activity high | A cancel request was forwarded to the sequencer                         |
| `ResponderBusy`            | warning high  | A dispatch arrived while a step was active (the new step fails)         |
| `UnexpectedSequenceDone`   | warning low   | A sequence completion arrived with no step active                       |
| `SequencerUnconnected`     | warning high  | `seqRunOut` is unconnected (the step fails)                             |
| `SequenceFileNameTooLong`  | warning high  | The file name does not fit a `Fw::FileNameString` (the step fails)      |

## 3. Configuration

`configure(directory)` (any `Fw::StringBase`) sets the sequence directory. Sequences are compiled with `fprime-seqgen` against the
deployment dictionary and placed at `<directory>/<step name>.seq`. Note that `Svc.CmdSequencer` stores the file name
in an `Fw::CmdStringArg` (`FW_CMD_STRING_MAX_SIZE`, 40 by default), so the directory must be short enough for the
longest step name to fit; a longer path is silently truncated by the sequencer and reported as `CS_FileNotFound`. Deployments should dedicate a sequencer (e.g. a
second `Svc.CmdSequencer`) to fault responses so that ground sequences cannot block a response; see the Ref
deployment's `fpSeq`.
