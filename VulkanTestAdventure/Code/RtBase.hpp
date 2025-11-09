#pragma once
#include "Application.hpp"
#include "resource_allocator.hpp"
#include "staging.hpp"
#include "sampler_pool.hpp"
#include "gbuffers.hpp"
#include "slang.hpp"
#include "camera_manipulator.hpp"
#include "graphics_pipeline.hpp"
#include "descriptors.hpp"
#include "../Common/gltf_utils.hpp"
#include "sky.hpp"
#include "tonemapper.hpp"
#include "formats.hpp"
#include "utils.hpp"
#include "shaderio.h"
#include "default_structs.hpp"

#include "sky_simple.slang.h"
#include "tonemapper.slang.h"
#include "rtbasic.slang.h"
#include "foundation.slang.h"

namespace vk_test {
    //---------------------------------------------------------------------------------------
    // Ray Tracing Tutorial
    //
    // This is the base class before starting the ray tracing tutorial.
    // It shows the rasterizer rendering of a scene with a teapot and a plane.
    // The tutorial is starting from this class, and will add the ray tracing rendering.
    //
    class RtBasic : public vk_test::IAppElement {
        // Type of GBuffers
        enum {
            eImgRendered,
            eImgTonemapped
        };

    public:
        RtBasic()           = default;
        ~RtBasic() override = default;

        //-------------------------------------------------------------------------------
        // Create the what is needed
        // - Called when the application initialize
        void onAttach(Application* app) override {
            m_App = app;

            // Initialize the VMA allocator
            VmaAllocatorCreateInfo allocator_info = {
                .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                .physicalDevice   = app->getPhysicalDevice(),
                .device           = app->getDevice(),
                .instance         = app->getInstance(),
                .vulkanApiVersion = VK_API_VERSION_1_4,
            };
            m_Allocator.init(allocator_info);

            // m_Allocator.setLeakID(14);  // Set a leak ID for the allocator to track memory leaks

            // The VMA allocator is used for all allocations, the staging uploader will use it for staging buffers and images
            m_StagingUploader.init(&m_Allocator, true);

            // Setting up the Slang compiler for hot reload shader
            m_SlangCompiler.addSearchPaths({ PATH.getShadersPath() });
            m_SlangCompiler.defaultTarget();
            m_SlangCompiler.defaultOptions();
            m_SlangCompiler.addOption({ slang::CompilerOptionName::DebugInformation,
                                        { slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_MAXIMAL } });
#if defined(AFTERMATH_AVAILABLE)
            // This aftermath callback is used to report the shader hash (Spirv) to the Aftermath library.
            m_slangCompiler.setCompileCallback([&](const std::filesystem::path& sourceFile, const uint32_t* spirvCode, size_t spirvSize) {
                std::span<const uint32_t> data(spirvCode, spirvSize / sizeof(uint32_t));
                AftermathCrashTracker::getInstance().addShaderBinary(data);
            });
#endif

            // Acquiring the texture sampler which will be used for displaying the GBuffer
            m_SamplerPool.init(app->getDevice());
            VkSampler linear_sampler{};
            m_SamplerPool.acquireSampler(linear_sampler);

            // Create the G-Buffers
            GBufferInitInfo g_buffer_init{
                .allocator       = &m_Allocator,
                .color_formats   = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM }, // Render target, tonemapped
                .depth_format    = findDepthFormat(m_App->getPhysicalDevice()),
                .image_sampler   = linear_sampler,
                .descriptor_pool = m_App->getTextureDescriptorPool(),
            };
            m_GBuffers.init(g_buffer_init);

            createScene();                       // Create the scene with a teapot and a plane
            createGraphicsDescriptorSetLayout(); // Create the descriptor set layout for the graphics pipeline
            createGraphicsPipelineLayout();      // Create the graphics pipeline layout
            compileAndCreateGraphicsShaders();   // Compile the graphics shaders and create the shader modules
            updateTextures();                    // Update the textures in the descriptor set (if any)

            // Initialize the Sky with the pre-compiled shader
            m_SkySimple.init(&m_Allocator, std::span(sky_simple_slang));

            // Initialize the tonemapper also with proe-compiled shader
            m_Tonemapper.init(&m_Allocator, std::span(tonemapper_slang));

            // Get ray tracing properties
            VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            m_RtProperties.pNext = &m_AsProperties;
            prop2.pNext          = &m_RtProperties;
            vkGetPhysicalDeviceProperties2(m_App->getPhysicalDevice(), &prop2);

            // Set up acceleration structure infrastructure
            createBottomLevelAS(); // Set up BLAS infrastructure
            createTopLevelAS();    // Set up TLAS infrastructure

            // Set up ray tracing pipeline infrastructure
            createRaytraceDescriptorLayout(); // Create descriptor layout
            createRayTracingPipeline();       // Create pipeline structure and SBT
        }

        //-------------------------------------------------------------------------------
        // Destroy all elements that were created
        // - Called when the application is shutting down
        //
        void onDetach() override {
            vkQueueWaitIdle(m_App->getQueue(0).queue);

            VkDevice device = m_App->getDevice();

            m_DescPack.deinit();
            vkDestroyPipelineLayout(device, m_GraphicPipelineLayout, nullptr);
            vkDestroyShaderEXT(device, m_VertexShader, nullptr);
            vkDestroyShaderEXT(device, m_FragmentShader, nullptr);

            m_Allocator.destroyBuffer(m_SceneResource.b_scene_info);
            m_Allocator.destroyBuffer(m_SceneResource.b_meshes);
            m_Allocator.destroyBuffer(m_SceneResource.b_materials);
            m_Allocator.destroyBuffer(m_SceneResource.b_instances);
            for (auto& gltf_data : m_SceneResource.b_gltf_datas) {
                m_Allocator.destroyBuffer(gltf_data);
            }
            for (auto& texture : m_Textures) {
                m_Allocator.destroyImage(texture);
            }

            m_GBuffers.deinit();
            m_StagingUploader.deinit();
            m_SkySimple.deinit();
            m_Tonemapper.deinit();
            m_SamplerPool.deinit();

            // Cleanup acceleration structures
            for (auto& blas : m_BlasAccel) {
                m_Allocator.destroyAcceleration(blas);
            }
            m_Allocator.destroyAcceleration(m_TlasAccel);
            vkDestroyPipelineLayout(device, m_RtPipelineLayout, nullptr);
            vkDestroyPipeline(device, m_RtPipeline, nullptr);
            m_RtDescPack.deinit();
            m_Allocator.destroyBuffer(m_SbtBuffer);

            m_Allocator.deinit();
        }

