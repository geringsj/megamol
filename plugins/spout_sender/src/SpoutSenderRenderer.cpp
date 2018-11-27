/*
* ReplacementRenderer.cpp
*
* Copyright (C) 2017 by VISUS (Universitaet Stuttgart).
* Alle Rechte vorbehalten.
*/

#include "stdafx.h"

#include "SpoutSenderRenderer.h"

#include "mmcore/param/BoolParam.h"
//#include "mmcore/param/FloatParam.h"
//#include "mmcore/param/EnumParam.h"
//#include "mmcore/param/IntParam.h"
//#include "mmcore/param/ButtonParam.h"

#include "vislib/graphics/gl/IncludeAllGL.h"
#include "vislib/sys/Log.h"


using namespace megamol;
using namespace megamol::core;
using namespace megamol::spout_sender;
using namespace vislib;

/*
* SpoutSenderRenderer::SpoutSenderRenderer (CTOR)
*/
SpoutSenderRenderer::SpoutSenderRenderer(void) 
	: Renderer3DModule()
	, m_toggleSpoutSending(true)
	, m_sender()
	, m_senderName()
	, m_texture(0)
	, m_textureWidth(1), m_textureHeight(1)
	, m_rendererCallerSlot("renderer", "outgoing renderer")
	, m_toggleSpoutSendingParam("01_toggleSpoutSender", "Toggle sending texture via Spout.")
{
	// init variables
	this->m_rendererCallerSlot.SetCompatibleCall<view::CallRender3DDescription>();
	this->MakeSlotAvailable(&this->m_rendererCallerSlot);

	this->m_toggleSpoutSendingParam.SetParameter(new param::BoolParam(this->m_toggleSpoutSending));
	this->MakeSlotAvailable(&this->m_toggleSpoutSendingParam);
}

/*
* SpoutSenderRenderer::~ReplacementRenderer (DTOR)
*/
SpoutSenderRenderer::~SpoutSenderRenderer(void) {
	this->Release();
}

/*
* SpoutSenderRenderer::release
*/
void SpoutSenderRenderer::release(void) {
	m_sender.ReleaseSender();
}

/*
* SpoutSenderRenderer::create
*/
bool SpoutSenderRenderer::create(void) {
	m_senderName = "SpoutSender_MegaMol_Mono";
	m_senderName.resize(256, '\0');
	
	if (!m_sender.CreateSender(m_senderName.c_str(), m_textureWidth, m_textureHeight)) {
		// TODO sender creation failed ??
	}

	return true;
}

/*
* SpoutSenderRenderer::GetCapabilities
*/
bool SpoutSenderRenderer::GetCapabilities(Call& call) { 
	return true;
}

/*
* SpoutSenderRenderer::GetExtents
*/
bool SpoutSenderRenderer::GetExtents(Call& call) {

	view::CallRender3D *cr3d_in = dynamic_cast<view::CallRender3D*>(&call);
	if (cr3d_in == nullptr) return false;

	// Propagate changes made in GetExtents() from outgoing CallRender3D (cr3d_out) to incoming  CallRender3D (cr3d_in).
	view::CallRender3D *cr3d_out = this->m_rendererCallerSlot.CallAs<view::CallRender3D>();
	if ((cr3d_out != nullptr) && (*cr3d_out)(core::view::AbstractCallRender::FnGetExtents)) {
		unsigned int timeFramesCount = cr3d_out->TimeFramesCount();
		cr3d_in->SetTimeFramesCount((timeFramesCount > 0) ? (timeFramesCount) : (1));
		cr3d_in->SetTime(cr3d_out->Time());
		cr3d_in->AccessBoundingBoxes() = cr3d_out->AccessBoundingBoxes();

		//this->bbox = cr3d_out->AccessBoundingBoxes().WorldSpaceBBox();
	}

	return true;
}


/*
* SpoutSenderRenderer::Render
*/
bool SpoutSenderRenderer::Render(Call& call) {

	view::CallRender3D *cr3d_in = dynamic_cast<view::CallRender3D*>(&call);
	if (cr3d_in == nullptr)  return false;

	// Update parameters
	if (this->m_toggleSpoutSendingParam.IsDirty()) {
		this->m_toggleSpoutSendingParam.ResetDirty();

		this->m_toggleSpoutSending = !this->m_toggleSpoutSending;
		this->m_toggleSpoutSendingParam.Param<param::BoolParam>()->SetValue(this->m_toggleSpoutSending, false);
	}

	// Call render function of slave renderer
	view::CallRender3D *cr3d_out = this->m_rendererCallerSlot.CallAs<view::CallRender3D>();
	if (cr3d_out != nullptr) {
		*cr3d_out = *cr3d_in;
		(*cr3d_out)(core::view::AbstractCallRender::FnRender);
	}

	// send fbo texture via spout
	if (this->m_toggleSpoutSending) {
		this->updateTextureDimensions();
		this->copyTexture();
		this->sendTexture();
	}

	return true;
}


bool SpoutSenderRenderer::isNewTextureSize(const GLint width, const GLint height) {
	return 
		m_textureWidth != static_cast<unsigned int>(width)
		|| m_textureHeight != static_cast<unsigned int>(height);
}

void SpoutSenderRenderer::updateTextureDimensions() {
	// check FBO size
	GLint dims[4] = { 0 };
	glGetIntegerv(GL_VIEWPORT, dims);
	GLint fbWidth = dims[2]; GLint fbHeight = dims[3];

	if (this->isNewTextureSize(fbWidth, fbHeight))
	{
		m_textureWidth = static_cast<unsigned int>(fbWidth);
		m_textureHeight = static_cast<unsigned int>(fbHeight);

		this->initTexture(m_textureWidth, m_textureHeight);

		m_sender.UpdateSender(m_senderName.c_str(), m_textureWidth, m_textureHeight);
	}
}

void SpoutSenderRenderer::copyTexture() {
	glReadBuffer(GL_BACK);
	glCopyTextureSubImage2D(m_texture, 0 /*lvl*/, 0, 0/*x, y texture offset*/, 0, 0 /*window coords offset*/, m_textureWidth, m_textureHeight);
	std::cout << "SPOUT_SENDER w=" << m_textureWidth << ", h=" << m_textureHeight << std::endl;
}


void SpoutSenderRenderer::sendTexture() {
	m_sender.SendTexture(m_texture, GL_TEXTURE_2D, m_textureWidth, m_textureHeight);
}

void SpoutSenderRenderer::initTexture(const unsigned int width, const unsigned int height) {
	if (m_texture == 0)
		glGenTextures(1, &m_texture);

	GLenum format = GL_RGBA;
	GLint oldTexture = 0;

	//glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, GL_UNSIGNED_BYTE, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, oldTexture);
}

void SpoutSenderRenderer::destroyTexture() {
	glDeleteTextures(1, &m_texture);
}
