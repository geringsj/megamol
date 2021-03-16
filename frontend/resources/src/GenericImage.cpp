/*
 * GenericImage.cpp
 *
 * Copyright (C) 2021 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */


#include "GenericImage.h"

#include <algorithm>

#include "glad/glad.h"

using namespace megamol::frontend_resources;

static void init_texture(unsigned int& handle) {
    int old_handle = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_handle);

    glCreateTextures(GL_TEXTURE_2D, 1, &handle);

    glBindTexture(GL_TEXTURE_2D, old_handle);
}

static std::tuple<int, int, int> getInternalformatFormatType(GenericImage::DataChannels channels) {
    const auto internalformat = channels == GenericImage::DataChannels::RGB8 ? GL_RGB8 : GL_RGBA8;
    const auto format = channels == GenericImage::DataChannels::RGB8 ? GL_RGB : GL_RGBA;
    const auto type = GL_UNSIGNED_BYTE;

    return {internalformat, format, type};
}

static void set_and_resize_texture(unsigned int& handle, GenericImage::ImageSize size, GenericImage::DataChannels channels, const void* data = nullptr) {
    if (!handle)
        init_texture(handle);

    int old_handle = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_handle);

    glBindTexture(GL_TEXTURE_2D, handle);
    
    glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    const auto [internalformat, format, type] = getInternalformatFormatType(channels);

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, size.width, size.height, 0, format, type, data);

    glBindTexture(GL_TEXTURE_2D, old_handle);
}

static void copy_texture(unsigned int from_handle, unsigned int to_handle, GenericImage::ImageSize size) {
    glCopyImageSubData(from_handle, GL_TEXTURE_2D, 0, 0, 0, 0, to_handle, GL_TEXTURE_2D, 0, 0, 0, 0, size.width, size.height, 1);
}

static void delete_texture(unsigned int& handle) {
    glDeleteTextures(1, &handle);
    handle = 0;
}

static void download_texture_to_vector(unsigned int handle, GenericImage::ImageSize size, GenericImage::DataChannels channels, std::vector<unsigned char>& target) {
    target.resize(size.width * size.height * channels_count(channels));
    
    const auto [internalformat, format, type] = getInternalformatFormatType(channels);

    glGetTextureSubImage(handle, 0, 0, 0, 0, size.width, size.height, 0, format, type, target.size(), target.data());
}

static void upload_texture_from_vector(unsigned int& handle, GenericImage::ImageSize size, GenericImage::DataChannels channels, std::vector<unsigned char> const& source) {
    set_and_resize_texture(handle, size, channels, source.data());
}

GenericImage::~GenericImage() {
    delete_texture(gl_texture_handle);
}

GenericImage::GenericImage(GenericImage& other) {
    image_data = other.image_data;
    image_type = other.image_type;
    channels = other.channels;
    image_size = {0,0};
    gl_texture_handle = 0;

    switch (image_type) {
    case GenericImageType::GLTexureHandle:
        set_and_resize_texture(gl_texture_handle, image_size, channels);
        copy_texture(gl_texture_handle, other.gl_texture_handle, image_size);
        break;
    case GenericImageType::ByteArray:
        set_data<GenericImageType::ByteArray>(other.image_data, other.image_size);
        break;
    default:
        break;
    }

    image_size = other.image_size;
    new_data = other.new_data;
    new_size = other.new_size;
}
GenericImage& GenericImage::operator=(GenericImage const& other) {
    image_data = other.image_data;
    image_type = other.image_type;
    channels = other.channels;
    image_size = {0,0};
    gl_texture_handle = 0;

    switch (image_type) {
    case GenericImageType::GLTexureHandle:
        set_and_resize_texture(gl_texture_handle, image_size, channels);
        copy_texture(gl_texture_handle, other.gl_texture_handle, image_size);
        break;
    case GenericImageType::ByteArray:
        set_data<GenericImageType::ByteArray>(other.image_data, other.image_size);
        break;
    default:
        break;
    }

    image_size = other.image_size;
    new_data = other.new_data;
    new_size = other.new_size;

    return *this;
}
GenericImage::GenericImage(GenericImage&& other) noexcept {
    image_data = std::move(other.image_data);
    image_size = std::move(other.image_size);
    image_type = std::move(other.image_type);
    channels = std::move(other.channels);
    new_data = std::move(other.new_data);
    new_size = std::move(other.new_size);
    gl_texture_handle = std::move(other.gl_texture_handle);
    other.gl_texture_handle = 0;
}
GenericImage& GenericImage::operator=(GenericImage&& other) noexcept {
    image_data = std::move(other.image_data);
    image_size = std::move(other.image_size);
    image_type = std::move(other.image_type);
    channels = std::move(other.channels);
    new_data = std::move(other.new_data);
    new_size = std::move(other.new_size);
    gl_texture_handle = std::move(other.gl_texture_handle);
    other.gl_texture_handle = 0;

    return *this;
}

GenericImage::GenericImage(ImageSize size, DataChannels channels, GenericImageType type, const void* data)
    : image_size{size}
    , channels{channels}
    , image_type{type}
{

    switch (image_type) {
    case GenericImageType::GLTexureHandle:
        set_and_resize_texture(gl_texture_handle, image_size, channels, data);
        break;
    case GenericImageType::ByteArray:
        resize(image_size);
        std::memcpy(image_data.data(), data, image_data.size());
        break;
    default:
        break;
    }
}

size_t GenericImage::channels_count() const {
    return megamol::frontend_resources::channels_count(channels);
}

void GenericImage::resize(ImageSize const& size) {
    new_size = true;
    image_size = size;
    byte init_value = 0;

    switch (image_type) {
    case GenericImageType::GLTexureHandle:
        set_and_resize_texture(gl_texture_handle, image_size, channels);
        break;
    case GenericImageType::ByteArray:
        image_data.resize(image_size.width * image_size.height * channels_count());
        break;
    default:
        break;
    }
}

