#include <cglib/core/glheaders.h>
#include <cglib/core/assert.h>
#include <cglib/gl/fbo.h>

#include <iostream>

using namespace std;


void FBO::
bind()
{
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fb);
	glGetIntegerv(GL_VIEWPORT, old_view);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, this->width, this->height);
#ifndef NDEBUG
	cg_assert(!is_bound);
	is_bound = true;
	cg_assert(unsigned(old_fb) != fbo);
#endif
}

void FBO::
unbind()
{
#ifndef NDEBUG
	cg_assert(is_bound);
	is_bound = false;
#endif
	glBindFramebuffer(GL_FRAMEBUFFER, old_fb);
	glViewport(old_view[0], old_view[1], old_view[2], old_view[3]);
}

static GLuint
gen_renderbuffer_ms(GLenum internal_format, int w, int h, int samples)
{
	GLuint rb;
	glGenRenderbuffers(1, &rb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
			internal_format, w, h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return rb;
}

static GLuint
gen_texture_ms(GLenum internal_format, int w, int h, int samples)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
#ifndef RENDER_UTILS_GLES
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples,
			internal_format, w, h, GL_TRUE);
#endif
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	return tex;
}

static GLuint
gen_renderbuffer(GLenum internal_format, int w, int h)
{
	GLuint rb;
	glGenRenderbuffers(1, &rb);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glRenderbufferStorage(GL_RENDERBUFFER, internal_format, w, h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return rb;
}

static GLuint
gen_texture(GLenum internal_format, int w, int h)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexStorage2D(GL_TEXTURE_2D, 1, internal_format, w, h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

static GLuint
gen_texture_with_mip(GLenum internal_format, int w, int h)
{
	int num_levels = 0;
	for(int i = w > h ? w : h; i > 0; i /= 2)
		num_levels++;

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexStorage2D(GL_TEXTURE_2D, num_levels, internal_format, w, h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

static GLuint
gen_fbo()
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	return fbo;
}

static void
check_framebuffer_complete()
{
	auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		//cerr << "error setting up fbo: "
		//	 << GLUtil::map_gl_enum_to_string(status)
		//	 << std::endl;
	}
}

void FBO::
init_fbo_ms(const FBOInit &fbo_init)
{
	int w = width, h = height;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo = gen_fbo());

	GLenum active_attachments[FBOInit::MAX_COLOR_ATTACHMENTS];
	GLenum f;
	for(int i = 0; i < FBOInit::MAX_COLOR_ATTACHMENTS; i++) {
		if((f = fbo_init.color[i])) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
					GL_TEXTURE_2D_MULTISAMPLE,
					tex_color[i] = gen_texture_ms(f, w, h, fbo_init.samples), 0);
			active_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		else {
			active_attachments[i] = GL_NONE;
		}
	}
	glDrawBuffers(FBOInit::MAX_COLOR_ATTACHMENTS, active_attachments);

	if((f = fbo_init.rb)) {
		GLenum attach = GL_DEPTH_ATTACHMENT;
		if(f == GL_DEPTH24_STENCIL8 || f == GL_DEPTH32F_STENCIL8) {
			attach = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER,
				rb = gen_renderbuffer_ms(f, w, h, fbo_init.samples));
	}

	if((f = fbo_init.depth)) {
		tex_depth = gen_texture_ms(f, w, h, fbo_init.samples);
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
#ifndef RENDER_UTILS_GLES
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, tex_depth, 0);
		glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR, l_ClampColor);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_BORDER);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_BORDER);
#else
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, tex_depth, 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex_depth);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_MODE,     GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri (GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_COMPARE_FUNC,     GL_LEQUAL);
		glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BORDER_COLOR_EXT, l_ClampColor);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#endif
	}

	check_framebuffer_complete();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::
init_fbo(const FBOInit &fbo_init)
{
	int w = width, h = height;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo = gen_fbo());

	GLenum active_attachments[FBOInit::MAX_COLOR_ATTACHMENTS];

	GLenum f;
	for(int i = 0; i < FBOInit::MAX_COLOR_ATTACHMENTS; i++) {
		if((f = fbo_init.color[i])) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
					GL_TEXTURE_2D, tex_color[i] = (fbo_init.color_mip ? gen_texture_with_mip : gen_texture)(f, w, h), 0);
			active_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		else {
			active_attachments[i] = GL_NONE;
		}
	}
	glDrawBuffers(FBOInit::MAX_COLOR_ATTACHMENTS, active_attachments);

	if((f = fbo_init.rb)) {
		GLenum attach = GL_DEPTH_ATTACHMENT;
		if(f == GL_DEPTH24_STENCIL8 || f == GL_DEPTH32F_STENCIL8) {
			attach = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER,
				rb = gen_renderbuffer(f, w, h));
	}

	if((f = fbo_init.depth)) {

		tex_depth = (fbo_init.depth_mip ? gen_texture_with_mip : gen_texture)(f, w, h);
#ifndef RENDER_UTILS_GLES
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, l_ClampColor);
		//glTextureParameteri (tex_depth, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		//glTextureParameteri (tex_depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_BORDER);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_BORDER);
