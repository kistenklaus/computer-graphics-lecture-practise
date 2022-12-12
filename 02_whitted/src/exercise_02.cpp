#include "cglib/rt/epsilon.h"
#include <cglib/rt/intersection.h>
#include <cglib/rt/intersection_tests.h>
#include <cglib/rt/light.h>
#include <cglib/rt/material.h>
#include <cglib/rt/ray.h>
#include <cglib/rt/raytracing_context.h>
#include <cglib/rt/render_data.h>
#include <cglib/rt/renderer.h>
#include <cglib/rt/scene.h>
#include <glm/geometric.hpp>

//#define USE_SOFT_SHADOWS
#define SOFT_SHADOW_KERNEL_RADIUS 2
#if SOFT_SHADOW_KERNEL_RADIUS == 1
#define SOFT_SHADOW_KERNEL {\
  {1/16.f, 2/16.f, 1/16.f}, \
  {2/16.f, 4/16.f, 2/16.f}, \
  {1/16.f, 2/16.f, 1/16.f}}
#elif SOFT_SHADOW_KERNEL_RADIUS == 2 
#define SOFT_SHADOW_KERNEL { \
  {1/273.f,  4/273.f,  7/273.f,  4/273.f, 1/273.f},\
  {4/273.f, 16/273.f, 26/273.f, 16/273.f, 4/273.f},\
  {7/273.f, 26/273.f, 41/273.f, 26/273.f, 7/273.f}, \
  {4/273.f, 16/273.f, 26/273.f, 16/273.f, 4/273.f},\
  {1/273.f,  4/273.f,  7/273.f,  4/273.f, 1/273.f}}
#endif
#define SOFT_SHADOW_LIGHT_SIZE 1.f

/*
 * TODO: implement a ray-sphere intersection test here.
 * The sphere is defined by its center and radius.
 *
 * Return true, if (and only if) the ray intersects the sphere.
 * In this case, also fill the parameter t with the distance such that
 *    ray_origin + t * ray_direction
 * is the intersection point.
 */
bool intersect_sphere(
    glm::vec3 const &ray_origin,    // starting point of the ray
    glm::vec3 const &ray_direction, // direction of the ray
    glm::vec3 const &center,        // position of the sphere
    float radius,                   // radius of the sphere
    float *t) // output parameter which contains distance to the hit point
{
  cg_assert(t);
  cg_assert(std::fabs(glm::length(ray_direction) - 1.f) < EPSILON);

  glm::vec3 o2c = ray_origin - center;
  const float r2 = radius * radius;

  const float a = glm::dot(ray_direction, ray_direction);
  const float b = 2 * glm::dot(ray_direction, o2c);
  const float c = glm::dot(o2c, o2c) - r2;

  const float disc = b * b - 4 * a * c;
  if (disc < 0) {
    return false; // no intersection.
  }

  const float sqrt_disc = glm::sqrt(disc);
  const float t1 = (-b + sqrt_disc) / (2 * a);
  const float t2 = (-b - sqrt_disc) / (2 * a);

  if (t1 < 0 && t2 < 0)
    return false; // sphere behind view.
  else if (t1 > 0 && t2 > 0)
    *t = std::min(t1, t2); // sphere infront of view.
  else
    *t = std::max(t1, t2); // view inside of sphere.
  return true;
}

/*
 * emission characteristic of a spotlight
 */
glm::vec3 SpotLight::getEmission(glm::vec3 const &omega // world space direction
) const {
  cg_assert(std::fabs(glm::length(omega) - 1.f) < EPSILON);
  return power * ((falloff+2) * std::pow(std::max(0.f, glm::dot(omega, direction)), falloff));
}

