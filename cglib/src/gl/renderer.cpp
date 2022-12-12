#include <cglib/core/assert.h>
#include <cglib/gl/fbo.h>

#include <cglib/gl/renderer.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cglib/core/glmstream.h>

#include <random>

#include <cglib/imgui/imgui.h>

void Renderer::set_interactive(bool render_interactive)
{
	interactive = render_interactive;
}

void Renderer::
activate()
{
	if(camera)
		camera->set_active();

	_activate();
}

void Renderer::
deactivate()
{
	_deactivate();
}

void Renderer::
update_objects(double time_step)
{
	current_time += time_step;

	if(camera)
		camera->update_time_dependant((float)time_step);

	_update_objects(time_step);
}

void Renderer::
resize(int width, int height)
{
	resize_was_called = true;
	projection = glm::perspective(
			glm::radians(fov),
			float(width) / float(height),
			near_clip, far_clip);

	_resize(width, height);
}

void Renderer::
draw()
{
	cg_assert(resize_was_called);
	glEnable(GL_FRAMEBUFFER_SRGB);
	_draw();
	glDisable(GL_FRAMEBUFFER_SRGB);
}

void Renderer::
destroy()
{
   _destroy();
}

int Renderer::
initialize()
{
	return _initialize();
}
