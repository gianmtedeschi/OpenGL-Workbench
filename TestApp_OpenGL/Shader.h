#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>


namespace VertexSource_Geometry
{
    const std::string EXP_VERTEX =
        R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal;
    uniform mat4 model;
    uniform mat4 normalMatrix;
    uniform mat4 view;
    uniform mat4 proj;
    out vec3 worldNormal;
    out vec3 fragPosWorld;
    //[DEFS_SHADOWS]
    void main()
    {
        fragPosWorld = (model * vec4(position.x, position.y, position.z, 1.0)).xyz;
        gl_Position = proj * view * model * vec4(position.x, position.y, position.z, 1.0);
        worldNormal = normalize((normalMatrix * vec4(normal, 0.0f)).xyz);
        //[CALC_SHADOWS]
    }
    )";

    const std::string DEFS_SHADOWS =
        R"(
    out vec4 posLightSpace;
    uniform mat4 LightSpaceMatrix;
)";

    const std::string CALC_SHADOWS =
        R"(
    posLightSpace = LightSpaceMatrix * model * vec4(position.x, position.y, position.z, 1.0);
)";

    static std::map<std::string, std::string> Expansions = {

        { "DEFS_SHADOWS",  VertexSource_Geometry::DEFS_SHADOWS     },
        { "CALC_SHADOWS",  VertexSource_Geometry::CALC_SHADOWS     },
    };

    std::string Expand(std::string source, std::vector<std::string> expStrings)
    {
        std::string result = source;

        for (int i = 0; i < expStrings.size(); i++)
        {
            std::string token = "//[" + expStrings[i] + "]";
            size_t start_pos = result.find(token);
            std::string expansion = VertexSource_Geometry::Expansions[expStrings[i]];
            result.replace(start_pos, token.length(), expansion);
        }

        return result;
    }
}

namespace FragmentSource_Geometry
{
    
    const std::string DEFS_MATERIAL =
        R"(
    struct Material {
        vec4 Diffuse;
        vec4 Specular;
        float Shininess;
    };

