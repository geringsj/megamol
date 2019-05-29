/*
 * SpoutView3D.h
 *
 * Copyright (C) 2018 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_SPOUT_VIEW3D_H_INCLUDED
#define MEGAMOL_SPOUT_VIEW3D_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#    pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */


#include "mmcore/view/View3D.h"

#include <atomic>     // std::atomic
#include <functional> // std::hash
#include <map>        // std::map
#include <thread>     // std::thread

#include "interop.hpp"
//#include "OSCHandler.h"
//#include "Spout.h"

namespace megamol {
namespace spout_sender {
/**
 * Base class of rendering graph calls
 */
class SpoutView3D : public megamol::core::view::View3D {

public:
    using Base = megamol::core::view::View3D;

    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName(void) { return "SpoutView3D"; }

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static const char* Description(void) {
        return "3D View Module which broadcasts FBO contents to other applications via Spout and allows external "
               "control of camera";
    }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static bool IsAvailable(void) { return true; }

    /** Ctor. */
    SpoutView3D(void);

    /** Dtor. */
    virtual ~SpoutView3D(void);

    /**
     * Renders this AbstractView3D in the currently active OpenGL context.
     *
     * @param context
     */
    virtual void Render(const mmcRenderViewContext& context);

    void updateParameters();

    /**
     * Resizes the AbstractView3D.
     *
     * @param width The new width.
     * @param height The new height.
     */
    virtual void Resize(unsigned int width, unsigned int height);

    /**
     * Callback requesting a rendering of this view
     *
     * @param call The calling call
     *
     * @return The return value
     */
    virtual bool OnRenderView(megamol::core::Call& call);

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
    using CameraOpenGL = vislib::graphics::gl::CameraOpenGL;

public:
    using FramebufferObject = vislib::graphics::gl::FramebufferObject;

private:

    std::string m_interopSenderId{"/UnityInterop/MegaMol/"};

    interop::BoundingBoxCorners m_dataBbox;
    interop::StereoCameraConfiguration m_stereoCamConfig;
    interop::CameraConfiguration m_cameraConfig_L;
    interop::CameraConfiguration m_cameraConfig_R;
	interop::DataReceiver m_camReceiver;
	//interop::DataReceiver m_datasetPoseReceiver;

    using FramebufferObject = vislib::graphics::gl::FramebufferObject;
    FramebufferObject m_stereoFBO_L, m_stereoFBO_R;
    interop::TextureSender m_stereoImageSender_L;
    interop::TextureSender m_stereoImageSender_R;

    void applyCameraConfig(CameraOpenGL& cam, const interop::CameraConfiguration& conf);
    void sendData();

	GLint m_fbWidth = 1, m_fbHeight = 1;
	bool isNewFbSize(unsigned int width, unsigned int height);
    void broadcastFramebuffer(FramebufferObject& fbo, interop::TextureSender& textureSender);
    void renderFromCamera(const CameraOpenGL& viewCamera, const mmcRenderViewContext& context);
    void checkOneTimeDataShare(const mmcRenderViewContext& context);
    bool oneTimeDataIsShared = false;

    // we overwrite the view camera / camera parameters with our settings
    // and call the View3D::Render() implementation to produce the image we need
    CameraOpenGL m_monoCam;
    vislib::SmartPtr<vislib::graphics::CameraParameters> m_monoCamParameters;

    CameraOpenGL m_stereoLCam;
    vislib::SmartPtr<vislib::graphics::CameraParameters> m_stereoLCamParameters;

    CameraOpenGL m_stereoRCam;
    vislib::SmartPtr<vislib::graphics::CameraParameters> m_stereoRCamParameters;

    /*** Camera handling */
    enum class CameraMode { Mono = 0, Stereo = 1 };
    CameraMode m_currentRenderingMode = CameraMode::Mono;


    /**********************************************************************
     * parameters
     **********************************************************************/

    bool m_spoutSenderActive;
    core::param::ParamSlot m_spoutSenderActiveParam;
};

} /* end namespace spout_sender */
} /* end namespace megamol */

#endif /* MEGAMOL_SPOUT_VIEW3D_H_INCLUDED */
