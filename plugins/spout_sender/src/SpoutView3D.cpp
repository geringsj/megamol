/*
 * SpoutView3D.cpp
 *
 * Copyright (C) 2008 - 2010, 2018 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */


#include "SpoutView3D.h"

#include "mmcore/view/CallRender3D.h"
#include "mmcore/param/BoolParam.h"

#include <iostream>

using namespace megamol::spout_sender;
using namespace megamol::core;

using CameraParamsStore = vislib::graphics::CameraParamsStore;

/*
 * spout_sender::SpoutView3D::SpoutView3D
 */
SpoutView3D::SpoutView3D(void)
    : View3D()
	, m_spoutSenderActive(true)
	, m_spoutSenderActiveParam("spout::senderActive", "Toggle sending texture via Spout.")
{
	// init variables
	this->m_spoutSenderActiveParam.SetParameter(new param::BoolParam(this->m_spoutSenderActive));
	this->MakeSlotAvailable(&this->m_spoutSenderActiveParam);
}


/*
 * spout_sender::SpoutView3D::~SpoutView3D
 */
SpoutView3D::~SpoutView3D(void)
{
	this->Release();
}

namespace {
	auto areDimsDifferent = [](auto width1, auto height1, auto width2, auto height2) -> bool
	{
		return
			static_cast<decltype(width2)>(width1) != width2
			|| static_cast<decltype(height2)>(height1) != height2;
	};
}

/*
 * spout_sender::SpoutView3D::Render
 */
void SpoutView3D::Render(const mmcRenderViewContext& context) {
	updateParameters();
	m_monoFBO.Disable();
	checkOneTimeDataShare(context);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // to get the Bbox, we issue a Render(). clean it up.
	//checkFramebufferSize();

	//if(!m_unitySharedData.oneTimeDataIsShared)
		sendData();

	if (this->m_udpReceiveSocketPtr == nullptr)
	{
		try {
			//auto& ipEndpointName = m_oscSendChannel.m_sendingEndpoint;
			this->m_udpReceiveSocketPtr = 
				std::make_unique<UdpListeningReceiveSocket>(IpEndpointName("127.0.0.1", 4243), &m_oscReceiveChannel);

			m_oscReceiveThread = std::thread{[&]() { this->m_udpReceiveSocketPtr->Run(); }};
		}
		catch (...)
		{
			std::cout << "Failed to set up OSC Receiver via UDP" << std::endl;
		}
	}

	if (m_oscReceiveChannel.hasData())
	{
		const CameraConfig stereoLConf = m_oscReceiveChannel.stereoLCam.load();
		const CameraConfig stereoRConf = m_oscReceiveChannel.stereoRCam.load();
		const CameraConfig monoConf = m_oscReceiveChannel.getData<CameraConfig>();
		this->applyCameraConfig(m_monoCam,    monoConf);
		this->applyCameraConfig(m_stereoLCam, stereoLConf);
		this->applyCameraConfig(m_stereoRCam, stereoRConf);

		const auto resizeFBO = [&](auto& fbo, const auto& camConfig)
		{
			// maybe FBO texture resolution needs to change
			int width = camConfig.viewWidth_px, height = camConfig.viewHeight_px;
			if (areDimsDifferent(width, height, fbo.GetWidth(), fbo.GetHeight()))
			{ 

				if (fbo.IsValid())
					fbo.Release();
				fbo.Create(width, height,
					GL_RGBA8,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					FramebufferObject::ATTACHMENT_TEXTURE,
					GL_DEPTH_COMPONENT24,
					FramebufferObject::ATTACHMENT_DISABLED,
					GL_STENCIL_INDEX);
				//m_texture.create(width, height);

				std::cout << "===> FBO Size (" <<
					fbo.GetWidth() << ", " << 
					fbo.GetHeight()
					<< ")" << std::endl;
			}
		};

		// maybe FBO texture resolution needs to change
		resizeFBO(m_monoFBO, monoConf);
		resizeFBO(m_stereoFBO_L, stereoLConf);
		resizeFBO(m_stereoFBO_R, stereoRConf);
	}

	if (!m_monoFBO.IsValid())
		std::cout << "FBO NOT VALID BRUH" << std::endl;

	const auto setFBO = [&](auto& fbo)
	{
		int fboVp[4] = { 0, 0, fbo.GetWidth(), fbo.GetHeight() };
		Base::overrideViewport = fboVp;
		Base::rendererSlot.CallAs<view::CallRender3D>()->SetOutputBuffer(&fbo);
		fbo.Enable();
	};

	if (isNewFbSize(m_monoFBO.GetWidth(), m_monoFBO.GetHeight()))
		Base::Resize(m_monoFBO.GetWidth(), m_monoFBO.GetHeight());

	//Base::rendererSlot.CallAs<view::CallRender3D>()->EnableOutputBuffer();

	std::cout << "View Size (" <<
		this->cam.Parameters()->TileRect().Width() << ", " << 
		this->cam.Parameters()->TileRect().Height()
		<< ")" << std::endl;

	// on screen resize
	// also sets camera aspect ratio
    //this->camParams->SetVirtualViewSize(
    //    static_cast<vislib::graphics::ImageSpaceType>(width), static_cast<vislib::graphics::ImageSpaceType>(height));

	// TODO check for new camera matrices / render options from unity

	//switch(m_currentRenderingMode) {
	//	case CameraMode::Mono:
			setFBO(m_monoFBO);
			renderFromCamera(m_monoCam, context);
			broadcastFramebuffer(m_monoFBO, m_monoImageData);
			m_monoFBO.Disable();
			m_monoFBO.DrawColourTexture();
	//		break;
	//	case CameraMode::Stereo:
			setFBO(m_stereoFBO_L);
			renderFromCamera(m_stereoLCam, context);
			broadcastFramebuffer(m_stereoFBO_L, m_stereoLImageData);
			m_stereoFBO_L.Disable();
			setFBO(m_stereoFBO_R);
			renderFromCamera(m_stereoRCam, context);
			broadcastFramebuffer(m_stereoFBO_R, m_stereoRImageData);
			m_stereoFBO_R.Disable();
	//		break;
	//}

	// framebuffer broadcasts applied new FB dimensions to textures
	m_hasNewFbSize = false;
}

