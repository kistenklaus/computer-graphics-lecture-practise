#pragma once

#include <memory>
#include <vector>
#include <string>

#include <cglib/core/assert.h>
#include <cglib/core/parameters.h>

class Renderer;

class DeviceRenderingContext
{
public:
	DeviceRenderingContext();
	~DeviceRenderingContext();
	std::shared_ptr<Renderer> get_current_renderer() const;
	static DeviceRenderingContext *get_active();
	static void reload_all_shaders();

	Parameters params;
	std::vector<std::shared_ptr<Renderer>> renderers;
	int current_renderer = 0;
private:
	static DeviceRenderingContext *current_context;
};
