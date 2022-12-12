#include <cglib/core/parameters.h>
#include <cglib/gl/device_rendering_context.h>
#include <cglib/gl/renderer.h>


DeviceRenderingContext *DeviceRenderingContext::current_context = nullptr;

DeviceRenderingContext::
DeviceRenderingContext()
{
	cg_assert(!current_context);
	current_context = this;
}

DeviceRenderingContext::
~DeviceRenderingContext()
{
	cg_assert_noexcept(current_context);
	current_context = nullptr;
}

DeviceRenderingContext *DeviceRenderingContext::
get_active()
{
	cg_assert(current_context);
	return current_context;
}

std::shared_ptr<Renderer> DeviceRenderingContext::
get_current_renderer() const
{
	auto it = renderers.at(current_renderer);
	return it;

	cg_assert(!"invalid current renderer, didn't find any matching "
			"renderer in renderers");
	return nullptr;
}

void DeviceRenderingContext::
reload_all_shaders()
{
	auto ctx = get_active();
	for(auto &i: ctx->renderers) {
		i->shader_manager.reload_shader();
	}
}
