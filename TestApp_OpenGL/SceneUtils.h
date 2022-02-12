#ifndef SCENEUTILS_H
#define SCENEUTILS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct PointLight
{
	glm::vec3 Position;

	// Alpha is intensity
	glm::vec4 Diffuse;
	glm::vec4 Specular;

	// Faloff
	float Constant = 1.0;
	float Linear;
	float Quadratic;
};

struct DirectionalLight
{
	glm::vec3 Direction;

	// Needed for shadowMap
	glm::vec3 Position;

	// Alpha is intensity
	glm::vec4 Diffuse;
	glm::vec4 Specular;

	// ShadowData
	unsigned int ShadowMapId;
	glm::mat4 LightSpaceMatrix;
	float Bias=0.001f;
	float SlopeBias=0.025f;
	float Softness = 0.025;
};

struct AmbientLight
{
	glm::vec4 Ambient;

	float aoStrength;
	float aoRadius;
	int aoSamples;
	int aoSteps;
	int aoBlurAmount;
	// SSAO
	unsigned int AoMapId;
};

struct Spotlight
{
	glm::vec3 Position;
	glm::vec3 Direction;

	// Alpha is intensity
	glm::vec4 Diffuse;
	glm::vec4 Specular;

	// Cone
	float InnerRadius;
	float OuterRadius;
};

struct SceneLights
{
	// ambientLight
	AmbientLight Ambient;

	// directionalLight
	DirectionalLight Directional;
};

struct DrawParams
{
	bool doShadows;
};

struct SceneParams
{
	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	SceneLights sceneLights;
	DrawParams drawParams;
};
struct Texture
{
	unsigned char* data;
	int width;
	int height;
};

struct Material
{
	glm::vec4 Diffuse;
	glm::vec4 Specular;

	float Shininess;
};

struct MaterialsCollection
{
	static const Material ShinyRed;
	static const Material PlasticGreen;
	static const Material Copper;
	static const Material PureWhite;
	static const Material MatteGray;
};

const Material MaterialsCollection::ShinyRed = Material{ glm::vec4(1, 0, 0, 1), glm::vec4(1, 1, 1, 1), 256 };
const Material MaterialsCollection::PlasticGreen = Material{ glm::vec4(0, 0.8, 0, 1), glm::vec4(0.8, 0.8, 0.8, 1), 32 };
const Material MaterialsCollection::Copper = Material{ glm::vec4(0.8, 0.3, 0, 1), glm::vec4(0.8, 0.3, 0, 1), 128 };
const Material MaterialsCollection::PureWhite = Material{ glm::vec4(0.9, 0.9, 0.9, 1), glm::vec4(0.8, 0.3, 0, 1), 128 };
const Material MaterialsCollection::MatteGray = Material{ glm::vec4(0.5, 0.5, 0.5, 1), glm::vec4(0.5, 0.5, 0.5, 1), 64 };

#endif