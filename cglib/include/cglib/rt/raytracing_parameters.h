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

		TextureFilterMode get_tex_filter_mode() const;
		TextureWrapMode get_tex_wrap_mode() const;

		enum RenderMode {
			RECURSIVE,
			DESATURATE,
			NUM_RAYS,
			NORMAL,
			TIME,
			DUDV,
			RENDER_MODE_COUNT
		};

		const char* render_mode_names[RENDER_MODE_COUNT] = {
			"Recursive", "Desaturate", "Number of Rays", "Normal", "Time",
			"du dv",
		};

		int active_scene = 0;

		enum FourierMode {
			FOURIER_AMPLITUDE,
			FOURIER_PHASE,
			FOURIER_RECONSTRUCTION,
			FOURIER_MODE_COUNT
		};

		const char* fourier_mode_names[FOURIER_MODE_COUNT] = {
			"Amplitude", "Phase", "Reconstructed Image"
		};

		int fourier_mode = FOURIER_AMPLITUDE;
		
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

		bool stratified = false;

		bool normal_mapping = false;
		bool transform_objects = true;
		int spp = 1; // number of samples per pixel

		int tex_filter_mode = TextureFilterMode::NEAREST;
		int tex_wrap_mode = TextureWrapMode::REPEAT;


	private:
};
