module Svc {
    @ ----------------------------------------------------------------------
    @ CCSDS SDLS Encryption
    @ ----------------------------------------------------------------------
    interface CcsdsSdlsEncrypt {
        @ Port to receive the security association index and iv/data buffer to encrypt
        guarded input port encryptIn: Svc.Ccsds.CcsdsSdlsEncryption

        @ Port for returning the iv/data buffer
        output port bufferOut: Fw.BufferSend
    }
}
