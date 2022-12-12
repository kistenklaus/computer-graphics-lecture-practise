#include <cglib/core/parameters.h>
#include <cglib/core/glheaders.h>
#include <cglib/core/camera.h>
#include <cglib/core/image.h>
#include <cglib/core/gui.h>
#include <cglib/imgui/imgui.h>
#include <cglib/imgui/imgui_impl_glfw_gl2.h>
#include <cglib/imgui/imgui_impl_glfw_gl3.h>

#include <iostream>
#include <algorithm>
#include <functional>
#include <cstdlib>

using std::cout;
using std::cerr;
using std::endl;
using std::flush;

static GLFWwindow *window;
static unsigned int tex_frame_buffer;
static unsigned int program_draw_texture;
static double cursor_x, cursor_y;
static bool in_camera_drag = false;

#ifdef __APPLE__
// this is for retina displays where
// we need to pass modified coordinates
// to antweakbar
static float scale_retina_x = 1.f;
static float scale_retina_y = 1.f;
#endif

static bool write_screenshot = false;

static Parameters* parameters = nullptr;
static bool modernOpenGLContext = false;
// -----------------------------------------------------------------------------

static void
glfw_error_callback(int error, const char *description)
{
	cerr << "GLFW Error: " << description << endl;
}

static void
glfw_key_callback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.WantCaptureKeyboard) {
		if (modernOpenGLContext) {
			ImGui_ImplGlfwGL3_KeyCallback(win, key, scancode, action, mods);
		} else {
			ImGui_ImplGlfwGL2_KeyCallback(win, key, scancode, action, mods);
		}
		return;
	}
	auto cam = Camera::get_active();
	if (cam && cam->handle_key_event(key, action)) {
		return;
	}

	switch(key) {
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(win, GL_TRUE);
		return;
	case GLFW_KEY_F4:
		write_screenshot |= (action == GLFW_PRESS);
		break;
	default:
		break;
	}
}

static void
glfw_char_callback(GLFWwindow *win, unsigned int chr)
{
	if (modernOpenGLContext) {
		ImGui_ImplGlfwGL3_CharCallback(win, chr);
	} else {
		ImGui_ImplGlfwGL2_CharCallback(win, chr);
	}
}

static void
glfw_button_callback(GLFWwindow *win, int button, int action, int mods)
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.WantCaptureMouse) {
		if (modernOpenGLContext) {
			ImGui_ImplGlfwGL3_MouseButtonCallback(win, button, action, mods);
		} else {
			ImGui_ImplGlfwGL2_MouseButtonCallback(win, button, action, mods);
		}
		return;
	}
	
	// camera drag end
	if (in_camera_drag && button == 0 && action == GLFW_RELEASE)
	{
		//glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		in_camera_drag = false;
	}

	auto cam = Camera::get_active();
	if (cam && cam->handle_mouse_button_event(button, action)) {
		return;
	}
}

static void
glfw_motion_callback(GLFWwindow *win, double x, double y)
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.WantCaptureMouse)
		return;

	auto cam = Camera::get_active();
	if (glfwGetMouseButton(win, 0) == GLFW_PRESS
		&& cam && cam->handle_mouse_drag_event(static_cast<float>(cursor_x - x), static_cast<float>(cursor_y - y)))
	{
		if (!in_camera_drag)
		{
			// drag start
//			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // broken on too many platforms right now, retry with newer version of GLFW
			in_camera_drag = true;
		}

		cursor_x = x;
		cursor_y = y;
		return;
	}

    cursor_x = x;
    cursor_y = y;
}

static void
glfw_mouse_wheel_callback(GLFWwindow *win, double x, double y)
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.WantCaptureMouse) {
		if (modernOpenGLContext) {
			ImGui_ImplGlfwGL3_ScrollCallback(win, x, y);
		} else {
			ImGui_ImplGlfwGL2_ScrollCallback(win, x, y);
		}
		return;
	}

	auto cam = Camera::get_active();
	if (cam && cam->handle_mouse_wheel_event(static_cast<float>(x), static_cast<float>(y)))
	{
		return;
	}
}