        //---------------------------------------------------------------------------------------------------------------
        // Rendering all UI elements, this includes the image of the GBuffer, the camera controls, and the sky parameters.
        // - Called every frame
        void onUIRender() override {
            //namespace PE = nvgui::PropertyEditor;
            //// Display the rendering GBuffer in the ImGui window ("Viewport")
            //if (ImGui::Begin("Viewport")) {
            //    ImGui::Image(ImTextureID(m_GBuffers.getDescriptorSet(eImgTonemapped)), ImGui::GetContentRegionAvail());
            //}
            //ImGui::End();

            //// Setting panel
            //if (ImGui::Begin("Settings")) {
            //    // Ray tracing toggle
            //    ImGui::Checkbox("Use Ray Tracing", &m_UseRayTracing);

            //    if (ImGui::CollapsingHeader("Camera")) {
            //        nvgui::CameraWidget(m_camera_manip);
            //    }
            //    if (ImGui::CollapsingHeader("Environment")) {
            //        ImGui::Checkbox("Use Sky", (bool*) &m_SceneResource.scene_info.use_sky);
            //        if (m_SceneResource.scene_info.use_sky) {
            //            nvgui::skySimpleParametersUI(m_SceneResource.scene_info.sky_simple_param);
            //        }
            //        else {
            //            PE::begin();
            //            PE::ColorEdit3("Background", (float*) &m_SceneResource.scene_info.background_color);
            //            PE::end();
            //            // Light
            //            PE::begin();
            //            if (m_SceneResource.scene_info.punctualLights[0].type == shaderio::GltfLightType::ePoint || m_SceneResource.scene_info.punctualLights[0].type == shaderio::GltfLightType::eSpot) {
            //                PE::DragFloat3("Light Position", glm::value_ptr(m_SceneResource.scene_info.punctualLights[0].position), 1.0f, -20.0f, 20.0f, "%.2f", ImGuiSliderFlags_None, "Position of the light");
            //            }
            //            if (m_SceneResource.scene_info.punctualLights[0].type == shaderio::GltfLightType::eDirectional || m_SceneResource.scene_info.punctualLights[0].type == shaderio::GltfLightType::eSpot) {
            //                PE::SliderFloat3("Light Direction", glm::value_ptr(m_SceneResource.scene_info.punctualLights[0].direction), -1.0f, 1.0f, "%.2f", ImGuiSliderFlags_None, "Direction of the light");
            //            }

            //            PE::SliderFloat("Light Intensity", &m_SceneResource.scene_info.punctualLights[0].intensity, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic, "Intensity of the light");
            //            PE::ColorEdit3("Light Color", glm::value_ptr(m_SceneResource.scene_info.punctualLights[0].color), ImGuiColorEditFlags_NoInputs, "Color of the light");
            //            PE::Combo("Light Type", (int*) &m_SceneResource.scene_info.punctualLights[0].type, "Point\0Spot\0Directional\0", 3, "Type of the light (Point, Spot, Directional)");
            //            if (m_SceneResource.scene_info.punctualLights[0].type == shaderio::GltfLightType::eSpot) {
            //                PE::SliderAngle("Cone Angle", &m_SceneResource.scene_info.punctualLights[0].coneAngle, 0.f, 90.f, "%.2f", ImGuiSliderFlags_AlwaysClamp, "Cone angle of the spot light");
            //            }
            //            PE::end();
            //        }
            //    }
            //    if (ImGui::CollapsingHeader("Tonemapper")) {
            //        nvgui::tonemapperWidget(m_tonemapper_data);
            //    }
            //    ImGui::Separator();
            //    PE::begin();
            //    PE::SliderFloat2("Metallic/Roughness Override", glm::value_ptr(m_metallicRoughnessOverride), -0.01f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp, "Override all material metallic and roughness");
            //    PE::end();
            //}
            //ImGui::End();
        }

        //---------------------------------------------------------------------------------------------------------------
        // When the viewport is resized, the GBuffer must be resized
        // - Called when the Window "viewport is resized
        void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override { m_GBuffers.update(cmd, size); }

        //---------------------------------------------------------------------------------------------------------------
        // Rendering the scene
        // The scene is rendered to a GBuffer and the GBuffer is displayed in the ImGui window.
        // Only the ImGui is rendered to the swapchain image.
        // - Called every frame
        void onRender(VkCommandBuffer cmd) override {
            // Update the scene information buffer, this cannot be done in between dynamic rendering
            updateSceneBuffer(cmd);

            if (m_UseRayTracing) {
                raytraceScene(cmd);
            }
            else {
                rasterScene(cmd);
            }

            postProcess(cmd);
        }

