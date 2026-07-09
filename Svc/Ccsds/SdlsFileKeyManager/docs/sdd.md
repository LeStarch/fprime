# Svc::Ccsds::SdlsFileKeyManager

The `Svc::Ccsds::SdlsFileKeyManager` component supplies SDLS keys read from a file via the `Svc.Ccsds.SdlsKeyInterface` interface. The key file path and the key length to read are set at runtime through a `configure()` call, since these vary across instances of the component. Each key request opens the file, reads exactly the configured number of bytes into the caller-provided `SdlsKeyBuffer`, closes the file, and returns `SdlsStatus.SUCCESS`. Any file error results in an `SdlsStatus.KEY_ERROR` return.

## Functionality

- `configure(path, keySize)` must be called during topology setup. It asserts that `keySize` is in `(0, MAX_SDLS_KEY_SIZE]`, where `MAX_SDLS_KEY_SIZE` is defined in the `SdlsKeyConfig` configuration module.
- Key requests arrive on the guarded `keyGet` port carrying a reference to an on-stack `SdlsKeyBuffer` (`Fw::LinearBufferTemplate<MAX_SDLS_KEY_SIZE>`).
- On each request the component opens the key file, reads exactly `keySize` bytes into the buffer, and returns `SUCCESS`. The file is opened and closed per request, so the key file may be replaced at runtime and no file handle is held open.
- On any failure — request before configuration, open error, read error, or short read — the component returns `KEY_ERROR` and emits a WARNING_HI event (`NotConfigured` or `KeyReadFailed`, the latter carrying the OS status and byte counts).

## Port Descriptions

| Kind          | Name   | Port Type          | Description |
|---------------|--------|--------------------|-------------|
| guarded input | keyGet | Svc.Ccsds.SdlsKey  | Receives a key request: fills the provided `SdlsKeyBuffer` with the key and returns the operation status. |

## Events

| Name | Severity | Description |
|------|----------|-------------|
| KeyReadFailed | WARNING_HI | The key file could not be read (open error, read error, or short read); carries the OS status and bytes read/expected. |
| NotConfigured | WARNING_HI | A key was requested before `configure()` was called. |

## Requirements

| Name | Description | Validation |
|------|-------------|------------|
| SVC-CCSDS-SDLS-FILE-KEY-MANAGER-001 | The SdlsFileKeyManager shall accept key requests via the `Svc.Ccsds.SdlsKeyInterface` interface (guarded `keyGet`), filling the provided `SdlsKeyBuffer`. | Unit Test |
| SVC-CCSDS-SDLS-FILE-KEY-MANAGER-002 | Upon a key request, the SdlsFileKeyManager shall read exactly the configured key length from the configured file into the provided buffer and return `SdlsStatus.SUCCESS`. | Unit Test |
| SVC-CCSDS-SDLS-FILE-KEY-MANAGER-003 | The SdlsFileKeyManager shall return `SdlsStatus.KEY_ERROR` and emit a WARNING_HI event on any file error (open error, read error, or short read) or when unconfigured. | Unit Test |
| SVC-CCSDS-SDLS-FILE-KEY-MANAGER-004 | The key file path and key length shall be supplied at runtime via `configure()`; the key length shall be in `(0, MAX_SDLS_KEY_SIZE]`. | Unit Test |

## See Also

- [`Svc/Ccsds/Interfaces/SdlsKey.fpp`](../../Interfaces/SdlsKey.fpp)
- [`Svc/Ccsds/Types/SdlsKeyBuffer.hpp`](../../Types/SdlsKeyBuffer.hpp)
