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

## 2. Design

`SequenceResponder` is a `passive` component implementing the `SyncResponder` interface. A dispatched step is
translated into a sequence run request on `seqRunOut` (`Svc.CmdSeqIn`) for the file `<directory>/<step name>.seq`.
The sequencer's completion arrives on `seqDoneIn` (`Fw.CmdResponse`) and is forwarded to the `FaultManager` as the
step completion. A cancel request forwards to `seqCancelOut` (`Svc.CmdSeqCancel`) and clears the active step first
so the sequencer's subsequent completion is reported as `UnexpectedSequenceDone` rather than as a step result.

The active step is protected by a mutex since dispatch, cancel, and completion arrive on different threads.

| Port                    | Kind         | Type                     | Description                             |
| ----------------------- | ------------ | ------------------------ | --------------------------------------- |
| `faultResponseDispatch` | `sync input` | `FaultResponseDispatch`  | Step dispatch from FaultManager         |
| `faultResponseCancel`   | `sync input` | `Fw.Signal`              | Step cancellation                       |
| `faultResponseComplete` | `output`     | `FaultResponseComplete`  | Step completion                         |
| `seqRunOut`             | `output`     | `Svc.CmdSeqIn`           | Sequence run request                    |
| `seqCancelOut`          | `output`     | `Svc.CmdSeqCancel`       | Sequence cancel request                 |
| `seqDoneIn`             | `sync input` | `Fw.CmdResponse`         | Sequence completion from the sequencer  |

## 3. Configuration

`configure(directory)` sets the sequence directory. Sequences are compiled with `fprime-seqgen` against the
deployment dictionary and placed at `<directory>/<step name>.seq`. Deployments should dedicate a sequencer (e.g. a
second `Svc.CmdSequencer`) to fault responses so that ground sequences cannot block a response; see the Ref
deployment's `fpSeq`.
