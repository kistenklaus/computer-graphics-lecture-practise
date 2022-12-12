#pragma once

#include <cglib/core/parameters.h>
#include <cglib/core/image.h>

#include <functional>

class Camera;

namespace GUI {
	enum {
		LEGACY_OPENGL        = 1 << 0,
		DEBUG_CONTEXT        = 1 << 1,

		DEFAULT_FLAGS        = DEBUG_CONTEXT,
	};

	enum {
		FLAG_REDRAW        = 1 << 0,
		FLAG_REFRESH_SCENE = 1 << 1,
	};

	bool keep_running();
	void cleanup();
	void poll_events();
	void next_frame(); 
	void draw();

	bool init_host(Parameters& params);
	int display_host(Image const& frame_buffer, std::function<void()> const& render_overlay);

	bool init_device(Parameters& params, int context_flags);
	void display_device();

}

