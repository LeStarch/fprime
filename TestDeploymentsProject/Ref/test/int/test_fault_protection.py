"""test_fault_protection.py:

Integration tests for the fault protection example in the Ref deployment: Ref.monitoredCounter counts rate group
cycles, its monitor reports the COUNTER_HIGH fault when the count stays above its threshold, and the fault manager
responds by running the RESET_COUNT_SEQUENCE and ACKNOWLEDGE_SEQUENCE sequences on the dedicated fpSeq sequencer.
The first resets the counter, correcting the fault.

The response sequences must be compiled and placed where the deployment expects them (see RefTopology.cpp):

    fprime-seqgen --dictionary <dictionary> Ref/sequences/RESET_COUNT_SEQUENCE.seq /tmp/fp-seq/RESET_COUNT_SEQUENCE.seq
    fprime-seqgen --dictionary <dictionary> Ref/sequences/ACKNOWLEDGE_SEQUENCE.seq /tmp/fp-seq/ACKNOWLEDGE_SEQUENCE.seq

Monitoring of the counter starts disabled (so a Ref without the sequences installed does not fault on its own); the
tests enable it with SET_MONITORING. The directory is kept short: CmdSequencer bounds the path to 40 characters.
"""

import subprocess
from pathlib import Path

import pytest

SEQUENCE_DIRECTORY = Path("/tmp/fp-seq")
SEQUENCE_SOURCES = Path(__file__).parent.parent.parent / "sequences"
STEPS = ["RESET_COUNT_SEQUENCE", "ACKNOWLEDGE_SEQUENCE"]

# One excursion takes COUNT_THRESHOLD + SYSTEM_ERROR_THRESHOLD cycles (14 s at 1 Hz) plus the response countdown and
# two sequences; allow generous margin for a loaded test machine
EXCURSION_TIMEOUT = 60


@pytest.fixture(scope="session", autouse=True)
def response_sequences(fprime_test_api_session):
    """Compile the response sequences into the directory the deployment reads them from"""
    SEQUENCE_DIRECTORY.mkdir(parents=True, exist_ok=True)
    for step in STEPS:
        result = subprocess.run(
            [
                "fprime-seqgen",
                "--dictionary",
                str(fprime_test_api_session.dictionaries.dictionary_path),
                str(SEQUENCE_SOURCES / f"{step}.seq"),
                str(SEQUENCE_DIRECTORY / f"{step}.seq"),
            ]
        )
        assert result.returncode == 0, f"Failed to compile {step}.seq"


@pytest.fixture(scope="session", autouse=True)
def monitoring_enabled(fprime_test_api_session, response_sequences):
    """Enable the counter monitor once the response sequences are in place"""
    counter = fprime_test_api_session.get_mnemonic("Ref.MonitoredCounter")
    fprime_test_api_session.send_and_assert_command(f"{counter}.SET_MONITORING", ["ENABLED"], max_delay=5)
    fprime_test_api_session.clear_histories()


def names(fprime_test_api):
    """Resolve the deployment instance names of the components under test"""
    return (
        fprime_test_api.get_mnemonic("Svc.FaultProtection.FaultManager"),
        fprime_test_api.get_mnemonic("Svc.FaultProtection.SequenceResponder"),
        fprime_test_api.get_mnemonic("Ref.MonitoredCounter"),
    )


def await_excursion(fprime_test_api):
    """Wait for one complete fault -> response -> correction excursion and return its events"""
    FAULT_MANAGER, SEQUENCE_RESPONDER, COUNTER = names(fprime_test_api)
    fprime_test_api.clear_histories()
    return fprime_test_api.assert_event_sequence(
        [
            f"{COUNTER}.CountHighWarning",
            f"{COUNTER}.CountHighFault",
            f"{FAULT_MANAGER}.FaultReported",
            f"{FAULT_MANAGER}.ResponseStarted",
            f"{FAULT_MANAGER}.StepStarted",
            f"{SEQUENCE_RESPONDER}.SequenceStarted",
            f"{COUNTER}.CountReset",
            f"{SEQUENCE_RESPONDER}.SequenceCompleted",
            f"{FAULT_MANAGER}.StepCompleted",
            f"{FAULT_MANAGER}.StepStarted",
            f"{SEQUENCE_RESPONDER}.SequenceStarted",
            "CdhCore.cmdDisp.NoOpStringReceived",
            f"{SEQUENCE_RESPONDER}.SequenceCompleted",
            f"{FAULT_MANAGER}.StepCompleted",
            f"{FAULT_MANAGER}.ResponseCompleted",
        ],
        timeout=EXCURSION_TIMEOUT,
    )


def test_counter_fault_corrected_by_sequence(fprime_test_api):
    """The COUNTER_HIGH fault runs the two-step sequence response which resets the counter"""
    FAULT_MANAGER, SEQUENCE_RESPONDER, COUNTER = names(fprime_test_api)
    results = await_excursion(fprime_test_api)
    reported = results[2]
    assert reported.get_args()[0].val == "COUNTER_HIGH"
    started = results[3]
    assert [arg.val for arg in started.get_args()] == ["RESET_COUNTER_RESPONSE", "COUNTER_HIGH"]
    first_step, second_step = results[4], results[9]
    assert first_step.get_args()[0].val == "RESET_COUNT_SEQUENCE"
    assert second_step.get_args()[0].val == "ACKNOWLEDGE_SEQUENCE"
    first_sequence = results[5]
    assert first_sequence.get_args()[2].val == str(SEQUENCE_DIRECTORY / "RESET_COUNT_SEQUENCE.seq")
    # The correction is visible in telemetry: the monitor recovers and the count restarts from the reset
    fprime_test_api.assert_telemetry(f"{COUNTER}.Monitor", value="GREEN", timeout=5)
    # Fault manager counters are written on response completion, which is within the excursion history
    fprime_test_api.assert_telemetry(f"{FAULT_MANAGER}.ResponsesFailed", value=0, timeout=5)
    fprime_test_api.clear_histories()
    count = fprime_test_api.assert_telemetry(f"{COUNTER}.Count", timeout=5)
    assert count.get_val() < 10


