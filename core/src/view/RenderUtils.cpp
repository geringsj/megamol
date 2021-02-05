/*
 * RenderUtils.h
 *
 * Copyright (C) 2021 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */

#include "mmcore/view/RenderUtils.h"

namespace megamol::core::view {

// STATIC functions -------------------------------------------------------

bool RenderUtils::LoadTextureFromFile(std::unique_ptr<glowl::Texture2D>& out_texture_ptr, const std::wstring& filename) {

    if (filename.empty())
        return false;
    bool retval = false;

    static vislib::graphics::BitmapImage img;
    static sg::graphics::PngBitmapCodec pbc;
    pbc.Image() = &img;
    void* buf = nullptr;
    size_t size = 0;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    size = megamol::core::utility::FileUtils::LoadRawFile(filename, &buf);
    if (size > 0) {
        if (pbc.Load(buf, size)) {
            img.Convert(vislib::graphics::BitmapImage::TemplateFloatRGBA);
            retval =
                RenderUtils::LoadTextureFromData(out_texture_ptr, img.Width(), img.Height(), img.PeekDataAs<FLOAT>());
        } else {
            megamol::core::utility::log::Log::DefaultLog.WriteError(
                "[GUI] Unable to read texture: %s [%s, %s, line %d]\n", filename.c_str(), __FILE__, __FUNCTION__,
                __LINE__);
            retval = false;
        }
    } else {
        retval = false;
    }

    ARY_SAFE_DELETE(buf);
    return retval;
}


bool RenderUtils::LoadTextureFromData(
    std::unique_ptr<glowl::Texture2D>& out_texture_ptr, int width, int height, float* data) {

    if (data == nullptr)
        return false;
    try {
        glowl::TextureLayout tex_layout(GL_RGBA32F, width, height, 1, GL_RGBA, GL_FLOAT, 1);
        if (out_texture_ptr == nullptr) {
            out_texture_ptr =
                std::make_unique<glowl::Texture2D>("image_widget", tex_layout, static_cast<GLvoid*>(data), false);
        } else {
            // Reload data
            out_texture_ptr->reload(tex_layout, static_cast<GLvoid*>(data), false);
        }
    } catch (glowl::TextureException& e) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] Error during texture creation: '%s'. [%s, %s, line %d]\n", e.what(), __FILE__, __FUNCTION__,
            __LINE__);
        return false;
    }
    return true;
}


