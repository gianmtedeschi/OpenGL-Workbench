#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Shader.h"
#include "Mesh.h"
#include "stb_image.h"
#include "Camera.h"
#include "Mesh.h"
#include "GeometryHelper.h"
#include "SceneUtils.h"
#include "FrameBuffer.h"
#include "Shader_util.h"

// CONSTANTS ======================================================
const char* glsl_version = "#version 130";

const unsigned int shadowMapResolution = 2048;
const int SSAO_MAX_SAMPLES = 64;
const int SSAO_BLUR_MAX_RADIUS = 16;




void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_scroll_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

Camera camera = Camera(25, 0, 0);

bool firstMouse = true;
float lastX, lastY, yaw, pitch, roll;
unsigned int width, height;


// Debug ===========================================================
bool showGrid = false;
bool showLights = false;
bool showBoundingBox = false;
bool showAO = false;


// Camera ==========================================================
bool perspective = true;
float fov = 45.0f;

// Scene Parameters ==========================================================
SceneParams sceneParams = SceneParams();

// BBox ============================================================
BoundingBox sceneBB = BoundingBox(std::vector<glm::vec3>{});

// ImGUI ============================================================
const char* ao_comboBox_items[] = { "SSAO", "HBAO" };
static const char* ao_comboBox_current_item = "SSAO";

const char* AOShaderFromItem(const char* item)
{
    if (!strcmp("SSAO", item))
        return "SSAO";
    else if (!strcmp("HBAO", item))
        return "HBAO";
    else return "SSAO";
}

