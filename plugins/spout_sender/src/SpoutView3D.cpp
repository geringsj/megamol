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

	const auto resizeFBO = [&](auto& fbo, const int width, const int height)
	{
		// maybe FBO texture resolution needs to change
		if (areDimsDifferent(width, height, fbo.GetWidth(), fbo.GetHeight()))
		{
			if (fbo.IsValid())
				fbo.Release();
	
			fbo.Create(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, SpoutView3D::FramebufferObject::ATTACHMENT_TEXTURE,
				GL_DEPTH_COMPONENT24, SpoutView3D::FramebufferObject::ATTACHMENT_DISABLED, GL_STENCIL_INDEX);
		}
	};
}

/*
 * spout_sender::SpoutView3D::Render
 */
void SpoutView3D::Render(const mmcRenderViewContext& context) {
	updateParameters();

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // to get the Bbox, we issue a Render(). clean it up.

	checkOneTimeDataShare(context);
	sendData();

    bool hasCam = m_camReceiver.getData<interop::StereoCameraConfiguration>(m_stereoCamConfig);

	if (hasCam)
	{
        m_cameraConfig_L = m_stereoCamConfig.cameraLeftEye;
        m_cameraConfig_R = m_stereoCamConfig.cameraRightEye;
		this->applyCameraConfig(m_stereoLCam, m_cameraConfig_L);
		this->applyCameraConfig(m_stereoRCam, m_cameraConfig_R);
	}

	const auto setFBO = [&](FramebufferObject& fbo)
	{
		int fboVp[4] = { 0, 0, fbo.GetWidth(), fbo.GetHeight() };
		Base::overrideViewport = fboVp;
		Base::rendererSlot.CallAs<view::CallRender3D>()->SetOutputBuffer(&fbo);
		fbo.Enable();
	};

	if (hasCam && isNewFbSize(m_cameraConfig_L.projectionParameters.pixelWidth, m_cameraConfig_L.projectionParameters.pixelHeight))
	{
		Base::Resize(m_cameraConfig_L.projectionParameters.pixelWidth, m_cameraConfig_L.projectionParameters.pixelHeight);
        resizeFBO(m_stereoFBO_L, m_cameraConfig_L.projectionParameters.pixelWidth, m_cameraConfig_L.projectionParameters.pixelHeight);
        resizeFBO(m_stereoFBO_R, m_cameraConfig_R.projectionParameters.pixelWidth, m_cameraConfig_R.projectionParameters.pixelHeight);
	}

	// on screen resize
	// also sets camera aspect ratio
    //this->camParams->SetVirtualViewSize(
    //    static_cast<vislib::graphics::ImageSpaceType>(width), static_cast<vislib::graphics::ImageSpaceType>(height));

	// TODO check for new camera matrices / render options from unity

	//switch(m_currentRenderingMode) {
	//	case CameraMode::Mono:
			// setFBO(m_monoFBO);
			// renderFromCamera(m_monoCam, context);
			// broadcastFramebuffer(m_monoFBO, m_monoImageData);
			// m_monoFBO.Disable();
			// m_monoFBO.DrawColourTexture();
	//		break;
	//	case CameraMode::Stereo:
			setFBO(m_stereoFBO_L);
			renderFromCamera(m_stereoLCam, context);
			broadcastFramebuffer(m_stereoFBO_L, m_stereoImageSender_L);
			m_stereoFBO_L.Disable();

			setFBO(m_stereoFBO_R);
			renderFromCamera(m_stereoRCam, context);
			broadcastFramebuffer(m_stereoFBO_R, m_stereoImageSender_R);
			m_stereoFBO_R.Disable();
	//		break;
	//}
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
    auto defBbox = m_dataBbox;

	if (!oneTimeDataIsShared)
	{
		// get one time data: bbox, ??
		Base::Render(context); // base render sets bbox of dataset

		//m_dataBbox = this->bboxs.ObjectSpaceBBox();
		const auto bbox = this->bboxs.WorldSpaceBBox();
		// bbox is such a fuckup

		defBbox.min = interop::vec4();
		defBbox.max = interop::vec4();

		// std::cout << "is OS Bbox valid: " << this->bboxs.IsObjectSpaceBBoxValid() << ", ObjectSpaceScale: " << this->bboxs.ObjectSpaceScale() << std::endl;
		// std::cout << "is WS Bbox valid: " << this->bboxs.IsWorldSpaceBBoxValid() << std::endl;
		oneTimeDataIsShared = true;
        m_dataBbox = defBbox;
	}
}