static void
glfw_cursor_enter_callback(GLFWwindow *win, int i)
{
    glfwGetCursorPos(win, &cursor_x, &cursor_y);
}

static void
glfw_resize_callback(GLFWwindow *win, int w, int h)
{
#ifndef __APPLE__
	glViewport(0, 0, w, h);
	if (parameters)
	{
		parameters->screen_width = w;
		parameters->screen_height = h;
	}
#endif
}

#ifdef __APPLE__
static void
glfw_framebuffer_resize_callback(GLFWwindow *win, int w, int h)
{
	glViewport(0, 0, w, h);
	if (parameters)
	{
		parameters->screen_width = w;
		parameters->screen_height = h;
	}
}
#endif

// -----------------------------------------------------------------------------

static unsigned int
compile_src(const std::vector<const char *> &src, int type)
{
	GLuint shader_obj = glCreateShader(type);

	// glShaderSource wants char const**, not char const* const*.
	std::vector<const char*> srcs(src);

	glShaderSource(shader_obj, src.size(), srcs.data(), nullptr);
	glCompileShader(shader_obj);

	int success;
	glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success);
	if(!success) {
		char infolog[2048];
		glGetShaderInfoLog(shader_obj, sizeof infolog, NULL, infolog);
		printf("error compiling shader: %s\n", infolog);
	}

	return shader_obj;
}

static unsigned int
compile_shader(
		const std::vector<const char *> &src_vertex,
		const std::vector<const char *> &src_fragment)
{
	GLuint prog = glCreateProgram();

	unsigned int vert = 0, frag = 0;

	glAttachShader(prog, vert = compile_src(src_vertex, GL_VERTEX_SHADER));
	glAttachShader(prog, frag = compile_src(src_fragment, GL_FRAGMENT_SHADER));
	glLinkProgram(prog);

	int success;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if(!success) {
		char infolog[2048];
		glGetProgramInfoLog(prog, sizeof infolog, NULL, infolog);
		printf("error linking shaderprogram: %s\n", infolog);
	}

	glDeleteShader(vert);
	glDeleteShader(frag);

	return prog;
}

// -----------------------------------------------------------------------------

