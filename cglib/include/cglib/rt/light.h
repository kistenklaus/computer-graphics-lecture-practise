#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <iostream>

// This is the default light implementation
// which represents a point light and emitts
// light equally in all directions
class Light
{
public:
    Light(glm::vec3 const& position_, glm::vec3 const& power_) :
        position(position_), power(power_)
    {
    }

	glm::vec3 getPosition() const { return position; }

	// evaluate the emission of the light in direction omega
	// (pointing away from the light).
	virtual glm::vec3 getEmission(glm::vec3 const& omega) const { return power; }
	virtual glm::vec3 getPower() const { return power; }

protected:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 power = glm::vec3(0.0f);
};

class SpotLight : public Light
{
public:
    SpotLight(glm::vec3 const& position_, glm::vec3 const& power_, glm::vec3 const& direction_, float falloff_) :
        Light(position_, power_),
		direction(direction_),
		falloff(falloff_)
    {
    }

	// emission characteristic of a spotlight
	// TODO: implement this in exercise.cpp
	glm::vec3 getEmission(glm::vec3 const& omega) const;

private:
	glm::vec3 direction = glm::vec3(0.0f);
	float falloff;
};