void ShowImGUIWindow()
{
    /*ImGui::ShowDemoWindow(&showWindow);*/

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Camera"))
        {
            ImGui::Checkbox("Perspective", &perspective);
            ImGui::SliderFloat("FOV", &fov, 10.0f, 100.0f);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lights"))
        {
            if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_None))
            {
                ImGui::ColorEdit4("Color", (float*)&(sceneParams.sceneLights.Ambient.Ambient));
                if (ImGui::BeginCombo("AO Tecnique", ao_comboBox_current_item)) // The second parameter is the label previewed before opening the combo.
                {
                    for (int n = 0; n < IM_ARRAYSIZE(ao_comboBox_items); n++)
                    {
                        bool is_selected = (ao_comboBox_current_item == ao_comboBox_items[n]); // You can store your selection however you want, outside or inside your objects

                        if (ImGui::Selectable(ao_comboBox_items[n], is_selected))
                            ao_comboBox_current_item = ao_comboBox_items[n];

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)

                    }
                    ImGui::EndCombo();
                }
                ImGui::DragFloat("AORadius", &sceneParams.sceneLights.Ambient.aoRadius, 0.001f, 0.001f, 1.0f);
                ImGui::DragInt("AOSamples", &sceneParams.sceneLights.Ambient.aoSamples, 1, 1, 64);
                ImGui::DragInt("AOSteps", &sceneParams.sceneLights.Ambient.aoSteps, 1, 1, 64);
                ImGui::DragInt("AOBlur", &sceneParams.sceneLights.Ambient.aoBlurAmount, 1, 0, SSAO_BLUR_MAX_RADIUS - 1);
                ImGui::DragFloat("AOStrength", &sceneParams.sceneLights.Ambient.aoStrength, 0.1f, 0.0f, 5.0f);
            }

            if (ImGui::CollapsingHeader("Directional", ImGuiTreeNodeFlags_None))
            {
                ImGui::DragFloat3("Direction", (float*)&(sceneParams.sceneLights.Directional.Direction), 0.05f, -1.0f, 1.0f);
                ImGui::ColorEdit4("Diffuse", (float*)&(sceneParams.sceneLights.Directional.Diffuse));
                ImGui::ColorEdit4("Specular", (float*)&(sceneParams.sceneLights.Directional.Specular));

                if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_None))
                {
                    ImGui::Checkbox("Enable", &sceneParams.drawParams.doShadows);
                    ImGui::DragFloat("Bias", &sceneParams.sceneLights.Directional.Bias, 0.001f, 0.0f, 0.05f);
                    ImGui::DragFloat("SlopeBias", &sceneParams.sceneLights.Directional.SlopeBias, 0.001f, 0.0f, 0.05f);
                    ImGui::DragFloat("Softness", &sceneParams.sceneLights.Directional.Softness, 0.001f, 0.0, 1.0f);
                }
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Debug"))
        {
            ImGui::Checkbox("Grid", &showGrid);
            ImGui::Checkbox("BoundingBox", &showBoundingBox);
            ImGui::Checkbox("Show Lights", &showLights);
            ImGui::Checkbox("AO Pass", &showAO);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void LoadScene_Primitives(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh boxMesh = Mesh::Box(1, 1, 1);
    MeshRenderer box =
        MeshRenderer(glm::vec3(0, 0, 0), 0.0, glm::vec3(0, 0, 1), glm::vec3(1, 1, 1),
            &boxMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::ShinyRed);

    sceneMeshCollection->push_back(box);

    Mesh coneMesh = Mesh::Cone(0.8, 1.6, 16);
    MeshRenderer cone =
        MeshRenderer(glm::vec3(1, 2, 1), 1.5, glm::vec3(0, -1, 1), glm::vec3(1, 1, 1),
            &coneMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::PlasticGreen);

    sceneMeshCollection->push_back(cone);

    Mesh cylMesh = Mesh::Cylinder(0.6, 1.6, 16);
    MeshRenderer cylinder =
        MeshRenderer(glm::vec3(-1, -0.5, 1), 1.1, glm::vec3(0, 1, 1), glm::vec3(1, 1, 1),
            &cylMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::Copper);

    sceneMeshCollection->push_back(cylinder);

    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);

    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Monkeys(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase> shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &shadersCollection["LIT_WITH_SHADOWS_SSAO"], &shadersCollection["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);

    FileReader reader = FileReader("./Assets/Models/suzanne.obj");
    reader.Load();
    Mesh monkeyMesh = reader.Meshes()[0];
    MeshRenderer monkey1 =
        MeshRenderer(glm::vec3(0, -1.0, 2.0), 1.3, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &shadersCollection["LIT_WITH_SHADOWS_SSAO"], &shadersCollection["LIT_WITH_SSAO"], MaterialsCollection::ShinyRed);

    MeshRenderer monkey2 =
        MeshRenderer(glm::vec3(-2.0, 1.0, 0.8), 1.3, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &shadersCollection["LIT_WITH_SHADOWS_SSAO"], &shadersCollection["LIT_WITH_SSAO"], MaterialsCollection::PlasticGreen);

    MeshRenderer monkey3 =
        MeshRenderer(glm::vec3(2.0, 1.0, 0.8), 1.3, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &shadersCollection["LIT_WITH_SHADOWS_SSAO"], &shadersCollection["LIT_WITH_SSAO"], MaterialsCollection::Copper);

    sceneMeshCollection->push_back(monkey1);
    sceneMeshCollection->push_back(monkey2);
    sceneMeshCollection->push_back(monkey3);

    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_ALotOfMonkeys(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);

    FileReader reader = FileReader("./Assets/Models/suzanne.obj");
    reader.Load();
    Mesh monkeyMesh = reader.Meshes()[0];
    MeshRenderer monkey1 =
        MeshRenderer(glm::vec3(-1.0, -1.0, 0.4), 0.9, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::ShinyRed);
    monkey1.Transform(glm::vec3(0, 0, 0), glm::pi<float>() / 4.0, glm::vec3(0, 0.0, -1.0), glm::vec3(1.0, 1.0, 1.0), true);

    MeshRenderer monkey2 =
        MeshRenderer(glm::vec3(-1.0, 1.0, 0.4), 0.9, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::PlasticGreen);
    monkey2.Transform(glm::vec3(-1.5, 1.5, 0), 3.0 * glm::pi<float>() / 4.0, glm::vec3(0, 0.0, -1.0), glm::vec3(1.0, 1.0, 1.0), true);

    MeshRenderer monkey3 =
        MeshRenderer(glm::vec3(0.0, 0.5, 1.0), 1.2, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::Copper);
    //monkey3.Transform(glm::vec3(0, 0, 0), 3.0 * glm::pi<float>() / 4.0, glm::vec3(0, 0.0, 1.0), glm::vec3(1.0, 1.0, 1.0), true);

    MeshRenderer monkey4 =
        MeshRenderer(glm::vec3(1.0, -1.0, 0.4), 0.9, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &monkeyMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::PureWhite);
    monkey4.Transform(glm::vec3(0, 0, 0), glm::pi<float>() / 4.0, glm::vec3(0, 0.0, 1.0), glm::vec3(1.0, 1.0, 1.0), true);

    sceneMeshCollection->push_back(monkey1);
    sceneMeshCollection->push_back(monkey2);
    sceneMeshCollection->push_back(monkey3);
    sceneMeshCollection->push_back(monkey4);

    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Cadillac(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);

    FileReader reader = FileReader("./Assets/Models/Cadillac.obj");
    reader.Load();
    Mesh cadillacMesh0 = reader.Meshes()[0];
    MeshRenderer cadillac0 =
        MeshRenderer(glm::vec3(-1.0, -1.0, 0.4), glm::pi<float>() * 0.5, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &cadillacMesh0, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::ShinyRed);
    cadillac0.Transform(glm::vec3(1, 0.5, -0.5), 0.0, glm::vec3(0, 0.0, -1.0), glm::vec3(1.0, 1.0, 1.0), true);

    Mesh cadillacMesh1 = reader.Meshes()[1];
    MeshRenderer cadillac1 =
        MeshRenderer(glm::vec3(-1.0, -1.0, 0.4), glm::pi<float>() * 0.5, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &cadillacMesh1, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::ShinyRed);
    cadillac1.Transform(glm::vec3(1, 0.5, -0.5), 0.0, glm::vec3(0, 0.0, -1.0), glm::vec3(1.0, 1.0, 1.0), true);

    Mesh cadillacMesh2 = reader.Meshes()[2];
    MeshRenderer cadillac2 =
        MeshRenderer(glm::vec3(-1.0, -1.0, 0.4), glm::pi<float>() * 0.5, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &cadillacMesh2, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::ShinyRed);
    cadillac2.Transform(glm::vec3(1, 0.5, -0.5), 0.0, glm::vec3(0, 0.0, -1.0), glm::vec3(1.0, 1.0, 1.0), true);


    sceneMeshCollection->push_back(cadillac0);
    sceneMeshCollection->push_back(cadillac1);
    sceneMeshCollection->push_back(cadillac2);


    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Dragon(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);


    FileReader reader = FileReader("./Assets/Models/Dragon.obj");
    reader.Load();
    Mesh dragonMesh = reader.Meshes()[0];
    MeshRenderer dragon =
        MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0.0, glm::vec3(0, 1, 0), glm::vec3(1, 1, 1),
            &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(dragon);



    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Nefertiti(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);

    FileReader reader = FileReader("./Assets/Models/Nefertiti.obj");
    reader.Load();
    Mesh dragonMesh = reader.Meshes()[0];
    MeshRenderer dragon =
        MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0.5, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(dragon);



    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Knob(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);
    sceneBoundingBox->Update((&plane)->GetTransformedPoints());

    FileReader reader = FileReader("./Assets/Models/Knob.obj");
    reader.Load();
    for (int i = 0; i < reader.Meshes().size(); i++)
    {
        Mesh dragonMesh = reader.Meshes()[i];
        MeshRenderer dragon =
            MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0.5f, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
                &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

        sceneMeshCollection->push_back(dragon);
        sceneBoundingBox->Update((&dragon)->GetTransformedPoints());
    }
}

void LoadScene_Bunny(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);

    FileReader reader = FileReader("./Assets/Models/Bunny.obj");
    reader.Load();
    Mesh dragonMesh = reader.Meshes()[0];
    MeshRenderer dragon =
        MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(dragon);



    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Jinx(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    //TODO texturize the plane
    //unsigned int texture;
    //glGenTextures(1, &texture);
    //glBindTexture(GL_TEXTURE_2D, texture);
    //// set the texture wrapping/filtering options (on the currently bound texture object)
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //// load and generate the texture
    //int width, height, nrChannels;
    //unsigned char* data = stbi_load("./Assets/Textures/floor.png", &width, &height, &nrChannels, 0);
    //if (data)
    //{
    //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //    glGenerateMipmap(GL_TEXTURE_2D);
    //}
    //else
    //{
    //    std::cout << "Failed to load texture" << std::endl;
    //}
    //stbi_image_free(data);

    //glBindTexture(GL_TEXTURE_2D, texture);

    //// render container
    //ourShader.use();
    //glBindVertexArray(VAO);


    sceneMeshCollection->push_back(plane);
    sceneBoundingBox->Update((&plane)->GetTransformedPoints());

    FileReader reader = FileReader("./Assets/Models/Jinx.obj");
    reader.Load();
    for (int i = 0; i < reader.Meshes().size(); i++)
    {
        Mesh dragonMesh = reader.Meshes()[i];
        MeshRenderer dragon =
            MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
                &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

        sceneMeshCollection->push_back(dragon);
        sceneBoundingBox->Update((&dragon)->GetTransformedPoints());
    }

}

void LoadScene_Engine(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);
    sceneBoundingBox->Update((&plane)->GetTransformedPoints());

    FileReader reader = FileReader("./Assets/Models/Engine.obj");
    reader.Load();
    for (int i = 0; i < reader.Meshes().size(); i++)
    {
        Mesh dragonMesh = reader.Meshes()[i];
        MeshRenderer dragon =
            MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0, glm::vec3(1, 0, 0), glm::vec3(0.6, 0.6, 0.6),
                &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::PureWhite);

        sceneMeshCollection->push_back(dragon);
        sceneBoundingBox->Update((&dragon)->GetTransformedPoints());
    }

}

