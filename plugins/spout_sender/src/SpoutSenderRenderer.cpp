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
	, rendererCallerSlot("renderer", "outgoing renderer")
	//, replacementRenderingParam("01_replacementRendering", "Show/hide replacement rendering for the model.")
	, toggleSpoutSenderRenderingParam("02_toggleSpoutSender", "Toggle sending via spout rendering.")
	//, replacementKeyParam("03_replacmentKeyAssign", "Assign a key to replacement rendering button.")
	//, alphaParam("04_alpha", "The alpha value of the replacement rendering.")
{
	// init variables
	this->toggleSpoutSenderRendering = false;

	this->rendererCallerSlot.SetCompatibleCall<view::CallRender3DDescription>();
	this->MakeSlotAvailable(&this->rendererCallerSlot);
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

}

/*
* SpoutSenderRenderer::create
*/
bool SpoutSenderRenderer::create(void) {

	return true;
}

/*
* SpoutSenderRenderer::GetCapabilities
*/
bool SpoutSenderRenderer::GetCapabilities(Call& call) {

	view::CallRender3D *cr3d_in = dynamic_cast<view::CallRender3D*>(&call);
	if (cr3d_in == nullptr) return false;

	// Propagate changes made in GetCapabilities() from outgoing CallRender3D (cr3d_out) to incoming CallRender3D (cr3d_in).
	view::CallRender3D *cr3d_out = this->rendererCallerSlot.CallAs<view::CallRender3D>();
	if ((cr3d_out != nullptr) && (*cr3d_out)(2)) {
		cr3d_in->AddCapability(cr3d_out->GetCapabilities());
	}

	cr3d_in->AddCapability(view::CallRender3D::CAP_RENDER);

	return true;
}

/*
* SpoutSenderRenderer::GetExtents
*/
bool SpoutSenderRenderer::GetExtents(Call& call) {

	view::CallRender3D *cr3d_in = dynamic_cast<view::CallRender3D*>(&call);
	if (cr3d_in == nullptr) return false;

	// Propagate changes made in GetExtents() from outgoing CallRender3D (cr3d_out) to incoming  CallRender3D (cr3d_in).
	view::CallRender3D *cr3d_out = this->rendererCallerSlot.CallAs<view::CallRender3D>();
	if ((cr3d_out != nullptr) && (*cr3d_out)(1)) {
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
	if (this->toggleSpoutSenderRenderingParam.IsDirty()) {
		this->toggleSpoutSenderRenderingParam.ResetDirty();

		this->toggleSpoutSenderRendering = !this->toggleSpoutSenderRendering;
		this->toggleSpoutSenderRenderingParam.Param<param::BoolParam>()->SetValue(this->toggleSpoutSenderRendering, false);
	}

	// Call render function of slave renderer
	view::CallRender3D *cr3d_out = this->rendererCallerSlot.CallAs<view::CallRender3D>();
	if (cr3d_out != nullptr) {
		*cr3d_out = *cr3d_in;
		(*cr3d_out)(0);
	}

	// send fbo texture via spout
	if (this->toggleSpoutSenderRendering) {

		//// Set opengl states
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		//glDisable(GL_LIGHTING);
		//glEnable(GL_DEPTH_TEST);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//// Draw bounding box
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		//this->drawBoundingBox();
		//glCullFace(GL_BACK);
		//this->drawBoundingBox();
		//glDisable(GL_CULL_FACE);

		//// Reset opengl states
		//glDisable(GL_BLEND);
	}

	return true;
}


/*
* ReplacementRenderer::drawBoundingBox
*/
//void ReplacementRenderer::drawBoundingBox() {
//
//	float alpha = alphaParam.Param<param::FloatParam>()->Value();
//
//	glBegin(GL_QUADS);
//
//	glEdgeFlag(true);
//
//	//glColor4f(0.5f, 0.5f, 0.5f, alpha);
//	glColor4f(0.0f, 0.0f, 0.25f, alpha);
//	glVertex3f(this->bbox.Left(), this->bbox.Bottom(), this->bbox.Back());
//	glVertex3f(this->bbox.Left(), this->bbox.Top(), this->bbox.Back());
//	glVertex3f(this->bbox.Right(), this->bbox.Top(), this->bbox.Back());
//	glVertex3f(this->bbox.Right(), this->bbox.Bottom(), this->bbox.Back());
//
//	//glColor4f(0.5f, 0.5f, 0.5f, alpha);
//	glColor4f(0.0f, 0.0f, 0.75f, alpha);
//	glVertex3f(this->bbox.Left(), this->bbox.Bottom(), this->bbox.Front());
//	glVertex3f(this->bbox.Right(), this->bbox.Bottom(), this->bbox.Front());
//	glVertex3f(this->bbox.Right(), this->bbox.Top(), this->bbox.Front());
//	glVertex3f(this->bbox.Left(), this->bbox.Top(), this->bbox.Front());
//
//	//glColor4f(0.75f, 0.75f, 0.75f, alpha);
//	glColor4f(0.0f, 0.75f, 0.0f, alpha);
//	glVertex3f(this->bbox.Left(), this->bbox.Top(), this->bbox.Back());
//	glVertex3f(this->bbox.Left(), this->bbox.Top(), this->bbox.Front());
//	glVertex3f(this->bbox.Right(), this->bbox.Top(), this->bbox.Front());
//	glVertex3f(this->bbox.Right(), this->bbox.Top(), this->bbox.Back());
//
//	//glColor4f(0.75f, 0.75f, 0.75f, alpha);
//	glColor4f(0.0f, 0.25f, 0.0f, alpha);
//	glVertex3f(this->bbox.Left(), this->bbox.Bottom(), this->bbox.Back());
//	glVertex3f(this->bbox.Right(), this->bbox.Bottom(), this->bbox.Back());
//	glVertex3f(this->bbox.Right(), this->bbox.Bottom(), this->bbox.Front());
//	glVertex3f(this->bbox.Left(), this->bbox.Bottom(), this->bbox.Front());
//
//	//glColor4f(0.25f, 0.25f, 0.25f, alpha);
//	glColor4f(0.25f, 0.0f, 0.0f, alpha);
//	glVertex3f(this->bbox.Left(), this->bbox.Bottom(), this->bbox.Back());
//	glVertex3f(this->bbox.Left(), this->bbox.Bottom(), this->bbox.Front());
//	glVertex3f(this->bbox.Left(), this->bbox.Top(), this->bbox.Front());
//	glVertex3f(this->bbox.Left(), this->bbox.Top(), this->bbox.Back());
//
//	//glColor4f(0.25f, 0.25f, 0.25f, alpha);
//	glColor4f(0.75f, 0.0f, 0.0f, alpha);
//	glVertex3f(this->bbox.Right(), this->bbox.Bottom(), this->bbox.Back());
//	glVertex3f(this->bbox.Right(), this->bbox.Top(), this->bbox.Back());
//	glVertex3f(this->bbox.Right(), this->bbox.Top(), this->bbox.Front());
//	glVertex3f(this->bbox.Right(), this->bbox.Bottom(), this->bbox.Front());
//
//	glEnd();
//
//}