void  megamol::spout_sender::SpoutView3D::renderFromCamera(const CameraOpenGL & viewCamera, const mmcRenderViewContext& context)
{
	// we would like to disable the camera updating itself, if that feature wasn't protected...
	// this->cam.markAsUpdated();
	this->cam = viewCamera;
	Base::Render(context);
}

void megamol::spout_sender::SpoutView3D::checkOneTimeDataShare(const mmcRenderViewContext& context)
{
	if (!m_unitySharedData.oneTimeDataIsShared)
	{
		// get one time data: bbox, ??
		Base::Render(context); // base render sets bbox of dataset

		//m_dataBbox = this->bboxs.ObjectSpaceBBox();
		m_dataBbox = this->bboxs.WorldSpaceBBox();
		// bbox is such a fuckup

		const auto printBbox = [&](std::string name, auto bbox) {
			std::cout << name << " Bbox: min("
				<< bbox.GetLeftBottomBack().GetX() << ", "
				<< bbox.GetLeftBottomBack().GetY() << ", "
				<< bbox.GetLeftBottomBack().GetZ()
				<< "), max("
				<< bbox.GetRightTopFront().GetX() << ", "
				<< bbox.GetRightTopFront().GetY() << ", "
				<< bbox.GetRightTopFront().GetZ()
				<< ")" << std::endl;
		};

		std::cout << "is OS Bbox valid: " << this->bboxs.IsObjectSpaceBBoxValid() << ", ObjectSpaceScale: " << this->bboxs.ObjectSpaceScale() << std::endl;
		printBbox("Object Space", this->bboxs.ObjectSpaceBBox());
		std::cout << "is WS Bbox valid: " << this->bboxs.IsWorldSpaceBBoxValid() << std::endl;
		printBbox("World Space", this->bboxs.WorldSpaceBBox());

		m_unitySharedData.oneTimeDataIsShared = true;
	}
}

