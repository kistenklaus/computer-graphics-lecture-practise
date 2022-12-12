#pragma once

#include <math.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

class Image;

enum TextureFilterMode {
	NEAREST, 
	BILINEAR, 
	TRILINEAR, 
	DEBUG_MIP,
	WHITE,
	TEXTURE_FILTER_MODE_COUNT
};

extern const char* tex_filter_mode_names[TEXTURE_FILTER_MODE_COUNT];

enum TextureWrapMode {
	REPEAT, 
	CLAMP, 
	ZERO,
	TEXTURE_WRAP_MODE_COUNT
};

extern const char* tex_wrap_mode_names[TEXTURE_WRAP_MODE_COUNT];

class Texture
{ 
public:
	virtual ~Texture() {}
	virtual glm::vec4 evaluate(glm::vec2 const& uv, glm::vec2 const& dudv = glm::vec2(0.f)) const = 0;
};

class ConstTexture : public Texture
{
public:
    ConstTexture(glm::vec3 const& value_) :
        Texture(),
        value(value_)
    {}

	glm::vec4 evaluate(glm::vec2 const& /*uv*/, glm::vec2 const& /*dudv*/) const override
    {
        return glm::vec4(value, 0.0f);
    }

private:
    glm::vec3 value = glm::vec3(0.0f);
};