    uniform Material material;
)";
    const std::string DEFS_SSAO =
        R"(
    uniform sampler2D aoMap;
    uniform float aoStrength;
)";

    const std::string DEFS_SHADOWS =
        R"(

    #define BLOCKER_SEARCH_SAMPLES 16
    #define PCF_SAMPLES 16
    #define PI 3.14159265358979323846
    in vec4 posLightSpace;
    uniform sampler2D shadowMap;
    uniform float bias;
    uniform float slopeBias;
    uniform float softness;  

    vec2 poissonDisk[16] = vec2[](
     vec2( -0.94201624, -0.39906216 ),
     vec2( 0.94558609, -0.76890725 ),
     vec2( -0.094184101, -0.92938870 ),
     vec2( 0.34495938, 0.29387760 ),
     vec2( -0.91588581, 0.45771432 ),
     vec2( -0.81544232, -0.87912464 ),
     vec2( -0.38277543, 0.27676845 ),
     vec2( 0.97484398, 0.75648379 ),
     vec2( 0.44323325, -0.97511554 ),
     vec2( 0.53742981, -0.47373420 ),
     vec2( -0.26496911, -0.41893023 ),
     vec2( 0.79197514, 0.19090188 ),
     vec2( -0.24188840, 0.99706507 ),
     vec2( -0.81409955, 0.91437590 ),
     vec2( 0.19984126, 0.78641367 ),
     vec2( 0.14383161, -0.14100790 )
    ); 

    vec2 RandomDirection(int i)
    {
        return poissonDisk[i];
    }

    float SearchWidth(float uvLightSize, float receiverDistance)
    {
	    return uvLightSize * (receiverDistance - NEAR) / receiverDistance;
    }

    float FindBlockerDistance_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize)
    {
	    int blockers = 0;
	    float avgBlockerDistance = 0;
	    float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
	    for (int i = 0; i < BLOCKER_SEARCH_SAMPLES; i++)
	    {
		    float z = texture(shadowMap, shadowCoords.xy + RandomDirection(i) * searchWidth).r;
		    if (z + max(bias, slopeBias*(1-abs(dot(worldNormal, lights.Directional.Direction)))) < shadowCoords.z )
		    {
			    blockers++;
			    avgBlockerDistance += z;
		    }
	    }
	    if (blockers > 0)
		    return avgBlockerDistance / blockers;
	    else
		    return -1;
    }

    float PCF_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvRadius)
    {
        
	    float sum = 0;

	    for (int i = 0; i < PCF_SAMPLES; i++)
	    {
		    float closestDepth=texture(shadowMap, shadowCoords.xy + RandomDirection(i)*uvRadius).r;
        
            sum+= (closestDepth + max(bias, slopeBias*(1-abs(dot(worldNormal, lights.Directional.Direction))))) < shadowCoords.z 
                    ? 0.0 : 1.0;
	    }
	    return sum / PCF_SAMPLES;
    }

    float PCSS_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize)
    {
	    // blocker search
	    float blockerDistance = FindBlockerDistance_DirectionalLight(shadowCoords, shadowMap, uvLightSize);
	    if (blockerDistance == -1)
		    return 1.0;		

	    // penumbra estimation
	    float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	    // percentage-close filtering
	    float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z;
	    return PCF_DirectionalLight(shadowCoords, shadowMap, uvRadius);
    }

    float ShadowCalculation(vec4 fragPosLightSpace)
    {
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
        projCoords=projCoords*0.5 + vec3(0.5, 0.5, 0.5);

        return PCSS_DirectionalLight(projCoords, shadowMap, softness);
        
    }
    
)";

    const std::string DEFS_LIGHTS =
        R"(
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

    uniform SceneLights lights;
    uniform vec3 eyeWorldPos;

)";

    const std::string CALC_LIT_MAT =
        R"(
        vec4 baseColor=vec4(material.Diffuse.rgb, 1.0);
        vec4 baseSpecular=vec4(material.Specular.rgb, 1.0);
        vec3 viewDir=normalize(eyeWorldPos - fragPosWorld);
	    CommonLightData commonData=CommonLightData(baseColor, baseSpecular, viewDir, eyeWorldPos, normalize(worldNormal));

	    // Ambient
	    ambient=computeLight_Ambient(lights.Ambient, commonData);

	    // Directional
	    directional=computeLight_Directional(lights.Directional, commonData);
)";

    const std::string CALC_UNLIT_MAT =
        R"(
        vec4 baseColor=vec4(material.Diffuse.rgb, 1.0);
        finalColor=baseColor;
)";

    const std::string DEFS_NORMALS =
        R"(
        uniform mat4 view;
)";

    const std::string CALC_NORMALS =
        R"(
        vec4 baseColor=vec4(normalize((view * vec4(worldNormal, 0.0)).xyz), 1.0);
        finalColor=baseColor;
)";

    const std::string CALC_SHADOWS =
        R"(
        directional*= ShadowCalculation(posLightSpace);
)";

    const std::string CALC_SSAO =
        R"(
        vec2 aoMapSize=textureSize(aoMap, 0);
        float aoFac = (1.0-texture(aoMap, gl_FragCoord.xy/aoMapSize).r) * aoStrength;
        ambient*= 1.0-aoFac;
)";

    const std::string EXP_FRAGMENT =
    R"(
    #version 330 core
    in vec3 fragPosWorld;
    in vec3 worldNormal;
    out vec4 FragColor;

    #define NEAR 0.1
    
    //[DEFS_MATERIAL] 
    //[DEFS_LIGHTS]
    //[DEFS_SHADOWS]
    //[DEFS_NORMALS]
    //[DEFS_SSAO]
    void main()
    {
        vec4 finalColor=vec4(0.0f, 0.0f, 0.0f, 0.0f);
        vec4 ambient=vec4(0.0f, 0.0f, 0.0f, 0.0f);
        vec4 directional=vec4(0.0f, 0.0f, 0.0f, 0.0f);

        //[CALC_LIT_MAT]
        //[CALC_UNLIT_MAT]	
        //[CALC_SHADOWS]
        //[CALC_NORMALS]
        //[CALC_SSAO]
        FragColor = finalColor + ambient + directional;
    }
    )";

    static std::map<std::string, std::string> Expansions = {

        { "DEFS_MATERIAL",  FragmentSource_Geometry::DEFS_MATERIAL     },
        { "DEFS_LIGHTS",    FragmentSource_Geometry::DEFS_LIGHTS       },
        { "DEFS_SHADOWS",   FragmentSource_Geometry::DEFS_SHADOWS      },
        { "DEFS_SSAO",      FragmentSource_Geometry::DEFS_SSAO         },
        { "CALC_LIT_MAT",   FragmentSource_Geometry::CALC_LIT_MAT      },
        { "CALC_UNLIT_MAT", FragmentSource_Geometry::CALC_UNLIT_MAT    },
        { "CALC_SHADOWS",   FragmentSource_Geometry::CALC_SHADOWS      },
        { "CALC_SSAO",      FragmentSource_Geometry::CALC_SSAO         },
        { "DEFS_NORMALS",   FragmentSource_Geometry::DEFS_NORMALS      },
        { "CALC_NORMALS",   FragmentSource_Geometry::CALC_NORMALS      }
    };

    std::string Expand(std::string source, std::vector<std::string> expStrings)
    {
        std::string result = source;

        for (int i = 0; i < expStrings.size(); i++)
        {
            std::string token = "//[" + expStrings[i] + "]";
            size_t start_pos = result.find(token);
            std::string expansion = FragmentSource_Geometry::Expansions[expStrings[i]];
            result.replace(start_pos, token.length(), expansion);
        }

        return result;
    }
}

