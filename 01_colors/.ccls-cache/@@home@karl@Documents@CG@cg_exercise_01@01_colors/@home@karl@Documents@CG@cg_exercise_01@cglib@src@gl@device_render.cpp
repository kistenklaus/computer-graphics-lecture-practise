#include <cglib/core/assert.h>
#include <cglib/core/gui.h>
#include <cglib/core/image.h>
#include <cglib/core/timer.h>
#include <cglib/gl/device_render.h>
#include <cglib/gl/device_rendering_context.h>
#include <cglib/gl/renderer.h>
#include <cglib/imgui/imgui.h>
#include <cstring>

#ifdef _WIN32
	// Use discrete NVIDIA GPU on laptops
	extern "C" _declspec(dllexport) DWORD const NvOptimusEnablement = 1;
#endif

int DeviceRender::
run(DeviceRenderingContext &context, int context_flags)
{
	if(!GUI::init_device(context.params, context_flags)) {
		return 1;
	}

	int status = 0;
	try {
		for(auto &i: context.renderers) {
			if(i->initialize()) {
				std::cerr << "error initializing renderers" << std::endl;
				return 1;
			}
		}
		cg_assert(context.get_current_renderer());
		context.get_current_renderer()->activate();

		if(context.params.interactive) {
			status = run_interactive(context);
		}
		else {
			status = run_noninteractive(context);
		}

		context.get_current_renderer()->deactivate();
		for(auto &i: context.renderers) {
			i->destroy();
		}
	}
	catch(...) {
		GUI::cleanup();
		throw;
	}
	GUI::cleanup();

	return status;
}

// -----------------------------------------------------------------------------

int DeviceRender::
run_noninteractive(DeviceRenderingContext &context)
{
	int status = 0;

	context.get_current_renderer()->resize(context.params.screen_width, context.params.screen_height);
	context.get_current_renderer()->update_objects(1e-8f);
	context.get_current_renderer()->draw();
	Image frame_buffer(context.params.image_width, context.params.image_height);
	glReadPixels(0, 0, context.params.image_width, context.params.image_height,
			GL_RGBA, GL_FLOAT, (float *)frame_buffer.getPixels());

	frame_buffer.save(context.params.output_file_name.c_str(), 1.0f);

	return status;
}

// -----------------------------------------------------------------------------

int DeviceRender::
run_interactive(DeviceRenderingContext &context)
{
	int status = 0;

	std::vector<const char *> renderer_names;
	renderer_names.reserve(context.renderers.size());
	for(auto &i: context.renderers) {
		renderer_names.push_back(i->get_name());
	}

	auto old_params = context.params;
	auto current_renderer = context.get_current_renderer();
	Timer timer;
	double frame_time = 1e-8f;
	current_renderer->resize(context.params.screen_width, context.params.screen_height);
	while (GUI::keep_running())
	{
		timer.start();
		GUI::poll_events();

		if(context.params.screen_width  != old_params.screen_width
		|| context.params.screen_height != old_params.screen_height)
		{
			glViewport(0, 0, context.params.screen_width, context.params.screen_height);
			current_renderer->resize(context.params.screen_width, context.params.screen_height);
		}
		old_params = context.params;
		GUI::next_frame();

		if(shader_error_log.length() > 0) {
			ImGui::Begin("Shader Compile Errors");
			ImGui::TextWrapped("%s", shader_error_log.c_str());
			if(ImGui::Button("Clear Shader Error")) {
				shader_error_log.clear();
			}
			ImGui::End();
		}

		if(context.renderers.size() > 1) { // show selection box only when there are multiple renderers
			if(ImGui::Combo("Current renderer", &context.current_renderer, renderer_names.data(), renderer_names.size())) {
				current_renderer->deactivate();
				current_renderer = context.get_current_renderer();
				current_renderer->activate();
				current_renderer->resize(context.params.screen_width, context.params.screen_height);
			}
		}
		ImGui::Separator();

		current_renderer->update_objects(frame_time);
		current_renderer->draw();
		if (ImGui::CollapsingHeader("Gui Settings"))
		{
			if (ImGui::DragFloat("Scale Font", &ImGui::GetIO().FontGlobalScale, 0.005, 0.3f, 2.5f, "%.1f"))
			{
				ImGui::ForceIniSettingsDirty();
			}
		}
		GUI::draw();
		GUI::display_device();
		timer.stop();
		frame_time = static_cast<double>(timer.getElapsedTimeInSec());
		timer.reset();
	}
	return status;
}