void megamol::spout_sender::SpoutView3D::sendData()
{
	if (!m_oscSendChannel.isReady())
		return;

	// send one time data
	const auto msgName = m_interopSenderId + "Bbox";
	m_oscSendChannel << osc::BeginBundleImmediate
		<< osc::BeginMessage(msgName.c_str())
			// min bbox point
			<< m_dataBbox.GetLeftBottomBack().GetX()
			<< m_dataBbox.GetLeftBottomBack().GetY()
			<< m_dataBbox.GetLeftBottomBack().GetZ()
			// max bbox point
			<< m_dataBbox.GetRightTopFront().GetX()
			<< m_dataBbox.GetRightTopFront().GetY()
			<< m_dataBbox.GetRightTopFront().GetZ()
		<< osc::EndMessage
	<< osc::EndBundle;

	m_oscSendChannel.send();
	m_oscSendChannel.clearStream();
}

void  megamol::spout_sender::SpoutView3D::broadcastFramebuffer(FramebufferObject& fbo, ImageDataSender& sender)
{
	if (!this->m_spoutSenderActive)
		return;

	//m_texture.copyFB();
	//sender.sendTexture(m_texture.handle, m_texture.width, m_texture.height);
	sender.sendTexture(fbo.GetColourTextureID(), fbo.GetWidth(), fbo.GetHeight());
}

void megamol::spout_sender::SpoutView3D::updateParameters()
{
	// Update own parameters
	if (this->m_spoutSenderActiveParam.IsDirty()) {
		this->m_spoutSenderActiveParam.ResetDirty();

		this->m_spoutSenderActive = !this->m_spoutSenderActive;
		this->m_spoutSenderActiveParam.Param<param::BoolParam>()->SetValue(this->m_spoutSenderActive, false);
	}
}

bool megamol::spout_sender::SpoutView3D::isNewFbSize(unsigned int width, unsigned int height)
{
	const auto newWidth = static_cast<GLint>(width);
	const auto newHeight = static_cast<GLint>(height);

	bool isNew = areDimsDifferent(newWidth, newHeight, m_fbWidth, m_fbHeight);

	m_fbWidth = newWidth;
	m_fbHeight = newHeight;

	return isNew;
}

/*
 * spout_sender::SpoutView3D::Resize
 */
void SpoutView3D::Resize(unsigned int width, unsigned int height) {
	Base::Resize(width, height);
}

void SpoutView3D::checkFramebufferSize()
{
	GLint dims[4] = { 0 };
	glGetIntegerv(GL_VIEWPORT, dims);
	const GLint fbWidth = dims[2];
	const GLint fbHeight = dims[3];

	m_hasNewFbSize = isNewFbSize(fbWidth, fbHeight);
	if (m_hasNewFbSize)
		Resize(fbWidth, fbHeight);
}

/*
 * spout_sender::SpoutView3D::OnRenderView
 */
bool SpoutView3D::OnRenderView(Call& call) {
	Base::OnRenderView(call);
    return true;
}

/*
 * spout_sender::SpoutView3D::create
 */
