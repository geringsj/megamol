/*
 * VrInteropView3D.cpp
 *
 * Copyright (C) 2008 - 2010, 2018 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */


#include "VrInteropView3D_2.h"

#include "mmcore/CoreInstance.h"
#include "mmcore/Module.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ButtonParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/FilePathParam.h"
#include "mmcore/param/FlexEnumParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore/param/StringParam.h"
#include "mmcore/param/TernaryParam.h"
#include "mmcore/param/TransferFunctionParam.h"
#include "mmcore/param/Vector2fParam.h"
#include "mmcore/param/Vector3fParam.h"
#include "mmcore/param/Vector4fParam.h"
#include "mmcore/utility/ResourceWrapper.h"
#include "mmcore/versioninfo.h"
#include "mmcore/view/CallRender3D_2.h"
#include "mmcore/view/CallRenderView.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>

using namespace megamol::vrinterop;
using namespace megamol::core;

using CameraParamsStore = vislib::graphics::CameraParamsStore;

/*
 * vrinterop::VrInteropView3D::VrInteropView3D
 */
VrInteropView3D_2::VrInteropView3D_2(void) : View3D_2() {
    // init variables
}


/*
 * vrinterop::VrInteropView3D_2::~VrInteropView3D_2
 */
VrInteropView3D_2::~VrInteropView3D_2(void) { this->Release(); }

namespace {
auto areDimsDifferent = [](auto width1, auto height1, auto width2, auto height2) -> bool {
    return static_cast<decltype(width2)>(width1) != width2 || static_cast<decltype(height2)>(height1) != height2;
};

const auto resizeFBO = [&](auto& fbo, const int width, const int height) {
    // maybe FBO texture resolution needs to change
    if (areDimsDifferent(width, height, fbo.GetWidth(), fbo.GetHeight())) {
        if (fbo.IsValid()) fbo.Release();

        std::cout << "resize FBO to width=" << width << ", height=" << height << std::endl;
        fbo.Create(width, height);
    }
};
} // namespace

/*
 * vrinterop::VrInteropView3D::Render
 */
void VrInteropView3D_2::Render(const mmcRenderViewContext& context) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // to get the Bbox, we issue a Render(). clean it up.

    doParameterShare(context);
    doBboxDataShare(context);


    bool hasModelPose = m_datasetPoseReceiver.getData<interop::ModelPose>(m_datasetPose);
    hasModelPose = true; // for now, we use relative dataset positioning from unity - no model matrix is sent.
    bool hasCamView = m_stereoViewReceiver.getData<interop::StereoCameraView>(m_stereoCamView);

	interop::ParameterBool m_testParam;
    bool hasTest = m_TestReceiver.getData<interop::ParameterBool>(m_testParam);
    //bool hasTest = m_TestReceiver.getData<interop::ParameterInt>(m_testParam);

	

    if (hasTest) {
        vislib::sys::Log::DefaultLog.WriteError("[View3D] receive param: %s", m_testParam.name);
        vislib::sys::Log::DefaultLog.WriteError("[View3D] receive param modul name: %s", m_testParam.param ? "true" : "false");

		this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {

			auto param = slot.Parameter();
            std::string slotName = slot.Name().PeekBuffer();
            std::string modName = mod.FullName().PeekBuffer();

			//vislib::sys::Log::DefaultLog.WriteError(               "[View3D] slotName %s", slotName);
            if (modName.compare(m_testParam.modulFullName) == 0 && slotName.compare(m_testParam.name) == 0) {

                if (!param.IsNull()) {

                    if (auto* p = slot.template Param<core::param::BoolParam>()) {

                        p->SetValue(m_testParam.param);
                        vislib::sys::Log::DefaultLog.WriteError(
                            "[View3D] bool param changed to %s", m_testParam.param ? "true" : "false");
                    }
                }
            }
        });
	}

    static bool hasCamProj = m_camProjectionReceiver.getData<interop::CameraProjection>(m_cameraProjection);

    if (hasCamView && hasCamProj && hasModelPose) {
        this->applyCameraConfig(m_stereoLCamParameters, m_stereoCamView.leftEyeView, m_cameraProjection, m_datasetPose);
        this->applyCameraConfig(
            m_stereoRCamParameters, m_stereoCamView.rightEyeView, m_cameraProjection, m_datasetPose);
    } else
        return;

    static int fboVp[4] = {0, 0, m_cameraProjection.pixelWidth, m_cameraProjection.pixelHeight};
    const auto setFBO = [&](FramebufferObject& fbo) {
        Base::overrideViewport = fboVp;
        Base::rendererSlot.CallAs<view::CallRender3D_2>()->SetOutputBuffer(&fbo);
        Base::overrideCall =
            Base::rendererSlot.CallAs<view::CallRender3D_2>(); // this nudges View3D_2 to render into our FBO
    };


    if (isNewFbSize(m_cameraProjection.pixelWidth, m_cameraProjection.pixelHeight)) {
        Base::Resize(m_cameraProjection.pixelWidth, m_cameraProjection.pixelHeight);
        resizeFBO(m_stereoFBO_L, m_cameraProjection.pixelWidth, m_cameraProjection.pixelHeight);
        resizeFBO(m_stereoFBO_R, m_cameraProjection.pixelWidth, m_cameraProjection.pixelHeight);
    }

    setFBO(m_stereoFBO_L);
    renderFromCamera(m_stereoLCamParameters, context);

    setFBO(m_stereoFBO_R);
    renderFromCamera(m_stereoRCamParameters, context);

    Base::overrideViewport = nullptr;
    Base::overrideCall = nullptr;
    m_stereoFBO_R.Disable();

    broadcastFramebuffer(m_stereoFBO_L, m_stereoImageSender_L);
    broadcastFramebuffer(m_stereoFBO_R, m_stereoImageSender_R);

    m_stereoFBO_L.DrawColourTexture();
}

