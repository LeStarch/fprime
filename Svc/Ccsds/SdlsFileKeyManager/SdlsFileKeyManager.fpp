module Svc {
module Ccsds {
    @ SdlsFileKeyManager is a passive component supplying SDLS keys read from a file.
    @ The file path and key length are set at runtime via a configure() call. Each key
    @ request opens the file, reads exactly the configured number of bytes into the
    @ caller-provided key buffer, and returns SUCCESS. Any file error (open, read, or
    @ short read), or a request before configuration, returns KEY_ERROR.
    passive component SdlsFileKeyManager {

        @ Event raised when the key file cannot be read
        event KeyReadFailed(status: I32, bytesRead: FwSizeType, expectedSize: FwSizeType) \
            severity warning high \
            format "Failed to read key file: status {}, read {} of {} bytes"

        @ Event raised when a key is requested before the component is configured
        event NotConfigured() severity warning high format "Key requested before configure() was called"

        ##############################################################################
        #### Interfaces used by the component                                     ####
        ##############################################################################

        # This component supplies SDLS keys on request
        import Svc.Ccsds.SdlsKeyInterface

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################

        import Fw.Event

        @ Port for requesting the current time
        time get port timeCaller

    }
}
}
