/*
 * GenericImage.h
 *
 * Copyright (C) 2021 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <functional>

namespace megamol {
namespace frontend_resources {

enum class GenericImageType {
    GLTexureHandle, // data array holds a GL texture handle
    ByteArray           // data array holds image data as raw bytes
};
static const GenericImageType GLTexureHandle = GenericImageType::GLTexureHandle;
static const GenericImageType ByteArray = GenericImageType::ByteArray;

// the idea is that each AbstractView (via the concrece View implementation)
// fills a GenericImage for the frontend to use.
// depending on the GenericImageType and DataChannels the frontend knows
// what it can do with each GenericImage
//   - either use the contained texture directly for GL rendering
//   - or forward the contained byte data to interested sources: show in window, write to screenshot file, send via network...
// we expect the content type and channel count to stay the same across the lifetime of the corresponding source View
// so after a view finishes rendering, it writes the final FBO state into a GenericImage
struct GenericImage {

    enum class DataChannels {
        // for texture and byte array, tells us how many channels there are
        RGB8,
        RGBA8,
    };
    struct ImageSize {
        size_t width = 0;
        size_t height = 0;

        bool operator!=(ImageSize const& other) { return width != other.width || height != other.height; }
    };

    GenericImage(ImageSize size, DataChannels channels, GenericImageType type, const void* data);
   
    GenericImage() = default;
    ~GenericImage(); // GL texture deletion
    // rule of five
    GenericImage(GenericImage& other);
    GenericImage& operator=(GenericImage const& other);
    GenericImage(GenericImage&& other) noexcept;
    GenericImage& operator=(GenericImage&& other) noexcept;

    ImageSize image_size;
    GenericImageType image_type;
    DataChannels channels;
    bool new_data = false;
    bool new_size = false;

    using byte = unsigned char;
    // if byte image, image_data holds (width * height * channels) entries.
    // row-major image starting at bottom-left pixel
    // as in https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glReadPixels.xhtml
    mutable std::vector<byte> image_data; // mutable for texture download

    // note that gl.h defines "typedef unsigned int GLuint;"
    mutable unsigned int gl_texture_handle = 0; // mutable for texture upload
    void download_texture() const; // downloads texture into local image_data
    void upload_texture() const; // downloads texture into local image_data
    void copy_texture_from(unsigned int source_gl_texture_handle);
    void copy_vector_from(std::vector<unsigned char> const& source_image_data);

    template <GenericImageType>
    void set_data(unsigned int source_gl_texture_handle, ImageSize new_size);
    
    template <GenericImageType>
    void set_data(std::vector<unsigned char> const& source_image_data, ImageSize new_size);

    size_t channels_count() const;
    void resize(ImageSize const& size);
    const ImageSize& size() const { return image_size; }
    bool has_new_data() const { return new_data; }
    bool has_new_size() const { return new_size; }
};

template <>
void GenericImage::set_data<GenericImageType::GLTexureHandle>(unsigned int source_gl_texture_handle, ImageSize new_size);
template <>
void GenericImage::set_data<GenericImageType::ByteArray>(unsigned int source_gl_texture_handle, ImageSize new_size);

template <>
void GenericImage::set_data<GenericImageType::GLTexureHandle>(std::vector<unsigned char> const& source_image_data, ImageSize new_size);
template <>
void GenericImage::set_data<GenericImageType::ByteArray>(std::vector<unsigned char> const& source_image_data, ImageSize new_size);


template <GenericImageType>
GenericImage make_image(GenericImage::ImageSize size, GenericImage::DataChannels channels = GenericImage::DataChannels::RGB8, const void* data = nullptr);

template <>
GenericImage make_image<GenericImageType::GLTexureHandle>(GenericImage::ImageSize size, GenericImage::DataChannels channels, const void* data);

template <>
GenericImage make_image<GenericImageType::ByteArray>(GenericImage::ImageSize size, GenericImage::DataChannels channels, const void* data);

template <GenericImageType>
GenericImage make_image(GenericImage const& source_image);

template <>
GenericImage make_image<GenericImageType::GLTexureHandle>(GenericImage const& source_image);

template <>
GenericImage make_image<GenericImageType::ByteArray>(GenericImage const& source_image);

size_t channels_count(GenericImage::DataChannels channels);

} /* end namespace frontend_resources */
} /* end namespace megamol */
