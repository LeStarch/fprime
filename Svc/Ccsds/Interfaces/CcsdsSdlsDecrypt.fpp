module Svc {
module Ccsds {

    @ Interface for components performing CCSDS SDLS (Space Data Link Security) decryption:
    @ receives a security association index and iv/data buffer, and returns the iv/data buffer
    interface CcsdsSdlsDecrypt {
        @ Port to receive the security association index and iv/data buffer to decrypt
        guarded input port decryptIn: Svc.Ccsds.CcsdsSdlsEncryption

        @ Port for returning the iv/data buffer
        output port bufferOut: Fw.BufferSend
    }

}
}
