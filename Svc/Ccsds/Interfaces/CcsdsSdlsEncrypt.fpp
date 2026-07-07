module Svc {
module Ccsds {

    @ Interface for components performing CCSDS SDLS (Space Data Link Security) encryption:
    @ receives a security association index and iv/data buffer, and returns the iv/data buffer
    interface CcsdsSdlsEncrypt {
        @ Port to receive the security association index and iv/data buffer to encrypt
        guarded input port encryptIn: Svc.Ccsds.CcsdsSdlsEncryption

        @ Port for returning the iv/data buffer
        output port bufferOut: Fw.BufferSend
    }

}
}