static void APIENTRY
debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void *userParam)
{
	bool Error = false;
	bool Info = false;

	char const* srcS = "Unknown Source";
	if (source == GL_DEBUG_SOURCE_API_ARB)
		srcS = "OpenGL";
	else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
		srcS = "Windows";
	else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
		srcS = "Shader Compiler";
	else if (source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
		srcS = "Third Party";
	else if (source == GL_DEBUG_SOURCE_APPLICATION_ARB)
		srcS = "Application";
	else if (source == GL_DEBUG_SOURCE_OTHER_ARB)
		srcS = "Other";

	char const* typeS = "unknown error";
	if (type == GL_DEBUG_TYPE_ERROR_ARB)
		typeS = "error";
	else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
		typeS = "deprecated behavior";
	else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
		typeS = "undefined behavior";
	else if (type == GL_DEBUG_TYPE_PORTABILITY_ARB)
		typeS = "portability";
	else if (type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
		typeS = "performance";
	else if (type == GL_DEBUG_TYPE_OTHER_ARB)
	{
		typeS = "message";
		Info = false
			|| id == 131076 // NVIDIA: Small pointer value, intended to be used as an offset into a buffer? Yes.
			|| (strstr(message, "info:") != nullptr);
	}

	char const* severityS = "unknown severity";
	if (severity == GL_DEBUG_SEVERITY_HIGH_ARB)
	{
		severityS = "high";
		Error = true;
	}
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
		severityS = "medium";
	else if (severity == GL_DEBUG_SEVERITY_LOW_ARB)
		severityS = "low";

	if (Error || !Info)
		std::cout << srcS << ": " << typeS << "(" << severityS << ") " << id << ": " << message << std::endl;
}

bool
create_context(Parameters const& params, int context_flags)
{
	glfwSetErrorCallback(glfw_error_callback);
	if(!glfwInit())
		return false;

	bool debug_context = !!(context_flags & GUI::DEBUG_CONTEXT);
	bool modern_gl = !(context_flags & GUI::LEGACY_OPENGL);

	modernOpenGLContext = modern_gl;

	if (debug_context)
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	if(!params.interactive) /* window should have fixed size for testing */
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	if(modern_gl) {
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
	#ifdef __linux__
		/* check if using mesa, context flags don't work with mesa */
		if(!getenv("MESA_GL_VERSION_OVERRIDE"))
	#endif
		{
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		}
	}
	else {
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	}
	/* hiding the window will discard all pixel operations on the window
	 * frame buffer. To obtain the results, it is required to render into
	 * a FBO */
	glfwWindowHint(GLFW_VISIBLE, params.interactive);

	if(!(window = glfwCreateWindow(params.screen_width, params.screen_height, "cg framework", nullptr, nullptr))) {
		glfwTerminate();
		return false;
	}
	

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetCharCallback(window, glfw_char_callback);
	glfwSetCursorPosCallback(window, glfw_motion_callback);
	glfwSetMouseButtonCallback(window, glfw_button_callback);
	glfwSetScrollCallback(window, glfw_mouse_wheel_callback);
	glfwSetWindowSizeCallback(window, glfw_resize_callback);
#ifdef __APPLE__
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resize_callback);
#endif
	glfwSetCursorEnterCallback(window, glfw_cursor_enter_callback);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK) {
		cerr << "Error: " << glewGetErrorString(err) << endl;
		return false;
	}
	while(glGetError() != GL_NO_ERROR);

	if(debug_context) {
		if (glDebugMessageControl) {
			GLuint mask_ids[] = {
				131076, // Generic Vertex Attrib foo
				131185, // memory type for buffer operations
				131218, // shader recompile
				131186, // buffer moved
			};
			glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE,
					sizeof(mask_ids) / sizeof(mask_ids[0]), mask_ids, GL_FALSE);

			glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE,
					sizeof(mask_ids) / sizeof(mask_ids[0]), mask_ids, GL_FALSE);

		}
		if (glDebugMessageCallback) {
			glDebugMessageCallback(&debugCallback, NULL);
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}
	}

#ifdef __APPLE__
	{
		int win_width, win_height;
		glfwGetWindowSize(window, &win_width, &win_height);
		int fb_width, fb_height;
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		scale_retina_x = float(fb_width)  / float(win_width);
		scale_retina_y = float(fb_height) / float(win_height);
		std::cout
			<< "scale for retina? scale_x: " << scale_retina_x
			<< ", scale_y: " << scale_retina_y << std::endl;
	}
#endif

	return true;
}