        // Apply post-processing
        void postProcess(VkCommandBuffer cmd) {
            // Default post-processing: tonemapping
            m_Tonemapper.runCompute(cmd, m_GBuffers.getSize(), m_TonemapperData, m_GBuffers.getDescriptorImageInfo(eImgRendered), m_GBuffers.getDescriptorImageInfo(eImgTonemapped));

            // Barrier to make sure the image is ready for been display
            cmdMemoryBarrier(cmd, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        }

        //---------------------------------------------------------------------------------------------------------------
        // This renders the toolbar of the window
        // - Called when the ImGui menu is rendered
        void onUIMenu() override {
            /*bool reload = false;
            if (ImGui::BeginMenu("Tools")) {
                reload |= ImGui::MenuItem("Reload Shaders", "F5");
                ImGui::EndMenu();
            }
            reload |= ImGui::IsKeyPressed(ImGuiKey_F5);
            if (reload) {
                vkQueueWaitIdle(m_App->getQueue(0).queue);

                if (m_UseRayTracing) {
                    createRayTracingPipeline();
                }
                else {
                    compileAndCreateGraphicsShaders();
                }
            }*/
        }

        //---------------------------------------------------------------------------------------------------------------
        // Create the scene for this sample
        // - Load a teapot, a plane and an image.
        // - Create instances for them, assign a material and a transformation
        void createScene() {
            SCOPED_TIMER(__FUNCTION__);

            VkCommandBuffer cmd = m_App->createTempCmdBuffer();

            // Load the GLTF resources
            {
                tinygltf::Model teapot_model =
                    loadGltfResources(findFile("teapot.gltf", { PATH.getResourcesPath() })); // Load the GLTF resources from the file

                tinygltf::Model plane_model =
                    loadGltfResources(findFile("plane.gltf", { PATH.getResourcesPath() })); // Load the GLTF resources from the file

                // Textures
                {
                    std::filesystem::path image_filename = findFile("tiled_floor.png", { PATH.getResourcesPath() });
                    Image                 texture        = loadAndCreateImage(cmd, m_StagingUploader, m_App->getDevice(), image_filename); // Load the image from the file and create a texture from it
                    m_SamplerPool.acquireSampler(texture.descriptor.sampler);
                    m_Textures.emplace_back(texture); // Store the texture in the vector of textures
                }

                // Upload the GLTF resources to the GPU
                {
                    importGltfData(m_SceneResource, teapot_model, m_StagingUploader); // Import the GLTF resources
                    importGltfData(m_SceneResource, plane_model, m_StagingUploader);  // Import the GLTF resources
                }
            }

            m_SceneResource.materials = {
                // Teapot material
                { .base_color_factor = glm::vec4(0.8F, 1.0F, 0.6F, 1.0F), .metallic_factor = 0.5F, .roughness_factor = 0.5F },
                // Plane material with texture
                { .base_color_factor = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F), .metallic_factor = 0.1F, .roughness_factor = 0.8F, .base_color_texture_index = 1 }
            };

            m_SceneResource.instances = {
                // Teapot
                { .transform      = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(0.5F)),
                  .material_index = 0,
                  .mesh_index     = 0 },
                // Plane
                { .transform = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -0.9F, 0)), glm::vec3(2.F)), .material_index = 1, .mesh_index = 1 },
            };

            createGltfSceneInfoBuffer(m_SceneResource, m_StagingUploader); // Create buffers for the scene data (GPU buffers)

            m_StagingUploader.cmdUploadAppended(cmd); // Upload the scene information to the GPU

            // Scene information
            shaderio::GltfSceneInfo& scene_info      = m_SceneResource.scene_info;
            scene_info.use_sky                       = 0;                                                                      // Use light
            scene_info.instances                     = (shaderio::GltfInstance*) m_SceneResource.b_instances.address;          // Address of the instance buffer
            scene_info.meshes                        = (shaderio::GltfMesh*) m_SceneResource.b_meshes.address;                 // Address of the mesh buffer
            scene_info.materials                     = (shaderio::GltfMetallicRoughness*) m_SceneResource.b_materials.address; // Address of the material buffer
            scene_info.background_color              = { 0.85F, 0.85F, 0.85F };                                                // The background color
            scene_info.num_lights                    = 1;
            scene_info.punctual_lights[0].color      = glm::vec3(1.0F, 1.0F, 1.0F);
            scene_info.punctual_lights[0].intensity  = 4.0F;
            scene_info.punctual_lights[0].position   = glm::vec3(1.0F, 1.0F, 1.0F); // Position of the light
            scene_info.punctual_lights[0].direction  = glm::vec3(1.0F, 1.0F, 1.0F); // Direction to the light
            scene_info.punctual_lights[0].type       = shaderio::GltfLightType::ePoint;
            scene_info.punctual_lights[0].cone_angle = 0.9F; // Cone angle for spot lights (0 for point and directional lights)

            m_App->submitAndWaitTempCmdBuffer(cmd); // Submit the command buffer to upload the resources

            // Default camera
            m_CameraManip->setClipPlanes({ 0.01F, 100.0F });
            m_CameraManip->setLookat({ 0.0F, 0.5F, 5.0 }, { 0.F, 0.F, 0.F }, { 0.0F, 1.0F, 0.0F });
        }

        //---------------------------------------------------------------------------------------------------------------
        // The Vulkan descriptor set defines the resources that are used by the shaders.
        // Here we add the bindings for the textures.
        void createGraphicsDescriptorSetLayout() {
            DescriptorBindings bindings;
            bindings.addBinding({ .binding         = shaderio::BindingPoints::eTextures,
                                  .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .descriptorCount = 10, // Maximum number of textures used in the scene
                                  .stageFlags      = VK_SHADER_STAGE_ALL },
                                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
            // Creating the descriptor set and set layout from the bindings
            m_DescPack.init(bindings, m_App->getDevice(), 1, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
        }

        //--------------------------------------------------------------------------------------------------
        // The graphic pipeline is all the stages that are used to render a section of the scene.
        // Stages like: vertex shader, fragment shader, rasterization, and blending.
        //
        void createGraphicsPipelineLayout() {
            // Push constant is used to pass data to the shader at each frame
            const VkPushConstantRange push_constant_range{
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset     = 0,
                .size       = sizeof(shaderio::TutoPushConstant)
            };

            // The pipeline layout is used to pass data to the pipeline, anything with "layout" in the shader
            const VkPipelineLayoutCreateInfo pipeline_layout_info{
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount         = 1,
                .pSetLayouts            = m_DescPack.getLayoutPtr(),
                .pushConstantRangeCount = 1,
                .pPushConstantRanges    = &push_constant_range,
            };
            vkCreatePipelineLayout(m_App->getDevice(), &pipeline_layout_info, nullptr, &m_GraphicPipelineLayout);
        }

        //--------------------------------------------------------------------------------------------------
        // Update the textures: this is called when the scene is loaded
        // Textures are updated in the descriptor set (0)
        void updateTextures() {
            if (m_Textures.empty()) {
                return;
            }

            // Update the descriptor set with the textures
            WriteSetContainer    write{};
            VkWriteDescriptorSet all_textures = m_DescPack.makeWrite(shaderio::BindingPoints::eTextures, 0, 1, uint32_t(m_Textures.size()));
            Image*               all_images   = m_Textures.data();
            write.append(all_textures, all_images);
            vkUpdateDescriptorSets(m_App->getDevice(), write.size(), write.data(), 0, nullptr);
        }

        // This function is used to compile the Slang shader, and when it fails, it will use the pre-compiled shaders
        VkShaderModuleCreateInfo compileSlangShader(const std::filesystem::path& filename, const std::span<const uint32_t>& spirv) {
            SCOPED_TIMER(__FUNCTION__);

            // Use pre-compiled shaders by default
            VkShaderModuleCreateInfo shader_code = getShaderModuleCreateInfo(spirv);

            // Try compiling the shader
            std::filesystem::path shader_source = findFile(filename, { PATH.getShadersPath() });
            if (m_SlangCompiler.compileFile(shader_source)) {
                // Using the Slang compiler to compile the shaders
                shader_code.codeSize = m_SlangCompiler.getSpirvSize();
                shader_code.pCode    = m_SlangCompiler.getSpirv();
            }
            else {
                VK_TEST_SAY("Error compiling shaders : " << shader_source.string().c_str() << '\n'
                                                         << m_SlangCompiler.getLastDiagnosticMessage().c_str());
            }
            return shader_code;
        }

        //---------------------------------------------------------------------------------------------------------------
        // Compile the graphics shaders and create the shader modules.
        // This function only creates vertex and fragment shader modules for the graphics pipeline.
        // The actual graphics pipeline is created elsewhere and uses these shader modules.
        // This function will use the pre-compiled shaders if the compilation fails.
        void compileAndCreateGraphicsShaders() {
            SCOPED_TIMER(__FUNCTION__);

            // Use pre-compiled shaders by default
            VkShaderModuleCreateInfo shader_code = compileSlangShader("foundation.slang", foundation_slang);

            // Destroy the previous shaders if they exist
            vkDestroyShaderEXT(m_App->getDevice(), m_VertexShader, nullptr);
            vkDestroyShaderEXT(m_App->getDevice(), m_FragmentShader, nullptr);

            // Push constant is used to pass data to the shader at each frame
            const VkPushConstantRange push_constant_range{
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset     = 0,
                .size       = sizeof(shaderio::TutoPushConstant),
            };

            // Shader create information, this is used to create the shader modules
            VkShaderCreateInfoEXT shader_info{
                .sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
                .codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT,
                .pName                  = "main",
                .setLayoutCount         = 1,
                .pSetLayouts            = m_DescPack.getLayoutPtr(),
                .pushConstantRangeCount = 1,
                .pPushConstantRanges    = &push_constant_range,
            };

            // Vertex Shader
            shader_info.stage     = VK_SHADER_STAGE_VERTEX_BIT;
            shader_info.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_info.pName     = "vertexMain"; // The entry point of the vertex shader
            shader_info.codeSize  = shader_code.codeSize;
            shader_info.pCode     = shader_code.pCode;
            vkCreateShadersEXT(m_App->getDevice(), 1U, &shader_info, nullptr, &m_VertexShader);

            // Fragment Shader
            shader_info.stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_info.nextStage = 0;
            shader_info.pName     = "fragmentMain"; // The entry point of the vertex shader
            shader_info.codeSize  = shader_code.codeSize;
            shader_info.pCode     = shader_code.pCode;
            vkCreateShadersEXT(m_App->getDevice(), 1U, &shader_info, nullptr, &m_FragmentShader);
        }

        //---------------------------------------------------------------------------------------------------------------
        // The update of scene information buffer (UBO)
        //
        void updateSceneBuffer(VkCommandBuffer cmd) {
            const glm::mat4& view_matrix = m_CameraManip->getViewMatrix();
            const glm::mat4& proj_matrix = m_CameraManip->getPerspectiveMatrix();

            m_SceneResource.scene_info.view_proj_matrix = proj_matrix * view_matrix;                                              // Combine the view and projection matrices
            m_SceneResource.scene_info.proj_inv_matrix  = glm::inverse(proj_matrix);                                              // Inverse projection matrix
            m_SceneResource.scene_info.view_inv_matrix  = glm::inverse(view_matrix);                                              // Inverse view matrix
            m_SceneResource.scene_info.camera_position  = m_CameraManip->getEye();                                                // Get the camera position
            m_SceneResource.scene_info.instances        = (shaderio::GltfInstance*) m_SceneResource.b_instances.address;          // Get the address of the instance buffer
            m_SceneResource.scene_info.meshes           = (shaderio::GltfMesh*) m_SceneResource.b_meshes.address;                 // Get the address of the mesh buffer
            m_SceneResource.scene_info.materials        = (shaderio::GltfMetallicRoughness*) m_SceneResource.b_materials.address; // Get the address of the material buffer

            // Making sure the scene information buffer is updated before rendering
            // Wait that the fragment shader is done reading the previous scene information and wait for the transfer to complete
            cmdBufferMemoryBarrier(cmd, { m_SceneResource.b_scene_info.buffer, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT });
            vkCmdUpdateBuffer(cmd, m_SceneResource.b_scene_info.buffer, 0, sizeof(shaderio::GltfSceneInfo), &m_SceneResource.scene_info);
            cmdBufferMemoryBarrier(cmd, { m_SceneResource.b_scene_info.buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT });
        }

        //---------------------------------------------------------------------------------------------------------------
        // Recording the commands to render the scene
        //
        void rasterScene(VkCommandBuffer cmd) {
            // Push constant information, see usage later
            shaderio::TutoPushConstant push_values{
                .sceneInfoAddress          = (shaderio::GltfSceneInfo*) m_SceneResource.b_scene_info.address, // Pass the address of the scene information buffer to the shader
                .metallicRoughnessOverride = m_MetallicRoughnessOverride,                                     // Override the metallic and roughness values
            };
            const VkPushConstantsInfo push_info{
                .sType      = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
                .layout     = m_GraphicPipelineLayout,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset     = 0,
                .size       = sizeof(shaderio::TutoPushConstant),
                .pValues    = &push_values, // Other values are passed later
            };

            // Rendering the Sky
            if (m_SceneResource.scene_info.use_sky != 0) {
                const glm::mat4& view_matrix = m_CameraManip->getViewMatrix();
                const glm::mat4& proj_matrix = m_CameraManip->getPerspectiveMatrix();
                m_SkySimple.runCompute(cmd, m_App->getViewportSize(), view_matrix, proj_matrix, m_SceneResource.scene_info.sky_simple_param, m_GBuffers.getDescriptorImageInfo(eImgRendered));
            }

            // Rendering to the GBuffer
            VkRenderingAttachmentInfo color_attachment = DEFAULT_VkRenderingAttachmentInfo;
            color_attachment.loadOp                    = (m_SceneResource.scene_info.use_sky != 0) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR; // Load the previous content of the GBuffer color attachment (Sky rendering)
            color_attachment.imageView                 = m_GBuffers.getColorImageView(eImgRendered);
            color_attachment.clearValue                = { .color = { { m_SceneResource.scene_info.background_color.x,
                                                                        m_SceneResource.scene_info.background_color.y,
                                                                        m_SceneResource.scene_info.background_color.z,
                                                                        1.0F } } };

            VkRenderingAttachmentInfo depth_attachment = DEFAULT_VkRenderingAttachmentInfo;
            depth_attachment.imageView                 = m_GBuffers.getDepthImageView();
            depth_attachment.clearValue                = { .depthStencil = DEFAULT_VkClearDepthStencilValue };

            // Create the rendering info
            VkRenderingInfo rendering_info      = DEFAULT_VkRenderingInfo;
            rendering_info.renderArea           = DEFAULT_VkRect2D(m_GBuffers.getSize());
            rendering_info.colorAttachmentCount = 1;
            rendering_info.pColorAttachments    = &color_attachment;
            rendering_info.pDepthAttachment     = &depth_attachment;

            // Change the GBuffer layout to prepare for rendering (attachment)
            cmdImageMemoryBarrier(cmd, { m_GBuffers.getColorImage(eImgRendered), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

            // Bind the descriptor sets for the graphics pipeline (making textures available to the shaders)
            const VkBindDescriptorSetsInfo bind_descriptor_sets_info{ .sType              = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
                                                                      .stageFlags         = VK_SHADER_STAGE_ALL_GRAPHICS,
                                                                      .layout             = m_GraphicPipelineLayout,
                                                                      .firstSet           = 0,
                                                                      .descriptorSetCount = 1,
                                                                      .pDescriptorSets    = m_DescPack.getSetPtr() };
            vkCmdBindDescriptorSets2(cmd, &bind_descriptor_sets_info);

            // ** BEGIN RENDERING **
            vkCmdBeginRendering(cmd, &rendering_info);

            // All dynamic states are set here
            m_DynamicPipeline.rasterizationState.cullMode = VK_CULL_MODE_NONE; // Don't cull any triangles (double-sided rendering)
            m_DynamicPipeline.cmdApplyAllStates(cmd);
            vk_test::GraphicsPipelineState::cmdSetViewportAndScissor(cmd, m_App->getViewportSize());
            vkCmdSetDepthTestEnable(cmd, VK_TRUE);

            // Same shader for all meshes
            vk_test::GraphicsPipelineState::cmdBindShaders(cmd, { .vertex = m_VertexShader, .fragment = m_FragmentShader });

            // We don't send vertex attributes, they are pulled in the shader
            VkVertexInputBindingDescription2EXT   binding_description   = {};
            VkVertexInputAttributeDescription2EXT attribute_description = {};
            vkCmdSetVertexInputEXT(cmd, 0, nullptr, 0, nullptr);

            for (size_t i = 0; i < m_SceneResource.instances.size(); i++) {
                uint32_t                      mesh_index = m_SceneResource.instances[i].mesh_index = 0;
                const shaderio::GltfMesh&     gltf_mesh                                            = m_SceneResource.meshes[mesh_index];
                const shaderio::TriangleMesh& tri_mesh                                             = gltf_mesh.tri_mesh;

                // Push constant is information that is passed to the shader at each draw call.
                push_values.normalMatrix  = glm::transpose(glm::inverse(glm::mat3(m_SceneResource.instances[i].transform)));
                push_values.instanceIndex = int(i); // The index of the instance in the m_instances vector
                vkCmdPushConstants2(cmd, &push_info);

                // Get the buffer directly using the pre-computed mapping
                uint32_t      buffer_index = m_SceneResource.mesh_to_buffer_index[mesh_index] = 0;
                const Buffer& v                                                               = m_SceneResource.b_gltf_datas[buffer_index];

                // Bind index buffers
                vkCmdBindIndexBuffer(cmd, v.buffer, tri_mesh.indices.offset, VkIndexType(gltf_mesh.index_type));

                // Draw the mesh
                vkCmdDrawIndexed(cmd, tri_mesh.indices.count, 1, 0, 0, 0); // All indices
            }

            // ** END RENDERING **
            vkCmdEndRendering(cmd);
            cmdImageMemoryBarrier(cmd, { m_GBuffers.getColorImage(eImgRendered), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL });
        }

        void onLastHeadlessFrame() override {
            //m_App->saveImageToFile(m_GBuffers.getColorImage(eImgTonemapped), m_GBuffers.getSize(), getExecutablePath().replace_extension(".jpg").string());
        }

        // Accessor for camera manipulator
        std::shared_ptr<CameraManipulator> getCameraManipulator() const { return m_CameraManip; }

        //--------------------------------------------------------------------------------------------------
        // Converting a PrimitiveMesh as input for BLAS
        //
        static void primitiveToGeometry(const shaderio::GltfMesh&                 gltf_mesh,
                                        VkAccelerationStructureGeometryKHR&       geometry,
                                        VkAccelerationStructureBuildRangeInfoKHR& range_info) {
            const shaderio::TriangleMesh tri_mesh       = gltf_mesh.tri_mesh;
            const auto                   triangle_count = static_cast<uint32_t>(tri_mesh.indices.count / 3U);

            // Describe buffer as array of VertexObj.
            VkAccelerationStructureGeometryTrianglesDataKHR triangles{
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT, // vec3 vertex position data
                .vertexData   = { .deviceAddress = VkDeviceAddress(gltf_mesh.gltf_buffer) + tri_mesh.positions.offset },
                .vertexStride = tri_mesh.positions.byte_stride,
                .maxVertex    = tri_mesh.positions.count - 1,
                .indexType    = VkIndexType(gltf_mesh.index_type), // Index type (VK_INDEX_TYPE_UINT16 or VK_INDEX_TYPE_UINT32)
                .indexData    = { .deviceAddress = VkDeviceAddress(gltf_mesh.gltf_buffer) + tri_mesh.indices.offset },
            };

            // Identify the above data as containing opaque triangles.
            geometry = VkAccelerationStructureGeometryKHR{
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
                .geometry     = { .triangles = triangles },
                .flags        = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR,
            };

            range_info = VkAccelerationStructureBuildRangeInfoKHR{ .primitiveCount = triangle_count };
        }

        //--------------------------------------------------------------------------------------------------
        // Generic function to create an acceleration structure (BLAS or TLAS)
        // Note: This function creates and destroys a scratch buffer for each call.
        // Not optimal but easier to read and understand. See Helper function for a better approach.
        void createAccelerationStructure(VkAccelerationStructureTypeKHR            as_type,             // The type of acceleration structure (BLAS or TLAS)
                                         AccelerationStructure&                    accel_struct,        // The acceleration structure to create
                                         VkAccelerationStructureGeometryKHR&       as_geometry,         // The geometry to build the acceleration structure from
                                         VkAccelerationStructureBuildRangeInfoKHR& as_build_range_info, // The range info for building the acceleration structure
                                         VkBuildAccelerationStructureFlagsKHR      flags                // Build flags (e.g. prefer fast trace)
        ) {
            VkDevice device = m_App->getDevice();

            // Helper function to align a value to a given alignment
            auto align_up = [](auto value, size_t alignment) noexcept { return ((value + alignment - 1) & ~(alignment - 1)); };

            // Fill the build information with the current information, the rest is filled later (scratch buffer and destination AS)
            VkAccelerationStructureBuildGeometryInfoKHR as_build_info{
                .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .type          = as_type,                                        // The type of acceleration structure (BLAS or TLAS)
                .flags         = flags,                                          // Build flags (e.g. prefer fast trace)
                .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, // Build mode vs update
                .geometryCount = 1,                                              // Deal with one geometry at a time
                .pGeometries   = &as_geometry,                                   // The geometry to build the acceleration structure from
            };

            // One geometry at a time (could be multiple)
            std::vector<uint32_t> max_prim_count(1);
            max_prim_count[0] = as_build_range_info.primitiveCount;

            // Find the size of the acceleration structure and the scratch buffer
            VkAccelerationStructureBuildSizesInfoKHR as_build_size{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_build_info, max_prim_count.data(), &as_build_size);

            // Make sure the scratch buffer is properly aligned
            VkDeviceSize scratch_size = align_up(as_build_size.buildScratchSize = 0, m_AsProperties.minAccelerationStructureScratchOffsetAlignment);

            // Create the scratch buffer to store the temporary data for the build
            Buffer scratch_buffer;
            m_Allocator.createBuffer(scratch_buffer, scratch_size, VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VMA_MEMORY_USAGE_AUTO, {}, m_AsProperties.minAccelerationStructureScratchOffsetAlignment);

            // Create the acceleration structure
            VkAccelerationStructureCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
                .size  = as_build_size.accelerationStructureSize, // The size of the acceleration structure
                .type  = as_type,                                 // The type of acceleration structure (BLAS or TLAS)
            };
            m_Allocator.createAcceleration(accel_struct, create_info);

            // Build the acceleration structure
            {
                VkCommandBuffer cmd = m_App->createTempCmdBuffer();

                // Fill with new information for the build,scratch buffer and destination AS
                as_build_info.dstAccelerationStructure  = accel_struct.accel;
                as_build_info.scratchData.deviceAddress = scratch_buffer.address;

                VkAccelerationStructureBuildRangeInfoKHR* p_build_range_info = &as_build_range_info;
                vkCmdBuildAccelerationStructuresKHR(cmd, 1, &as_build_info, &p_build_range_info);

                m_App->submitAndWaitTempCmdBuffer(cmd);
            }
            // Cleanup the scratch buffer
            m_Allocator.destroyBuffer(scratch_buffer);
        }

        //---------------------------------------------------------------------------------------------------------------
        // Create bottom-level acceleration structures
        void createBottomLevelAS() {
            SCOPED_TIMER(__FUNCTION__);

            // Prepare geometry information for all meshes
            m_BlasAccel.resize(m_SceneResource.meshes.size());

            // One BLAS per primitive
            for (uint32_t blas_id = 0; blas_id < m_SceneResource.meshes.size(); blas_id++) {
                VkAccelerationStructureGeometryKHR       as_geometry{};
                VkAccelerationStructureBuildRangeInfoKHR as_build_range_info{};

                // Convert the primitive information to acceleration structure geometry
                primitiveToGeometry(m_SceneResource.meshes[blas_id], as_geometry, as_build_range_info);

                createAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, m_BlasAccel[blas_id], as_geometry, as_build_range_info, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
            }
        }

        //--------------------------------------------------------------------------------------------------
        // Create the top level acceleration structures, referencing all BLAS
        //
        void createTopLevelAS() {
            SCOPED_TIMER(__FUNCTION__);

            // VkTransformMatrixKHR is row-major 3x4, glm::mat4 is column-major; transpose before memcpy.
            auto to_transform_matrix_khr = [](const glm::mat4& m) {
                VkTransformMatrixKHR t;
                memcpy(&t, glm::value_ptr(glm::transpose(m)), sizeof(t));
                return t;
            };

            // First create the instance data for the TLAS
            std::vector<VkAccelerationStructureInstanceKHR> tlas_instances;
            tlas_instances.reserve(m_SceneResource.instances.size());
            for (const shaderio::GltfInstance& instance : m_SceneResource.instances) {
                VkAccelerationStructureInstanceKHR as_instance{};
                as_instance.transform                              = to_transform_matrix_khr(instance.transform);       // Position of the instance
                as_instance.instanceCustomIndex                    = instance.mesh_index;                               // gl_InstanceCustomIndexEXT
                as_instance.accelerationStructureReference         = m_BlasAccel[instance.mesh_index].address;          // Address of the BLAS
                as_instance.instanceShaderBindingTableRecordOffset = 0;                                                 // We will use the same hit group for all objects
                as_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV; // No culling - double sided
                as_instance.mask                                   = 0xFF;
                tlas_instances.emplace_back(as_instance);
            }

            // Then create the buffer with the instance data
            Buffer tlas_instances_buffer;
            {
                VkCommandBuffer cmd = m_App->createTempCmdBuffer();

                // Create the instances buffer and upload the instance data
                m_Allocator.createBuffer(
                    tlas_instances_buffer,
                    std::span<VkAccelerationStructureInstanceKHR const>(tlas_instances).size_bytes(),
                    VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT);
                m_StagingUploader.appendBuffer(tlas_instances_buffer, 0, std::span<VkAccelerationStructureInstanceKHR const>(tlas_instances));
                m_StagingUploader.cmdUploadAppended(cmd);
                m_App->submitAndWaitTempCmdBuffer(cmd);
            }

            // Then create the TLAS geometry
            {
                VkAccelerationStructureGeometryKHR       as_geometry{};
                VkAccelerationStructureBuildRangeInfoKHR as_build_range_info{};

                // Convert the instance information to acceleration structure geometry, similar to primitiveToGeometry()
                VkAccelerationStructureGeometryInstancesDataKHR geometry_instances{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                                                                                    .data  = { .deviceAddress = tlas_instances_buffer.address } };
                as_geometry         = { .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                                        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
                                        .geometry     = { .instances = geometry_instances } };
                as_build_range_info = { .primitiveCount = static_cast<uint32_t>(m_SceneResource.instances.size()) };

                createAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, m_TlasAccel, as_geometry, as_build_range_info, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
            }

            m_Allocator.destroyBuffer(tlas_instances_buffer); // Cleanup
        }

        //--------------------------------------------------------------------------------------------------
        // Create the descriptor set layout for ray tracing
        void createRaytraceDescriptorLayout() {
            SCOPED_TIMER(__FUNCTION__);
            DescriptorBindings bindings;
            bindings.addBinding({ .binding         = shaderio::BindingPoints::eTlas,
                                  .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                                  .descriptorCount = 1,
                                  .stageFlags      = VK_SHADER_STAGE_ALL });
            bindings.addBinding({ .binding         = shaderio::BindingPoints::eOutImage,
                                  .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                  .descriptorCount = 1,
                                  .stageFlags      = VK_SHADER_STAGE_ALL });

            // Creating a PUSH descriptor set and set layout from the bindings
            m_RtDescPack.init(bindings, m_App->getDevice(), 0, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        }

        //--------------------------------------------------------------------------------------------------
        // Create ray tracing pipeline structure
        // We create the entries for ray generation, miss, and closest hit shaders.
        // We also create the shader groups and the pipeline layout.
        // The pipeline is used to execute the ray tracing pipeline.
        // We also create the SBT (Shader Binding Table)
        void createRayTracingPipeline() {
            SCOPED_TIMER(__FUNCTION__);
            // For re-creation
            vkDestroyPipeline(m_App->getDevice(), m_RtPipeline, nullptr);
            vkDestroyPipelineLayout(m_App->getDevice(), m_RtPipelineLayout, nullptr);

            // Creating all shaders
            enum StageIndices {
                eRaygen,
                eMiss,
                eClosestHit,
                eShaderGroupCount
            };
            std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
            for (auto& s : stages) {
                s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            }

            // Compile shader, fallback to pre-compiled
            VkShaderModuleCreateInfo shader_code = compileSlangShader("rtbasic.slang", rtbasic_slang);

            stages[eRaygen].pNext     = &shader_code;
            stages[eRaygen].pName     = "rgenMain";
            stages[eRaygen].stage     = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            stages[eMiss].pNext       = &shader_code;
            stages[eMiss].pName       = "rmissMain";
            stages[eMiss].stage       = VK_SHADER_STAGE_MISS_BIT_KHR;
            stages[eClosestHit].pNext = &shader_code;
            stages[eClosestHit].pName = "rchitMain";
            stages[eClosestHit].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

            // Shader groups
            VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
            group.anyHitShader       = VK_SHADER_UNUSED_KHR;
            group.closestHitShader   = VK_SHADER_UNUSED_KHR;
            group.generalShader      = VK_SHADER_UNUSED_KHR;
            group.intersectionShader = VK_SHADER_UNUSED_KHR;

            std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups;
            // Raygen
            group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            group.generalShader = eRaygen;
            shader_groups.push_back(group);

            // Miss
            group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            group.generalShader = eMiss;
            shader_groups.push_back(group);

            // closest hit shader
            group.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            group.generalShader    = VK_SHADER_UNUSED_KHR;
            group.closestHitShader = eClosestHit;
            shader_groups.push_back(group);

            // Push constant: we want to be able to update constants used by the shaders
            const VkPushConstantRange push_constant{ VK_SHADER_STAGE_ALL, 0, sizeof(shaderio::TutoPushConstant) };

            VkPipelineLayoutCreateInfo pipeline_layout_create_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
            pipeline_layout_create_info.pushConstantRangeCount = 1;
            pipeline_layout_create_info.pPushConstantRanges    = &push_constant;

            // Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
            std::array<VkDescriptorSetLayout, 2> layouts = { { m_DescPack.getLayout(), m_RtDescPack.getLayout() } };
            pipeline_layout_create_info.setLayoutCount   = uint32_t(layouts.size());
            pipeline_layout_create_info.pSetLayouts      = layouts.data();
            vkCreatePipelineLayout(m_App->getDevice(), &pipeline_layout_create_info, nullptr, &m_RtPipelineLayout);

            // Assemble the shader stages and recursion depth info into the ray tracing pipeline
            VkRayTracingPipelineCreateInfoKHR rt_pipeline_info{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
            rt_pipeline_info.stageCount                   = static_cast<uint32_t>(stages.size()); // Stages are shaders
            rt_pipeline_info.pStages                      = stages.data();
            rt_pipeline_info.groupCount                   = static_cast<uint32_t>(shader_groups.size());
            rt_pipeline_info.pGroups                      = shader_groups.data();
            rt_pipeline_info.maxPipelineRayRecursionDepth = std::max(3U, m_RtProperties.maxRayRecursionDepth); // Ray depth
            rt_pipeline_info.layout                       = m_RtPipelineLayout;
            vkCreateRayTracingPipelinesKHR(m_App->getDevice(), {}, {}, 1, &rt_pipeline_info, nullptr, &m_RtPipeline);

            // Create the shader binding table for this pipeline
            createShaderBindingTable(rt_pipeline_info);
        }

        //--------------------------------------------------------------------------------------------------
        // Create the shader binding table
        // The shader binding table is a buffer that contains the shader handles for the ray tracing pipeline,
        // used to identify the shaders for the ray tracing pipeline.
        void createShaderBindingTable(const VkRayTracingPipelineCreateInfoKHR& rt_pipeline_info) {
            SCOPED_TIMER(__FUNCTION__);

            m_Allocator.destroyBuffer(m_SbtBuffer); // Cleanup when re-creating

            VkDevice device      = m_App->getDevice();
            uint32_t handle_size = m_RtProperties.shaderGroupHandleSize = 0;
            uint32_t handle_alignment = m_RtProperties.shaderGroupHandleAlignment = 0;
            uint32_t base_alignment = m_RtProperties.shaderGroupBaseAlignment = 0;
            uint32_t group_count                                              = rt_pipeline_info.groupCount;

            // Get shader group handles
            size_t data_size = handle_size * group_count;
            m_ShaderHandles.resize(data_size);
            vkGetRayTracingShaderGroupHandlesKHR(device, m_RtPipeline, 0, group_count, data_size, m_ShaderHandles.data());

            // Calculate SBT buffer size with proper alignment
            auto     align_up      = [](uint32_t size, uint32_t alignment) { return (size + alignment - 1) & ~(alignment - 1); };
            uint32_t raygen_size   = align_up(handle_size, handle_alignment);
            uint32_t miss_size     = align_up(handle_size, handle_alignment);
            uint32_t hit_size      = align_up(handle_size, handle_alignment);
            uint32_t callable_size = 0; // No callable shaders in this tutorial

            // Ensure each region starts at a baseAlignment boundary
            uint32_t raygen_offset   = 0;
            uint32_t miss_offset     = align_up(raygen_size, base_alignment);
            uint32_t hit_offset      = align_up(miss_offset + miss_size, base_alignment);
            uint32_t callable_offset = align_up(hit_offset + hit_size, base_alignment);

            size_t buffer_size = callable_offset + callable_size;

            // Create SBT buffer
            m_Allocator.createBuffer(m_SbtBuffer, buffer_size, VK_BUFFER_USAGE_2_SHADER_BINDING_TABLE_BIT_KHR, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

            // Populate SBT buffer
            uint8_t* p_data = m_SbtBuffer.mapping = nullptr;

            // Ray generation shader (group 0)
            memcpy(p_data + raygen_offset, m_ShaderHandles.data() + (0 * handle_size), handle_size);
            m_RaygenRegion.deviceAddress = m_SbtBuffer.address + raygen_offset;
            m_RaygenRegion.stride        = raygen_size;
            m_RaygenRegion.size          = raygen_size;

            // Miss shader (group 1)
            memcpy(p_data + miss_offset, m_ShaderHandles.data() + (1 * handle_size), handle_size);
            m_MissRegion.deviceAddress = m_SbtBuffer.address + miss_offset;
            m_MissRegion.stride        = miss_size;
            m_MissRegion.size          = miss_size;

            // Hit shader (group 2)
            memcpy(p_data + hit_offset, m_ShaderHandles.data() + (2 * handle_size), handle_size);
            m_HitRegion.deviceAddress = m_SbtBuffer.address + hit_offset;
            m_HitRegion.stride        = hit_size;
            m_HitRegion.size          = hit_size;

            // Callable shaders (none in this tutorial)
            m_CallableRegion.deviceAddress = 0;
            m_CallableRegion.stride        = 0;
            m_CallableRegion.size          = 0;
        }

        //---------------------------------------------------------------------------------------------------------------
        // Ray tracing rendering method
        void raytraceScene(VkCommandBuffer cmd) {
            // Ray trace pipeline
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RtPipeline);

            // Bind the descriptor sets for the graphics pipeline (making textures available to the shaders)
            const VkBindDescriptorSetsInfo bind_descriptor_sets_info{ .sType              = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
                                                                      .stageFlags         = VK_SHADER_STAGE_ALL,
                                                                      .layout             = m_RtPipelineLayout,
                                                                      .firstSet           = 0,
                                                                      .descriptorSetCount = 1,
                                                                      .pDescriptorSets    = m_DescPack.getSetPtr() };
            vkCmdBindDescriptorSets2(cmd, &bind_descriptor_sets_info);

            // Push descriptor sets for ray tracing
            WriteSetContainer write{};
            write.append(m_RtDescPack.makeWrite(shaderio::BindingPoints::eTlas), m_TlasAccel);
            write.append(m_RtDescPack.makeWrite(shaderio::BindingPoints::eOutImage), m_GBuffers.getColorImageView(eImgRendered), VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RtPipelineLayout, 1, write.size(), write.data());

            // Push constant information
            shaderio::TutoPushConstant push_values{
                .sceneInfoAddress          = (shaderio::GltfSceneInfo*) m_SceneResource.b_scene_info.address,
                .metallicRoughnessOverride = m_MetallicRoughnessOverride,
            };
            const VkPushConstantsInfo push_info{ .sType      = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
                                                 .layout     = m_RtPipelineLayout,
                                                 .stageFlags = VK_SHADER_STAGE_ALL,
                                                 .size       = sizeof(shaderio::TutoPushConstant),
                                                 .pValues    = &push_values };
            vkCmdPushConstants2(cmd, &push_info);

            // Ray trace
            const VkExtent2D& size = m_App->getViewportSize();
            vkCmdTraceRaysKHR(cmd, &m_RaygenRegion, &m_MissRegion, &m_HitRegion, &m_CallableRegion, size.width, size.height, 1);

            // Barrier to make sure the image is ready for Tonemapping
            cmdMemoryBarrier(cmd, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        }

    private:
        // Application and core components
        Application*      m_App{};           // The application framework
        ResourceAllocator m_Allocator;       // Resource allocator for Vulkan resources, used for buffers and images
        StagingUploader   m_StagingUploader; // Utility to upload data to the GPU, used for staging buffers and images
        SamplerPool       m_SamplerPool;     // Texture sampler pool, used to acquire texture samplers for images
        GBuffer           m_GBuffers;        // The G-Buffer
        SlangCompiler     m_SlangCompiler;   // The Slang compiler used to compile the shaders

        // Camera manipulator
        std::shared_ptr<vk_test::CameraManipulator> m_CameraManip{ std::make_shared<vk_test::CameraManipulator>() };

        // Pipeline
        GraphicsPipelineState m_DynamicPipeline;         // The dynamic pipeline state used to set the graphics pipeline state, like viewport, scissor, and depth test
        DescriptorPack        m_DescPack;                // The descriptor bindings used to create the descriptor set layout and descriptor sets
        VkPipelineLayout      m_GraphicPipelineLayout{}; // The pipeline layout use with graphics pipeline

        // Shaders
        VkShaderEXT m_VertexShader{};   // The vertex shader used to render the scene
        VkShaderEXT m_FragmentShader{}; // The fragment shader used to render the scene

        // Scene information buffer (UBO)
        GltfSceneResource  m_SceneResource{}; // The GLTF scene resource, contains all the buffers and data for the scene
        std::vector<Image> m_Textures;        // Textures used in the scene

        SkySimple                m_SkySimple;                                   // Sky rendering
        Tonemapper               m_Tonemapper;                                  // Tonemapper for post-processing effects
        shaderio::TonemapperData m_TonemapperData{};                            // Tonemapper data used to pass parameters to the tonemapper shader
        glm::vec2                m_MetallicRoughnessOverride{ -0.01F, -0.01F }; // Override values for metallic and roughness, used in the UI to control the material properties

        // Ray Tracing Pipeline Components
        DescriptorPack   m_RtDescPack;         // Ray tracing descriptor bindings
        VkPipeline       m_RtPipeline{};       // Ray tracing pipeline
        VkPipelineLayout m_RtPipelineLayout{}; // Ray tracing pipeline layout

        // Acceleration Structure Components
        std::vector<AccelerationStructure> m_BlasAccel;
        AccelerationStructure              m_TlasAccel;

        // Direct SBT management
        Buffer                          m_SbtBuffer;        // Buffer for shader binding table
        std::vector<uint8_t>            m_ShaderHandles;    // Storage for shader group handles
        VkStridedDeviceAddressRegionKHR m_RaygenRegion{};   // Ray generation shader region
        VkStridedDeviceAddressRegionKHR m_MissRegion{};     // Miss shader region
        VkStridedDeviceAddressRegionKHR m_HitRegion{};      // Hit shader region
        VkStridedDeviceAddressRegionKHR m_CallableRegion{}; // Callable shader region

        // Ray Tracing Properties
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR    m_RtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
        VkPhysicalDeviceAccelerationStructurePropertiesKHR m_AsProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };

        // Ray tracing toggle
        bool m_UseRayTracing = true; // Set to true to use ray tracing, false for rasterization
    };
} // namespace vk_test