namespace VertexSource_PostProcessing
{
    const std::string EXP_VERTEX =
        R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    
    void main()
    {
        gl_Position = vec4(position.xyz, 1.0);
    }
    )";

    static std::map<std::string, std::string> Expansions = {


    };

    std::string Expand(std::string source, std::vector<std::string> expStrings)
    {
        std::string result = source;

        for (int i = 0; i < expStrings.size(); i++)
        {
            std::string token = "//[" + expStrings[i] + "]";
            size_t start_pos = result.find(token);
            std::string expansion = VertexSource_PostProcessing::Expansions[expStrings[i]];
            result.replace(start_pos, token.length(), expansion);
        }

        return result;
    }
}
namespace FragmentSource_PostProcessing
{

    const std::string DEFS_BLUR =
        R"(
    uniform sampler2D u_texture;
    uniform int u_size;
)";
    const std::string DEFS_GAUSSIAN_BLUR =
        R"(
    uniform sampler2D u_texture;
    uniform usampler2D u_weights_texture;
    uniform int u_radius;
    uniform bool u_hor;
)";

    const std::string DEFS_AO =
        R"(
    uniform sampler2D u_depthTexture;
    uniform sampler2D u_viewPosTexture;
    uniform sampler2D u_viewNormalsTexture;
    uniform sampler2D u_rotVecs;
    uniform vec3[64]  u_rays;
    uniform float u_radius;
    uniform int u_numSamples;
    uniform int u_numSteps;
    #define NOISE_SIZE 4

    uniform float u_near;
    uniform float u_far;
    uniform mat4  u_proj;

    #define SQRT_TWO 1.41421356237;
    #define PI 3.14159265358979323846
    #define COS_PI4 0.70710678118
    vec2 offset[8] = vec2[](
        vec2(-COS_PI4,-COS_PI4),
        vec2(-1.0, 0.0),
        vec2(-COS_PI4, COS_PI4),
        vec2( 0.0, 1.0),
        vec2( COS_PI4, COS_PI4),
        vec2( 1.0, 0.0),
        vec2( COS_PI4,-COS_PI4),
        vec2( 0.0,-1.0)
        ); 

    float LinearDepth(float depthValue, float near, float far)
{
    //http://www.songho.ca/opengl/gl_projectionmatrix.html

    float res = (2.0 * near) / ( far + near - depthValue * ( far - near));
   
    return res;
}

