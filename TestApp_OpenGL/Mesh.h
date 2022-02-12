#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <assert.h>
#include "Shader.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SceneUtils.h"
#include "Assimp/Importer.hpp"
#include"Assimp/scene.h"
#include "Assimp/postprocess.h"

class Mesh : public RenderableBasic
{

private:
    std::vector<glm::vec3> _vertices;
    int _vertices_count;

    std::vector<glm::vec3> _normals;
    int _normals_count;

    std::vector<int> _indices;
    int _triangles_count;

public:
    Mesh(std::vector<glm::vec3> verts, std::vector<glm::vec3>  normals, std::vector<int> tris)
    {
        int numVerts = verts.size();
        int numNormals = normals.size();
        int numTris = tris.size();

        // Error conditions
        if (numVerts != numNormals)
            throw "number of vertices not compatible with number of normals";
        if (numTris < 3)
            throw "a mesh must contain at least one triangle";

        _vertices_count = numVerts;
        _vertices = std::vector<glm::vec3>();
        for (int i = 0; i < _vertices_count; i++)
        {
            _vertices.push_back(glm::vec3(verts.at(i)));
        }

        _normals_count = numNormals;
        _normals = std::vector<glm::vec3>();
        for (int i = 0; i < _normals_count; i++)
        {
            _normals.push_back(glm::vec3(normals.at(i)));
        }

        _triangles_count = numTris;
        _indices = std::vector<int>();
        for (int i = 0; i < _triangles_count; i++)
        {
            _indices.push_back(tris.at(i));
        }
    }
public:
    std::vector<glm::vec3> GetPositions() override { return std::vector<glm::vec3>(_vertices); };
    std::vector<glm::vec3> GetNormals() override { return std::vector<glm::vec3>(_normals); };
    std::vector<int> GetIndices() override { return std::vector<int>(_indices); };
    int NumVertices() { return _vertices_count; }
    int NumNormals() { return _normals_count; }
    int NumIndices() { return _triangles_count; }

