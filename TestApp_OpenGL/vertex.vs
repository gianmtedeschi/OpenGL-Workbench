#version 330 core

// vertex attribs
layout (location = 0) in vec3 position;

layout (location = 1) in vec3 normal;


// uniforms
uniform mat4 model;

uniform mat4 normalMatrix;

uniform mat4 view;

uniform mat4 proj;

// out
out vec3 worldNormal;

out vec3 fragPosWorld;

void main()

{

	fragPosWorld= (model * vec4(position.x , position.y, position.z, 1.0)).xyz;

   gl_Position = proj * view * model * vec4(position.x , position.y, position.z, 1.0);

   worldNormal=normalize((normalMatrix * vec4(normal, 0.0f)).xyz);

}