void LoadScene_AoTest(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{

    FileReader reader = FileReader("./Assets/Models/aoTest.obj");
    reader.Load();
    Mesh dragonMesh = reader.Meshes()[0];
    MeshRenderer dragon =
        MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0, glm::vec3(1, 0, 0), glm::vec3(1, 1, 1),
            &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(dragon);



    for (int i = 0; i < sceneMeshCollection->size(); i++)
    {
        MeshRenderer* mr = &(sceneMeshCollection->data()[i]);
        sceneBoundingBox->Update(mr->GetTransformedPoints());
    }
}

void LoadScene_Porsche(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    Mesh planeMesh = Mesh::Box(1, 1, 1);
    MeshRenderer plane =
        MeshRenderer(glm::vec3(-4, -4, -0.25), 0.0, glm::vec3(0, 0, 1), glm::vec3(8, 8, 0.25),
            &planeMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::MatteGray);

    sceneMeshCollection->push_back(plane);
    sceneBoundingBox->Update((&plane)->GetTransformedPoints());

    FileReader reader = FileReader("./Assets/Models/Porsche911.obj");
    reader.Load();
    for (int i = 0; i < reader.Meshes().size(); i++)
    {
        Mesh dragonMesh = reader.Meshes()[i];
        MeshRenderer dragon =
            MeshRenderer(glm::vec3(0, 0, 0), glm::pi<float>() * 0, glm::vec3(1, 0, 0), glm::vec3(0.6, 0.6, 0.6),
                &dragonMesh, &(*shadersCollection)["LIT_WITH_SHADOWS_SSAO"], &(*shadersCollection)["LIT_WITH_SSAO"], MaterialsCollection::PureWhite);

        sceneMeshCollection->push_back(dragon);
        sceneBoundingBox->Update((&dragon)->GetTransformedPoints());
    }

}

void SetupScene(std::vector<MeshRenderer>* sceneMeshCollection, std::map<std::string, ShaderBase>* shadersCollection, BoundingBox* sceneBoundingBox)
{
    //LoadScene_ALotOfMonkeys(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Primitives(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Cadillac(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Dragon(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Nefertiti(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Bunny(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    LoadScene_Jinx(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Engine(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Knob(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_AoTest(sceneMeshCollection, shadersCollection, sceneBoundingBox);
    //LoadScene_Porsche(sceneMeshCollection, shadersCollection, sceneBoundingBox);

}

std::vector<std::vector<int>> ComputePascalOddRows(int maxLevel)
{
    std::vector<std::vector<int>> triangle;
    std::vector<int> firstRow;

    firstRow.push_back(1);
    for (int j = 1; j < maxLevel; j++)
    {
        firstRow.push_back(0);
    }

    triangle.push_back(firstRow);

    for (int l = 1; l < maxLevel; l++)
    {
        std::vector<int> row;
        for (int j = 0; j <= l; j++)
        {
            int
                k, km1, kp1;
            k = km1 = kp1 = 0;

            if (j < l)
                k = triangle.at(l - 1).at(j);
            if (j < l - 1)
                kp1 = triangle.at(l - 1).at(j + 1);
            if (j > 0)
                km1 = triangle.at(l - 1).at(j - 1);
            else if (l > 1)
                km1 = triangle.at(l - 1).at(j + 1);

            int item = k + kp1;
            item += k + km1;

            row.push_back(item);
        }
        for (int j = l + 1; j < maxLevel; j++)
        {
            row.push_back(0);
        }

        triangle.push_back(row);
    }

    return triangle;
}

// https://stackoverflow.com/questions/17294629/merging-flattening-sub-vectors-into-a-single-vector-c-converting-2d-to-1d
template <typename T>
std::vector<T> flatten(const std::vector<std::vector<T>>& v) {
    std::size_t total_size = 0;
    for (const auto& sub : v)
        total_size += sub.size(); // I wish there was a transform_accumulate
    std::vector<T> result;
    result.reserve(total_size);
    for (const auto& sub : v)
        result.insert(result.end(), sub.begin(), sub.end());
    return result;

}

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}


