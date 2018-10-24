/*
* ReplacementRenderer.h
*
* Copyright (C) 2018 by VISUS (Universitaet Stuttgart).
* Alle Rechte vorbehalten.
*/

#ifndef MEGAMOL_SPOUT_H_INCLUDED
#define MEGAMOL_SPOUT_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include "mmcore/CallerSlot.h"
#include "mmcore/view/CallRender3D.h"
#include "mmcore/view/Renderer3DModule.h"
#include "mmcore/view/AbstractCallRender3D.h"
#include "mmcore/param/ParamSlot.h"

namespace megamol {
	namespace spout_sender {

		/*
		* Spout Sender Renderer class
		*/

		class SpoutSenderRenderer : public megamol::core::view::Renderer3DModule
		{
		public:


			/**
			* Answer the name of this module.
			*
			* @return The name of this module.
			*/
			static const char *ClassName(void)
			{
				return "SpoutSenderRenderer";
			}

			/**
			* Answer a human readable description of this module.
			*
			* @return A human readable description of this module.
			*/
			static const char *Description(void)
			{
				return "Locally forwards rendered images via Spout for usage in other applications.";
			}

			/**
			* Answers whether this module is available on the current system.
			*
			* @return 'true' if the module is available, 'false' otherwise.
			*/
			static bool IsAvailable(void)
			{
				return true;
			}

			/** Ctor. */
			SpoutSenderRenderer(void);

			/** Dtor. */
			virtual ~SpoutSenderRenderer(void);

		protected:

			/**
			* Implementation of 'Create'.
			*
			* @return 'true' on success, 'false' otherwise.
			*/
			virtual bool create(void);

			/**
			* Implementation of 'release'.
			*/
			virtual void release(void);

			/**
			* The get capabilities callback. The module should set the members
			* of 'call' to tell the caller its capabilities.
			*
			* @param call The calling call.
			*
			* @return The return value of the function.
			*/
			virtual bool GetCapabilities(megamol::core::Call& call);

			/**
			* The get extents callback. The module should set the members of
			* 'call' to tell the caller the extents of its data (bounding boxes
			* and times).
			*
			* @param call The calling call.
			*
			* @return The return value of the function.
			*/
			virtual bool GetExtents(megamol::core::Call& call);

			/**
			* The Open GL Render callback.
			*
			* @param call The calling call.
			* @return The return value of the function.
			*/
			virtual bool Render(megamol::core::Call& call);

		private:

			/**********************************************************************
			* variables
			**********************************************************************/

			bool toggleSpoutSenderRendering;

			/**********************************************************************
			* functions
			**********************************************************************/

			/** */
			//void drawBoundingBox(void);

			/**********************************************************************
			* callback stuff
			**********************************************************************/

			/** The renderer caller slot */
			core::CallerSlot rendererCallerSlot;

			/**********************************************************************
			* parameters
			**********************************************************************/

			core::param::ParamSlot toggleSpoutSenderRenderingParam;
		};


	} /* end namespace spout_sender */
} /* end namespace megamol */

#endif // MEGAMOL_SPOUT_H_INCLUDED
