#include "cglib/rt/transform.h"
#include <cglib/core/thread_local_data.h>
#include <cglib/rt/intersection.h>
#include <cglib/rt/object.h>
#include <cglib/rt/ray.h>
#include <cglib/rt/raytracing_context.h>
#include <cglib/rt/render_data.h>
#include <cglib/rt/renderer.h>
#include <cglib/rt/texture.h>
#include <cglib/rt/texture_mapping.h>

#include <cglib/core/assert.h>
#include <cglib/core/glmstream.h>
#include <cglib/core/image.h>

#include <algorithm>
#include <glm/detail/type_mat.hpp>
#include <glm/matrix.hpp>
#include <memory>

// -----------------------------------------------------------------------------

/*
 * Evaluates a texture for the given uv-coordinate without filtering.
 *
 * This method transformes the uv-coordinate into a st-coordinate and
 * rounds down to integer pixel values.
 *
 * The parameter level in [0, mip_levels.size()-1] is the miplevel of
 * the texture that is to be evaluated.
 */
glm::vec4 ImageTexture::evaluate_nearest(int level, glm::vec2 const &uv) const {
  cg_assert(level >= 0 && level < static_cast<int>(mip_levels.size()));
  cg_assert(mip_levels[level]);

  // TODO: compute the st-coordinates for the given uv-coordinates and mipmap
  // level
  int s = mip_levels[level]->getWidth() * uv.x;
  int t = mip_levels[level]->getHeight() * uv.y;

  // get the value of pixel (s, t) of miplevel level
  return get_texel(level, s, t);
}

// -----------------------------------------------------------------------------

/*
 * Implement clamping here.
 *
 * The input "val" may be arbitrary, including negative and very large positive
 * values. The method shall always return a value in [0, size). Out-of-bounds
 * inputs must be clamped to the nearest boundary.
 */
int ImageTexture::wrap_clamp(int val, int size) {
  cg_assert(size > 0);
  return std::max(0, std::min(val, size - 1));
}

/*
 * Implement repeating here.
 *
 * The input "val" may be arbitrary, including negative and very large positive
 * values. The method shall always return a value in [0, size). Out-of-bounds
 * inputs must be mapped back into [0, size) so that the texture repeats
 * infinitely.
 */
int ImageTexture::wrap_repeat(int val, int size) {
  cg_assert(size > 0);
  return ((val % size) + size) % size;
}

// -----------------------------------------------------------------------------

/*
 * Implement bilinear filtering here.
 *
 * Use mip_levels[level] as the image to filter.
 * The input uv coordinates may be arbitrary, including values outside [0, 1).
 *
 * Callers of this method must guarantee that level is valid, and
 * that mip_levels[level] is properly initialized. Compare the assertions.
 *
 * The color of a texel is to be interpreted as the color at the texel center.
 */
glm::vec4 ImageTexture::evaluate_bilinear(int level,
                                          glm::vec2 const &uv) const {
  cg_assert(level >= 0 && level < static_cast<int>(mip_levels.size()));
  cg_assert(mip_levels[level]);

  const glm::vec2 st = uv * glm::vec2(mip_levels[level]->getWidth(),
                                      mip_levels[level]->getHeight());
  const glm::vec2 ist = glm::vec2((int)(st.x + 0.5f), (st.y + 0.5f));
  const float alpha = st.x - (ist.x - 0.5f);
  const float beta = st.y - (ist.y - 0.5f);
  const glm::vec4 A = alpha * get_texel(level, ist.x - 1, ist.y) +
                      (1.0f - alpha) * get_texel(level, ist.x, ist.y);
  const glm::vec4 B = alpha * get_texel(level, ist.x, ist.y - 1) +
                      (1.0f - alpha) * get_texel(level, ist.x - 1, ist.y - 1);
  return beta * A + (1.0f - beta) * B;
}

// -----------------------------------------------------------------------------

/*
 * This method creates a mipmap hierarchy for
 * the texture.
 *
 * This is done by iteratively reducing the
 * dimenison of a mipmap level and averaging
 * pixel values until the size of a mipmap
 * level is [1, 1].
 *
 * The original data of the texture is stored
 * in mip_levels[0].
 *
 * You can allocale memory for a new mipmap level
 * with dimensions (size_x, size_y) using
 *		mip_levels.emplace_back(new Image(size_x, size_y));
 */
void ImageTexture::create_mipmap() {
  /* this are the dimensions of the original texture/image */
  int size_x = mip_levels[0]->getWidth();
  int size_y = mip_levels[0]->getHeight();

  cg_assert("must be power of two" && !(size_x & (size_x - 1)));
  cg_assert("must be power of two" && !(size_y & (size_y - 1)));
  while (true) {
    if (size_x >= 2)
      size_x /= 2;
    if (size_y >= 2)
      size_y /= 2;
    std::shared_ptr<Image> prev_mipmap = mip_levels.back();
    std::shared_ptr<Image> next_mipmap =
        std::make_shared<Image>(size_x, size_y);
    for (int x = 0; x < size_x; x++) {
      for (int y = 0; y < size_y; y++) {
        glm::vec4 sum = glm::vec4(0);
        int count = 0;
        for (int dx = 0; dx < 2 && (2 * x + dx) < prev_mipmap->getWidth(); dx++) {
          for (int dy = 0; dy < 2 && (2 * y + dy) < prev_mipmap->getHeight(); dy++) {
            sum += prev_mipmap->getPixel(2 * x + dx, 2 * y + dy);
            count += 1;
          }
        }
        next_mipmap->setPixel(x, y, sum * (1.0f / count));
      }
    }
    mip_levels.push_back(next_mipmap);
    if (size_x == 1 && size_y == 1)
      break;
  }
}

