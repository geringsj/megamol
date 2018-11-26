/*
 * SpoutView3D.h
 *
 * Copyright (C) 2008 - 2010, 2018 by VISUS (Universitaet Stuttgart). 
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_SPOUT_OSCHANDLER_H_INCLUDED
#define MEGAMOL_SPOUT_OSCHANDLER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

 // oscpack send
#include "../oscpack/osc/OscOutboundPacketStream.h"

// oscpack receive/listen
#include "../oscpack/osc/OscReceivedElements.h"
#include "../oscpack/osc/OscPacketListener.h"

// oscpack both
#include "../oscpack/ip/UdpSocket.h"

#include <vector>

namespace megamol {
namespace spout_sender {

	class OSCHandler {
	public:
		OSCHandler();

		~OSCHandler();

		void send();

		bool isReady();
		void clearStream();

		template<typename T>
		osc::OutboundPacketStream& operator<<(T t) { return m_sendingStream << t; }

		IpEndpointName    m_sendingEndpoint;
		UdpTransmitSocket m_sendingSocket;

	private:
		std::vector<char>         m_sendingBuffer;
		osc::OutboundPacketStream m_sendingStream;
	};

} /* end namespace spout_sender */
} /* end namespace megamol */

#endif /* MEGAMOL_SPOUT_OSCHANDLER_H_INCLUDED */