vec2 TexCoords(vec3 viewCoords, mat4 projMatrix)
{
        vec4 samplePoint_ndc = projMatrix * vec4(viewCoords, 1.0);
        samplePoint_ndc.xyz /= samplePoint_ndc.w;
        samplePoint_ndc.xyz = samplePoint_ndc.xyz * 0.5 + 0.5;
        return samplePoint_ndc.xy;
}

vec3 EyeCoords(float xNdc, float yNdc, float zEye)
{
    //http://www.songho.ca/opengl/gl_projectionmatrix.html

    float xC = xNdc * zEye;
    float yC = yNdc * zEye;
    float xEye= (xC - u_proj[0][2] * zEye) / u_proj[0][0];
    float yEye= (yC - u_proj[1][2] * zEye) / u_proj[1][1];

    return vec3(xEye, yEye, zEye);
}

vec3 EyeNormal(vec2 uvCoords, vec3 eyePos)
    {
        vec2 texSize=textureSize(u_viewPosTexture, 0);
        vec3 normal = vec3(0,0,0);

        // TODO: an incredible waste of resources....please fix this for loop
        for(int u = 7; u >= 1; u--)
        {
            vec2 offset_coords0=(gl_FragCoord.xy + offset[u]*0.8)/texSize;
            vec2 offset_coords1=(gl_FragCoord.xy + offset[u-1]*0.8)/texSize;

            vec3 eyePos_offset0 = texture(u_viewPosTexture, offset_coords0).rgb;
            vec3 eyePos_offset1 = texture(u_viewPosTexture, offset_coords1).rgb;

            vec3 pln_x = normalize(eyePos_offset0 - eyePos);
            vec3 pln_y = normalize(eyePos_offset1 - eyePos);


            normal+=normalize(cross(pln_y, pln_x));
        }

        normal = normalize(normal); 

        return normal; 
    }

float Attenuation(float distance, float radius)
{
    float x = (distance / radius);
    return max(0.0, (1.0-(x*x)));
}

//https://stackoverflow.com/questions/26070410/robust-atany-x-on-glsl-for-converting-xy-coordinate-to-angle
float atan2(in float y, in float x)
{
    bool s = (abs(x) > abs(y));
    return mix(PI/2.0 - atan(x,y), atan(y,x), s);
}