std::map<std::string, ShaderBase> InitializeShaders()
{
    BasicShader basicLit(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_LIGHTS",
            "DEFS_MATERIAL",
            "CALC_LIT_MAT"
            }
    ));

    BasicShader basicLit_withShadows(
        std::vector<std::string>(
            {
            "DEFS_SHADOWS",
            "CALC_SHADOWS"
            }
            ),
        std::vector<std::string>(
            {
            "DEFS_LIGHTS",
            "DEFS_MATERIAL",
            "DEFS_SHADOWS",
            "CALC_LIT_MAT",
            "CALC_SHADOWS",
            }
    ));

    BasicShader basicLit_withShadows_ssao(
        std::vector<std::string>(
            {
            "DEFS_SHADOWS",
            "CALC_SHADOWS",
            }
            ),
        std::vector<std::string>(
            {
            "DEFS_LIGHTS",
            "DEFS_MATERIAL",
            "DEFS_SHADOWS",
            "DEFS_SSAO",
            "CALC_LIT_MAT",
            "CALC_SHADOWS",
            "CALC_SSAO",
            }
    ));

    BasicShader basicLit_withSsao(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_LIGHTS",
            "DEFS_MATERIAL",
            "DEFS_SSAO",
            "CALC_LIT_MAT",
            "CALC_SSAO",
            }
    ));

    BasicShader basicUnlit(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_MATERIAL",
            "CALC_UNLIT_MAT"
            }
    ));

    BasicShader viewNormals(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_NORMALS",
            "CALC_NORMALS"
            }
    ));

    return
    {

        { "LIT",                    basicLit                   },
        { "LIT_WITH_SSAO",          basicLit_withSsao          },
        { "UNLIT",                  basicUnlit                 },
        { "LIT_WITH_SHADOWS",       basicLit_withShadows       },
        { "LIT_WITH_SHADOWS_SSAO",  basicLit_withShadows_ssao  },
        { "VIEWNORMALS",            viewNormals                }

    };
}