void megamol::vrinterop::VrInteropView3D_2::renderFromCamera(
    const Camera::minimal_state_type& viewCamera, const mmcRenderViewContext& context) {

    // View3D_2::Render() calls View3D_2::adaptCameraValues() before calling the CallRender and
    // View3D_2::setCameraValues() after the former syncs dirty camera parameters to the used camera. then the call to
    // the renderers happens. the latter writes the camera state into the camera parameters and sets the parameters
    // dirty. this is done so in network environments the head node distributes dirty camera parameters to other nodes
    // via lua.

    // for stereo rendering, we need to un-dirty the camera parameters, or else the parameters for the right camera are
    // overwritten by the camera-params-readback before the rendering.
    this->cameraPositionParam.ResetDirty();
    this->cameraOrientationParam.ResetDirty();
    this->cameraProjectionTypeParam.ResetDirty();
    this->cameraNearPlaneParam.ResetDirty();
    this->cameraFarPlaneParam.ResetDirty();
    this->cameraConvergencePlaneParam.ResetDirty();
    this->cameraEyeParam.ResetDirty();
    this->cameraGateScalingParam.ResetDirty();
    this->cameraCenterOffsetParam.ResetDirty();
    this->cameraHalfApertureRadiansParam.ResetDirty();
    this->cameraHalfDisparityParam.ResetDirty();

    this->cam = viewCamera;
    Base::rendererSlot.CallAs<view::CallRender3D_2>()->SetCameraState(this->cam);
    Base::Render(context);
}

void megamol::vrinterop::VrInteropView3D_2::doBboxDataShare(const mmcRenderViewContext& context) {
    auto defBbox = m_dataBbox;

    if (!oneTimeDataIsShared) {
        // get one time data: bbox
        Base::Render(context); // base render sets bbox of dataset
    }

    // m_dataBbox = this->bboxs.ObjectSpaceBBox();
    auto bbox = this->bboxs.BoundingBox();
    // bbox is such a fuckup

    defBbox.min = interop::vec4{
        bbox.GetLeftBottomBack().GetX(), bbox.GetLeftBottomBack().GetY(), bbox.GetLeftBottomBack().GetZ(), 0.0f};

    defBbox.max = interop::vec4{
        bbox.GetRightTopFront().GetX(), bbox.GetRightTopFront().GetY(), bbox.GetRightTopFront().GetZ(), 0.0f};

    oneTimeDataIsShared = true;
    m_dataBbox = defBbox;

    // auto resetBool = this->resetViewOnBBoxChange;

    m_bboxSender.sendData<interop::BoundingBoxCorners>("BoundingBoxCorners", m_dataBbox);
    // m_bboxSender.sendData<interop::BoundingBoxCorners>("Alpha", m_dataBbox);
    //vislib::sys::Log::DefaultLog.WriteError("[View3D] Finished");
    /*doParameterShare(context);
    vislib::sys::Log::DefaultLog.WriteError("[View3D] Finished");*/
    /*getRenderMode();
    vislib::sys::Log::DefaultLog.WriteError("[View3D] Finished");*/
}