//https://www.researchgate.net/publication/215506032_Image-space_horizon-based_ambient_occlusion
//https://developer.download.nvidia.com/presentations/2008/SIGGRAPH/HBAO_SIG08b.pdf
float OcclusionInDirection(vec3 p, vec3 direction, float radius, int numSteps, sampler2D viewCoordsTex, mat4 projMatrix, float jitter)
{
       
       float t = atan2(direction.z,length(direction.xy));
       float increment = radius / numSteps;
       increment +=(0.2)*jitter*increment;
       float bias = (PI/6);
       float slopeBias  = bias * ((3*t)/PI);
       
       float m = t;
       float wao = 0;
       float maxLen = 0;

       for (int i=1; i<=numSteps; i++)
       {
            vec3 samplePosition = (p*vec3(1,1,-1)) + (direction*increment*i);
            vec2 sampleUV = TexCoords(samplePosition, projMatrix);
                
           
            vec3 D = texture(viewCoordsTex, sampleUV).xyz - p;
            
            // Ignore samples outside radius
            float l=length(D);
            if(l>radius || l<0.001)
                continue;

            float h = atan2( -D.z,length(D.xy));

            if(h > m + bias + slopeBias)
            {
                m = h;
                maxLen = l;
            }
       }
wao = (sin(m) - sin(t))* Attenuation(maxLen, radius);
        
        return wao;
}
//https://www.researchgate.net/publication/215506032_Image-space_horizon-based_ambient_occlusion
//https://developer.download.nvidia.com/presentations/2008/SIGGRAPH/HBAO_SIG08b.pdf
float OcclusionInDirectionTEMP(vec3 p, vec3 direction, float radius, int numSteps, sampler2D viewCoordsTex, mat4 projMatrix)
{
       float increment = radius / numSteps;
       
       float t = atan(-direction.z/length(direction.xy)) - (PI/6); //offset tangent by 30deg
       float m = t;
       float horizonDist = 0;

       for (int i=1; i<=numSteps; i++)
       {
            vec3 samplePosition = (p*vec3(1,1,-1)) + (direction*increment*i);
            vec2 sampleUV = TexCoords(samplePosition, projMatrix);

            vec3 D = texture(viewCoordsTex, sampleUV).xyz - p;

            // Ignore samples outside radius
            if(length(D)>radius)
                continue;

            float h = atan( -D.z/length(D.xy));

            if(abs(h)>abs(m))//TODO...what? why?
            {
                m = h;
                horizonDist = length(D);
            }
       }
        
        float res = (sin(m) - sin(t)) * Attenuation(horizonDist, radius);
        return res;
}

