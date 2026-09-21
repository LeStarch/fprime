# Svc::ComRetry

## 1. Introduction

The `Svc::ComRetry` component forwards messages from upstream to downstream components, resending messages on failure. Any topology requiring retry capabilities must place this component in the pipeline before a `ComStub` or `Radio` component. This component expects a `ComStatus` response per the [Communication Adapter Protocol](../../../docs/reference/communication-adapter-interface.md#communication-adapter-protocol). It acts as a pass-through component in case of a successful delivery, i.e. when it receives `Fw::Success::SUCCESS`. On receiving `Fw::Success::FAILURE`, it resends the message until it exceeds the maximum number of retries. After all retries are exhausted, it emits `Fw::Success::FAILURE` upstream, and the downstream communication adapter is responsible for eventually emitting a recovery `Fw::Success::SUCCESS` to resume data flow.

`Svc::ComRetry` can be used alongside the other F´ communication components (`Svc::Framer`, `Svc::Deframer`, `Svc::ComQueue`).

## 2. Requirements

| Requirement     | Description                                                                                                                 | Rationale                                                   | Verification Method |
|-----------------|-----------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------|---------------------|
| SVC-COMRETRY-001 | `Svc::ComRetry` shall accept incoming downlink data as `Fw::Buffer` and pass them to an `Svc.ComDataWithContext` port                    | The component must forward messages without modifying them | Unit Test           |
| SVC-COMRETRY-002 | `Svc::ComRetry` shall store `Fw::Buffer` and its context on receiving buffer ownership through `dataReturnIn` | Store the buffer in case a retry is required  | Unit test           |
| SVC-COMRETRY-003 | `Svc::ComRetry` shall pause delivery on receiving `Fw::Success::FAILURE` | `Svc::ComRetry` should not send to a failing communication adapter.  | Unit test           |
| SVC-COMRETRY-004 | `Svc::ComRetry` shall resend `Fw::Buffer` on receiving `Fw::Success::SUCCESS` after prior failure if retries are available | Retry delivery of buffer  | Unit test           |
| SVC-COMRETRY-005 | `Svc::ComRetry` shall pass through the initial start-up `Fw::Success::SUCCESS` and any non-data statuses upstream when no buffer is pending  | The initial SUCCESS must reach `Svc::ComQueue` to initiate data flow per the [Communication Queue Protocol](../../../docs/reference/communication-adapter-interface.md#communication-queue-protocol)  | Unit test           |
| SVC-COMRETRY-006 | The maximum number of retries shall be configurable | The number of retries should be adaptable for projects  | Inspection           |
| SVC-COMRETRY-007 | `Svc::ComRetry` shall return buffer ownership to the upstream component on receiving `Fw::Success::SUCCESS` or after all retry attempts fail | Memory management       | Unit Test           |
| SVC-COMRETRY-008 | `Svc::ComRetry` shall send `ComStatus` upstream on successful delivery or after all retry attempts fail                             | Status of message delivery must be passed up the stack      | Unit Test           |
| SVC-COMRETRY-009 | `Svc::ComRetry` shall provide a configuration option, `recover_on_sender_thread`, defaulting to off | Projects choose where retries execute; default behavior is unchanged | Unit Test |
| SVC-COMRETRY-010 | When `recover_on_sender_thread` is off, `Svc::ComRetry` shall issue the resend from the `comStatusIn` call that delivers the recovery `Fw::Success::SUCCESS` | Existing behavior | Unit Test |
| SVC-COMRETRY-011 | When `recover_on_sender_thread` is on, `dataIn` shall not return until the buffer is delivered or all retries are exhausted | The caller's thread owns the whole delivery, including retries | Unit Test |
| SVC-COMRETRY-012 | When `recover_on_sender_thread` is on, a recovery `Fw::Success::SUCCESS` received on `comStatusIn` shall only record state and notify the blocked `dataIn` caller; the resend shall be issued from the `dataIn` caller's thread | No blocking send may execute on the thread that emits the status (e.g. a radio receive thread) | Unit Test |
| SVC-COMRETRY-013 | The wait in `dataIn` shall be implemented with an `Os::ConditionVariable` guarded by an `Os::Mutex`, without polling or timeout | Per the Communication Adapter Protocol, the adapter that emitted `FAILURE` owes the recovery `SUCCESS` | Inspection |
| SVC-COMRETRY-014 | `Svc::ComRetry` shall support adapters that deliver `dataReturnIn` and `comStatusIn` synchronously from within the `dataOut` call, in both modes | Passive adapters respond inline on the caller's thread | Unit Test |

## 3. Design

`Svc::ComRetry` implements `Svc.Framer`. It is a passive component: every handler executes on the thread of the
component that invokes it.

### 3.1 State machine

| State | Meaning |
|---|---|
| `WAITING_FOR_SEND` | No buffer is pending; statuses are passed straight upstream |
| `WAITING_FOR_STATUS` | A buffer has been sent downstream and its status is awaited |
| `RETRYING` | Delivery failed with retries remaining; awaiting the adapter's recovery `SUCCESS` |
| `RESEND_READY` | (sender-thread mode only) Recovery `SUCCESS` received; the blocked `dataIn` caller will resend |

### 3.2 Where retries execute

In the default mode the resend is issued from within `comStatusIn_handler` when the recovery `SUCCESS` arrives.
Because `Svc::ComRetry` is passive, that resend — and the adapter's send that it triggers — executes on whatever
thread the communication adapter used to emit the status. For adapters that emit the recovery `SUCCESS` from a
receive callback, interrupt-driven work queue, or rate-group tick, this places a potentially blocking transmit on a
thread that must not block.

The `recover_on_sender_thread` option **pulls recovery back onto the thread that called `dataIn`** (typically the
`Svc::ComQueue` task). It is implemented with an `Os::ConditionVariable` (`m_condition`) and an `Os::Mutex`
(`m_mutex`):

```
dataIn caller thread                          status-emitting thread (adapter)
--------------------                          --------------------------------
dataIn_handler:
  state = WAITING_FOR_STATUS
  dataOut_out(buffer)         ------------>   adapter refuses / fails
                              <------------   dataReturnIn(buffer); comStatusIn(FAILURE)
  (state = RETRYING)
  lock; while state != WAITING_FOR_SEND:
      m_condition.wait(m_mutex)  ... blocked ...
                                              adapter recovers
                              <------------   comStatusIn(SUCCESS):
                                                state = RESEND_READY
                                                m_condition.notify()      <- no send here
  wakes; state == RESEND_READY
  resend(): dataOut_out(buffer) ---------->   adapter sends
                              <------------   dataReturnIn(buffer); comStatusIn(SUCCESS)
                                                finish(): state = WAITING_FOR_SEND,
                                                          dataReturnOut, comStatusOut, notify
  loop exits; dataIn returns
```

Properties of this mode:

- Every `dataOut` invocation — the first attempt and every resend — happens on the `dataIn` caller's thread.
  `comStatusIn_handler` never invokes `dataOut` in this mode; it only updates state and calls `notify()`.
- `dataIn` blocks until the buffer is delivered (`SUCCESS`) or retries are exhausted (`FAILURE`). This is
  consistent with the Communication Queue Protocol: `Svc::ComQueue` does not send again until it receives a
  status, so holding its thread costs nothing.
- The wait is unbounded by design. The Communication Adapter Protocol requires the adapter that emitted `FAILURE`
  to eventually emit a recovery `SUCCESS`; the retry does not second-guess the adapter with a timeout.
- The mutex is never held across an output port call, so adapters may answer synchronously (nested inside
  `dataOut`) or asynchronously from another thread.
- The final `dataReturnOut`/`comStatusOut` are emitted from the thread that delivered the final status. These
  upstream ports are non-blocking by protocol (`Svc::ComQueue` receives them on an async port).

### 3.3 Configuration

```c++
comRetry.configure(num_retries, recover_on_sender_thread);
```

| Argument | Default | Description |
|---|---|---|
| `num_retries` | 3 | Resend attempts before `FAILURE` is passed upstream |
| `recover_on_sender_thread` | `false` | Block `dataIn` and issue all resends from its caller's thread (Section 3.2) |

When an adapter uses `FAILURE` to defer a send (for example, a half-duplex radio refusing to transmit while a
receive is in progress), each deferral consumes one retry, so `num_retries` should be sized to the number of
consecutive deferrals the link may plausibly produce.

## 4. Unit Tests

| Test | Description |
|---|---|
| `Nominal.NullBuffer` | Statuses with no pending buffer pass straight upstream |
| `Nominal.Send` | Nominal delivery returns the buffer and forwards `SUCCESS` |
| `Nominal.Retry` | One failure then recovery resends the buffer from the status call (default mode) |
| `Nominal.RetryTillFailure` | Retries exhaust and `FAILURE` is passed upstream (default mode) |
| `RecoverOnSenderThread.Retry` | `dataIn` on a separate task blocks; recovery `SUCCESS` from the test thread does not resend inline; the resend is issued by the blocked task |
| `RecoverOnSenderThread.RetryTillFailure` | Exhausted retries release the blocked `dataIn` caller with `FAILURE` upstream |
| `RecoverOnSenderThread.NestedStatus` | Adapter answers synchronously from inside `dataOut`; recovery from another thread completes delivery |