bool SpoutView3D::create(void) {
	Base::create();

	m_dataBbox.SetNull();

	// inherit initial camera parameters
	// TODO: overwrite with cam data from unity
	const CameraParamsStore camParams{ *this->cam.Parameters() };
	//this->m_monoCamParameters = new CameraParamsStore(camParams);
	this->m_stereoLCamParameters = new CameraParamsStore(camParams);
	this->m_stereoRCamParameters = new CameraParamsStore(camParams);

	this->m_monoCam = this->cam;// CameraOpenGL(m_monoCamParameters);
	this->m_stereoLCam = CameraOpenGL(m_stereoLCamParameters);
	this->m_stereoRCam = CameraOpenGL(m_stereoRCamParameters);
	
	// m_fbWidth, m_fbHeight initially 0
	// spout exported images
	std::string name = m_interopSenderId + "Image/";
	this->m_monoImageData.init(name + "Mono", 1, 1);// m_fbWidth, m_fbHeight);
	this->m_stereoLImageData.init(name + "StereoL", 1, 1);// m_fbWidth, m_fbHeight);
	this->m_stereoRImageData.init(name + "StereoR", 1, 1);// m_fbWidth, m_fbHeight);

	m_monoFBO.Create(1, 1);
	m_stereoFBO_L.Create(1, 1);
	m_stereoFBO_R.Create(1, 1);

	//m_texture.create(1, 1);

    return true;
}


/*
 * spout_sender::SpoutView3D::release
 */
void SpoutView3D::release(void) {
	Base::release();

	if (this->m_udpReceiveSocketPtr)
	{
		this->m_udpReceiveSocketPtr->AsynchronousBreak();
		m_oscReceiveThread.join();
	}

	this->m_monoImageData.destroy();
	this->m_stereoLImageData.destroy();
	this->m_stereoRImageData.destroy();

	m_monoFBO.Release();
	m_stereoFBO_L.Release();
	m_stereoFBO_R.Release();

	//m_texture.destroy();
}



// ***************************************
// *** implement struct ImageShareData ***
// ***************************************

bool SpoutView3D::ImageDataSender::init(std::string const& name, const unsigned int width, const unsigned int height)
{
	this->textureWidth = width;
	this->textureHeight = height;
	return initSender(name);
}
bool SpoutView3D::ImageDataSender::initSender(std::string const& name)
{
	// spout userguide says names _must_ use char arrays of size 256
	if (name.size() > 256)
		return false;

	senderName = name;
	senderName.resize(256, '\0');

	spoutSender.SetCPUmode(true); // work around GL-DX interop fail on AMD
	spoutSender.SetMemoryShareMode(false); // spout has a bug in new CPU-memory mode, so use shared memorymode, so use shared memory
	spoutSender.SetDX9(true);
	return spoutSender.CreateSender(senderName.c_str(), textureWidth, textureHeight);
}

void SpoutView3D::ImageDataSender::updateTextureDimensions(const GLint fbWidth, const GLint fbHeight)
{
	if (areDimsDifferent(textureWidth, textureHeight, fbWidth, fbHeight))
	{
		textureWidth = static_cast<unsigned int>(fbWidth);
		textureHeight = static_cast<unsigned int>(fbHeight);

		spoutSender.UpdateSender(senderName.c_str(), textureWidth, textureHeight);
	}
}

void SpoutView3D::ImageDataSender::sendTexture(const GLuint glTex, GLint width, GLint height)
{
	if (width == 0 || height == 0)
		return;

	updateTextureDimensions(width, height);

	std::cout << "SPOUT SENDINT TEXTURE " << textureWidth << ", " << textureHeight << std::endl;

	spoutSender.SendTexture(glTex, GL_TEXTURE_2D, textureWidth, textureHeight);
}

void SpoutView3D::ImageDataSender::destroy()
{
	destroySender();
}
void SpoutView3D::ImageDataSender::destroySender()
{
	spoutSender.ReleaseSender();
}


// OSC receive messages

template <typename T>
void pushToVec(std::vector<char>& vec, T val)
{
	char* ptr = reinterpret_cast<char*>(&val);

	for (size_t i = 0; i < sizeof(T); i++)
		vec.push_back(*(ptr++));
}