vec3 EyeNormal_dz(vec2 uvCoords, vec3 eyePos)
    {
        vec2 texSize=textureSize(u_viewPosTexture, 0);

        
        float p_dzdx = texelFetch(u_viewPosTexture, ivec2((gl_FragCoord.x + 1), gl_FragCoord.y),0).z - eyePos.z;
        float p_dzdy = texelFetch(u_viewPosTexture, ivec2(gl_FragCoord.x, (gl_FragCoord.y + 1)),0).z - eyePos.z;

        float m_dzdx = -texelFetch(u_viewPosTexture, ivec2((gl_FragCoord.x - 1), gl_FragCoord.y),0).z + eyePos.z;
        float m_dzdy = -texelFetch(u_viewPosTexture, ivec2(gl_FragCoord.x, (gl_FragCoord.y - 1)),0).z + eyePos.z;
        
        bool px = abs(p_dzdx)<abs(m_dzdx);
        bool py = abs(p_dzdy)<abs(m_dzdy);

        float p_dx=texelFetch(u_viewPosTexture, ivec2((gl_FragCoord.x + 1.0 ), gl_FragCoord.y),0).x - eyePos.x;
        float p_dy=texelFetch(u_viewPosTexture, ivec2(gl_FragCoord.x, (gl_FragCoord.y + 1.0)),0).y  - eyePos.y; 

        float m_dx= - texelFetch(u_viewPosTexture, ivec2((gl_FragCoord.x - 1.0 ), gl_FragCoord.y),0).x + eyePos.x;
        float m_dy= - texelFetch(u_viewPosTexture, ivec2(gl_FragCoord.x, (gl_FragCoord.y - 1.0)),0).y  + eyePos.y; 

        vec3 normal = normalize(vec3(px?p_dzdx:m_dzdx, py?p_dzdy:m_dzdy, length(vec2(px?p_dx:m_dx, py?p_dy:m_dy)))); 
        return normal;
    }

)";

    const std::string CALC_GAUSSIAN_BLUR =
        R"(
    
        vec2 texSize=textureSize(u_texture, 0);
        ivec2 weights_textureSize=textureSize(u_weights_texture, 0);        

        float sum=0;
        vec4 color=vec4(0.0,0.0,0.0,0.0);

        float weight=texelFetch(u_weights_texture, ivec2(0, u_radius), 0).r;
        color+=texture(u_texture, (gl_FragCoord.xy)/texSize) * weight;
        sum +=weight;

        for(int i = 1; i <= u_radius ; i++)
        {
            float weight=texelFetch(u_weights_texture, ivec2(i, u_radius), 0).r;

            vec2 offset = vec2(u_hor?1:0, u_hor?0:1);  

            color+=texture(u_texture, (gl_FragCoord.xy + i * offset)/texSize)* weight;
            color+=texture(u_texture, (gl_FragCoord.xy - i * offset)/texSize)* weight;

            sum+=2.0*weight;
        }
        
        color/=sum;

        FragColor=color;
)";

    const std::string CALC_BLUR =
        R"(
    
        vec2 texSize=textureSize(u_texture, 0);
        vec4 color=vec4(0.0,0.0,0.0,0.0);
        for(int u = -u_size; u <= u_size ; u++)
        {
           for(int v = -u_size; v <= u_size ; v++)
            {
               color+=texture(u_texture, (gl_FragCoord.xy + vec2(u, v))/texSize);
            }
        }
        color/=u_size*u_size*4.0;

        FragColor=color;
)";

    const std::string CALC_SSAO =
        R"(
    
    //TODO: please agree on what you consider view space, if you need to debug invert the z when showing the result
    vec2 texSize=textureSize(u_viewPosTexture, 0);
    vec2 uvCoords = gl_FragCoord.xy / texSize;

    vec3 eyePos = texelFetch(u_viewPosTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    
    if(eyePos.z>u_far-0.001)
        {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
            return;
        }

    vec3 normal = EyeNormal_dz(uvCoords, eyePos) /** vec3(1.0, 1.0, -1.0)*/;
    //vec3 normal = texture(u_viewNormalsTexture, uvCoords).rgb;

    vec3 rotationVec = texture(u_rotVecs, uvCoords * texSize/NOISE_SIZE).rgb;
    vec3 tangent     = normalize(rotationVec - normal * dot(rotationVec, normal));
    vec3 bitangent   = cross(normal, tangent);
    mat3 TBN         = mat3(tangent, bitangent, normal); 
    
    float ao=0;
    for(int k=0; k < u_numSamples; k++)
    {
        vec3 samplePoint_view = vec3(eyePos.x, eyePos.y, eyePos.z*-1.0) + ( u_radius * ( TBN * u_rays[k]));
        vec4 samplePoint_ndc = u_proj * vec4(samplePoint_view, 1.0);
        samplePoint_ndc.xyz /= samplePoint_ndc.w;
        samplePoint_ndc.xyz = samplePoint_ndc.xyz * 0.5 + 0.5;
    
        if(!(samplePoint_view.z <u_far-0.1))
            continue;
        float delta = texture(u_viewPosTexture, samplePoint_ndc.xy).b + samplePoint_view.z;        
        ao+= (delta  < 0 ? 1.0 : 0.0) * smoothstep(0.0, 1.0, 1.0 - (abs(delta) / u_radius));       
    }
    ao /= u_numSamples;

    FragColor = vec4(1.0, 1.0, 1.0, 1.0) * (1-ao); 
)";

    const std::string CALC_HBAO =
        R"(
    
    //TODO: please agree on what you consider view space, if you need to debug invert the z when showing the result
    vec2 texSize=textureSize(u_viewPosTexture, 0);
    vec2 uvCoords = gl_FragCoord.xy / texSize;

    vec3 eyePos = texelFetch(u_viewPosTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    
    if(eyePos.z>u_far-0.001)
        {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
            return;
        }

    vec3 normal = EyeNormal_dz(uvCoords, eyePos) /** vec3(1.0, 1.0, -1.0)*/;
    
    vec3 rotationVec = texture(u_rotVecs, uvCoords * texSize/NOISE_SIZE).rgb;
    vec3 tangent     = normalize(rotationVec - normal * dot(rotationVec, normal));
    vec3 bitangent   = cross(normal, tangent);
    mat3 TBN         = mat3(tangent, bitangent, normal); 
    
    float ao;
    float increment = ((2.0*PI)/u_numSamples);
    vec2 mirrorCoords = (vec2(1,1) - uvCoords);

    for(int k=0; k < u_numSamples; k++)
    {
        //TODO: pass the directions as uniforms???
        float angle=increment * k;
        vec3 sampleDirection = TBN * vec3(cos(angle), sin(angle), 0.0);
        vec2 jitterCoords = (gl_FragCoord.xy + vec2(k, 0)) / texSize;
        float jitter = texture(u_rotVecs, jitterCoords * texSize/NOISE_SIZE).r;        
        ao += OcclusionInDirection(eyePos, sampleDirection, u_radius, u_numSteps, u_viewPosTexture, u_proj, jitter);     
    }
    ao /= u_numSamples;

    FragColor = vec4(1.0, 1.0, 1.0, 1.0)*(1.0-ao); 
)";
    const std::string CALC_POSITIONS =
        R"(

    ivec2 texSize=textureSize(u_depthTexture, 0);

    float depthValue = texture(u_depthTexture, gl_FragCoord.xy/vec2(texSize) ).r;
    float zEye =  LinearDepth( depthValue, u_near, u_far );
    zEye = zEye*(u_far - u_near) + u_near;
    vec3 eyePos = EyeCoords(((gl_FragCoord.x/float(texSize.x)) * 2.0)  - 1.0, ((gl_FragCoord.y/float(texSize.y)) * 2.0)  - 1.0, zEye);

    
    FragColor = vec4(eyePos, 1.0); 

)";

    const std::string EXP_FRAGMENT =
        R"(
    #version 330 core
    out vec4 FragColor;

    //[DEFS_SSAO]
    //[DEFS_BLUR]
    //[DEFS_GAUSSIAN_BLUR]
    void main()
    {
        //[CALC_POSITIONS]
        //[CALC_SSAO]
        //[CALC_HBAO]
        //[CALC_BLUR]
        //[CALC_GAUSSIAN_BLUR]
    }
    )";

    static std::map<std::string, std::string> Expansions = {

       { "DEFS_SSAO",           FragmentSource_PostProcessing::DEFS_AO            },
       { "DEFS_BLUR",           FragmentSource_PostProcessing::DEFS_BLUR            },
       { "DEFS_GAUSSIAN_BLUR",  FragmentSource_PostProcessing::DEFS_GAUSSIAN_BLUR   },
       { "CALC_POSITIONS",      FragmentSource_PostProcessing::CALC_POSITIONS       },
       { "CALC_SSAO",           FragmentSource_PostProcessing::CALC_SSAO            },
       { "CALC_HBAO",           FragmentSource_PostProcessing::CALC_HBAO            },
       { "CALC_BLUR",           FragmentSource_PostProcessing::CALC_BLUR            },
       { "CALC_GAUSSIAN_BLUR",  FragmentSource_PostProcessing::CALC_GAUSSIAN_BLUR   },

    };

    std::string Expand(std::string source, std::vector<std::string> expStrings)
    {
        std::string result = source;

        for (int i = 0; i < expStrings.size(); i++)
        {
            std::string token = "//[" + expStrings[i] + "]";
            size_t start_pos = result.find(token);
            std::string expansion = FragmentSource_PostProcessing::Expansions[expStrings[i]];
            result.replace(start_pos, token.length(), expansion);
        }

        return result;
    }
}