def test_fault_counters(fprime_test_api):
    """Fault manager telemetry counts reports and completed responses across excursions"""
    FAULT_MANAGER, SEQUENCE_RESPONDER, COUNTER = names(fprime_test_api)
    await_excursion(fprime_test_api)
    reported = fprime_test_api.assert_telemetry(f"{FAULT_MANAGER}.FaultsReported", timeout=5).get_val()
    completed = fprime_test_api.assert_telemetry(f"{FAULT_MANAGER}.ResponsesCompleted", timeout=5).get_val()
    await_excursion(fprime_test_api)
    fprime_test_api.assert_telemetry(f"{FAULT_MANAGER}.FaultsReported", value=reported + 1, timeout=5)
    fprime_test_api.assert_telemetry(f"{FAULT_MANAGER}.ResponsesCompleted", value=completed + 1, timeout=5)


def test_disabled_fault_not_responded(fprime_test_api):
    """A disabled fault is acknowledged with FaultDisabled and no response runs"""
    FAULT_MANAGER, SEQUENCE_RESPONDER, COUNTER = names(fprime_test_api)
    fprime_test_api.send_and_assert_command(
        f"{FAULT_MANAGER}.SET_FAULT_ENABLED", args=["COUNTER_HIGH", "DISABLED"], max_delay=5
    )
    fprime_test_api.assert_event(f"{FAULT_MANAGER}.FaultEnabledSet", timeout=5)
    try:
        fprime_test_api.clear_histories()
        fprime_test_api.assert_event_sequence(
            [f"{COUNTER}.CountHighFault", f"{FAULT_MANAGER}.FaultDisabled"], timeout=EXCURSION_TIMEOUT
        )
        fprime_test_api.assert_event_count(0, events=f"{FAULT_MANAGER}.ResponseStarted")
        # The counter stays uncorrected until commanded by the ground
        fprime_test_api.assert_telemetry(f"{COUNTER}.Monitor", value="RED", timeout=5)
        fprime_test_api.send_and_assert_command(f"{COUNTER}.RESET_COUNT", max_delay=5)
        fprime_test_api.assert_event(f"{COUNTER}.CountReset", timeout=5)
        fprime_test_api.assert_telemetry(f"{COUNTER}.Monitor", value="GREEN", timeout=5)
    finally:
        fprime_test_api.send_and_assert_command(
            f"{FAULT_MANAGER}.SET_FAULT_ENABLED", args=["COUNTER_HIGH", "ENABLED"], max_delay=5
        )
    # Protection is restored: the next excursion is corrected again
    await_excursion(fprime_test_api)


def test_disabled_response_skips_steps(fprime_test_api):
    """A disabled response walks its steps as skipped and completes without running sequences"""
    FAULT_MANAGER, SEQUENCE_RESPONDER, COUNTER = names(fprime_test_api)
    fprime_test_api.send_and_assert_command(
        f"{FAULT_MANAGER}.SET_RESPONSE_ENABLED", args=["RESET_COUNTER_RESPONSE", "DISABLED"], max_delay=5
    )
    try:
        fprime_test_api.clear_histories()
        results = fprime_test_api.assert_event_sequence(
            [
                f"{COUNTER}.CountHighFault",
                f"{FAULT_MANAGER}.FaultReported",
                f"{FAULT_MANAGER}.ResponseStarted",
                f"{FAULT_MANAGER}.StepSkipped",
                f"{FAULT_MANAGER}.StepSkipped",
                f"{FAULT_MANAGER}.ResponseCompleted",
            ],
            timeout=EXCURSION_TIMEOUT,
        )
        assert [r.get_args()[0].val for r in results[3:5]] == STEPS
        fprime_test_api.assert_event_count(0, events=f"{SEQUENCE_RESPONDER}.SequenceStarted")
    finally:
        # The monitor reports once per excursion: the count must be reset by hand to arm the next excursion
        fprime_test_api.send_and_assert_command(
            f"{FAULT_MANAGER}.SET_RESPONSE_ENABLED", args=["RESET_COUNTER_RESPONSE", "ENABLED"], max_delay=5
        )
        fprime_test_api.send_and_assert_command(f"{COUNTER}.RESET_COUNT", max_delay=5)
    await_excursion(fprime_test_api)


def test_invalid_command_arguments_rejected(fprime_test_api):
    """Out-of-range enumeration values from the ground are rejected with a validation error, not an assertion"""
    FAULT_MANAGER, SEQUENCE_RESPONDER, COUNTER = names(fprime_test_api)
    for command, args, event in [
        (f"{FAULT_MANAGER}.SET_FAULT_ENABLED", ["NUM_FAULTS", "DISABLED"], "InvalidFaultArgument"),
        (f"{FAULT_MANAGER}.SET_RESPONSE_ENABLED", ["NUM_RESPONSES", "DISABLED"], "InvalidResponseArgument"),
        (f"{FAULT_MANAGER}.UPDATE_STEP_FAILURE_MODE", ["SKIP", "IGNORE"], "InvalidStepArgument"),
    ]:
        fprime_test_api.clear_histories()
        fprime_test_api.send_command(command, args=args)
        fprime_test_api.assert_event(f"{FAULT_MANAGER}.{event}", timeout=5)
        fprime_test_api.assert_event("CdhCore.cmdDisp.OpCodeError", timeout=5)
    # The system is still protected
    await_excursion(fprime_test_api)
