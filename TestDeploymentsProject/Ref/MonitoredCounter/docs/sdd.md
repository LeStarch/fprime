# Ref::MonitoredCounter

Demonstration of the fault protection monitor pattern: counts rate group cycles, monitors the count, reports the
`COUNTER_HIGH` fault, and is corrected by the fault response.

## 1. Requirements

| ID                       | Description (shall)                                                                                                            | Verification |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------ | ------------ |
| REF_MONITOREDCOUNTER_001 | MonitoredCounter shall increment a count on each `run` cycle and report it as telemetry.                                        | Unit-Test    |
| REF_MONITOREDCOUNTER_002 | MonitoredCounter shall cycle the `Svc.FaultProtection.MonitorMachine` each cycle with the test "count exceeds `COUNT_THRESHOLD`". | Unit-Test |
| REF_MONITOREDCOUNTER_003 | MonitoredCounter shall turn YELLOW and emit a warning when the error count reaches `LOCAL_ERROR_THRESHOLD`.                     | Unit-Test    |
| REF_MONITOREDCOUNTER_004 | MonitoredCounter shall turn RED and report `COUNTER_HIGH` once per excursion when the error count reaches `SYSTEM_ERROR_THRESHOLD`. | Unit-Test |
| REF_MONITOREDCOUNTER_005 | MonitoredCounter shall provide a `RESET_COUNT` command setting the count to zero.                                               | Unit-Test    |
| REF_MONITOREDCOUNTER_006 | MonitoredCounter shall provide a `SET_MONITORING` command controlling the monitor precondition; a disabled monitor is BLACK.    | Unit-Test    |
| REF_MONITOREDCOUNTER_007 | MonitoredCounter shall return to GREEN and decay the error count on cycles passing the test.                                   | Unit-Test    |

## 2. Design

`MonitoredCounter` is an `active` component on the 1 Hz rate group. Each cycle increments `Count` and sends the
`cycle` signal to its `MonitorMachine` instance, which evaluates the monitor on the component's thread:

- precondition (`SET_MONITORING` enabled) false → BLACK; monitoring starts DISABLED so that a deployment can enable
  it once the response sequences are in place;
- test (`Count > COUNT_THRESHOLD`) false → error count decremented, GREEN;
- test true → error count incremented; at `SYSTEM_ERROR_THRESHOLD` → RED and the system response reports
  `COUNTER_HIGH` through `faultOut` (once per excursion); else at `LOCAL_ERROR_THRESHOLD` → YELLOW and the local
  response emits `CountHighWarning`.

In the Ref deployment `COUNTER_HIGH` maps to `RESET_COUNTER_RESPONSE`, a two-step sequence response:
`RESET_COUNT_SEQUENCE` commands `RESET_COUNT`, then `ACKNOWLEDGE_SEQUENCE` announces completion. After the reset the
monitor returns to GREEN on the next cycle and the count climbs again, so the fault recurs periodically — a
continuous demonstration of fault → response → correction visible in events and telemetry.

### Commands, parameters, events, telemetry

| Kind      | Name                                                              |
| --------- | ----------------------------------------------------------------- |
| Command   | `RESET_COUNT`, `SET_MONITORING(enabled)`                          |
| Parameter | `COUNT_THRESHOLD` (10), `LOCAL_ERROR_THRESHOLD` (2), `SYSTEM_ERROR_THRESHOLD` (4) |
| Event     | `CountReset`, `MonitoringSet`, `MonitorColorChanged`, `CountHighWarning`, `CountHighFault` |
| Telemetry | `Count`, `ErrorCount`, `Monitor`                                   |

## 3. Running the demonstration

Compile the response sequences against the deployment dictionary into the directory configured in
`RefTopology.cpp` (`/tmp/fp-seq`, separate from the file-uplink directory):

```
fprime-seqgen --dictionary <dict> Ref/sequences/RESET_COUNT_SEQUENCE.seq /tmp/fp-seq/RESET_COUNT_SEQUENCE.seq
fprime-seqgen --dictionary <dict> Ref/sequences/ACKNOWLEDGE_SEQUENCE.seq /tmp/fp-seq/ACKNOWLEDGE_SEQUENCE.seq
```

Then run `fprime-gds`, send `Ref.monitoredCounter.SET_MONITORING ENABLED`, and watch the `Ref.monitoredCounter`, `Svc.FaultProtection.faultManager`,
`Svc.FaultProtection.sequenceResponder`, and `Ref.fpSeq` events. The integration tests in `Ref/test/int/test_fault_protection.py`
automate this flow.