class ShaderCode
{
public:
    unsigned int ID;

    ShaderCode()
    {
        ID = 0;
    }
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    ShaderCode(const std::string vShaderCode, std::string fShaderCode)
    {
               
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);

        const char* vsc = vShaderCode.data();
        glShaderSource(vertex, 1, &vsc, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment ShaderCode
        fragment = glCreateShader(GL_FRAGMENT_SHADER);

        const char* fsc = fShaderCode.data();
        glShaderSource(fragment, 1, &fsc, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
  
private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

class RenderableBasic
{
public:
    virtual std::vector<glm::vec3> GetPositions() { return std::vector<glm::vec3>(1); };
    virtual std::vector<glm::vec3> GetNormals() { return std::vector<glm::vec3>(0); };
    virtual std::vector<int> GetIndices() { return std::vector<int>(0); };
};

class ShaderBase
{
private:
    ShaderCode _shaderCode;
    std::string _vertexCode;
    std::string _fragmentCode;

    
public:
    ShaderBase()
    {

    }
    ShaderBase( std::string vertexSource, std::string fragmentSource) :
        _vertexCode(vertexSource), _fragmentCode(fragmentSource), _shaderCode(vertexSource, fragmentSource)
    {
    }

    void SetCurrent() { glUseProgram(_shaderCode.ID); }
    int UniformLocation(std::string name) { return glGetUniformLocation(_shaderCode.ID, name.c_str()); };

    unsigned int ShaderCodeId() { return _shaderCode.ID; };

    virtual  int PositionLayout() { return 0; };
    virtual  int NormalLayout() { return 1; };

    virtual std::string UniformName_ModelMatrix() { return "model"; };
    virtual std::string UniformName_ViewMatrix() { return "view"; };
    virtual std::string UniformName_ProjectionMatrix() { return "proj"; };
    virtual std::string UniformName_NormalMatrix() { return "normalMatrix"; };
    virtual std::string UniformName_Material() { return "lights"; };
    virtual std::string UniformName_Lights() { return "material"; };
    virtual std::string UniformName_CameraPosition() { return "eyeWorldPos"; };
    virtual std::string UniformName_MaterialDiffuse() { return "material.Diffuse"; };
    virtual std::string UniformName_MaterialSpecular() { return "material.Specular"; };
    virtual std::string UniformName_MaterialShininess() { return "material.Shininess"; };
    virtual std::string UniformName_LightsAmbient() { return "lights.Ambient"; };
    virtual std::string UniformName_LightsDirectionsDirection() { return "lights.Directional.Direction"; };
    virtual std::string UniformName_LightsDirectionalDiffuse() { return "lights.Directional.Diffuse"; };
    virtual std::string UniformName_LightsDirectionalSpecular() { return "lights.Directional.Specular"; };
    virtual std::string UniformName_ShadowMapSampler2D() { return "shadowMap"; };
    virtual std::string UniformName_AoMapSampler2D() { return "aoMap"; };
    virtual std::string UniformName_AoStrength() { return "aoStrength"; };
    virtual std::string UniformName_LightSpaceMatrix() { return "LightSpaceMatrix"; };
    virtual std::string UniformName_Bias() { return "bias"; };
    virtual std::string UniformName_SlopeBias() { return "slopeBias"; };
    virtual std::string UniformName_Softness() { return "softness"; };

};

class BasicShader : public ShaderBase
{
public:
    BasicShader(std::vector<std::string> vertexExpansions, std::vector<std::string> fragmentExpansions) :
        ShaderBase(

            VertexSource_Geometry::Expand(
                VertexSource_Geometry::EXP_VERTEX,
                vertexExpansions),

            FragmentSource_Geometry::Expand(
                FragmentSource_Geometry::EXP_FRAGMENT,
                fragmentExpansions)) {};

    virtual int PositionLayout() { return 0; }; 
    virtual int NormalLayout() { return 1; };


};

class PostProcessingShader : public ShaderBase
{
public:
    PostProcessingShader(std::vector<std::string> vertexExpansions, std::vector<std::string> fragmentExpansions) :
        ShaderBase(

            VertexSource_PostProcessing::Expand(
                VertexSource_PostProcessing::EXP_VERTEX,
                vertexExpansions),

            FragmentSource_PostProcessing::Expand(
                FragmentSource_PostProcessing::EXP_FRAGMENT,
                fragmentExpansions)) {};

    virtual int PositionLayout() { return 0; };
};
#endif