    static Mesh Box(float width, float height, float depth)
    {
        glm::vec3 boxVerts[] =
        {
            //bottom
            glm::vec3(0 ,0, 0),
            glm::vec3(width ,0, 0),
            glm::vec3(width ,height, 0),

            glm::vec3(0 ,0, 0),
            glm::vec3(width ,height, 0),
            glm::vec3(0 ,height, 0),

            //front
            glm::vec3(0 ,0, 0),
            glm::vec3(width ,0, 0),
            glm::vec3(width ,0, depth),

            glm::vec3(0 ,0, 0),
            glm::vec3(width ,0, depth),
            glm::vec3(0 ,0, depth),

            //right
            glm::vec3(width ,0, 0),
            glm::vec3(width ,height, 0),
            glm::vec3(width ,height, depth),

            glm::vec3(width ,0, 0),
            glm::vec3(width ,height, depth),
            glm::vec3(width ,0, depth),

            //back
            glm::vec3(0 ,height, 0),
            glm::vec3(width ,height, 0),
            glm::vec3(width ,height, depth),

            glm::vec3(0 ,height, 0),
            glm::vec3(width ,height, depth),
            glm::vec3(0 ,height, depth),

            //left
            glm::vec3(0 ,0, 0),
            glm::vec3(0 ,height, 0),
            glm::vec3(0 ,height, depth),

            glm::vec3(0 ,0, 0),
            glm::vec3(0 ,height, depth),
            glm::vec3(0 ,0, depth),

            //top
            glm::vec3(0 ,0, depth),
            glm::vec3(width ,0, depth),
            glm::vec3(width ,height, depth),

            glm::vec3(0 ,0, depth),
            glm::vec3(width ,height, depth),
            glm::vec3(0 ,height, depth),
        };
        glm::vec3 boxNormals[] =
        {
            //bottom
            glm::vec3(0 ,0, -1),
            glm::vec3(0 ,0, -1),
            glm::vec3(0 ,0, -1),
            glm::vec3(0 ,0, -1),
            glm::vec3(0 ,0, -1),
            glm::vec3(0 ,0, -1),

            //front
            glm::vec3(0 ,-1, 0),
            glm::vec3(0 ,-1, 0),
            glm::vec3(0 ,-1, 0),
            glm::vec3(0 ,-1, 0),
            glm::vec3(0 ,-1, 0),
            glm::vec3(0 ,-1, 0),

            //right
            glm::vec3(1 ,0, 0),
            glm::vec3(1 ,0, 0),
            glm::vec3(1 ,0, 0),
            glm::vec3(1 ,0, 0),
            glm::vec3(1 ,0, 0),
            glm::vec3(1 ,0, 0),

            //back
            glm::vec3(0 ,1, 0),
            glm::vec3(0 ,1, 0),
            glm::vec3(0 ,1, 0),
            glm::vec3(0 ,1, 0),
            glm::vec3(0 ,1, 0),
            glm::vec3(0 ,1, 0),

            //left
            glm::vec3(-1 ,0, 0),
            glm::vec3(-1 ,0, 0),
            glm::vec3(-1 ,0, 0),
            glm::vec3(-1 ,0, 0),
            glm::vec3(-1 ,0, 0),
            glm::vec3(-1 ,0, 0),


            //top
            glm::vec3(0 ,0, 1),
            glm::vec3(0 ,0, 1),
            glm::vec3(0 ,0, 1),
            glm::vec3(0 ,0, 1),
            glm::vec3(0 ,0, 1),
            glm::vec3(0 ,0, 1),
        };

        int boxTris[] =
        {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            12, 13, 14, 15, 16 ,17 ,18 ,19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30 ,31, 32, 33, 34, 35,
            36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
        };

        return Mesh(
            std::vector<glm::vec3>(std::begin(boxVerts), std::end(boxVerts)),
            std::vector<glm::vec3>(std::begin(boxNormals), std::end(boxNormals)),
            std::vector<int>(std::begin(boxTris), std::end(boxTris)));
    }
    static Mesh TruncCone(float radius, float tipRadius, float height, int subdivisions)
    {
        // Side
        std::vector<glm::vec3> vertices=std::vector<glm::vec3>();
        std::vector<glm::vec3> normals = std::vector<glm::vec3>();
        std::vector<int> triangles = std::vector<int>();

        float dAngle = glm::pi<float>()*2 / subdivisions;

        for (int i= 0;  i< subdivisions; i++)
        {
            float
                cosA = glm::cos(i * dAngle),
                sinA = glm::sin(i * dAngle);

            vertices.push_back(glm::vec3(
                cosA*radius,
                sinA*radius,
                0
            ));

            normals.push_back(glm::vec3(
                cosA,
                sinA,
                (radius-tipRadius)/height
            ));

           //tips
            vertices.push_back(glm::vec3(
                cosA *tipRadius,
                sinA *tipRadius,
                height
            ));

            normals.push_back(glm::vec3(
                cosA,
                sinA,
                (radius - tipRadius) / height

            ));
        }

        for (int k = 0; k < subdivisions; k++)
        {
            int i = k * 2;

            triangles.push_back(i);
            triangles.push_back((i + 2) % (subdivisions * 2));
            triangles.push_back((i + 3) % (subdivisions * 2));

            triangles.push_back(i);
            triangles.push_back((i + 3) % (subdivisions * 2));
            triangles.push_back((i + 1) % (subdivisions * 2));
        }

        // bottom
        for (int i = 0; i < subdivisions; i++)
        {
            float
                cosA = glm::cos(i * dAngle),
                sinA = glm::sin(i * dAngle);

            vertices.push_back(glm::vec3(
                cosA*radius,
                sinA*radius,
                0
            ));

            normals.push_back(glm::vec3(
                0,
                0,
                -1
            ));

        }

        int offset = subdivisions * 2;
        for (int k = 0; k < subdivisions-2; k++)
        {
            int i = offset + k;
            triangles.push_back(offset);
            triangles.push_back(i+1);
            triangles.push_back(i+2);
        }

        // top
        for (int i = 0; i < subdivisions; i++)
        {
            float
                cosA = glm::cos(i * dAngle),
                sinA = glm::sin(i * dAngle);

            vertices.push_back(glm::vec3(
                cosA*tipRadius,
                sinA*tipRadius,
                height
            ));

            normals.push_back(glm::vec3(
                0,
                0,
                1
            ));

        }

        offset = subdivisions * 3;
        for (int k = 0; k < subdivisions - 2; k++)
        {
            int i = offset + k;
            triangles.push_back(offset);
            triangles.push_back(i + 1);
            triangles.push_back(i + 2);
        }
        return Mesh(vertices, normals,triangles);
    }
    static Mesh Cone(float radius, float height, int subdivisions) { return TruncCone(radius, 0.001, height, subdivisions); }
    static Mesh Cylinder(float radius, float height, int subdivisions) { return TruncCone(radius, radius, height, subdivisions); }
    static Mesh Arrow(float cylRadius, float cylLenght, float coneRadius, float coneLength, int subdivisions)
    {
        Mesh cyl = Cylinder(cylRadius, cylLenght, subdivisions);
        Mesh cone = Cone(coneRadius, coneLength, subdivisions);

        std::vector<glm::vec3> cylVertices = cyl.GetPositions();
        std::vector<glm::vec3> coneVertices = cone.GetPositions();

        glm::vec3 offset = glm::vec3(0.0, 0.0, cylLenght);
        for (int i = 0; i < coneVertices.size(); i++)
        {
            coneVertices.at(i) += offset;
        }

        std::vector<glm::vec3> cylNormals = cyl.GetNormals();
        std::vector<glm::vec3> coneNormals = cone.GetNormals();

        std::vector<int> cylIndices = cyl.GetIndices();
        std::vector<int> coneIndices = cone.GetIndices();

        std::vector<glm::vec3> arrowVertices;
        arrowVertices.insert(arrowVertices.end(), cylVertices.begin(), cylVertices.end());
        arrowVertices.insert(arrowVertices.end(), coneVertices.begin(), coneVertices.end());

        std::vector<glm::vec3> arrowNormals;
        arrowNormals.insert(arrowNormals.end(), cylNormals.begin(), cylNormals.end());
        arrowNormals.insert(arrowNormals.end(), coneNormals.begin(), coneNormals.end());

        std::vector<int> arrowIndices;
        arrowIndices.insert(arrowIndices.end(), cylIndices.begin(), cylIndices.end());
        arrowIndices.insert(arrowIndices.end(), coneIndices.begin(), coneIndices.end());
        int k = cylVertices.size();
        for (int j = cylIndices.size(); j < cylIndices.size() + coneIndices.size(); j++)
        {
            arrowIndices.at(j) += k;
        }

        return Mesh(arrowVertices, arrowNormals,arrowIndices);
    }
};

