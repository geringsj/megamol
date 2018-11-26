/*
 * SpoutView3D.cpp
 *
 * Copyright (C) 2008 - 2010, 2018 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */


#include "OSCHandler.h"

using namespace megamol::spout_sender;

OSCHandler::OSCHandler()
	: m_sendingEndpoint{"127.0.0.1", 4243}
	, m_sendingSocket{ m_sendingEndpoint }
	, m_sendingBuffer(1024, '\0')
	, m_sendingStream{ m_sendingBuffer.data(), m_sendingBuffer.size() }
{
}

OSCHandler::~OSCHandler()
{

}
void OSCHandler::send()
{
	m_sendingSocket.Send(m_sendingStream.Data(), m_sendingStream.Size());
}

bool OSCHandler::isReady()
{
	return m_sendingStream.IsReady();
}

void OSCHandler::clearStream()
{
	m_sendingStream.Clear();
}