/*
 * Compute the dimensions of the pixel footprint's AABB in uv-space.
 *
 * First intersect the four rays through the pixel corners with
 * the tangent plane at the given intersection.
 *
 * Then the given code computes uv-coordinates for these
 * intersection points.
 *
 * Finally use the uv-coordinates and compute the AABB in
 * uv-space.
 *
 * Return width (du) and height (dv) of the AABB.
 *
 */
glm::vec2 Object::compute_uv_aabb_size(const Ray rays[4],
                                       Intersection const &isect) {
  // TODO: compute intersection positions
  glm::vec3 intersection_positions[4] = {isect.position, isect.position,
                                         isect.position, isect.position};

  for (int i = 0; i < 4; ++i) {
    // todo: compute intersection positions using a ray->plane
    // intersection
  }

  // compute uv coordinates from intersection positions
  glm::vec2 intersection_uvs[4];
  get_intersection_uvs(intersection_positions, isect, intersection_uvs);

  // TODO: compute dudv = length of sides of AABB in uv space
  return glm::vec2(0.0);
}

/*
 * Implement trilinear filtering at a given uv-position.
 *
 * Transform the AABB dimensions dudv in st-space and
 * take the maximal coordinate as the 1D footprint size T.
 *
 * Determine mipmap levels i and i+1 such that
 *		texel_size(i) <= T <= texel_size(i+1)
 *
 *	Hint: use std::log2(T) for that.
 *
 *	Perform bilinear filtering in both mipmap levels and
 *	linearly interpolate the resulting values.
 *
 */
glm::vec4 ImageTexture::evaluate_trilinear(glm::vec2 const &uv,
                                           glm::vec2 const &dudv) const {
  return glm::vec4(0.f);
}

/*
 * Transform the given direction d using the matrix transform.
 *
 * The output direction must be normalized, even if d is not.
 */
glm::vec3 transform_direction(glm::mat4 const &transform, glm::vec3 const &d) {
  return glm::normalize(glm::vec3(transform * glm::vec4(d, 0.f)));
}

/*
 * Transform the given position p using the matrix transform.
 */
glm::vec3 transform_position(glm::mat4 const &transform, glm::vec3 const &p) {
  glm::vec4 v = transform * glm::vec4(p, 1.f);
  return glm::vec3(v) * (1.0f / v.w);
}

/*
 * Intersect with the ray, but do so in object space.
 *
 * First, transform ray into object space. Use the methods you have
 * implemented for this.
 * Then, intersect the object with the transformed ray.
 * Finally, make sMan kann Strahlschnitte in Objektkoordinaten beispielsweise
einsetzen, um mit dem Strahl- schnitt f  ̈ur Kugeln auch Ellipsoide darzustellen.
Ein Beispiel k  ̈onnen Sie in der ”Go-Brett”- Szene sehen (Abbildung 2a). Die
Spielsteine sind mit transformierten Kugeln modellierture you transform the
intersection back into world space.
 *
 * isect is guaranteed to be a valid pointer.
 * The method shall return true if an intersection was found and false
 * otherwise.
 *
 * isect must be filled properly if an intersection was found.
 */
bool Object::intersect(Ray const &ray, Intersection *isect) const {
  cg_assert(isect);

  if (RaytracingContext::get_active()->params.transform_objects) {
    Ray transformedRay = transform_ray(ray, transform_world_to_object);
    if (!geo->intersect(transformedRay, isect))
      return false;
    *isect = transform_intersection(*isect, transform_object_to_world,
                                    transform_object_to_world_normal);
    isect->t = glm::length(ray.origin - isect->position);
    return true;
  }
  return geo->intersect(ray, isect);
}

/*
 * Transform a direction from tangent space to object space.
 *
 * Tangent space is a right-handed coordinate system where
 * the tangent is your thumb, the normal is the index finger, and
 * the bitangent is the middle finger.
 *
 * normal, tangent, and bitangent are given in object space.
 * Build a matrix that rotates d from tangent space into object space.
 * Then, transform d with this matrix to obtain the result.
 *
 * You may assume that normal, tangent, and bitangent are normalized
 * to length 1.
 *
 * The output vector must be normalized to length 1, even if d is not.
 */
glm::vec3 transform_direction_to_object_space(glm::vec3 const &d,
                                              glm::vec3 const &normal,
                                              glm::vec3 const &tangent,
                                              glm::vec3 const &bitangent) {
  cg_assert(std::fabs(glm::length(normal) - 1.0f) < 1e-4f);
  cg_assert(std::fabs(glm::length(tangent) - 1.0f) < 1e-4f);
  cg_assert(std::fabs(glm::length(bitangent) - 1.0f) < 1e-4f);

  glm::mat3 transform = glm::mat3(tangent, normal, bitangent);
  return glm::normalize(transform * d);
}

// -----------------------------------------------------------------------------

// Optional bonus assignment! The fourier scene uses evaluate_bilinear(..)
// for displaying amplitude, phase and result images so you need a working
// solution for the bilinear filtering!
//
// Spectrum and reconstruction are 2D memory blocks of the same size [sx, sy]
// in row major layout, i.e one row of size sx after the other.
//
// Spectrum contains the complex fourier-coefficients \hat{x}_{kl}.
//
// Reconstruct the original grayscale image using fourier transformation!
void DiscreteFourier2D::reconstruct(int M, int N,
                                    std::complex<float> const *spectrum,
                                    std::complex<float> *reconstruction) {}
// CG_REVISION 038eeebbaeab0d05d9997f0e5cb15ad0d3c889a7