class Wire
{
private:
    std::vector<glm::vec3> _points;
    int _points_count;

public:
    Wire(glm::vec3* points, int count)
    {
        _points_count = count;
        _points = std::vector<glm::vec3>();
        for (int i = 0; i < count; i++)
        {
            _points.push_back(glm::vec3(points[i]));
        }
    }

    std::vector<glm::vec3> GetPositions() { return std::vector<glm::vec3>(_points); };

    int NumPoints() { return _points_count; };

    static Wire Grid(glm::vec2 min, glm::vec2 max, int step)
    {
        int numLinesX = ((max.x - min.x) / step) + 1;
        int numLinesY = ((max.y - min.y) / step) + 1;

        int numLines = numLinesX + numLinesY;

        std::vector<glm::vec3> points;

        int k = 0;

        for (int i = 0; i < numLinesX; i++)
        {
            // start
            points.push_back(glm::vec3(min.x + i * step, min.y, 0.0f));

            // end
            points.push_back(glm::vec3(min.x + i * step, max.y, 0.0f));

        }

        for (int j = 0; j < numLinesY; j++)
        {
            // start
            points.push_back(glm::vec3(min.x , min.y + j * step, 0.0f));

            // end
            points.push_back(glm::vec3(max.x , min.y + j * step, 0.0f));
        }

        return Wire(points.data(), points.size());
    }
};