void GenericImage::download_texture() const {
    download_texture_to_vector(gl_texture_handle, image_size, channels, image_data);
}

void GenericImage::upload_texture() const {
    upload_texture_from_vector(gl_texture_handle, image_size, channels, image_data);
}

void GenericImage::copy_texture_from(unsigned int source_gl_texture_handle) {
    copy_texture(source_gl_texture_handle, gl_texture_handle, image_size);
}
void GenericImage::copy_vector_from(std::vector<unsigned char> const& source_image_data) {
    image_data.resize(source_image_data.size());
    std::copy(source_image_data.begin(), source_image_data.end(), image_data.begin());
}

template <>
void GenericImage::set_data<GenericImageType::GLTexureHandle>(
    unsigned int source_gl_texture_handle,
    ImageSize new_size)
{
    if (new_size != image_size) {
        image_size = new_size;
        set_and_resize_texture(gl_texture_handle, image_size, channels);
        this->new_size = true;
    }
    new_data = true;

    copy_texture_from(source_gl_texture_handle);
}

template <>
void GenericImage::set_data<GenericImageType::ByteArray>(
    unsigned int source_gl_texture_handle,
    ImageSize new_size)
{
    set_and_resize_texture(gl_texture_handle, new_size, channels);
    copy_texture_from(source_gl_texture_handle);
    download_texture();
}

template <>
void GenericImage::set_data<GenericImageType::GLTexureHandle>(
    std::vector<unsigned char> const& source_image_data,
    ImageSize new_size)
{
    copy_vector_from(source_image_data);
    upload_texture();
}

template <>
void GenericImage::set_data<GenericImageType::ByteArray>(
    std::vector<unsigned char> const& source_image_data,
    ImageSize new_size)
{
    if (new_size != image_size) {
        resize(new_size);
    }
    this->new_data = true;

    copy_vector_from(source_image_data);
}

template <>
GenericImage megamol::frontend_resources::make_image<GenericImageType::GLTexureHandle>(
    GenericImage::ImageSize size,
    GenericImage::DataChannels channels,
    const void* data)
{
    return GenericImage(size, channels, GenericImageType::GLTexureHandle, data);
}

template <>
GenericImage megamol::frontend_resources::make_image<GenericImageType::ByteArray>(
    GenericImage::ImageSize size,
    GenericImage::DataChannels channels,
    const void* data)
{
    return GenericImage(size, channels, GenericImageType::ByteArray, data);
}

template <>
GenericImage megamol::frontend_resources::make_image<GenericImageType::GLTexureHandle>(
    GenericImage const& source_image)
{
    GenericImage result{};

    switch (source_image.image_type) {
    case GenericImageType::GLTexureHandle:
        result = source_image;
        break;
    case GenericImageType::ByteArray:
        result = megamol::frontend_resources::make_image<GenericImageType::GLTexureHandle>(
            source_image.size(),
            source_image.channels,
            source_image.image_data.data());
        break;
    default:
        break;
    }

    return result;
}

template <>
GenericImage megamol::frontend_resources::make_image<GenericImageType::ByteArray>(
    GenericImage const& source_image)
{
    GenericImage result{};

    switch (source_image.image_type) {
    case GenericImageType::GLTexureHandle:
        source_image.download_texture();

        result = megamol::frontend_resources::make_image<GenericImageType::ByteArray>(
            source_image.size(),
            source_image.channels,
            source_image.image_data.data());
        break;
    case GenericImageType::ByteArray:
        result = source_image;
        break;
    default:
        break;
    }

    return result;
}

size_t megamol::frontend_resources::channels_count(GenericImage::DataChannels channels) {
    switch (channels) {
    case GenericImage::DataChannels::RGB8:
        return 3;
        break;
    case GenericImage::DataChannels::RGBA8:
        return 4;
        break;
    default:
        return 0;
    }
}

#define images \
    (*(static_cast< std::list<std::pair<std::string, megamol::frontend_resources::GenericImage>>* >(pimpl)))

megamol::frontend_resources::GenericImage& megamol::frontend_resources::GenericImageRegistry::make(std::string const& name) {
    images.push_back({name, megamol::frontend_resources::GenericImage{}});
    updates = true;
    return images.back().second;
}
bool megamol::frontend_resources::GenericImageRegistry::rename(std::string const& old_name, std::string const& new_name) {
    auto find_it = std::find_if(images.begin(), images.end(), [&](auto const& elem) { return elem.first == old_name; });
    if (find_it == images.end())
        return false;

    find_it->first = new_name;
    updates = true;
    return true;
}
bool megamol::frontend_resources::GenericImageRegistry::remove(std::string const& name) {
    auto find_it = std::find_if(images.begin(), images.end(), [&](auto const& elem) { return elem.first == name; });
    if (find_it == images.end())
        return false;

    images.erase(find_it);
    updates = true;
    return true;
}
std::optional<std::reference_wrapper<megamol::frontend_resources::GenericImage const>>
megamol::frontend_resources::GenericImageRegistry::find(std::string const& name) const {
    auto find_it = std::find_if(images.begin(), images.end(), [&](auto const& elem) { return elem.first == name; });
    if (find_it == images.end())
        return std::nullopt;

    return std::make_optional(std::reference_wrapper<megamol::frontend_resources::GenericImage const>{find_it->second});
}
void megamol::frontend_resources::GenericImageRegistry::iterate_over_entries(
    std::function<void(std::string /*name*/, GenericImage const& /*image*/)> const& callback) const
{
    std::for_each(images.begin(), images.end(), [&](auto const& elem) { callback(elem.first, elem.second); });
}
#undef images

