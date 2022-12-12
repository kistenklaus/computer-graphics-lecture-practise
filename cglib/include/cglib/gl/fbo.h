#ifndef  __FBO_HPP__
#define  __FBO_HPP__

#include <cglib/core/glheaders.h>
#include <cglib/core/assert.h>
#include <cstring>

struct FBOInit
{
	enum { MAX_COLOR_ATTACHMENTS = 8 };
	unsigned int color[MAX_COLOR_ATTACHMENTS] = {0};
	unsigned int depth = 0;
	unsigned int rb = 0;
	unsigned int samples = 0;
	int width, height;
	const char *label = nullptr;
	int depth_samples = 0, color_samples = 0;
	bool color_mip = false;
	bool depth_mip = false;

	FBOInit(int w, int h) : width(w), height(h) {}

	FBOInit & attach_color(int idx, GLenum type) {
		color[idx] = type;
		return *this;
	}

	FBOInit & attach_depth(GLenum type) {
		depth = type;
		return *this;
	}

	FBOInit & attach_rb(GLenum type) {
		rb = type;
		return *this;
	}

	FBOInit & num_samples(int s) {
		samples = s;
		return *this;
	}

	FBOInit & attach_label(const char *l) {
		label = l;
		return *this;
	}

	FBOInit & mixed_samples(int num_color_samples, int num_depth_samples) {
		color_samples = num_color_samples;
		depth_samples = num_depth_samples;
		cg_assert(color_samples > 0);
		cg_assert(depth_samples > 1);
		return *this;
	}

	FBOInit & use_color_mip() {
		color_mip = true;
		return *this;
	}

	FBOInit & use_depth_mip() {
		depth_mip = true;
		return *this;
	}
};

struct FBO
{
	int width, height;
	unsigned int fbo = 0;
	unsigned int tex_color[FBOInit::MAX_COLOR_ATTACHMENTS] = {0};
	unsigned int rb = 0;
	unsigned int tex_depth = 0;
	int old_view[4];
	int old_fb;
	bool is_bound = false;
	int free_mask = ~0;

	template<typename F>
	void
	render_into(F f)
	{
		this->bind();
		f();
		this->unbind();
	}

	void bind();
	void unbind();
	FBO & attach_texture(int idx_, int tex_);
	FBO & attach_rb(GLenum type, int rb_);
	FBO & attach_depth_tex(GLenum target, int tex_);

private:
	void init_fbo(const FBOInit &fbo_init);
	void init_fbo_ms(const FBOInit &fbo_init);
	void init_fbo_mixed_samples(const FBOInit &fbo_init);
	void attach_labels(const FBOInit &fbo_init);

public:
	FBO(const FBOInit &fbo_init)
		: width(fbo_init.width), height(fbo_init.height)
	{
		if(fbo_init.depth_samples > 0 && fbo_init.color_samples > 0) {
			init_fbo_mixed_samples(fbo_init);
		}
		else if(fbo_init.samples > 0)
			init_fbo_ms(fbo_init);
		else
			init_fbo(fbo_init);
		if(fbo_init.label)
			attach_labels(fbo_init);
	}

	FBO(const FBO &) = delete;
	FBO & operator=(const FBO &) = delete;

	FBO(FBO &&f) {
		std::memcpy(this, &f, sizeof(FBO));
		std::memset(&f, 0, sizeof(FBO));
	}

	FBO & operator=(FBO &&f) {
		std::memcpy(this, &f, sizeof(FBO));
		std::memset(&f, 0, sizeof(FBO));
		return *this;
	}

	~FBO();

};

#endif  /*__FBO_HPP__*/