class FileReader
{
private:
    const char* _path;
    std::vector<Mesh> _meshes;

    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<int> triangles;


        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            vertices.push_back(glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            ));
        }

        if (mesh->mNormals != NULL)
        {
            for (unsigned int i = 0; i < mesh->mNumVertices/*same as normals*/; i++)
            {
                normals.push_back(glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                ));
            }
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                triangles.push_back(face.mIndices[j]);
            }

        }

        return Mesh(vertices, normals, triangles);
    }

    void ProcessNode(aiNode* node, const aiScene* scene)
    {
        // process all the node's meshes (if any)
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes.push_back(ProcessMesh(mesh, scene));
        }
        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

public:
    FileReader(const char* path) : _path(path)
    {}

    void Load()
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(_path, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        _meshes = std::vector<Mesh>();

        ProcessNode(scene->mRootNode, scene);
    }

    std::vector<Mesh> Meshes()
    {
        return _meshes;
    }
};


class MeshRenderer
{

private:
    ShaderBase* _shader;
    ShaderBase* _shader_noShadows;
    Material _material;
    RenderableBasic* _mesh;
    int _numIndices;
    unsigned int _vao;
    unsigned int _vbo;
    unsigned int _ebo;

    glm::mat4 _modelMatrix;

public:
    MeshRenderer(glm::vec3 position, float rotation, glm::vec3 rotationAxis, glm::vec3 scale, RenderableBasic* mesh, ShaderBase* shader, ShaderBase* shaderNoShadows, Material mat)
        :
        _shader(shader), _shader_noShadows(shaderNoShadows),_mesh(mesh), _material(mat)
    {
        _mesh = mesh;

        // Compute Model Transform Matrix ====================================================================================================== //
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, position);

        model = glm::rotate(model, rotation, rotationAxis);

        model = glm::scale(model, scale);

        _modelMatrix = model;

        // Generate Graphics Data ============================================================================================================= //
        glGenVertexArrays(1, &_vao);

        glGenBuffers(1, &_vbo);

        glGenBuffers(1, &_ebo);

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

        std::vector<glm::vec3> positions = _mesh->GetPositions();
        std::vector<glm::vec3> normals = _mesh->GetNormals();
        std::vector<int> indices = _mesh->GetIndices();
        _numIndices = indices.size();

        std::vector<float> positionsFloatArray = std::vector<float>();
        for ( int i= 0; i < positions.size() ; i++)
        {
            positionsFloatArray.push_back(positions.at(i).x);
            positionsFloatArray.push_back(positions.at(i).y);
            positionsFloatArray.push_back(positions.at(i).z);
        }

        std::vector<float> normalsFloatArray = std::vector<float>();
        for (int i = 0; i < normals.size(); i++)
        {
            normalsFloatArray.push_back(normals.at(i).x);
            normalsFloatArray.push_back(normals.at(i).y);
            normalsFloatArray.push_back(normals.at(i).z);
        }

        glBufferData(GL_ARRAY_BUFFER, (positions.size() + normals.size()) * 3 * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * 3 * sizeof(float), positionsFloatArray.data());
        glBufferSubData(GL_ARRAY_BUFFER, positions.size() * 3 * sizeof(float), normals.size() * 3 * sizeof(float), normalsFloatArray.data());

