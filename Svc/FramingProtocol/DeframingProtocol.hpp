// ======================================================================
// \title  DeframingProtocol.hpp
// \author mstarch
// \brief  hpp file for DeframingProtocol class
//
// \copyright
// Copyright 2009-2021, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "Svc/FramingProtocol/DeframingProtocolInterface.hpp"
#include "Fw/Com/ComPacket.hpp"
#include "Utils/Types/CircularBuffer.hpp"

#ifndef DEFRAMING_PROTOCOL_HPP
#define DEFRAMING_PROTOCOL_HPP

namespace Svc {


/**
 * \brief Abstract base class representing a deframing protocol
 *
 * This class represents the basic interface for writing a deframing protocol. This class may be
 * subclassed to provide concrete implementations for the protocol. A DeframingProtocolInterface is
 * be supplied using the `setup` call and is usually a reference to the DeframingComponentImpl.
 *
 * Implementations are expected to call `m_interface.route` to send the deframed data and may call
 * `m_interface.allocate` to allocate new memory. Implementors are expected to only consume data (using ring.rotate)
 * when that data is not needed again.  Use `ring.peek` calls to preview data.  An example of this is the fprime
 * protocol, which rotates away 1 byte on errors and the whole frame on success.  All real data access is through peeks
 * until it is a known error or success.  Unless returning "MORE NEEDED" some data is expected to be consumed from the
 * ring buffer. Implementor should never require more data that ring.get_capacity() this will trigger an assert.
 */
class DeframingProtocol {
  public:
    /**
     * \brief Status of the deframing call
     */
    enum DeframingStatus {
        DEFRAMING_STATUS_SUCCESS, /*!< Successful deframing */
        DEFRAMING_INVALID_FRAME, /*!< Frame was invalid */
        DEFRAMING_INVALID_CHECKSUM, /*!< Invalid checksum */
        DEFRAMING_MORE_NEEDED, /*!< Successful deframing likely with more data */
        DEFRAMING_MAX_STATUS
    };
    //! Constructor
    //!
    DeframingProtocol();

    //! Setup the deframing protocol with the deframing interface
    //!
    void setup(DeframingProtocolInterface& interface /*!< Deframing interface */
    );

    //! Deframe packets from within the circular buffer
    //! \return deframing status of this deframe attempt
    virtual DeframingStatus deframe(Types::CircularBuffer& ring  /*!< Deframe from circular buffer */
    ) = 0;

  PROTECTED:
    DeframingProtocolInterface* m_interface;
};
};
#endif  // OWLS_PROTOCOL_HPP