void megamol::spout_sender::SpoutView3D::sendData()
{
}

void  megamol::spout_sender::SpoutView3D::broadcastFramebuffer(FramebufferObject& fbo, interop::TextureSender& sender)
{
	if (!this->m_spoutSenderActive)
		return;

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

	// inherit initial camera parameters
	// TODO: overwrite with cam data from unity
	const CameraParamsStore camParams{ *this->cam.Parameters() };
	this->m_monoCamParameters = new CameraParamsStore(camParams);
	this->m_stereoLCamParameters = new CameraParamsStore(camParams);
	this->m_stereoRCamParameters = new CameraParamsStore(camParams);

	this->m_monoCam = CameraOpenGL(m_monoCamParameters);
	this->m_stereoLCam = CameraOpenGL(m_stereoLCamParameters);
	this->m_stereoRCam = CameraOpenGL(m_stereoRCamParameters);


	// m_dataBbox - set in render()
	m_camReceiver.start("tcp://127.0.0.1:12345", "camera");
	// interop::DataReceiver m_datasetPoseReceiver;

	m_stereoFBO_L.Create(1, 1);
	m_stereoFBO_R.Create(1, 1);

    m_stereoImageSender_L.init("megamolLeft");
    m_stereoImageSender_R.init("megamolRight");

    return true;
}


/*
 * spout_sender::SpoutView3D::release
 */
void SpoutView3D::release(void) {
	Base::release();

	m_camReceiver.stop();

	m_stereoFBO_L.Release();
	m_stereoFBO_R.Release();

    m_stereoImageSender_L.destroy();
    m_stereoImageSender_R.destroy();
}

void SpoutView3D::applyCameraConfig(SpoutView3D::CameraOpenGL& cam, const interop::CameraConfiguration& conf)
{
	vislib::math::Point<vislib::graphics::SceneSpaceType, 3> position(
		conf.viewParameters.eyePos.x,
		conf.viewParameters.eyePos.y,
		conf.viewParameters.eyePos.z);

	vislib::math::Point<vislib::graphics::SceneSpaceType, 3> lookAt(
		conf.viewParameters.lookAtPos.x,
		conf.viewParameters.lookAtPos.y,
		conf.viewParameters.lookAtPos.z);

	vislib::math::Vector<vislib::graphics::SceneSpaceType, 3> up(
		conf.viewParameters.camUpDir.x,
		conf.viewParameters.camUpDir.y,
		conf.viewParameters.camUpDir.z);

	cam.Parameters()->SetView(position, lookAt, up);

	cam.Parameters()->SetApertureAngle(conf.projectionParameters.fieldOfViewY_rad * 180.f / vislib::math::PI_DOUBLE); // cam.Parameters()->SetApertureAngle(conf.vieldOfViewY_deg);
	cam.Parameters()->SetNearClip(conf.projectionParameters.nearClipPlane);
	cam.Parameters()->SetFarClip(conf.projectionParameters.farClipPlane);
    cam.Parameters()->SetVirtualViewSize(
		static_cast<vislib::graphics::ImageSpaceType>(conf.projectionParameters.pixelWidth),
		static_cast<vislib::graphics::ImageSpaceType>(conf.projectionParameters.pixelHeight));
    cam.Parameters()->SetTileRect(vislib::math::Rectangle<float>(0.0f, 0.0f, conf.projectionParameters.pixelWidth, conf.projectionParameters.pixelHeight));

	vislib::math::Vector<vislib::graphics::SceneSpaceType, 3> translate(0.f, 0.f, 0.f);
		//conf.dataBboxTranslate[0], conf.dataBboxTranslate[1], conf.dataBboxTranslate[2]);
	cam.SetTranslate(translate);
    cam.SetScale(1.0f);//conf.dataBboxScale);
}