        // Position
        glVertexAttribPointer( _shader->PositionLayout(), 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        // Normal
        glVertexAttribPointer(_shader->NormalLayout(), 3, GL_FLOAT, GL_FALSE, 0, (void*)(normals.size() * 3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( unsigned int), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CheckOGLErrors();
      
    }



public:
    void Draw(glm::mat4 view, glm::mat4 proj, glm::vec3 eye, SceneParams sceneParams)
    {

        ShaderBase* shader = sceneParams.drawParams.doShadows ? _shader : _shader_noShadows;

        shader->SetCurrent();

        // ModelViewProjection + NormalMatrix =========================================================================================================//
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_ModelMatrix()), 1, GL_FALSE, glm::value_ptr(_modelMatrix));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_NormalMatrix()), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(_modelMatrix))));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_ViewMatrix()), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_ProjectionMatrix()), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(shader->UniformLocation(shader->UniformName_CameraPosition()), 1, glm::value_ptr(eye));

        // Material Properties =========================================================================================================//
        glUniform4fv(shader->UniformLocation(shader->UniformName_MaterialDiffuse()), 1, glm::value_ptr(_material.Diffuse));
        glUniform4fv(shader->UniformLocation(shader->UniformName_MaterialSpecular()), 1, glm::value_ptr(_material.Specular));
        glUniform1f(shader->UniformLocation(shader->UniformName_MaterialShininess()), _material.Shininess);

        // Scene sceneParams.sceneLights =========================================================================================================//
        glUniform4fv(shader->UniformLocation(shader->UniformName_LightsAmbient()), 1, glm::value_ptr(sceneParams.sceneLights.Ambient.Ambient));
        glUniform3fv(shader->UniformLocation(shader->UniformName_LightsDirectionsDirection()), 1, glm::value_ptr(normalize(sceneParams.sceneLights.Directional.Direction)));
        glUniform4fv(shader->UniformLocation(shader->UniformName_LightsDirectionalDiffuse()), 1, glm::value_ptr(sceneParams.sceneLights.Directional.Diffuse));
        glUniform4fv(shader->UniformLocation(shader->UniformName_LightsDirectionalSpecular()), 1, glm::value_ptr(sceneParams.sceneLights.Directional.Specular));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_LightSpaceMatrix()), 1, GL_FALSE, glm::value_ptr(sceneParams.sceneLights.Directional.LightSpaceMatrix));
        glUniform1i(shader->UniformLocation(shader->UniformName_ShadowMapSampler2D()), 0);
        glUniform1i(shader->UniformLocation(shader->UniformName_AoMapSampler2D()), 1);
        glUniform1f(shader->UniformLocation(shader->UniformName_Bias()), sceneParams.sceneLights.Directional.Bias);
        glUniform1f(shader->UniformLocation(shader->UniformName_SlopeBias()), sceneParams.sceneLights.Directional.SlopeBias);
        glUniform1f(shader->UniformLocation(shader->UniformName_Softness()), sceneParams.sceneLights.Directional.Softness);
        
        if (sceneParams.sceneLights.Directional.ShadowMapId > 0)
        {
            glActiveTexture(GL_TEXTURE0); //ShadowMap
            glBindTexture(GL_TEXTURE_2D, sceneParams.sceneLights.Directional.ShadowMapId); 
        }

        if (sceneParams.sceneLights.Ambient.AoMapId > 0)
        {
            glActiveTexture(GL_TEXTURE1); //SSAO
            glBindTexture(GL_TEXTURE_2D, sceneParams.sceneLights.Ambient.AoMapId);
            glUniform1f(shader->UniformLocation(shader->UniformName_AoStrength()), sceneParams.sceneLights.Ambient.aoStrength);
        }

        // Draw Call =========================================================================================================//
        glBindVertexArray(_vao);
        glDrawElements(GL_TRIANGLES, _numIndices, GL_UNSIGNED_INT, nullptr); 
        glBindVertexArray(0);

        CheckOGLErrors();

    }

    void DrawCustom(glm::mat4 view, glm::mat4 proj, ShaderBase* shader)
    {

        shader->SetCurrent();

        CheckOGLErrors();

        // ModelViewProjection + NormalMatrix =========================================================================================================//
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_ModelMatrix()), 1, GL_FALSE, glm::value_ptr(_modelMatrix));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_NormalMatrix()), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(_modelMatrix))));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_ViewMatrix()), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(shader->UniformLocation(shader->UniformName_ProjectionMatrix()), 1, GL_FALSE, glm::value_ptr(proj));

        CheckOGLErrors();

        // Draw Call =========================================================================================================//
        glBindVertexArray(_vao);
        glDrawElements(GL_TRIANGLES, _numIndices, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        CheckOGLErrors();

    }

   
    void Transform(glm::vec3 position, float rotation, glm::vec3 rotationAxis, glm::vec3 scale, bool cumulative)
    {
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, position);  
        model = glm::rotate(model, rotation, rotationAxis);
        model = glm::scale(model, scale);

        if (!cumulative)
            _modelMatrix = model;
        else
            _modelMatrix = model* _modelMatrix;

    }

    std::vector<glm::vec3> GetTransformedPoints()
    {
        std::vector<glm::vec3> transformed = _mesh->GetPositions();

        for (int i = 0; i < transformed.size(); i++)
        {
            transformed.at(i) = (glm::vec3)(_modelMatrix * glm::vec4(transformed.at(i), 1.0f));
        }

        return transformed;
    }

    public:
        static void CheckOGLErrors()
        {
            GLenum error = 0;
            std::string log = "";
            while ((error = glGetError()) != GL_NO_ERROR)
            {
                log += error;
                log += ' ,';
            }
            if (!log.empty())
                throw log;
        }
};

