module Svc {
    @ ----------------------------------------------------------------------
    @ CCSDS SDLS Decryption
    @ ----------------------------------------------------------------------
    interface CcsdsSdlsDecrypt {
        @ Port to receive the security association index and iv/data buffer to decrypt
        guarded input port decryptIn: Svc.Ccsds.CcsdsSdlsEncryption

        @ Port for returning the iv/data buffer
        output port bufferOut: Fw.BufferSend
    }
}
