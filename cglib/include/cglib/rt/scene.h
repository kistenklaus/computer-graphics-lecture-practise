#pragma once

#include <cglib/rt/texture.h>

#include <vector>
#include <memory>

class Camera;
class Light;
class AreaLight;
class Object;
class RaytracingParameters;
class TriangleSoup;

#define SCENE_NAME(a) const char *get_name() override { return #a; } \
	static const char *get_name_static() { return #a; }

/*
 * Scene.
 *
 * This is the all the stuff that will be raytraced (camera, objects, materials, ...)
 * -- Use init_context to initially create objects and load textures. 
 *    This function will be called on startup and when parameters changed
 * -- Use init_camera to set up the camera.
 *
 */
class Scene
{
public:
	std::shared_ptr<Camera> camera;
	std::vector<std::unique_ptr<Light>> lights;
	std::vector<std::unique_ptr<Object>> objects;
	TextureContainer textures;
	ImageTexture* env_map = nullptr;

    virtual ~Scene();

	virtual void init_scene(RaytracingParameters const& params) {} 
    virtual void refresh_scene(RaytracingParameters const& params) {}
	virtual void init_camera(RaytracingParameters& params) {}
	virtual void set_active_camera();

	virtual const char *get_name() { return "unknown"; }
};


class TextureScene : public Scene
{
public:
	SCENE_NAME(TextureScene)
    TextureScene(RaytracingParameters& params);

	void init_scene(RaytracingParameters const& params);
    void refresh_scene(RaytracingParameters const& params);
	void init_camera(RaytracingParameters& params);
};

class TexturedSphereScene : public Scene
{
public:
	SCENE_NAME(TextureSphereScene)
    TexturedSphereScene(RaytracingParameters& params);

	void init_scene(RaytracingParameters const& params);
    void refresh_scene(RaytracingParameters const& params);
	void init_camera(RaytracingParameters& params);
};

class AliasingPlaneScene : public Scene
{
public:
	SCENE_NAME(AliasingPlane)
    AliasingPlaneScene(RaytracingParameters& params);

	void init_scene(RaytracingParameters const& params);
    void refresh_scene(RaytracingParameters const& params);
	void init_camera(RaytracingParameters& params);
};

class PoolTableScene : public Scene
{
public:
	SCENE_NAME(PoolTable)
	PoolTableScene(RaytracingParameters& params);

	void init_scene(RaytracingParameters const& params);
    void refresh_scene(RaytracingParameters const& params);
	void init_camera(RaytracingParameters& params);
};

class GoBoardScene : public Scene
{
public:
	SCENE_NAME(GoBoard)
	GoBoardScene(RaytracingParameters& params);

	void init_scene(RaytracingParameters const& params);
    void refresh_scene(RaytracingParameters const& params);
	void init_camera(RaytracingParameters& params);
};


class FourierScene : public Scene
{
public:
	SCENE_NAME(Fourier)
    FourierScene(RaytracingParameters& params);

	void init_scene(RaytracingParameters const& params);
    void refresh_scene(RaytracingParameters const& params);
	void create_texture_quad(std::shared_ptr<ImageTexture> const& texture);
};