void SpoutView3D::OscPacketListener::ProcessMessage(
	const osc::ReceivedMessage& msg,
	const IpEndpointName& remoteEndpoint)
{
	m_data.reserve(128); // ought to be enough for two 4x4 floating point matrices

	osc::ReceivedMessage::const_iterator it = msg.ArgumentsBegin();

	if (std::string(msg.AddressPattern()).find("Camera/") == std::string::npos)
		return;

	std::cout << "Received OSC Message, Address: " << msg.AddressPattern() << std::endl;

	while (it != msg.ArgumentsEnd())
	{
		if(it->IsFloat())
		{
			float f = it++->AsFloat();
			pushToVec(m_data, f);
		}
		else
		if(it->IsInt32())
		{
			int i = it++->AsInt32();
			pushToVec(m_data, i);
		}
	}

	CameraConfig tmp;
	std::memcpy(&tmp, m_data.data(), sizeof(CameraConfig));

	if (std::string(msg.AddressPattern()).find("Mono") != std::string::npos)
	{
		monoCam.exchange(tmp);
	}
	else 
	if (std::string(msg.AddressPattern()).find("StereoL") != std::string::npos)
	{
		stereoLCam.exchange(tmp);
	}
	else 
	if (std::string(msg.AddressPattern()).find("StereoR") != std::string::npos)
	{
		stereoRCam.exchange(tmp);
	}

	this->m_data.clear();

	m_hasData = true;
}

template<>
SpoutView3D::CameraConfig SpoutView3D::OscPacketListener::getData()
{
	this->unsetDataFlag();
	return monoCam.load();
	// CameraConfig ret;

	// if(m_data.size() * sizeof(char) != sizeof(CameraConfig))
	// 	return ret;

	// std::memcpy(&ret, m_data.data(), sizeof(CameraConfig));

	// this->unsetDataFlag();
	// this->m_data.clear(); // osc part receives only when array cleared
	// return ret;
}

const auto printVec = [](auto& vec) {
	std::cout << "(" 
		<< vec[0] << ", "
		<< vec[1] << ", "
		<< vec[2] << ")" << std::endl;
};

void SpoutView3D::applyCameraConfig(SpoutView3D::CameraOpenGL& cam, const SpoutView3D::CameraConfig& conf)
{
	vislib::math::Point<vislib::graphics::SceneSpaceType, 3> position(
		conf.cam_pos[0], conf.cam_pos[1], conf.cam_pos[2]);
	vislib::math::Point<vislib::graphics::SceneSpaceType, 3> lookAt(
		conf.lookAt_pos[0], conf.lookAt_pos[1], conf.lookAt_pos[2]);
	vislib::math::Vector<vislib::graphics::SceneSpaceType, 3> up(
		conf.up_vec[0], conf.up_vec[1], conf.up_vec[2]);

	printVec(position);
	printVec(lookAt);
	printVec(up);

	cam.Parameters()->SetView(position, lookAt, up);

	cam.Parameters()->SetApertureAngle(conf.vieldOfViewY_deg);
	cam.Parameters()->SetNearClip(conf.near_dist);
	cam.Parameters()->SetFarClip(conf.far_dist);
	cam.Parameters()->SetVirtualViewSize(
		static_cast<vislib::graphics::ImageSpaceType>(conf.viewWidth_px),
		static_cast<vislib::graphics::ImageSpaceType>(conf.viewHeight_px));
    cam.Parameters()->SetTileRect(vislib::math::Rectangle<float>(0.0f, 0.0f, conf.viewWidth_px, conf.viewHeight_px));

	cam.SetScale(conf.dataBboxScale);

	std::cout
		<< "FoV: " << conf.vieldOfViewY_deg << std::endl
		<< "near: " << conf.near_dist << std::endl
		<< "far: " << conf.far_dist << std::endl
		<< "view width: " << conf.viewWidth_px << std::endl
		<< "view height: " << conf.viewHeight_px << std::endl
		<< "bbox scale: " << conf.dataBboxScale << std::endl;
}
