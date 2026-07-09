// ======================================================================
// \title  SdlsFileKeyManager.cpp
// \author lestarch-autobot
// \brief  cpp file for SdlsFileKeyManager component implementation class
// ======================================================================

#include "Svc/Ccsds/SdlsFileKeyManager/SdlsFileKeyManager.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/File.hpp"

namespace Svc {

namespace Ccsds {

// ----------------------------------------------------------------------
// Component construction, configuration, and destruction
// ----------------------------------------------------------------------

SdlsFileKeyManager ::SdlsFileKeyManager(const char* const compName)
    : SdlsFileKeyManagerComponentBase(compName), m_path(), m_keySize(0), m_configured(false) {}

SdlsFileKeyManager ::~SdlsFileKeyManager() {}

void SdlsFileKeyManager ::configure(const char* path, FwSizeType keySize) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(keySize > 0, static_cast<FwAssertArgType>(keySize));
    FW_ASSERT(keySize <= SdlsCfg::MAX_SDLS_KEY_SIZE, static_cast<FwAssertArgType>(keySize));
    this->m_path = path;
    this->m_keySize = keySize;
    this->m_configured = true;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

Svc::Ccsds::SdlsStatus SdlsFileKeyManager ::keyGet_handler(FwIndexType portNum, Svc::Ccsds::SdlsKeyBuffer& key) {
    if (!this->m_configured) {
        this->log_WARNING_HI_NotConfigured();
        return SdlsStatus::KEY_ERROR;
    }
    Os::File file;
    const Os::File::Status openStatus = file.open(this->m_path.toChar(), Os::File::OPEN_READ);
    if (openStatus != Os::File::OP_OK) {
        this->log_WARNING_HI_KeyReadFailed(static_cast<I32>(openStatus), 0, this->m_keySize);
        return SdlsStatus::KEY_ERROR;
    }
    U8 keyData[SdlsCfg::MAX_SDLS_KEY_SIZE];
    FwSizeType readSize = this->m_keySize;
    const Os::File::Status readStatus = file.read(keyData, readSize);
    if ((readStatus != Os::File::OP_OK) || (readSize != this->m_keySize)) {
        this->log_WARNING_HI_KeyReadFailed(static_cast<I32>(readStatus), readSize, this->m_keySize);
        return SdlsStatus::KEY_ERROR;
    }
    const Fw::SerializeStatus setStatus = key.setBuff(keyData, this->m_keySize);
    FW_ASSERT(setStatus == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(setStatus));
    return SdlsStatus::SUCCESS;
}

}  // namespace Ccsds

}  // namespace Svc
