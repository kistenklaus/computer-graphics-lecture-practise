#pragma once

#include <functional>
#include <cglib/core/gui.h>

class DeviceRenderingContext;

/*
 * Use this class to render on the device (so primarily with OpenGL).
 */
class DeviceRender
{
public:
	/*
	 * The render and initialization functions.
	 */
	typedef std::function<bool(DeviceRenderingContext const&)> InitFunc;
	typedef std::function<void(DeviceRenderingContext const&)> RenderFunc;

	static int run(DeviceRenderingContext & context, bool b) {
		cg_assert(!"wrong overload");
	}
	static int run(DeviceRenderingContext & context, int context_flags);

	/*
	 * creates a context, executes func() and destroys the context
	 */
	static inline int run_in_context(
			Parameters &params,
			int context_flags,
			std::function<void()> func);

	static inline int run_in_context(
			Parameters &params,
			bool,
			std::function<void()> func) {
		cg_assert(!"wrong overload");
	}

private:
	static int run_interactive(DeviceRenderingContext & context);
	static int run_noninteractive(DeviceRenderingContext & context);
};

inline int DeviceRender::
run_in_context(Parameters &params, int context_flags, std::function<void()> func)
{
	if(!GUI::init_device(params, context_flags))
		return 1;
	try {
		func();
	} catch(...) {
		GUI::cleanup();
		throw;
	}
	GUI::cleanup();
	return 0;
}
