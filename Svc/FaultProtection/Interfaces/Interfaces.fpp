module Svc {
module FaultProtection {

@* Reporter of a fault
@*
@* Use this interface on any component that needs to report a fault to FaultManager. It consists of the single output
@* port `faultOut` used to report the fault.
@*
@* Components using this interface can connect to FaultManager using: fault connections instance <instance name>.
interface Reporter {
    @ Report a fault to FaultManager
    output port faultOut: Svc.FaultProtection.FaultReport
}

@* Responder to a fault (asynchronous)
@*
@* Use this interface on any component that performs a fault response step asynchronously. It consists of an async
@* input port `faultResponseDispatch` used to accept the fault response step dispatch, an output port
@* `faultResponseComplete` used to respond after completing a fault response, and sync input port `faultResponseCancel`
@* used to implement the synchronous canceling of a fault response step.
interface AsyncResponder {
    @ Start a fault response step
    async input port faultResponseDispatch: FaultResponseDispatch

    @ Cancel a running fault response step
    sync input port faultResponseCancel: Fw.Signal

    @ Send fault response step completion
    output port faultResponseComplete: FaultResponseComplete
}

@* Responder to a fault (synchronous)
@*
@* Use this interface on any component that performs a fault response step synchronously on the caller's thread. It
@* consists of a guarded input port `faultResponseDispatch` used to accept the fault response step dispatch, an output
@* port `faultResponseComplete` used to respond after completing a fault response, and guarded input port
@* `faultResponseCancel` used to implement the canceling of a fault response step. The ports are guarded so that the
@* responder's state is protected against concurrent dispatch, cancel, and completion from other threads.
@*
@* [!CAUTION]
@* This interface was intended for the case when the user needs to use internal queuing, pass-through delegation, and
@* other tasks that rely on another component's queue. It SHALL NOT block the calling queue.
interface SyncResponder {
    @ Start a fault response step
    guarded input port faultResponseDispatch: FaultResponseDispatch

    @ Cancel a running fault response step
    guarded input port faultResponseCancel: Fw.Signal

    @ Send fault response step completion
    output port faultResponseComplete: FaultResponseComplete
}
@* Monitor State Machine
@*
@* This machine performs a standard pattern for monitoring error conditions. This pattern is captured in the following
@* diagram:
@*
@*              +-------+
@*              | Cycle |
@*              +---+---+
@*                  |
@*                  v
@*      +-----------------------+
@*      |     Precondition      |
@*      +-----------+-----------+
@*         false   /   \   true
@*                v     v
@*      +---------+     +-----------------+
@*      |  BLACK  |     |       Test      |
@*      +----+----+     +---------+-------+
@*           v            false  /          \  true
@*      +---------+             v            v
@*      |   END   |   +-------------------+   +--------------------+
@*      +---------+   |  GREEN + decrease |   |  Increase error    |
@*                    |    error count    |   |       count        |
@*                    +---------+---------+   +---------+----------+
@*                              |                       |
@*                              v                       v
@*                         +---------+       +------------------------+
@*                         |   END   |       | Check system threshold |
@*                         +---------+       +-----------+------------+
@*                                              true   /    \  false
@*                                                    v      v
@*                                      +----------------+   +-----------------------+
@*                                      |  RED + system  |   | Check local threshold |
@*                                      |   response     |   +-----------+-----------+
@*                                      +--------+-------+
@*                                               |             true   /          \  false
@*                                               v                   v            v
@*                                          +---------+    +-------------------+  +---------+
@*                                          |   END   |    | YELLOW + local    |  |   END   |
@*                                          +---------+    |    response       |  +---------+
@*                                                         +---------+---------+
@*                                                                   |
@*                                                                   v
@*                                                              +---------+  
@*                                                              |   END   |  
@*                                                              +---------+  
@*
@* As shown above, on each cycle a precondition is check. When false, the state machine turns BLACK. This means the
@* rest of the evaluation is stopped, and the error count is neither incremented nor decremented.
@*
@* When the precondition is true the monitor test is evaluated. When the test is 'false' the tested condition is absent
@* and the monitor becomes GREEN and decreases the error count. 
@*
@* When the test is 'true' the monitor increases the error count and checks for thresholds on that count.
@*
@* The system response threshold is checked first. If that threshold is exceeded (true) then the monitor turns RED and
@* invokes a system response. Otherwise, the local response threshold is check. When local is exceeded (true) then the
@* monitor becomes YELLOW and a local response is invoked. No action above increasing the error count is invoke when
@* neither threshold is exceeded (both false).
@*
state machine MonitorMachine {
    @ Initial state is only state
    initial enter WAIT_CYCLE

    @ Signal to run the cycle check machine
    signal cycle

    @* Evaluate the precondition. If it evaluates to true, monitoring will continue otherwise it will mark the monitor
    @* black and return to WAIT_CYCLE.
    guard checkPrecondition

    @* Evaluate the monitor's test. If it evaluates to true, monitoring will increment the error count and continue.
    @* Otherwise it will set the monitor to green and return to WAIT_CYCLE.
    guard performTest

    @* Evaluate the error count against the system threshold. If it evaluates to true, a system response will be
    @* triggered and the monitor will be set RED, otherwise fault monitoring will continue to check local threshold.
    guard checkSystemThreshold

    @* Evaluate the local response threshold. If it evaluates to true, then a local response will be triggered and the
    @* monitor will be marked yellow. Otherwise the state machine will return to WAIT_CYCLE state.
    guard checkLocalThreshold

    @ Set the monitor to BLACK
    action doBlack

    @ Set the monitor to GREEN
    action doGreen

    @ Set the monitor to YELLOW
    action doYellow

    @ Set the monitor to RED
    action doRed

    @ Decrease the error count
    action doErrorDecrease

    @ Increment the error count
    action doErrorIncrease

    @ Perform system response
    action doSystemResponse

    @ Perform local response
    action doLocalResponse

    @* Check precondition of the monitor. When precondition is true, monitoring may continue. When it is not true then
    @* monitoring turns BLACK and waits for the next cycle.
    choice CHECK_PRECONDITION {
        if checkPrecondition enter PERFORM_TEST else do { doBlack } enter WAIT_CYCLE
    }
    @* Perform monitor's test. When the test evaluates to true, then the error count must be incremented and threshold
    @* checking begins. When the test returns false, the error count is decreased and the monitor becomes GREEN.
    choice PERFORM_TEST {
        if performTest do { doErrorIncrease } enter CHECK_SYSTEM_THRESHOLD else do { doErrorDecrease, doGreen } enter WAIT_CYCLE
    }

    @* Perform monitor threshold testing against the system threshold. When the threshold evaluates to true, the
    @* monitor becomes RED and a system responses is triggered. Otherwise local threshold checking is performed.
    choice CHECK_SYSTEM_THRESHOLD {
        if checkSystemThreshold do { doRed, doSystemResponse } enter WAIT_CYCLE else enter CHECK_LOCAL_THRESHOLD
    }


    @* Perform monitor threshold testing against the local threshold. When the threshold evaluates to true, the
    @* monitor becomes YELLOW and a local responses is triggered.
    choice CHECK_LOCAL_THRESHOLD {
        if checkLocalThreshold do { doYellow, doLocalResponse } enter WAIT_CYCLE else enter WAIT_CYCLE
    }

    @ Singular state to handle cycles
    state WAIT_CYCLE {
        on cycle enter CHECK_PRECONDITION
    }
}
} # FaultProtection
} # Svc