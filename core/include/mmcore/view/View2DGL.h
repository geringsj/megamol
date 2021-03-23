/*
 * View2DGL.h
 *
 * Copyright (C) 2008 - 2010 by VISUS (Universitaet Stuttgart). 
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOLCORE_VIEW2DGL_H_INCLUDED
#define MEGAMOLCORE_VIEW2DGL_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include "mmcore/BoundingBoxes_2.h"
#include "mmcore/CallerSlot.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore/view/AbstractView.h"
#include "mmcore/view/TimeControl.h"

#define GLOWL_OPENGL_INCLUDE_GLAD
#include <glowl/FramebufferObject.hpp>

namespace megamol {
namespace core {
namespace view {

/*
 * Forward declaration of incoming render calls
 */
class CallRenderViewGL;


/**
 * Base class of rendering graph calls
 */
class View2DGL: public AbstractView {
public:

    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char *ClassName(void) {
        return "View2DGL";
    }

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static const char *Description(void) {
        return "2D View Module";
    }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static bool IsAvailable(void) {
        return true;
    }

    /** Ctor. */
    View2DGL(void);

    /** Dtor. */
    virtual ~View2DGL(void);

    /**
     * Answer the camera synchronization number.
     *
     * @return The camera synchronization number
     */
    virtual unsigned int GetCameraSyncNumber(void) const;

    /**
     * ...
     */
    virtual void Render(double time, double instanceTime);

    /**
     * Resets the view. This normally sets the camera parameters to
     * default values.
     */
    virtual void ResetView(void);

    /**
     * Resizes the View2DGl framebuffer object.
     *
     * @param width The new width.
     * @param height The new height.
     */
    virtual void Resize(unsigned int width, unsigned int height) override;

    /**
     * Callback requesting a rendering of this view
     *
     * @param call The calling call
     *
     * @return The return value
     */
    virtual bool OnRenderView(Call& call);

    virtual bool GetExtents(Call& call) override;

    virtual bool OnKey(Key key, KeyAction action, Modifiers mods) override;

    virtual bool OnChar(unsigned int codePoint) override;

    virtual bool OnMouseButton(MouseButton button, MouseButtonAction action, Modifiers mods) override;

    virtual bool OnMouseMove(double x, double y) override;

    virtual bool OnMouseScroll(double dx, double dy) override;

    enum MouseMode : uint8_t { Propagate, Pan, Zoom };

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

private:

    /** The mouse drag mode */
    MouseMode _mouseMode;

    /** The mouse x coordinate */
    float _mouseX;

    /** The mouse y coordinate */
    float _mouseY;

    /** The view focus x coordinate */
    float _viewX;

    /** The view focus y coordinate */
    float _viewY;

    /** The view zoom factor */
    float _viewZoom;

    /** the update counter for the view settings */
    unsigned int _viewUpdateCnt;

    std::shared_ptr<glowl::FramebufferObject> _fbo;

    // the FBO type for the frontend image could be a class-wide static value for each View class
    static const auto FboType = megamol::frontend_resources::GenericImageType::GLTexureHandle;
    using GenericImage = megamol::frontend_resources::GenericImage;
    virtual GenericImage InitGenericImageWithImageType() override
    {
        // init the generic image with the image type this view outputs
        size_t image_width = 0;
        size_t image_height = 0;
        GenericImage::DataChannels channels = GenericImage::DataChannels::RGBA8;

        // the FboType encodes whether the frontend image uses a GL Texture or std::vector<byte> for image storage
        // the frontend asks the view to tell the image type via the template and the current image size and number of color channels via the arguments
        return megamol::frontend_resources::make_image<FboType>({image_width, image_height}, channels);
    }
    virtual void WriteRenderResultIntoGenericImage(GenericImage& frontend_image) override
    {
        // update the generic image with new FBO size and contents
        unsigned int fbo_color_buffer_gl_handle = _fbo->GetColourTextureID(0); // IS THIS SAFE?? IS THIS THE COLOR BUFFER??
        size_t fbo_width = _fbo->GetWidth();
        size_t fbo_height = _fbo->GetHeight();

        // the FboType encodes whether to use the GL Texture or std::vector<byte> for image update in frontend_image
        // the FboType must match the type used in InitGenericImageWithImageType()
        frontend_image.set_data<FboType>(fbo_color_buffer_gl_handle, {fbo_width, fbo_height});
    }
};
} /* end namespace view */
} /* end namespace core */
} /* end namespace megamol */

#endif /* MEGAMOLCORE_VIEW2DGL_H_INCLUDED */
