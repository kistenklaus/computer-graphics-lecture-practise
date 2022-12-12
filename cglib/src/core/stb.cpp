#include <cglib/core/assert.h>
#define STBI_ASSERT(x) cg_assert(x)

#define STB_IMAGE_IMPLEMENTATION
#include <cglib/core/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <cglib/core/stb_image_write.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <cglib/core/stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <cglib/core/stb_truetype.h>
