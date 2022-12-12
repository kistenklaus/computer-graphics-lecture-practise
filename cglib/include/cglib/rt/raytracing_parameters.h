#pragma once

#include <cglib/rt/texture.h>
#include <cglib/rt/epsilon.h>

#include <cglib/core/parameters.h>

#include <cglib/imgui/imgui.h>

/*
 * Raytracing parameters.
 *
 * Have a look at cglib/parameters.h to see which parameters are built-in already.
 * Feel free to add more parameters here. You can add them to the AntTweakBar gui,
 * aswell, to make interactive tweaking easier.
 */
class RaytracingParameters : public Parameters
{
	public:

		void initialize();

		int display_parameters();

		enum RenderMode {
			RECURSIVE,
			DESATURATE,
			NUM_RAYS,
			NORMAL,
			TIME,
			RENDER_MODE_COUNT
		};

		const char* render_mode_names[RENDER_MODE_COUNT] = {
			"Recursive", "Desaturate", "Number of Rays", "Normal", "Time",
		};

		int active_scene = 0;

		int render_mode = 0; /*This used to be a RenderMode enum, but that doesn't work with imgui */

		bool diffuse_white_mode = false;
		int max_depth           = 4;
		bool shadows            = true;
		bool ambient            = true;
		bool diffuse            = true;
		bool specular           = true;
		bool reflection         = true;
		bool transmission       = true;
		bool fresnel            = true;
		bool dispersion         = false;
		float scale_render_time = 10.0f;
		float ray_epsilon       = 7.f*1e-3f;
		float fovy              = 45.0f;

		bool draw_spheres = true;
		bool phong = true;
		bool spot_light = true;
		float spot_light_falloff = 40.0f;

		int tex_filter_mode = TextureFilterMode::NEAREST;
		int tex_wrap_mode = TextureWrapMode::REPEAT;


	private:
};
