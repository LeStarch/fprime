event FaultResponse(
                     sequence: string size 1024
                     fault: FaultCfg.FaultId
                     context: U32
                   ) \
  severity activity high \
  id 0 \
  format "Executing {} in response to fault {}. Fault context {}."

event SequenceDoesNotExist(
                            sequence: string size 1024
                          ) \
  severity warning high \
  id 1 \
  format "Sequence {} does not exist invalid. Future faults will FATAL."

event SequenceFailed(
                      sequence: string size 1024
                    ) \
  severity fatal \
  id 2 \
  format "Sequence {} failed"

event NoResponse(
                  fault: FaultCfg.FaultId
                  context: U32
                ) \
  severity fatal \
  id 3 \
  format "No response registered for fault {}. Fault context {}."
