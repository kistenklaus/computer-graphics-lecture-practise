#include <cglib/colors/cmf.h>
#include <cglib/colors/convert.h>
#include <cglib/colors/exercise.h>

#include <cglib/core/glheaders.h>
#include <cglib/core/glmstream.h>

#include <algorithm>
#include <cglib/core/assert.h>
#include <iostream>

#define SORT_BY_X_WHEN_INTEGRATING false

/*
 * Draw the given vertices directly as GL_TRIANGLES.
 * For each vertex, also set the corresponding color.
 */
void draw_triangles(std::vector<glm::vec3> const &vertices,
                    std::vector<glm::vec3> const &colors) {
  cg_assert(vertices.size() == colors.size());
  cg_assert(vertices.size() % 3 == 0);

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vertices.data());
  glColorPointer(3, GL_FLOAT, 0, colors.data());
  glDrawArrays(GL_TRIANGLES, 0, vertices.size());

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}

/*
 * Generate a regular grid of resolution N*N (2*N*N triangles) in the xy-plane
 * (z=0). Store the grid in vertex-index form.
 *
 * The vertices of the triangles should be in counter clock-wise order.
 *
 * The grid must fill exactly the square [0, 1]x[0, 1], and must
 * be generated as an Indexed Face Set (Shared Vertex representation).
 *
 * The first vertex should be at position (0,0,0) and the last
 * vertex at position (1,1,0)
 *
 * An example for N = 3:
 *
 *   ^
 *   |  ----------
 *   |  |\ |\ |\ |
 *   |  | \| \| \|
 *   |  ----------
 *   |  |\ |\ |\ |
 * y |  | \| \| \|
 *   |  ----------
 *   |  |\ |\ |\ |
 *   |  | \| \| \|
 *   |  ----------
 *   |
 *   |-------------->
 *          x
 *
 */
void generate_grid(std::uint32_t N, std::vector<glm::vec3> *vertices,
                   std::vector<glm::uvec3> *indices) {
  cg_assert(N >= 1);
  cg_assert(vertices);
  cg_assert(indices);

  vertices->clear();
  indices->clear();

  for (std::uint32_t y = 0; y <= N; y++) {
    for (std::uint32_t x = 0; x <= N; x++) {
      vertices->emplace_back(x / (float)N, y / (float)N, 0);
    }
  }
  uint32_t M = N + 1;
  for (std::uint32_t v = 0; v < N; v++) {
    for (std::uint32_t u = 0; u < N; u++) {
      const std::uint32_t i = u + v * M;
      indices->emplace_back(i, i + M, i + M + 1);
      indices->emplace_back(i + M + 1, i + 1, i);
    }
  }
}

/*
 * Draw the given vertices as indexed GL_TRIANGLES using glDrawElements.
 * For each vertex, also set the corresponding color.
 *
 * Don't forget to enable the correct client states. After drawing
 * the triangles, you need to disable the client states again.
 */
void draw_indexed_triangles(std::vector<glm::vec3> const &vertices,
                            std::vector<glm::vec3> const &colors,
                            std::vector<glm::uvec3> const &indices) {
  cg_assert(vertices.size() == colors.size());

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vertices.data());
  glColorPointer(3, GL_FLOAT, 0, colors.data());

  glDrawElements(GL_TRIANGLES, indices.size() * 3, GL_UNSIGNED_INT,
                 indices.data());

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}

/*
 * Generate a triangle strip with N segments (2*N triangles)
 * in the xy plane (z=0).
 *
 * The vertices of the triangles should be in counter clock-wise order.
 *
 * The triangle strip must fill exactly the square [0, 1]x[0, 1].
 *
 * The first vertex should be at position (0,1,0) and the last
 * vertex at position (1,0,0)
 *
 * An example for N = 3:
 *
 *   ^
 *   |  ----------
 *   |  | /| /| /|
 * y |  |/ |/ |/ |
 *   |  ----------
 *   |
 *   |-------------->
 *           x
 *
 */
void generate_strip(std::uint32_t N, std::vector<glm::vec3> *vertices) {
  cg_assert(N >= 1);
  cg_assert(vertices);

  vertices->clear();

  std::uint32_t M = N - 1;
  for (std::uint32_t n = 0; n <= N; n++) {
    vertices->emplace_back(n / (float)(N), 1, 0);
    vertices->emplace_back(n / (float)(N), 0, 0);
  }
}

/*
 * Draw the given vertices as a triangle strip.
 * For each vertex, also set the corresponding color.
 */
void draw_triangle_strip(std::vector<glm::vec3> const &vertices,
                         std::vector<glm::vec3> const &colors) {
  cg_assert(vertices.size() == colors.size());

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vertices.data());
  glColorPointer(3, GL_FLOAT, 0, colors.data());
  glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}

/*
 * Integrate the given piecewise linear function
 * using trapezoidal integration.
 *
 * The function is given at points
 *     x[0], ..., x[N]
 * and its corresponding values are
 *     y[0], ..., y[N]
 */
float integrate_trapezoidal_student(std::vector<float> const &x,
                                    std::vector<float> const &y) {
  cg_assert(x.size() == y.size());
  cg_assert(x.size() > 1);
  const size_t n = x.size();
  float sumedTrapeze = 0;

#if SORT_BY_X_WHEN_INTEGRATING 

  std::vector<std::pair<float, float>> f;
  for (std::uint32_t i = 0; i < n; i++) {
    f.emplace_back(x[i], y[i]);
  }
  std::sort(f.begin(), f.end()); // wow this is stupid!

  for (size_t i = 0; i < n - 1; i++) {
    sumedTrapeze += (f[i].second + f[i + 1].second) * 0.5f;
  }
  sumedTrapeze *= f.back().first - f.front().first;

#else

  for (size_t i = 0; i < n - 1; i++) {
    sumedTrapeze += (y[i] + y[i + 1]) * 0.5f;
  }
  sumedTrapeze *= x.back() - x.front();

#endif

  return sumedTrapeze;
}

/*
 * Convert the given spectrum to RGB using your
 * implementation of integrate_trapezoidal(...)
 *
 * The color matching functions and the wavelengths
 * for which they are given can be found in
 *     cglib/colors/cmf.h
 * and
 *     cglib/src/colors/cmf.cpp
 *
 * The wavelengths corresponding to the spectral values
 * given in spectrum are defined in cmf::wavelengths
 */
glm::vec3 spectrum_to_rgb(std::vector<float> const &spectrum) {
  cg_assert(spectrum.size() == cmf::wavelengths.size());
  cg_assert(spectrum.size() == cmf::x.size());
  cg_assert(spectrum.size() == cmf::y.size());
  cg_assert(spectrum.size() == cmf::z.size());
  const size_t n = spectrum.size();

  std::vector<float> spec_x = std::vector<float>(n);
  std::vector<float> spec_y = std::vector<float>(n);
  std::vector<float> spec_z = std::vector<float>(n);

  for (size_t i = 0; i < n; i++) {
    spec_x[i] = spectrum[i] * cmf::x[i];
    spec_y[i] = spectrum[i] * cmf::y[i];
    spec_z[i] = spectrum[i] * cmf::z[i];
  }

  const float x = integrate_trapezoidal(cmf::wavelengths, spec_x);
  const float y = integrate_trapezoidal(cmf::wavelengths, spec_y);
  const float z = integrate_trapezoidal(cmf::wavelengths, spec_z);

  return convert::xyz_to_rgb(glm::vec3(x, y, z));
}
// CG_REVISION 038eeebbaeab0d05d9997f0e5cb15ad0d3c889a7