bool RenderUtils::CreateShader(
    std::unique_ptr<glowl::GLSLProgram>& out_shader_ptr, const std::string& vertex_code, const std::string& fragment_code) {

    std::vector<std::pair<glowl::GLSLProgram::ShaderType, std::string>> shader_srcs;

    if (!vertex_code.empty())
        shader_srcs.push_back({glowl::GLSLProgram::ShaderType::Vertex, vertex_code});
    if (!fragment_code.empty())
        shader_srcs.push_back({glowl::GLSLProgram::ShaderType::Fragment, fragment_code});

    try {
        if (out_shader_ptr != nullptr)
            out_shader_ptr.reset();
        out_shader_ptr = std::make_unique<glowl::GLSLProgram>(shader_srcs);
    } catch (glowl::GLSLProgramException const& exc) {
        std::string debug_label;
        if (out_shader_ptr != nullptr) {
            debug_label = out_shader_ptr->getDebugLabel();
        }
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "[GUI] Error during shader program creation of\"%s\": %s. [%s, %s, line %d]\n ", debug_label.c_str(),
            exc.what(), __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    return true;
}


// LOCAL functions -------------------------------------------------------


RenderUtils::RenderUtils()
        : smooth(true), init_once(false), vertex_array(0), textures(), queues(), shaders(), buffers() {}


RenderUtils::~RenderUtils() {}


bool RenderUtils::InitPrimitiveRendering(megamol::core::utility::ShaderSourceFactory& factory) {

    if (this->init_once) {
        megamol::core::utility::log::Log::DefaultLog.WriteWarn(
            "Primitive rendering has already been initialized. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
    }

    // Create shaders
    std::string vertShaderCode = this->getShaderCode(factory, "primitives::points::vertex");
    std::string fragShaderCode = this->getShaderCode(factory, "primitives::points::fragment");
    if (!RenderUtils::CreateShader(this->shaders[Primitives::POINTS], vertShaderCode, fragShaderCode)) {
        this->shaders[Primitives::POINTS]->bindAttribLocation(Buffers::POSITION, "inPosition");
        this->shaders[Primitives::POINTS]->bindAttribLocation(Buffers::COLOR, "inColor");
        this->shaders[Primitives::POINTS]->bindAttribLocation(Buffers::TEXTURE_COORD, "inTexture");
        this->shaders[Primitives::POINTS]->bindAttribLocation(Buffers::ATTRIBUTES, "inAttributes");
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Failed to create point shader. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    vertShaderCode = this->getShaderCode(factory, "primitives::lines::vertex");
    fragShaderCode = this->getShaderCode(factory, "primitives::lines::fragment");
    if (!RenderUtils::CreateShader(this->shaders[Primitives::LINES], vertShaderCode, fragShaderCode)) {
        this->shaders[Primitives::LINES]->bindAttribLocation(Buffers::POSITION, "inPosition");
        this->shaders[Primitives::LINES]->bindAttribLocation(Buffers::COLOR, "inColor");
        this->shaders[Primitives::LINES]->bindAttribLocation(Buffers::TEXTURE_COORD, "inTexture");
        this->shaders[Primitives::LINES]->bindAttribLocation(Buffers::ATTRIBUTES, "inAttributes");
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Failed to create line shader. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    vertShaderCode = this->getShaderCode(factory, "primitives::quads::vertex");
    fragShaderCode = this->getShaderCode(factory, "primitives::quads::fragment");
    if (!RenderUtils::CreateShader(this->shaders[Primitives::QUADS], vertShaderCode, fragShaderCode)) {
        this->shaders[Primitives::QUADS]->bindAttribLocation(Buffers::POSITION, "inPosition");
        this->shaders[Primitives::QUADS]->bindAttribLocation(Buffers::COLOR, "inColor");
        this->shaders[Primitives::QUADS]->bindAttribLocation(Buffers::TEXTURE_COORD, "inTexture");
        this->shaders[Primitives::QUADS]->bindAttribLocation(Buffers::ATTRIBUTES, "inAttributes");
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Failed to create quad shader. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    vertShaderCode = this->getShaderCode(factory, "primitives::color_texture::vertex");
    fragShaderCode = this->getShaderCode(factory, "primitives::color_texture::fragment");
    if (!RenderUtils::CreateShader(this->shaders[Primitives::COLOR_TEXTURE], vertShaderCode, fragShaderCode)) {
        this->shaders[Primitives::COLOR_TEXTURE]->bindAttribLocation(Buffers::POSITION, "inPosition");
        this->shaders[Primitives::COLOR_TEXTURE]->bindAttribLocation(Buffers::COLOR, "inColor");
        this->shaders[Primitives::COLOR_TEXTURE]->bindAttribLocation(Buffers::TEXTURE_COORD, "inTexture");
        this->shaders[Primitives::COLOR_TEXTURE]->bindAttribLocation(Buffers::ATTRIBUTES, "inAttributes");
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Failed to create color texture shader. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    vertShaderCode = this->getShaderCode(factory, "primitives::depth_texture::vertex");
    fragShaderCode = this->getShaderCode(factory, "primitives::depth_texture::fragment");
    if (!RenderUtils::CreateShader(this->shaders[Primitives::DEPTH_TEXTURE], vertShaderCode, fragShaderCode)) {
        this->shaders[Primitives::DEPTH_TEXTURE]->bindAttribLocation(Buffers::POSITION, "inPosition");
        this->shaders[Primitives::DEPTH_TEXTURE]->bindAttribLocation(Buffers::COLOR, "inColor");
        this->shaders[Primitives::DEPTH_TEXTURE]->bindAttribLocation(Buffers::TEXTURE_COORD, "inTexture");
        this->shaders[Primitives::DEPTH_TEXTURE]->bindAttribLocation(Buffers::ATTRIBUTES, "inAttributes");
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Failed to create depth texture shader. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    // Create buffers
    this->buffers[Buffers::POSITION] =
        std::make_unique<glowl::BufferObject>(GL_ARRAY_BUFFER, nullptr, 0, GL_DYNAMIC_DRAW);
    this->buffers[Buffers::COLOR] = std::make_unique<glowl::BufferObject>(GL_ARRAY_BUFFER, nullptr, 0, GL_DYNAMIC_DRAW);
    this->buffers[Buffers::TEXTURE_COORD] =
        std::make_unique<glowl::BufferObject>(GL_ARRAY_BUFFER, nullptr, 0, GL_DYNAMIC_DRAW);
    this->buffers[Buffers::ATTRIBUTES] =
        std::make_unique<glowl::BufferObject>(GL_ARRAY_BUFFER, nullptr, 0, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &this->vertex_array);
    glBindVertexArray(this->vertex_array);

    this->buffers[Buffers::POSITION]->bind();
    glEnableVertexAttribArray(Buffers::POSITION);
    glVertexAttribPointer(Buffers::POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    this->buffers[Buffers::COLOR]->bind();
    glEnableVertexAttribArray(Buffers::COLOR);
    glVertexAttribPointer(Buffers::COLOR, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    this->buffers[Buffers::TEXTURE_COORD]->bind();
    glEnableVertexAttribArray(Buffers::TEXTURE_COORD);
    glVertexAttribPointer(Buffers::TEXTURE_COORD, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    this->buffers[Buffers::ATTRIBUTES]->bind();
    glEnableVertexAttribArray(Buffers::ATTRIBUTES);
    glVertexAttribPointer(Buffers::ATTRIBUTES, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(Buffers::POSITION);
    glDisableVertexAttribArray(Buffers::COLOR);
    glDisableVertexAttribArray(Buffers::TEXTURE_COORD);
    glDisableVertexAttribArray(Buffers::ATTRIBUTES);

    this->init_once = true;
 
    return true;
}


bool RenderUtils::LoadTextureFromFile(GLuint& out_texture_id, const std::wstring& filename) {

    out_texture_id = 0;
    this->textures.push_back(nullptr);
    if (RenderUtils::LoadTextureFromFile(this->textures.back(), filename)) {
        out_texture_id = this->textures.back()->getTextureHandle();
        return true;
    }
    return false;
}


void RenderUtils::PushPointPrimitive(const glm::vec3& pos_center, float size, const glm::vec3& cam_view,
    const glm::vec3& cam_pos, const glm::vec4& color, bool sort) {

    glm::vec3 distance = (pos_center - cam_pos);
    if (glm::dot(cam_view, distance) < 0.0f)
        return;

    float d = glm::length(distance);
    float radius = size / 2.0f;
    glm::vec3 rad = pos_center + glm::normalize(this->arbitraryPerpendicular(distance)) * radius;
    glm::vec4 attributes = {rad.x, rad.y, rad.z, d};
    this->pushShaderData(Primitives::POINTS, 0, pos_center, color, glm::vec2(0.0f, 0.0f), attributes);

    if (sort) {
        this->sortPrimitiveQueue(Primitives::POINTS);
    }
}


void RenderUtils::PushLinePrimitive(const glm::vec3& pos_start, const glm::vec3& pos_end, float line_width,
    const glm::vec3& cam_view, const glm::vec3& cam_pos, const glm::vec4& color, bool sort) {

    glm::vec3 normal = cam_view * (-1.0f);
    glm::vec3 linedir = (pos_start - pos_end);
    glm::vec3 w = glm::normalize(glm::cross(normal, linedir));
    glm::vec3 p1 = w * (line_width / 2.0f);
    glm::vec3 pos_bottom_left = pos_start - p1;
    glm::vec3 pos_upper_left = pos_start + p1;
    glm::vec3 pos_upper_right = pos_end + p1;
    glm::vec3 pos_bottom_right = pos_end - p1;
    glm::vec4 attributes = {0.0f, 0.0f, 0.0f, 0.0f};
    this->pushQuad(RenderUtils::Primitives::LINES, 0, pos_bottom_left, pos_upper_left, pos_upper_right,
        pos_bottom_right, color, attributes);

    if (sort) {
        this->sortPrimitiveQueue(Primitives::LINES);
    }
}


void RenderUtils::PushQuadPrimitive(const glm::vec3& pos_center, float width, float height, const glm::vec3& cam_view,
    const glm::vec3& cam_up, const glm::vec4& color, bool sort) {

    glm::vec3 normal = cam_view * (-1.0f);
    glm::vec3 p1 = glm::normalize(cam_up);
    glm::vec3 p2 = glm::cross(normal, p1);
    p1 = glm::normalize(p1) * (height / 2.0f);
    p2 = glm::normalize(p2) * (width / 2.0f);
    glm::vec3 pos_bottom_left = pos_center - p1 - p2;
    glm::vec3 pos_upper_left = pos_center + p1 - p2;
    glm::vec3 pos_upper_right = pos_center + p1 + p2;
    glm::vec3 pos_bottom_right = pos_center - p1 + p2;
    glm::vec4 attributes = {0.0f, 0.0f, 0.0f, 0.0f};
    this->pushQuad(RenderUtils::Primitives::QUADS, 0, pos_bottom_left, pos_upper_left, pos_upper_right,
        pos_bottom_right, color, attributes);

    if (sort) {
        this->sortPrimitiveQueue(Primitives::QUADS);
    }
}


void RenderUtils::PushQuadPrimitive(const glm::vec3& pos_bottom_left, const glm::vec3& pos_upper_left,
    const glm::vec3& pos_upper_right, const glm::vec3& pos_bottom_right, const glm::vec4& color, bool sort) {

    glm::vec4 attributes = {0.0f, 0.0f, 0.0f, 0.0f};
    this->pushQuad(RenderUtils::Primitives::QUADS, 0, pos_bottom_left, pos_upper_left, pos_upper_right,
        pos_bottom_right, color, attributes);

    if (sort) {
        this->sortPrimitiveQueue(Primitives::QUADS);
    }
}


void RenderUtils::Push2DColorTexture(GLuint texture_id, const glm::vec3& pos_bottom_left,
    const glm::vec3& pos_upper_left, const glm::vec3& pos_upper_right, const glm::vec3& pos_bottom_right, bool flip_y,
    const glm::vec4& color) {

    glm::vec3 pbl = pos_bottom_left;
    glm::vec3 pul = pos_upper_left;
    glm::vec3 pur = pos_upper_right;
    glm::vec3 pbr = pos_bottom_right;
    if (flip_y) {
        pbl.y = pos_upper_left.y;
        pul.y = pos_bottom_left.y;
        pur.y = pos_bottom_right.y;
        pbr.y = pos_upper_right.y;
    }
    glm::vec4 attributes = {0.0f, 0.0f, 0.0f, 0.0f};
    this->pushQuad(RenderUtils::Primitives::COLOR_TEXTURE, texture_id, pbl, pul, pur, pbr, color, attributes);
}


void RenderUtils::Push2DDepthTexture(GLuint texture_id, const glm::vec3& pos_bottom_left,
    const glm::vec3& pos_upper_left, const glm::vec3& pos_upper_right, const glm::vec3& pos_bottom_right, bool flip_y,
    const glm::vec4& color) {

    glm::vec3 pbl = pos_bottom_left;
    glm::vec3 pul = pos_upper_left;
    glm::vec3 pur = pos_upper_right;
    glm::vec3 pbr = pos_bottom_right;
    if (flip_y) {
        pbl.y = pos_upper_left.y;
        pul.y = pos_bottom_left.y;
        pur.y = pos_bottom_right.y;
        pbr.y = pos_upper_right.y;
    }
    glm::vec4 attributes = {0.0f, 0.0f, 0.0f, 0.0f};
    this->pushQuad(RenderUtils::Primitives::DEPTH_TEXTURE, texture_id, pbl, pul, pur, pbr, color, attributes);
}


void RenderUtils::drawPrimitives(RenderUtils::Primitives primitive, glm::mat4& mat_mvp, glm::vec2 dim_vp) {

    if (!this->init_once) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Primitive rendering must be initialized before drawing. [%s, %s, line %d]\n", __FILE__, __FUNCTION__,
            __LINE__);
        return;
    }
    GLsizei count = static_cast<GLsizei>(this->queues[primitive].position.size() / 3);
    if (count == 0)
        return;

    this->sortPrimitiveQueue(primitive);

    auto texture_id = this->queues[primitive].texture_id;
    this->buffers[Buffers::POSITION]->rebuffer<std::vector<float>>(this->queues[primitive].position);
    this->buffers[Buffers::COLOR]->rebuffer<std::vector<float>>(this->queues[primitive].color);
    this->buffers[Buffers::TEXTURE_COORD]->rebuffer<std::vector<float>>(this->queues[primitive].texture_coord);
    this->buffers[Buffers::ATTRIBUTES]->rebuffer<std::vector<float>>(this->queues[primitive].attributes);

    // Set OpenGL state ----------------------------------------------------
    // Blending
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (!blendEnabled) {
        glEnable(GL_BLEND);
    }
    GLint blendSrc;
    GLint blendDst;
    glGetIntegerv(GL_BLEND_SRC, &blendSrc);
    glGetIntegerv(GL_BLEND_DST, &blendDst);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Depth
    GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (!depthEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    // Cullling
    GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    if (cullEnabled) {
        glDisable(GL_CULL_FACE);
    }
    // Smoothing
    if (this->smooth) {
        glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
    }
    // Vertex Point Size
    bool vertexpointsizeEnabled = glIsEnabled(GL_VERTEX_PROGRAM_POINT_SIZE);
    if (!vertexpointsizeEnabled) {
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    }

    // Draw ----------------------------------------------------------------
    GLenum mode = GL_TRIANGLES;
    if (primitive == Primitives::POINTS) {
        mode = GL_POINTS;
    }
    if (primitive == Primitives::COLOR_TEXTURE) {
        glDisable(GL_DEPTH_TEST);
    }
    if (primitive == Primitives::DEPTH_TEXTURE) {
        glEnable(GL_DEPTH_TEST);
    }

    glBindVertexArray(this->vertex_array);
    this->shaders[primitive]->use();

    if (texture_id != 0) {
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1i(this->shaders[primitive]->getUniformLocation("tex"), static_cast<GLint>(0));
    }

    glUniform1i(this->shaders[primitive]->getUniformLocation("apply_smooth"), static_cast<GLint>(this->smooth));
    glUniform2fv(this->shaders[primitive]->getUniformLocation("viewport"), 1, glm::value_ptr(dim_vp));
    glUniformMatrix4fv(this->shaders[primitive]->getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mat_mvp));

    glDrawArrays(mode, 0, count);

    if (texture_id != 0) {
        glBindTexture(GL_TEXTURE_1D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    glUseProgram(0);
    glBindVertexArray(0);

    // Reset OpenGL state --------------------------------------------------
    // Vertex Point Size
    if (!vertexpointsizeEnabled) {
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    }
    // Smoothing
    /// not reset ...
    // Cullling
    if (cullEnabled) {
        glEnable(GL_CULL_FACE);
    }
    // Depth
    if (!depthEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
    // Blending
    glBlendFunc(blendSrc, blendDst);
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
}


void RenderUtils::sortPrimitiveQueue(Primitives primitive) {

    // Sort primitives by distance (stored in attribute.w) for correct blending.

    const size_t dim_pos = 3;
    const size_t dim_col = 4;
    const size_t dim_txc = 2;
    const size_t dim_atr = 4;

    float d;

    switch (primitive) {
    case (Primitives::POINTS): {

        auto size_pos = this->queues[primitive].position.size() / dim_pos;
        auto size_col = this->queues[primitive].color.size() / dim_col;
        auto size_txc = this->queues[primitive].texture_coord.size() / dim_txc;
        auto size_atr = this->queues[primitive].attributes.size() / dim_atr;

        if (!((size_pos == size_col) && (size_col == size_txc) && (size_txc == size_atr))) {
            megamol::core::utility::log::Log::DefaultLog.WriteError(
                "Primitive sorting fails due to inconsitent data list count - BUG. [%s, %s, line %d]\n", __FILE__,
                __FUNCTION__, __LINE__);
            return;
        }

        // for (size_t i = 0; i < size_atr; ++i) {
        //    d = this->queues[primitive].attributes[i*dim_atr + 3];


        //}


    } break;
    default:
        break;
    }
}


const std::string RenderUtils::getShaderCode(
    megamol::core::utility::ShaderSourceFactory& factory, std::string snippet_name) {

    vislib::graphics::gl::ShaderSource source;
    if (!factory.MakeShaderSource(snippet_name.c_str(), source)) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Failed to make vertex shader source. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return std::string("");
    }
    return std::string(source.WholeCode().PeekBuffer());
}


size_t RenderUtils::loadRawFile(std::wstring filename, BYTE** outData) {

    // Reset out data
    *outData = nullptr;

    vislib::StringW name = static_cast<vislib::StringW>(filename.c_str());
    if (name.IsEmpty()) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            " Unable to load texture file. No name given. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return 0;
    }
    if (!vislib::sys::File::Exists(name)) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Unable to load not existing file \"%s\". [%s, %s, line %d]\n", filename.c_str(), __FILE__, __FUNCTION__,
            __LINE__);
        return 0;
    }

    size_t size = static_cast<size_t>(vislib::sys::File::GetSize(name));
    if (size < 1) {
        megamol::core::utility::log::Log::DefaultLog.WriteError("Unable to load empty file \"%s\". [%s, %s, line %d]\n",
            filename.c_str(), __FILE__, __FUNCTION__, __LINE__);
        return 0;
    }

    vislib::sys::FastFile f;
    if (!f.Open(name, vislib::sys::File::READ_ONLY, vislib::sys::File::SHARE_READ, vislib::sys::File::OPEN_ONLY)) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "Unable to open file \"%s\". [%s, %s, line %d]\n", filename.c_str(), __FILE__, __FUNCTION__, __LINE__);
        return 0;
    }

    *outData = new BYTE[size];
    size_t num = static_cast<size_t>(f.Read(*outData, size));
    if (num != size) {
        megamol::core::utility::log::Log::DefaultLog.WriteError("Unable to read whole file \"%s\". [%s, %s, line %d]\n",
            filename.c_str(), __FILE__, __FUNCTION__, __LINE__);
        ARY_SAFE_DELETE(*outData);
        return 0;
    }

    return num;
}


void RenderUtils::pushQuad(Primitives primitive, GLuint texture_id, const glm::vec3& pos_bottom_left,
    const glm::vec3& pos_upper_left, const glm::vec3& pos_upper_right, const glm::vec3& pos_bottom_right,
    const glm::vec4& color, const glm::vec4& attributes) {

    // First triangle
    this->pushShaderData(primitive, texture_id, pos_bottom_left, color, glm::vec2(0.0f, 0.0f), attributes);
    this->pushShaderData(primitive, texture_id, pos_upper_right, color, glm::vec2(1.0f, 1.0f), attributes);
    this->pushShaderData(primitive, texture_id, pos_upper_left, color, glm::vec2(0.0f, 1.0f), attributes);

    // Second triangle
    this->pushShaderData(primitive, texture_id, pos_bottom_left, color, glm::vec2(0.0f, 0.0f), attributes);
    this->pushShaderData(primitive, texture_id, pos_bottom_right, color, glm::vec2(1.0f, 0.0f), attributes);
    this->pushShaderData(primitive, texture_id, pos_upper_right, color, glm::vec2(1.0f, 1.0f), attributes);
}


void RenderUtils::pushShaderData(Primitives primitive, GLuint texture_id, const glm::vec3& position,
    const glm::vec4& color, const glm::vec2& texture_coord, const glm::vec4& attributes) {

    this->queues[primitive].texture_id = texture_id;
    this->pushQueue(this->queues[primitive].position, position);
    this->pushQueue(this->queues[primitive].color, color);
    this->pushQueue(this->queues[primitive].texture_coord, texture_coord);
    this->pushQueue(this->queues[primitive].attributes, attributes);
}


void RenderUtils::clearQueue(Primitives primitive) {

    this->queues[primitive].texture_id = 0;
    this->queues[primitive].position.clear();
    this->queues[primitive].color.clear();
    this->queues[primitive].texture_coord.clear();
    this->queues[primitive].attributes.clear();
}


void RenderUtils::pushQueue(DataType& d, float v, UINT cnt) {

    d.emplace_back(v);
}


void RenderUtils::pushQueue(DataType& d, glm::vec2 v, UINT cnt) {

    for (unsigned int i = 0; i < cnt; ++i) {
        d.emplace_back(v.x);
        d.emplace_back(v.y);
    }
}


void RenderUtils::pushQueue(DataType& d, glm::vec3 v, UINT cnt) {

    for (unsigned int i = 0; i < cnt; ++i) {
        d.emplace_back(v.x);
        d.emplace_back(v.y);
        d.emplace_back(v.z);
    }
}


void RenderUtils::pushQueue(DataType& d, glm::vec4 v, UINT cnt) {

    for (unsigned int i = 0; i < cnt; ++i) {
        d.emplace_back(v.x);
        d.emplace_back(v.y);
        d.emplace_back(v.z);
        d.emplace_back(v.w);
    }
}


glm::vec3 RenderUtils::arbitraryPerpendicular(glm::vec3 in) {

    if ((in.x == 0.0f) && (in.y == 0.0f) && (in.z == 0.0f)) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    } else if (in.x == 0.0f) {
        return glm::vec3(1.0f, 0.0f, 0.0f);
    } else if (in.y == 0.0f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (in.z == 0.0f) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        return glm::vec3(1.0f, 1.0f, -1.0f * (in.x + in.y) / in.z);
    }
}

} // end namespace