class LinesRenderer
{

private:
    ShaderBase* _shader;
    Wire _wire;
    glm::vec4 _color;
    unsigned int _vao;
    unsigned int _vbo;
    unsigned int _numPoints;

    glm::mat4 _modelMatrix;

public:
    LinesRenderer(glm::vec3 position, float rotation, glm::vec3 rotationAxis, glm::vec3 scale, Wire wire, ShaderBase* shader, glm::vec4 color)
        :
        _shader(shader), _wire(wire), _color(color)
    {

        // Compute Model Transform Matrix ====================================================================================================== //
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, position);

        model = glm::rotate(model, rotation, rotationAxis);

        model = glm::scale(model, scale);

        _modelMatrix = model;

        // Generate Graphics Data ============================================================================================================= //
        glGenVertexArrays(1, &_vao);

        glGenBuffers(1, &_vbo);

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);


        std::vector<float> positionsFloatArray = std::vector<float>();
        _numPoints = wire.NumPoints();

        std::vector<glm::vec3> points = wire.GetPositions();

        for (int i = 0; i < _numPoints; i++)
        {
            positionsFloatArray.push_back(points.at(i).x);
            positionsFloatArray.push_back(points.at(i).y);
            positionsFloatArray.push_back(points.at(i).z);
        }

        
        glBufferData(GL_ARRAY_BUFFER, positionsFloatArray.size() * 3 * sizeof(float), positionsFloatArray.data(), GL_STATIC_DRAW);

        // Position
        glVertexAttribPointer(_shader->PositionLayout(), 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CheckOGLErrors();

    }



public:
    void Draw(glm::mat4 view, glm::mat4 proj, glm::vec3 eye, SceneLights lights)
    {

        _shader->SetCurrent();

        // ModelViewProjection + NormalMatrix =========================================================================================================//
        glUniformMatrix4fv(_shader->UniformLocation(_shader->UniformName_ModelMatrix()), 1, GL_FALSE, glm::value_ptr(_modelMatrix));
        glUniformMatrix4fv(_shader->UniformLocation(_shader->UniformName_NormalMatrix()), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(_modelMatrix))));
        glUniformMatrix4fv(_shader->UniformLocation(_shader->UniformName_ViewMatrix()), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(_shader->UniformLocation(_shader->UniformName_ProjectionMatrix()), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(_shader->UniformLocation(_shader->UniformName_CameraPosition()), 1, glm::value_ptr(eye));

        // Material Properties =========================================================================================================//
        glUniform4fv(_shader->UniformLocation(_shader->UniformName_MaterialDiffuse()), 1, glm::value_ptr(_color));

        // Draw Call =========================================================================================================//
        glBindVertexArray(_vao);
        glDrawArrays(GL_LINES, 0, _numPoints);
        glBindVertexArray(0);

        CheckOGLErrors();

    }

    void Transform(glm::vec3 position, float rotation, glm::vec3 rotationAxis, glm::vec3 scale)
    {
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, position);
        model = glm::rotate(model, rotation, rotationAxis);
        model = glm::scale(model, scale);

        _modelMatrix = model;

    }
private:
    void CheckOGLErrors()
    {
        GLenum error = 0;
        std::string log = "";
        while ((error = glGetError()) != GL_NO_ERROR)
        {
            log += error;
            log += ' ,';
        }
        if (!log.empty())
            throw log;
    }
};





#endif
