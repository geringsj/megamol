/*
 * SpoutView3D.h
 *
 * Copyright (C) 2018 by VISUS (Universitaet Stuttgart). 
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_SPOUT_VIEW3D_H_INCLUDED
#define MEGAMOL_SPOUT_VIEW3D_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */


#include "mmcore/view/View3D.h"

#include <thread> // std::thread
#include <functional> // std::hash
#include <map> // std::map
#include <atomic> // std::atomic

#include "Spout.h"
#include "OSCHandler.h"

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
        static const char *ClassName(void) {
            return "SpoutView3D";
        }

        /**
         * Answer a human readable description of this module.
         *
         * @return A human readable description of this module.
         */
        static const char *Description(void) {
            return "3D View Module which broadcasts FBO contents to other applications via Spout and allows external control of camera";
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

		void checkFramebufferSize();

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

		std::string m_interopSenderId{ "/UnityInterop/MegaMol/" };
		
		vislib::math::Cuboid<float> m_dataBbox;

		struct DatasetShareData{
			bool oneTimeDataIsShared = false;
			vislib::math::Cuboid<float> dataBbox;
		};
		DatasetShareData m_unitySharedData;

		OSCHandler m_oscSendChannel;

		// Unity OSC sends data in this exact order
		struct CameraConfig {
			float cam_pos[3]    = { 0.0f };
			float lookAt_pos[3] = { 0.0f, 0.0f, -1.0f };
			float up_vec[3]     = { 0.0f, 1.0f,  0.0f };

			float vieldOfViewY_deg = 90.0f;
			float near_dist        = 0.01f;
			float far_dist         = 100.0f;
			int viewWidth_px       = 600;
			int viewHeight_px      = 400;

			float dataBboxScale = 1.0f;
		};

		void applyCameraConfig(CameraOpenGL& cam, const CameraConfig& conf);

		class OscPacketListener : public osc::OscPacketListener {
		protected:
			virtual void ProcessMessage(
				const osc::ReceivedMessage& msg,
				const IpEndpointName& remoteEndpoint);

			std::vector<char> m_data;
			bool m_hasData = false;
			void unsetDataFlag() { m_hasData = false; }
		public:
			bool hasData() const { return m_hasData; }

			template<typename T>
			T getData();

			std::atomic<CameraConfig> monoCam;
			std::atomic<CameraConfig> stereoLCam;
			std::atomic<CameraConfig> stereoRCam;
		};

		OscPacketListener m_oscReceiveChannel;
		std::unique_ptr<UdpListeningReceiveSocket> m_udpReceiveSocketPtr{ nullptr };
		std::thread m_oscReceiveThread;

		void checkOneTimeDataShare(const mmcRenderViewContext& context);
		void sendData();

		struct GLTexture {
			GLuint handle = 0;
			GLint width = 1, height = 1;

			void create(int width, int height)
			{
				this->width = width;
				this->height = height;

				if(handle == 0)
				{
					glGenTextures(1, &handle);
				}
				this->bind();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				this->unbind();
			}

			void bind() { glBindTexture(GL_TEXTURE_2D, handle); }
			void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

			void copyFB()
			{
				glReadBuffer(GL_BACK);
				this->bind();
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, width, height, 0);
				this->unbind();
			}

			void destroy()
			{
				if(handle != 0)
				{
					glDeleteTextures(1, &handle);
				}
			}
		};
		GLTexture m_texture;

		/** Texture handling and sharing */
		struct ImageDataSender {
			SpoutSender spoutSender;
			std::string senderName;

			// keep width and height for spout
			// then resize according to incoming textures
			unsigned int textureWidth = 1, textureHeight = 1;

			bool initSender(std::string const& name);
			bool init(std::string const& name, const unsigned int width = 1, const unsigned int height = 1);

			bool isInit() { return !senderName.empty(); }

			void sendTexture(const GLuint glTex, GLint width, GLint height);
		private:
			void updateTextureDimensions(const GLint width, const GLint height);
		public:

			void destroySender();
			void destroy();
		};

		ImageDataSender m_monoImageData;
		ImageDataSender m_stereoLImageData;
		ImageDataSender m_stereoRImageData;

		GLint m_fbWidth = 1, m_fbHeight = 1;
		bool isNewFbSize(unsigned int width, unsigned int height); // returns true if FB size changed
		bool m_hasNewFbSize = false;

		using FramebufferObject = vislib::graphics::gl::FramebufferObject;
		FramebufferObject m_monoFBO;
		FramebufferObject m_stereoFBO_L, m_stereoFBO_R;


		// we overwrite the view camera / camera parameters with our settings
		// and call the View3D::Render() implementation to produce the image we need

		CameraOpenGL m_monoCam;
		vislib::SmartPtr<vislib::graphics::CameraParameters> m_monoCamParameters;

		CameraOpenGL m_stereoLCam;
		vislib::SmartPtr<vislib::graphics::CameraParameters> m_stereoLCamParameters;

		CameraOpenGL m_stereoRCam;
		vislib::SmartPtr<vislib::graphics::CameraParameters> m_stereoRCamParameters;

		/*** Camera handling */
		enum class CameraMode {
			Mono = 0,
			Stereo = 1
		};
		CameraMode m_currentRenderingMode = CameraMode::Mono;


		/**********************************************************************
		* functions
		**********************************************************************/


		void broadcastFramebuffer(FramebufferObject& fbo, ImageDataSender& textureData);

		void renderFromCamera(const CameraOpenGL & viewCamera, const mmcRenderViewContext& context);


		/**********************************************************************
		* parameters
		**********************************************************************/

		bool m_spoutSenderActive;
		core::param::ParamSlot m_spoutSenderActiveParam;

    };

template<>
SpoutView3D::CameraConfig SpoutView3D::OscPacketListener::getData<SpoutView3D::CameraConfig>();

} /* end namespace spout_sender */
} /* end namespace megamol */

#endif /* MEGAMOL_SPOUT_VIEW3D_H_INCLUDED */