#else
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
		glBindTexture(GL_TEXTURE_2D, tex_depth);
		//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,     GL_COMPARE_REF_TO_TEXTURE);
		//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,     GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_EXT, l_ClampColor);
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	}

	check_framebuffer_complete();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::
init_fbo_mixed_samples(const FBOInit &fbo_init)
{
	cg_assert(fbo_init.depth_samples > fbo_init.color_samples);
	cg_assert(fbo_init.depth_samples > 1);
	cg_assert(fbo_init.color_samples > 0);
	cg_assert(!fbo_init.color_mip);

	int w = width, h = height;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo = gen_fbo());

	GLenum active_attachments[FBOInit::MAX_COLOR_ATTACHMENTS];

	int nc = fbo_init.color_samples;
	int nd = fbo_init.depth_samples;

	GLenum f;
	for(int i = 0; i < FBOInit::MAX_COLOR_ATTACHMENTS; i++) {
		if((f = fbo_init.color[i])) {
//			if(nc > 1) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
						GL_TEXTURE_2D_MULTISAMPLE,
						tex_color[i] = gen_texture_ms(f, w, h, nc), 0);
//			}
//			else {
//				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
//						tex_color[i] = gen_texture(f, w, h), 0);
//			}
			active_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		else {
			active_attachments[i] = GL_NONE;
		}
	}
	glDrawBuffers(FBOInit::MAX_COLOR_ATTACHMENTS, active_attachments);

	if((f = fbo_init.rb)) {
		GLenum attach = GL_DEPTH_ATTACHMENT;
		if(f == GL_DEPTH24_STENCIL8 || f == GL_DEPTH32F_STENCIL8) {
			attach = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attach, GL_RENDERBUFFER,
				rb = gen_renderbuffer_ms(f, w, h, nd));
	}

	if((f = fbo_init.depth)) {
		tex_depth = gen_texture(f, w, h);
#ifndef RENDER_UTILS_GLES
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, l_ClampColor);
		//glTextureParameteri (tex_depth, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		//glTextureParameteri (tex_depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,       GL_CLAMP_TO_BORDER);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,       GL_CLAMP_TO_BORDER);
#else
		float l_ClampColor[] = {0.0, 0.0, 0.0, 0.0};
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
		glBindTexture(GL_TEXTURE_2D, tex_depth);
		//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,     GL_COMPARE_REF_TO_TEXTURE);
		//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,     GL_LEQUAL);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,           GL_CLAMP_TO_BORDER_EXT);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_EXT, l_ClampColor);
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	}

#ifndef RENDER_UTILS_GLES
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1);
#endif

	check_framebuffer_complete();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::
attach_labels(const FBOInit &fbo_init)
{
	// not sure if supported, needs testing
}

FBO::
~FBO()
{
#ifndef NDEBUG
	cg_assert(!is_bound);
#endif
	for(int i = 0; i < FBOInit::MAX_COLOR_ATTACHMENTS; i++) {
		if(free_mask & (1 << i)) {
			glDeleteTextures(1, &tex_color[i]);
		}
	}
	if(free_mask & (1 << (FBOInit::MAX_COLOR_ATTACHMENTS))) {
		glDeleteRenderbuffers(1, &rb);
	}
	if(free_mask & (1 << (FBOInit::MAX_COLOR_ATTACHMENTS + 1))) {
		glDeleteTextures(1, &tex_depth);
	}
	glDeleteFramebuffers(1, &fbo);
}

FBO & FBO::
attach_texture(int idx_, int tex_)
{
	cg_assert(!is_bound);
	if(free_mask & (1 << idx_)) {
		glDeleteTextures(1, &tex_color[idx_]);
	}
	free_mask &= ~(1 << idx_);
	tex_color[idx_] = tex_;

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx_,
			GL_TEXTURE_2D, tex_, 0);

	GLenum active_attachments[FBOInit::MAX_COLOR_ATTACHMENTS] = {0};
	for(int i = 0; i < FBOInit::MAX_COLOR_ATTACHMENTS; i++) {
		active_attachments[i] = tex_color[i] ? GL_COLOR_ATTACHMENT0 + i : GL_NONE;
	}
	this->bind();
	glDrawBuffers(FBOInit::MAX_COLOR_ATTACHMENTS, active_attachments);
	this->unbind();

	return *this;
}

FBO & FBO::
attach_depth_tex(GLenum target, int tex_)
{
	cg_assert(!is_bound);
	if(free_mask & (1 << (FBOInit::MAX_COLOR_ATTACHMENTS + 1))) {
		glDeleteTextures(1, &tex_depth);
	}
	free_mask &= ~(1 << (FBOInit::MAX_COLOR_ATTACHMENTS + 1));
	tex_depth = tex_;

	this->bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, tex_depth, 0);
	this->unbind();

	return *this;
}

FBO & FBO::
attach_rb(GLenum type_, int rb_)
{
	cg_assert(!is_bound);
	if(free_mask & (1 << FBOInit::MAX_COLOR_ATTACHMENTS)) {
		glDeleteRenderbuffers(1, &this->rb);
	}
	free_mask &= ~(1 << FBOInit::MAX_COLOR_ATTACHMENTS);

	this->bind();
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, type_, GL_RENDERBUFFER, rb_);
	this->unbind();

	return *this;
}