void
init_host_texture(Parameters const& params)
{
	program_draw_texture = compile_shader({
"#version 120\n"
"varying vec2 tex_coord;\n"
"uniform float w;\n"
"uniform float h;\n"
"void\n"
"main()\n"
"{\n"
"	vec2 v;\n"
"	vec4 p = gl_Vertex;\n"
"	tex_coord = p.xy * 0.5 + 0.5;\n"
"	gl_Position = vec4(vec2(w, h) * p.xy, 0.0, 1.0);\n"
"}\n"} , {
"#version 120\n"
"varying vec2 tex_coord;\n"
"uniform float gamma;\n"
"uniform float exposure;\n"
"uniform sampler2D framebuffer;\n"
"void\n"
"main()\n"
"{\n"
"	gl_FragColor = pow(2, exposure) * pow(texture2D(framebuffer, tex_coord), vec4(gamma));\n"
"}\n" }
);
	glGenTextures(1, &tex_frame_buffer);
	glBindTexture(GL_TEXTURE_2D, tex_frame_buffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, params.image_width, params.image_height, 0,
		GL_RGBA, GL_FLOAT, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// -----------------------------------------------------------------------------

bool GUI::
keep_running()
{
	return !glfwWindowShouldClose(window);
}

void GUI::
cleanup()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void GUI::
poll_events()
{
	return glfwPollEvents();
}

bool GUI::
init_host(Parameters& params)
{
	parameters = &params;

	if(!create_context(params, GUI::LEGACY_OPENGL)) {
		return false;
	}

	if (modernOpenGLContext) {
		ImGui_ImplGlfwGL3_Init(window, false);
	} else {
		ImGui_ImplGlfwGL2_Init(window, false);
	}

	//glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);
	//glfwSetScrollCallback(window, ImGui_ImplGlfwGL3_ScrollCallback);
	//glfwSetKeyCallback(window, ImGui_ImplGlfwGL3_KeyCallback);
	//glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);

	glClearColor(0, 0, 0, 0);
	init_host_texture(params);
	return true;
}

int GUI::
display_host(Image const& frame_buffer, std::function<void()> const& render_overlay)
{
	if (write_screenshot)
	{
		write_screenshot = false;
		frame_buffer.save("screenshot.png", 2.2f);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float w = float(parameters->screen_height) / float(frame_buffer.getHeight());
	float h = float(parameters->screen_width)  / float(frame_buffer.getWidth());
	float tmp = std::max < float> (w, h);
	{
		w /= tmp;
		h /= tmp;
	}

	glUseProgram(program_draw_texture);
	glUniform1f(glGetUniformLocation(program_draw_texture, "w"), w);
	glUniform1f(glGetUniformLocation(program_draw_texture, "h"), h);
	glUniform1f(glGetUniformLocation(program_draw_texture, "gamma"), 1.0f / parameters->gamma);
	glUniform1f(glGetUniformLocation(program_draw_texture, "exposure"), parameters->exposure);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_frame_buffer);
	glTexSubImage2D(GL_TEXTURE_2D, 0, /* x offset */ 0, /* y offset */ 0,
		frame_buffer.getWidth(), frame_buffer.getHeight(), GL_RGBA, GL_FLOAT, frame_buffer.raw_data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glRectf(-1.0f, -1.0f, 1.0f, 1.0f);

	glUseProgram(0);

	render_overlay();

	GUI::next_frame();
	ImGui::Begin("Settings");

	int update_flags = parameters->display_parameters();
	if (ImGui::CollapsingHeader("Gui Settings"))
	{
		if (ImGui::DragFloat("Scale Font", &ImGui::GetIO().FontGlobalScale, 0.005, 0.3f, 2.5f, "%.1f"))
		{
			ImGui::ForceIniSettingsDirty();
		}
	}

	ImGui::End();

	GUI::draw();

	glfwSwapBuffers(window);

	return update_flags;
}

bool GUI::
init_device(Parameters& params, int context_flags)
{
	parameters = &params;

	if(!create_context(params, context_flags)) {
		return false;
	}

	if (modernOpenGLContext) {
		ImGui_ImplGlfwGL3_Init(window, false);
	} else {
		ImGui_ImplGlfwGL2_Init(window, false);
	}

	return true;
}

void GUI::
next_frame()
{
	if (modernOpenGLContext) {
		ImGui_ImplGlfwGL3_NewFrame();
	} else {
		ImGui_ImplGlfwGL2_NewFrame();
	}
}

void GUI::
draw()
{
	ImGui::Render();
}

void GUI::
display_device()
{
	if (write_screenshot)
	{
		write_screenshot = false;
		Image frame_buffer(parameters->screen_width, parameters->screen_height);
		glReadPixels(0, 0, frame_buffer.getWidth(), frame_buffer.getHeight(), GL_RGBA, GL_FLOAT, frame_buffer.getPixels());
		frame_buffer.save("screenshot.png", 1.0f);
	}

	glfwSwapBuffers(window);
}