void megamol::vrinterop::VrInteropView3D_2::doParameterShare(const mmcRenderViewContext& context) {
    //this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
    //    auto param = slot.Parameter();
    //    std::string slotName = slot.Name().PeekBuffer();

    //    /*if (isModul(mod.FullName().PeekBuffer(), "SphereRenderer") && slotName.compare("useLocalBbox") == 0) {

    //        if (!param.IsNull()) {

    //            if (auto* p = slot.template Param<core::param::BoolParam>()) {

    //                p->SetValue(m_testParam.param);
    //                vislib::sys::Log::DefaultLog.WriteError(
    //                    "[View3D] bool param changed to %s", m_testParam.param ? "true" : "false");
    //            }
    //        }
    //    }*/
    //});
    getRenderMode();
}

void megamol::vrinterop::VrInteropView3D_2::getRenderMode() {
     try {
        this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
			//vislib::sys::Log::DefaultLog.WriteError("[View3D] modul name: %s", mod.FullName());
            auto param = slot.Parameter();
			std::string slotName = slot.Name().PeekBuffer();

            if (!slotName.empty()) {
                /*vislib::sys::Log::DefaultLog.WriteError("[View3D] mod name: %s", mod.FullName().PeekBuffer());
                vislib::sys::Log::DefaultLog.WriteError(
                    "[View3D] slot name: %s", (slotName.compare("enableBoundingBox") == 0 ? "true" : "false"));*/

                if (isModul(mod.FullName().PeekBuffer(), "BoundingBoxRenderer") &&
                    slotName.compare("enableBoundingBox") == 0) {

                    if (!param.IsNull()) {
                        // vislib::sys::Log::DefaultLog.WriteError("[View3D] Looking for VisBool");

                        if (auto* p = slot.template Param<core::param::BoolParam>()) {

                            // vislib::sys::Log::DefaultLog.WriteError("[View3D] Found VisBool, %s", p->Value());
                            bool value = p->Value();
                            // vislib::sys::Log::DefaultLog.WriteError("[View3D] auto value = p->Value(), value = %s",
                            // value);

                            interop::ParameterBool param{value, slot.Name().PeekBuffer(), mod.FullName().PeekBuffer()};
                            m_bboxSender.sendData<interop::ParameterBool>("ReceiveTest", param);
                            // vislib::sys::Log::DefaultLog.WriteError("[View3D] VisBool sent");
                        }
                    }
                }
			}
        });
    } catch (const std::exception& e) {
        vislib::sys::Log::DefaultLog.WriteError("[View3D] Exception: %s", e.what());
    }
    //interop::VisBool visbool = interop::VisBool{/*true, "test",*/ 5};
    //vislib::sys::Log::DefaultLog.WriteError("[View3D] interop::VisBool{true}; ");
    //m_testReceiver = visbool;
    //m_boolSender.sendData<interop::VisBool>("ReceiveTest", m_testReceiver);
    //vislib::sys::Log::DefaultLog.WriteError("[View3D] Send VisBool");
}

bool megamol::vrinterop::VrInteropView3D_2::isModul(const std::string& modname, std::string searchedName) {
    bool retval = false;
    // Empty module list means that all modules should be considered.
    retval = (modname.find(searchedName) != std::string::npos);
    return retval;
}

void megamol::vrinterop::VrInteropView3D_2::broadcastFramebuffer(
    FramebufferObject& fbo, interop::TextureSender& sender) {
    sender.sendTexture(fbo.GetColourTextureID(), fbo.GetWidth(), fbo.GetHeight());
}

bool megamol::vrinterop::VrInteropView3D_2::isNewFbSize(unsigned int width, unsigned int height) {
    const auto newWidth = static_cast<GLint>(width);
    const auto newHeight = static_cast<GLint>(height);

    bool isNew = areDimsDifferent(newWidth, newHeight, m_fbWidth, m_fbHeight);

    m_fbWidth = newWidth;
    m_fbHeight = newHeight;

    if (isNew) std::cout << "change FBO size to: w=" << newWidth << ", h=" << newHeight << std::endl;

    return isNew;
}