std::map<std::string, ShaderBase> InitializePostProcessingShaders()
{
    PostProcessingShader hbaoShader(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_SSAO",
            "CALC_HBAO"
            }
    ));

    PostProcessingShader ssaoShader(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_SSAO",
            "CALC_SSAO"
            }
    ));

    PostProcessingShader ssaoViewPos(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_SSAO",
            "CALC_POSITIONS"
            }
    ));

    PostProcessingShader blur(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_BLUR",
            "CALC_BLUR"
            }
    ));
    PostProcessingShader gaussianBlur(
        std::vector<std::string>({/* NO VERTEX_SHADER EXPANSIONS */ }),
        std::vector<std::string>(
            {
            "DEFS_GAUSSIAN_BLUR",
            "CALC_GAUSSIAN_BLUR"
            }
    ));


    return
    {

       { "SSAO",                ssaoShader            },
       { "HBAO",                hbaoShader            },
       { "SSAO_VIEWPOS",        ssaoViewPos           },
       { "BLUR",                blur                  },
       { "GAUSSIAN_BLUR",       gaussianBlur          },
    };
}



unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}



int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "ESIEE_OpenGL", NULL, NULL);


    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);


    static std::map<std::string, ShaderBase> Shaders = InitializeShaders();

    static std::map<std::string, ShaderBase> PostProcessingShaders = InitializePostProcessingShaders();

    glEnable(GL_DEPTH_TEST);

    int width = 800;
    int height = 800;

    glViewport(0, 0, 800, 800);

    // Scene View and Projection matrices
    glm::mat4 view = glm::mat4(1);
    float near = 0.1f, far = 50.0f;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), height / (float)width, near, far);

    // Shadow map View and Projection matrices
    glm::mat4 viewShadow, projShadow;

    std::vector<std::string> faces{
        "./Assets/Skybox/posx.jpg",
        "./Assets/Skybox/negx.jpg",
        "./Assets/Skybox/posy.jpg",
        "./Assets/Skybox/negy.jpg",
        "./Assets/Skybox/posz.jpg",
        "./Assets/Skybox/negz.jpg"
    };

    // Skybox first
    unsigned int cubemapTexture = loadCubemap(faces);

    Shader skyboxShader("skybox.vs", "skybox.fs");

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    for (int i = 0; i < 108; i++) {
        skyboxVertices[i] *= 50;
    }

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glDepthMask(GL_FALSE);


    std::vector<MeshRenderer> sceneMeshCollection;
    sceneBB =
        BoundingBox(std::vector<glm::vec3>{});

    SetupScene(&sceneMeshCollection, &Shaders, &sceneBB);

    camera.CameraOrigin = sceneBB.Center();
    camera.ProcessMouseMovement(0, 0);

    // DEBUG
    MeshRenderer lightMesh =
        MeshRenderer(glm::vec3(0, 0, 0), 0, glm::vec3(0, 1, 1), glm::vec3(0.3, 0.3, 0.3),
            &Mesh::Arrow(0.3, 1, 0.5, 0.5, 16), &Shaders["LIT"], &Shaders["LIT"], MaterialsCollection::PureWhite);

    LinesRenderer grid =
        LinesRenderer(glm::vec3(0, 0, 0), 0.0f, glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), Wire::Grid(glm::vec2(-5, -5), glm::vec2(5, 5), 1.0f),
            &Shaders["LIT"], glm::vec4(0.5, 0.5, 0.5, 1.0));


    std::vector<glm::vec3> bboxLines = sceneBB.GetLines();
    Wire bbWire = Wire(bboxLines.data(), bboxLines.size());
    LinesRenderer bbRenderer =
        LinesRenderer(glm::vec3(0, 0, 0), 0.0f, glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), bbWire,
            &Shaders["LIT"], glm::vec4(1.0, 0.0, 1.0, 1.0));

    bool showWindow = true;

    //Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    //Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    //Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Lights
    sceneParams.sceneLights.Ambient.Ambient = glm::vec4(1, 1, 1, 0.6);
    sceneParams.sceneLights.Ambient.aoRadius = 0.2;
    sceneParams.sceneLights.Ambient.aoSamples = 16;
    sceneParams.sceneLights.Ambient.aoSteps = 16;
    sceneParams.sceneLights.Ambient.aoBlurAmount = 3;
    sceneParams.sceneLights.Directional.Direction = glm::vec3(1, 1, -1);
    sceneParams.sceneLights.Directional.Diffuse = glm::vec4(1.0, 1.0, 1.0, 0.75);
    sceneParams.sceneLights.Directional.Specular = glm::vec4(1.0, 1.0, 1.0, 0.75);

    // ShadowMap
    FrameBuffer shadowFBO = FrameBuffer(shadowMapResolution, shadowMapResolution, false, 0, true);

    // SSAO
    FrameBuffer ssaoFBO = FrameBuffer(width, height, true, 2, true);

    // ssao random rotation texture
    unsigned int ssaoNoiseTexture;
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(
            distribution(generator),
            distribution(generator),
            0.0f);
        ssaoNoise.push_back(noise);
    }
    glGenTextures(1, &ssaoNoiseTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Gaussian kernel values
    // https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
    std::vector<std::vector<int>> pascalValues = ComputePascalOddRows(SSAO_BLUR_MAX_RADIUS);
    std::vector<int> flattenedPascalValues = flatten(pascalValues);

    unsigned int gaussianKernelValuesTexture = 0;
    glGenTextures(1, &gaussianKernelValuesTexture);
    glBindTexture(GL_TEXTURE_2D, gaussianKernelValuesTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, SSAO_BLUR_MAX_RADIUS, SSAO_BLUR_MAX_RADIUS, 0, GL_RED_INTEGER, GL_INT, &flattenedPascalValues[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ssao sample vectors
    std::vector<glm::vec3> ssaoSamples;
    for (unsigned int i = 0; i < SSAO_MAX_SAMPLES; i++)
    {
        glm::vec3 noise(
            distribution(generator),
            distribution(generator),
            distribution(generator) * 0.35f + 0.65f);
        noise = glm::normalize(noise);

        float x = distribution(generator) * 0.5f + 0.5f;
        noise *= Lerp(0.1, 1.0, x * x);
        ssaoSamples.push_back(noise);
    }

    // FullScreen Quad....all nice and hardCoded...TODO: improve code
    unsigned int ppQuad_vbo = 0;
    unsigned int ppQuad_vao = 0;
    glGenVertexArrays(1, &ppQuad_vao);
    glGenBuffers(1, &ppQuad_vbo);
    glBindVertexArray(ppQuad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ppQuad_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), Utils::fullScreenQuad_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    MeshRenderer::CheckOGLErrors();

    //this is the render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwMakeContextCurrent(window);

        // Input procesing
        processInput(window);

        glfwPollEvents();

        if (showWindow)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        glfwGetFramebufferSize(window, &width, &height);

        // SHADOW PASS ////////////////////////////////////////////////////////////////////////////////////////////////

        Utils::GetShadowMatrices(sceneParams.sceneLights.Directional.Position, sceneParams.sceneLights.Directional.Direction, sceneBB.GetPoints(), viewShadow, projShadow);
        sceneParams.sceneLights.Directional.LightSpaceMatrix = projShadow * viewShadow;
        sceneParams.sceneLights.Directional.Position = sceneBB.Center() - (glm::normalize(sceneParams.sceneLights.Directional.Direction) * sceneBB.Size() * 0.5f);

        if (sceneParams.drawParams.doShadows)
        {
            shadowFBO.Bind(true, true);

            glViewport(0, 0, shadowMapResolution, shadowMapResolution);
            glClear(GL_DEPTH_BUFFER_BIT);

            MeshRenderer::CheckOGLErrors();

            for (MeshRenderer mr : sceneMeshCollection)
            {
                // TODO: DrawForShadows()
                mr.Draw(viewShadow, projShadow, camera.Position, sceneParams);
            }
            sceneParams.sceneLights.Directional.ShadowMapId = shadowFBO.DepthTextureId();
            shadowFBO.Unbind();
        }

        // SSAO PASS ////////////////////////////////////////////////////////////////////////////////////////////////
        if (ssaoFBO.Height() != height || ssaoFBO.Width() != width)
        {
            ssaoFBO.FreeUnmanagedResources();
            ssaoFBO = FrameBuffer(width, height, true, 2, true);
        }

        ssaoFBO.Bind(true, true);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + 1);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        view = camera.GetViewMatrix();
        Utils::GetTightNearFar(sceneBB.GetPoints(), view, near, far);
        near = 0.1f;
        proj = glm::perspective(glm::radians(fov), width / (float)height, 0.1f, far); //TODO: near is brutally hard coded in PCSS calculations (#define NEAR 0.1)

        for (MeshRenderer mr : sceneMeshCollection)
        {
            //mr.Draw(view, proj, camera.Position, lights);
            mr.DrawCustom(view, proj, &Shaders["VIEWNORMALS"]);
        }

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        ssaoFBO.Unbind();

        // Extract view positions from depth
        ssaoFBO.Bind(false, true);
        glDepthMask(GL_FALSE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoFBO.DepthTextureId());
        glUseProgram((&PostProcessingShaders["SSAO_VIEWPOS"])->ShaderCodeId());
        glUniform1i((&PostProcessingShaders["SSAO_VIEWPOS"])->UniformLocation("u_depthTexture"), 0);
        glUniform1f((&PostProcessingShaders["SSAO_VIEWPOS"])->UniformLocation("u_near"), near);
        glUniform1f((&PostProcessingShaders["SSAO_VIEWPOS"])->UniformLocation("u_far"), far);
        glUniformMatrix4fv((&PostProcessingShaders["SSAO_VIEWPOS"])->UniformLocation("u_proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glBindVertexArray(ppQuad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        ssaoFBO.Unbind();

        // Compute SSAO
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoFBO.ColorTextureId());     // eye fragment positions => 0
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ssaoFBO.ColorTextureId(1));    // normals                => 1
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);             // random rotation        => 2


        const char* aoType = AOShaderFromItem(ao_comboBox_current_item);

        glUseProgram((&PostProcessingShaders[aoType])->ShaderCodeId());
        glUniform1f((&PostProcessingShaders[aoType])->UniformLocation("u_ssao_radius"), sceneParams.sceneLights.Ambient.aoRadius);
        glUniform1i((&PostProcessingShaders[aoType])->UniformLocation("u_viewPosTexture"), 0);
        glUniform1i((&PostProcessingShaders[aoType])->UniformLocation("u_viewNormalsTexture"), 1);
        glUniform1i((&PostProcessingShaders[aoType])->UniformLocation("u_rotVecs"), 2);
        glUniform3fv((&PostProcessingShaders[aoType])->UniformLocation("u_rays"), sceneParams.sceneLights.Ambient.aoSamples, (float*)&ssaoSamples[0]);
        glUniform1i((&PostProcessingShaders[aoType])->UniformLocation("u_numSamples"), sceneParams.sceneLights.Ambient.aoSamples);
        glUniform1i((&PostProcessingShaders[aoType])->UniformLocation("u_numSteps"), sceneParams.sceneLights.Ambient.aoSteps);
        glUniform1f((&PostProcessingShaders[aoType])->UniformLocation("u_radius"), sceneParams.sceneLights.Ambient.aoRadius);
        glUniform1f((&PostProcessingShaders[aoType])->UniformLocation("u_near"), near);
        glUniform1f((&PostProcessingShaders[aoType])->UniformLocation("u_far"), far);
        glUniformMatrix4fv((&PostProcessingShaders[aoType])->UniformLocation("u_proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glBindVertexArray(ppQuad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Blur pass
        ssaoFBO.CopyFromOtherFbo(0, true, 0, false, glm::vec2(0.0, 0.0), glm::vec2(width, height));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoFBO.ColorTextureId());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gaussianKernelValuesTexture);
        glUseProgram((&PostProcessingShaders["GAUSSIAN_BLUR"])->ShaderCodeId());
        glUniform1i((&PostProcessingShaders["GAUSSIAN_BLUR"])->UniformLocation("u_texture"), 0);
        glUniform1i((&PostProcessingShaders["GAUSSIAN_BLUR"])->UniformLocation("u_weights_texture"), 1);
        glUniform1i((&PostProcessingShaders["GAUSSIAN_BLUR"])->UniformLocation("u_radius"), sceneParams.sceneLights.Ambient.aoBlurAmount);

        glUniform1i((&PostProcessingShaders["GAUSSIAN_BLUR"])->UniformLocation("u_hor"), 1); // => HORIZONTAL PASS
        glBindVertexArray(ppQuad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ssaoFBO.CopyFromOtherFbo(0, true, 0, false, glm::vec2(0.0, 0.0), glm::vec2(width, height));

        glUniform1i((&PostProcessingShaders["GAUSSIAN_BLUR"])->UniformLocation("u_hor"), 0); // => VERTICAL PASS
        glBindVertexArray(ppQuad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDepthMask(GL_TRUE);
        ssaoFBO.CopyFromOtherFbo(0, true, 0, false, glm::vec2(0.0, 0.0), glm::vec2(width, height));

        sceneParams.sceneLights.Ambient.AoMapId = ssaoFBO.ColorTextureId();

        // OPAQUE PASS /////////////////////////////////////////////////////////////////////////////////////////////////////
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (showAO)
            ssaoFBO.CopyToOtherFbo(0, true, 10, false, glm::vec2(0.0, 0.0), glm::vec2(width, height));
        else
        {
            for (MeshRenderer mr : sceneMeshCollection)
            {
                mr.Draw(view, proj, camera.Position, sceneParams);
            }

            glDepthFunc(GL_LEQUAL);

            skyboxShader.use();

            skyboxShader.setInt("skybox", 0);
            skyboxShader.setMat4("view", camera.GetViewMatrix());
            skyboxShader.setMat4("projection", proj);
            skyboxShader.setInt("skybox", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            glDepthFunc(GL_LESS);
        }



        // UI /////////////////////////////////////////////////////////////////////////////////////////////////////
        if (showLights)
        {
            glm::quat orientation = glm::quatLookAt(-normalize(sceneParams.sceneLights.Directional.Direction), glm::vec3(0, 0, 1));
            lightMesh.Transform(sceneParams.sceneLights.Directional.Position, glm::angle(orientation), glm::axis(orientation), glm::vec3(0.3, 0.3, 0.3), false);
            lightMesh.Draw(camera.GetViewMatrix(), proj, camera.Position, sceneParams);
        }
        if (showGrid)
            grid.Draw(camera.GetViewMatrix(), proj, camera.Position, sceneParams.sceneLights);
        if (showBoundingBox)
            bbRenderer.Draw(camera.GetViewMatrix(), proj, camera.Position, sceneParams.sceneLights);


        // WINDOW /////////////////////////////////////////////////////////////////////////////////////////////////////
        if (showWindow)
        {
            ShowImGUIWindow();

        }

        // Rendering
        if (showWindow)
        {
            ImGui::Render();
            int display_w, display_h;
            //glfwGetFramebufferSize(window, &display_w, &display_h);
            //glViewport(0, 0, display_w, display_h);
            //glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        // Front and back buffers swapping
        glfwSwapBuffers(window);
    }

    glfwTerminate();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

}

void ResizeResources(int width, int height)
{
    //ssaoFBO.FreeUnmanagedResources();
    //ssaoFBO = FrameBuffer(width, height, true, true);
}

void framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;
    ResizeResources(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

    if (!firstMouse)
    {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;
        camera.ProcessMouseMovement(xoffset, yoffset);
    }

}
void mouse_scroll_callback(GLFWwindow* window, double xpos, double ypos)
{

    camera.ProcessMouseScroll(ypos);

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        firstMouse = true;
        camera.Reset();
    }
}


