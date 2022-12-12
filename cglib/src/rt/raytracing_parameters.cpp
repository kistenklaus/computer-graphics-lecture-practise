#include <cglib/rt/raytracing_parameters.h>
#include <cglib/core/gui.h>
#include <cglib/rt/raytracing_context.h>
#include <cglib/rt/scene.h>

/*
 * ImGui Notes:
 * - every element needs to have a unique name
 * - if you don't want to display a name next to the input field, use prefix ##
 * 		e.g. ImGui::InputFloat("##MyFloat", &some_float);
 * - never give two elements the same name, or they won't work properly
 */

void RaytracingParameters::initialize()
{
}

int RaytracingParameters::display_parameters()
{
	bool redraw = false;
	bool refresh_scene = false;

	bool draw_render_settings = true;
	bool draw_shading_settings = true;
	bool draw_texture_settings = true;

	refresh_scene |= ImGui::Combo("Scene", &active_scene, &RaytracingContext::get_active()->scene_names, RaytracingContext::get_active()->scene_names.size());

	ImGui::DragFloat("Exposure", &exposure, 0.1f, -100.f, 100.f);
	ImGui::DragFloat("Gamma", &gamma, 0.05f, 0.0f, 100.f);


	if(draw_render_settings && ImGui::CollapsingHeader("Render Settings"))
	{
		redraw |= ImGui::Combo("Render Mode", &render_mode, &render_mode_names[0], RENDER_MODE_COUNT);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(
"Recursive:               Whitted Style Raytracing\n"
"Desaturate:              Luminance of Whitted Style Raytracing\n"
"Number of Rays:          Number of rays created during recursive traversal\n"
"Normal:                  Surface normals\n"
"Time:                    Time spent on each pixel\n"
			);
		}
		if (render_mode == TIME
		)
		{
			redraw |= ImGui::DragFloat("Render Time Exposure", &scale_render_time, 0.1f, 0.f, 1000.f);
		}
		redraw |= ImGui::InputInt("Max Recursion Depth", &max_depth);
		redraw |= ImGui::DragFloat("Ray Epsilon", &ray_epsilon, 0.00001f, 0.0f, 0.f, "%.7f");
		redraw |= ImGui::DragFloat("Field of View Y", &fovy);
		redraw |= ImGui::InputInt("Render Threads", &num_threads);
		redraw |= ImGui::Checkbox("Stereo Rendering", &stereo);
		if (stereo) {
			redraw |= ImGui::DragFloat("Eye Separation", &eye_separation, 0.01f, 0.f, 0.f);
			redraw |= ImGui::DragFloat("Focal Distance", &focal_distance, 0.01f, 0.f, 0.f);
		}
	}

	if (draw_shading_settings && ImGui::CollapsingHeader("Shading Settings"))
	{
		redraw |= ImGui::Checkbox("Diffuse White", &diffuse_white_mode);
		redraw |= ImGui::Checkbox("Shadows", &shadows);
		redraw |= ImGui::Checkbox("Ambient Lighting", &ambient);
		redraw |= ImGui::Checkbox("Diffuse Lighting", &diffuse);
		redraw |= ImGui::Checkbox("Specular Lighting", &specular);
		redraw |= ImGui::Checkbox("Reflection", &reflection);
		redraw |= ImGui::Checkbox("Transmission", &transmission);
		redraw |= ImGui::Checkbox("Fresnel", &fresnel);
		redraw |= ImGui::Checkbox("Dispersion", &dispersion);
	}

	if (dynamic_cast<SpherePortrait *>(RaytracingContext::get_active()->get_active_scene()))
	{
		if (ImGui::CollapsingHeader("Spotlight Settings"))
		{
			refresh_scene |= ImGui::Checkbox("Spotlight", &spot_light);
			refresh_scene |= ImGui::InputFloat("Falloff", &spot_light_falloff);
		}
	}

	auto flags = 0
		| (redraw        ? GUI::FLAG_REDRAW        : 0)
		| (refresh_scene ? GUI::FLAG_REFRESH_SCENE : 0);

	return flags;
}

