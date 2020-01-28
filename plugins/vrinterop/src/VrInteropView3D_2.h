/*
 * SpoutView3D.h
 *
 * Copyright (C) 2018 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_SPOUT_VIEW3D_2_H_INCLUDED
#define MEGAMOL_SPOUT_VIEW3D_2_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#    pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */


#include "mmcore/view/View3D_2.h"

#include <atomic>     // std::atomic
#include <functional> // std::hash
#include <map>        // std::map
#include <thread>     // std::thread

#include "interop.hpp"

namespace megamol {
namespace vrinterop {
/**
 * Base class of rendering graph calls
 */
class VrInteropView3D_2 : public megamol::core::view::View3D_2 {

public:
    using Base = megamol::core::view::View3D_2;

    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName(void) { return "VrInteropView3D_2"; }

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static const char* Description(void) {
        return "3D View Module which broadcasts stereo FBO contents to other applications via Spout and allows "
               "external control of camera via Json messages over ZMQ. This module is to be used instead of View3D_2 "
               "views in projects.";
    }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static bool IsAvailable(void) { return true; }

    /** Ctor. */
    VrInteropView3D_2(void);

    /** Dtor. */
    virtual ~VrInteropView3D_2(void);

    /**
     * Renders this AbstractView3D in the currently active OpenGL context.
     *
     * @param context
     */
    virtual void Render(const mmcRenderViewContext& context);

protected:
    /**
     * Implementation of 'Create'.
     *
     * @return 'true' on success, 'false' otherwise.
     */
    virtual bool create(void);

    /**
     * Implementation of 'Release'.
     */
    virtual void release(void);


    /**********************************************************************
     * variables
     **********************************************************************/
    using Camera = core::view::Camera_2;

public:
    using FramebufferObject = vislib::graphics::gl::FramebufferObject;

private:
    std::string m_interopSenderId{"/UnityInterop/MegaMol/"};

    interop::BoundingBoxCorners m_dataBbox;
    interop::VisBool m_testReceiver;
    interop::StereoCameraView m_stereoCamView;
    interop::CameraProjection m_cameraProjection;
    interop::ModelPose m_datasetPose;
    interop::DataReceiver m_stereoViewReceiver;
    interop::DataReceiver m_camProjectionReceiver;
    interop::DataReceiver m_datasetPoseReceiver;
    interop::DataReceiver m_TestReceiver;
    interop::DataSender m_bboxSender;
	interop::DataSender m_boolSender;
    interop::DataSender m_vec4Sender;

    using FramebufferObject = vislib::graphics::gl::FramebufferObject;
    FramebufferObject m_stereoFBO_L, m_stereoFBO_R;
    interop::TextureSender m_stereoImageSender_L;
    interop::TextureSender m_stereoImageSender_R;

    void applyCameraConfig(Camera::minimal_state_type& cam, const interop::CameraView& view,
        const interop::CameraProjection proj, interop::ModelPose& pose);

    GLint m_fbWidth = 1, m_fbHeight = 1;
    bool isNewFbSize(unsigned int width, unsigned int height);
    void broadcastFramebuffer(FramebufferObject& fbo, interop::TextureSender& textureSender);
    void renderFromCamera(const Camera::minimal_state_type& viewCamera, const mmcRenderViewContext& context);
    void doBboxDataShare(const mmcRenderViewContext& context);
    void doParameterShare(const mmcRenderViewContext& context);
    void getRenderMode();
    bool isModul(const std::string& modname, std::string searchedName);
    bool oneTimeDataIsShared = false;

	

    // we overwrite the view camera / camera parameters with our settings
    // and call the View3D::Render() implementation to produce the image we need
    Camera::minimal_state_type m_stereoLCamParameters;
    Camera::minimal_state_type m_stereoRCamParameters;

    /**********************************************************************
     * parameters
     **********************************************************************/
};

} /* end namespace vrinterop */
} /* end namespace megamol */

#endif /* MEGAMOL_SPOUT_VIEW3D_2_H_INCLUDED */
