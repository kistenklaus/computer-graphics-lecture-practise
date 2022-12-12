#pragma once

#include <cglib/rt/raytracing_parameters.h>
#include <cglib/core/assert.h>

#include <memory>

class Scene;

struct RaytracingContext
{
	RaytracingContext();
	~RaytracingContext();
	static RaytracingContext *get_active();

    RaytracingParameters params;

	Scene *get_active_scene() const { return scenes[params.active_scene].get(); }
	void add_scene(std::shared_ptr<Scene> scene);

	const std::vector<std::shared_ptr<Scene>> &get_scenes() { return scenes; }
	std::vector<std::string> scene_names;


private:
	std::vector<std::shared_ptr<Scene>> scenes;
	static RaytracingContext *current_context;
	static RaytracingContext *old_context;
};