glm::vec3 evaluate_phong(
    RenderData &data,          // class containing raytracing information
    MaterialSample const &mat, // the material at position
    glm::vec3 const &P,        // world space position
    glm::vec3 const &N,        // normal at the position (already normalized)
    glm::vec3 const &V)        // view vector (already normalized)
{
  cg_assert(std::fabs(glm::length(N) - 1.f) < EPSILON);
  cg_assert(std::fabs(glm::length(V) - 1.f) < EPSILON);

  glm::vec3 contribution(0.f);

  // iterate over lights and sum up their contribution
  for (auto &light_uptr : data.context.get_active_scene()->lights) {
    // TODO: calculate the (normalized) direction to the light
    const Light *light = light_uptr.get();
    const glm::vec3 L = glm::normalize(light->getPosition() - P);
    const float L2 =
        glm::dot(light->getPosition() - P, light->getPosition() - P);
    const glm::vec3 emission = light->getEmission(-L);
    const float n_dot_l = glm::dot(N, L);

    float visibility = 1.f;
    if (data.context.params.shadows) {
#ifdef USE_SOFT_SHADOWS
      visibility = 0.f;
      const float offset = SOFT_SHADOW_LIGHT_SIZE / (2*SOFT_SHADOW_KERNEL_RADIUS +1);
      const int kernel_radius = SOFT_SHADOW_KERNEL_RADIUS;
      const float kernel[2*SOFT_SHADOW_KERNEL_RADIUS+1][2*SOFT_SHADOW_KERNEL_RADIUS+1] = SOFT_SHADOW_KERNEL;
      const glm::vec3 dright = glm::normalize(glm::cross(L, glm::vec3(0,1,0)));
      const glm::vec3 dup = glm::normalize(glm::cross(L, dright));
      for(int i=-kernel_radius;i<=kernel_radius;i++){
        for(int j=-kernel_radius;j<=kernel_radius;j++){
          visibility += kernel[kernel_radius + i][kernel_radius + j] *
            (visible(data, P, (i * offset) * dright + (j*offset)*dup + light->getPosition()) ? 1.f : 0.f);
        }
      }
#else
    visibility = visible(data, P, light->getPosition()) ? 1.f :  0.f;
#endif
    }

    glm::vec3 diffuse(0.f);
    if (data.context.params.diffuse) {
      diffuse = mat.k_d * std::max(0.f, n_dot_l);
    }

    glm::vec3 specular(0.f);
    if (data.context.params.specular) {
      glm::vec3 R = 2.f * N * n_dot_l - L;
      specular = mat.k_s * std::pow(std::max(0.f, glm::dot(R, V)), mat.n);
    }

    glm::vec3 ambient = data.context.params.ambient ? mat.k_a : glm::vec3(0.0f);

    contribution += (emission * visibility * (diffuse + specular) +
                     (ambient * light->getPower())) /
                    L2;
  }


  return contribution;
}

glm::vec3 evaluate_reflection(
    RenderData &data,   // class containing raytracing information
    int depth,          // the current recursion depth
    glm::vec3 const &P, // world space position
    glm::vec3 const &N, // normal at the position (already normalized)
    glm::vec3 const &V) // view vector (already normalized)
{
  glm::vec3 R = glm::normalize(reflect(V,N));
  glm::vec3 O = P + data.context.params.ray_epsilon * R;
  Ray ray(O,R);
  return trace_recursive(data, ray, depth+1);
}

glm::vec3 evaluate_transmission(
    RenderData &data,   // class containing raytracing information
    int depth,          // the current recursion depth
    glm::vec3 const &P, // world space position
    glm::vec3 const &N, // normal at the position (already normalized)
    glm::vec3 const &V, // view vector (already normalized)
    float eta)          // the relative refraction index
{
  glm::vec3 T;
  if(!refract(V, N, eta, &T)) return glm::vec3(0);
  T = glm::normalize(T);
  Ray ray(P + data.context.params.ray_epsilon * T, T);
  return trace_recursive(data, ray, depth+1);
}

glm::vec3 handle_transmissive_material_single_ior(
    RenderData &data,   // class containing raytracing information
    int depth,          // the current recursion depth
    glm::vec3 const &P, // world space position
    glm::vec3 const &N, // normal at the position (already normalized)
    glm::vec3 const &V, // view vector (already normalized)
    float eta)          // the relative refraction index
{
  if (data.context.params.fresnel) {
    // TODO: replace with proper fresnel handling.
    float F = fresnel(V, N, eta);
    return  F * evaluate_reflection(data, depth, P, N, V) 
      + (1-F) * evaluate_transmission(data, depth, P, N, V, eta) ;
  } else {
    // just regular transmission
    return evaluate_transmission(data, depth, P, N, V, eta);
  }
}

glm::vec3 handle_transmissive_material(
    RenderData &data,   // class containing raytracing information
    int depth,          // the current recursion depth
    glm::vec3 const &P, // world space position
    glm::vec3 const &N, // normal at the position (already normalized)
    glm::vec3 const &V, // view vector (already normalized)
    glm::vec3 const &eta_of_channel) // relative refraction index of red, green
                                     // and blue color channel
{
  if (data.context.params.dispersion &&
      !(eta_of_channel[0] == eta_of_channel[1] &&
        eta_of_channel[0] == eta_of_channel[2])) {
    // TODO: split ray into 3 rays (one for each color channel) and implement
    // dispersion here

    glm::vec3 contribution(0.f);
    glm::vec3 reflection = evaluate_reflection(data,depth, P,N,V);
    for(glm::length_t i=0;i<eta_of_channel.length();i++){
      float F = fresnel(V, N, eta_of_channel[i]);
      contribution[i] = F * reflection[i] + 
        (1-F) * evaluate_transmission(data, depth, P, N, V, eta_of_channel[i])[i];
    }
    return contribution;
  } else {
    // dont handle transmission, take average refraction index instead.
    const float eta =
        1.f / 3.f * (eta_of_channel[0] + eta_of_channel[1] + eta_of_channel[2]);
    return handle_transmissive_material_single_ior(data, depth, P, N, V, eta);
  }
  return glm::vec3(0.f);
}
// CG_REVISION 038eeebbaeab0d05d9997f0e5cb15ad0d3c889a7