/*
 * vrinterop::VrInteropView3D_2::create
 */
bool VrInteropView3D_2::create(void) {
    Base::create();

    const std::string baseAdr = {"tcp://127.0.0.1"};
    const std::string recvPort = {":12345"};
    const std::string sendPort = {":12346"};

    // inherit initial camera parameters
    Camera::minimal_state_type camParams;
    this->cam.get_minimal_state(camParams);

    this->m_stereoLCamParameters = camParams;
    this->m_stereoRCamParameters = camParams;

    // m_dataBbox - set in render()
    const auto radr = baseAdr + recvPort;
    m_stereoViewReceiver.start(radr, "StereoCameraViewRelative");
    m_camProjectionReceiver.start(radr, "CameraProjection");
    m_datasetPoseReceiver.start(radr, "ModelPose");
    m_TestReceiver.start(radr, "SendTest");

    const auto sadr = baseAdr + sendPort;
    
	m_bboxSender.start(sadr, "BoundingBoxCorners");
    // m_bboxSender.start(sadr, "Alpha");
    m_boolSender.start(sadr, "ReceiveTest");
    m_vec4Sender.start(sadr, "Vec4Test");

    m_stereoFBO_L.Create(1, 1);
    m_stereoFBO_R.Create(1, 1);
    m_stereoImageSender_L.init("/UnityInterop/DefaultNameLeft");
    m_stereoImageSender_R.init("/UnityInterop/DefaultNameRight");

    return true;
}


/*
 * vrinterop::VrInteropView3D_2::release
 */
void VrInteropView3D_2::release(void) {
    Base::release();

    m_stereoFBO_L.Release();
    m_stereoFBO_R.Release();
    m_stereoImageSender_L.destroy();
    m_stereoImageSender_R.destroy();

    m_stereoViewReceiver.stop();
    m_camProjectionReceiver.stop();
    m_datasetPoseReceiver.stop();
    m_TestReceiver.stop();
    m_bboxSender.stop();
    m_boolSender.stop();
    m_vec4Sender.stop();
}

void VrInteropView3D_2::applyCameraConfig(Camera::minimal_state_type& cam, const interop::CameraView& view,
    const interop::CameraProjection proj, interop::ModelPose& pose) {
    Camera::minimal_state_type cam_parameters = cam;

    cam_parameters.half_disparity = 0.0f;

    cam_parameters.position = {view.eyePos.x, view.eyePos.y, view.eyePos.z};

    // auto viewMat = glm::lookAt(
    //	glm::vec3{view.eyePos.x, view.eyePos.y, view.eyePos.z},  // eye
    //	glm::vec3{view.lookAtPos.x, view.lookAtPos.y, view.lookAtPos.z},  // center
    //	glm::vec3{view.camUpDir.x, view.camUpDir.y, view.camUpDir.z}); // up

    const auto to_vec3 = [](auto vec) -> glm::vec3 { return glm::vec3{vec.x, vec.y, vec.z}; };
    glm::vec3 view_ = -(to_vec3(view.lookAtPos) - to_vec3(view.eyePos));
    glm::vec3 right_ = glm::cross(to_vec3(view.camUpDir), view_);
    glm::vec3 up_ = glm::cross(view_, right_);
    glm::vec3 right_norm = glm::normalize(right_);
    glm::vec3 up_norm = glm::normalize(up_);
    glm::vec3 view_norm = glm::normalize(view_);
    glm::mat3 viewMat = glm::mat3{right_norm, up_norm, view_norm};

    auto view_quat = glm::normalize(glm::quat_cast(viewMat));
    cam_parameters.orientation = {view_quat.x, view_quat.y, view_quat.z, view_quat.w};

    cam_parameters.half_aperture_angle_radians = proj.fieldOfViewY_rad * 0.5f;
    cam_parameters.near_clipping_plane = proj.nearClipPlane;
    cam_parameters.far_clipping_plane = proj.farClipPlane;

    const auto width = static_cast<int>(proj.pixelWidth);
    const auto height = static_cast<int>(proj.pixelHeight);
    cam_parameters.resolution_gate = {width, height};
    cam_parameters.centre_offset = {0, 0};
    cam_parameters.image_tile = {0, height, 0, width};

    cam = cam_parameters;
}
