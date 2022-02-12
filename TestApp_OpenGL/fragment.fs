#version 330 core

in vec3 fragPosWorld;

in vec3 worldNormal;

out vec4 FragColor;

uniform vec3 eyeWorldPos;

struct Material {
    vec4 Diffuse;
    vec4 Specular;
    float Shininess;
}; 

uniform Material material;

struct DirectionalLight
{
	vec3 Direction;
	vec4 Diffuse;
	vec4 Specular;
};

struct SceneLights
{
	// ambientLight
	vec4 Ambient;

	// directionalLight
	DirectionalLight Directional;
};

uniform SceneLights lights;

struct CommonLightData
{
	vec4 baseColor;
	vec4 baseSpecular;
	vec3 eyeDir;
	vec3 eyePos;
	vec3 worldNormal;
};


vec4 computeLight_Directional(DirectionalLight d, CommonLightData data)
{
	float diffuseIntensity=max(0.0f, dot(-d.Direction, data.worldNormal));
	vec4 diffuseColor=vec4(d.Diffuse.rgb*d.Diffuse.a, 1.0f)*data.baseColor*diffuseIntensity;

	float specularIntensity=pow(max(0.0f, dot(data.eyeDir, reflect(normalize(d.Direction), data.worldNormal))), material.Shininess);
	vec4 specularColor=vec4(d.Specular.rgb*d.Specular.a, 1.0f)*data.baseSpecular*specularIntensity;

	return  diffuseColor+specularColor;
}

vec4 computeLight_Ambient(vec4 a, CommonLightData data)
{
	vec4 ambientColor=vec4(a.rgb*a.a, 1.0f)*data.baseColor;
	return ambientColor;

}


void main()

{
	
    vec4 baseColor=vec4(material.Diffuse.rgb, 1.0);

    vec4 baseSpecular=vec4(material.Specular.rgb, 1.0);
    
    vec3 viewDir=normalize(eyeWorldPos - fragPosWorld);

	CommonLightData commonData=CommonLightData(baseColor, baseSpecular, viewDir, eyeWorldPos, normalize(worldNormal));

	vec4 finalColor=vec4(0.0f, 0.0f, 0.0f, 0.0f);

	// Ambient
	finalColor+=computeLight_Ambient(lights.Ambient, commonData);
	
	// Directional
	finalColor+=computeLight_Directional(lights.Directional, commonData);
	
    FragColor =  finalColor;
}