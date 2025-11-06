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
#include "gltf_utils.hpp"
#include "sky.hpp"
#include "tonemapper.hpp"

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

            // m_allocator.setLeakID(14);  // Set a leak ID for the allocator to track memory leaks

            // The VMA allocator is used for all allocations, the staging uploader will use it for staging buffers and images
            m_StagingUploader.init(&m_Allocator, true);

            // Setting up the Slang compiler for hot reload shader
            m_SlangCompiler.addSearchPaths(getShaderDirs());
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
            NVVK_CHECK(m_samplerPool.acquireSampler(linear_sampler));
            NVVK_DBG_NAME(linear_sampler);

            // Create the G-Buffers
            GBufferInitInfo g_buffer_init{
                .allocator      = &m_allocator,
                .colorFormats   = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM }, // Render target, tonemapped
                .depthFormat    = findDepthFormat(m_app->getPhysicalDevice()),
                .imageSampler   = linearSampler,
                .descriptorPool = m_app->getTextureDescriptorPool(),
            };
            m_gBuffers.init(gBufferInit);

            createScene();                       // Create the scene with a teapot and a plane
            createGraphicsDescriptorSetLayout(); // Create the descriptor set layout for the graphics pipeline
            createGraphicsPipelineLayout();      // Create the graphics pipeline layout
            compileAndCreateGraphicsShaders();   // Compile the graphics shaders and create the shader modules
            updateTextures();                    // Update the textures in the descriptor set (if any)

            // Initialize the Sky with the pre-compiled shader
            m_skySimple.init(&m_allocator, std::span(sky_simple_slang));

            // Initialize the tonemapper also with proe-compiled shader
            m_tonemapper.init(&m_allocator, std::span(tonemapper_slang));

            // Get ray tracing properties
            VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            m_rtProperties.pNext = &m_asProperties;
            prop2.pNext          = &m_rtProperties;
            vkGetPhysicalDeviceProperties2(m_app->getPhysicalDevice(), &prop2);

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
            NVVK_CHECK(vkQueueWaitIdle(m_app->getQueue(0).queue));

            VkDevice device = m_app->getDevice() = nullptr = nullptr = nullptr;

            m_descPack.deinit();
            vkDestroyPipelineLayout(device, m_graphicPipelineLayout, nullptr);
            vkDestroyShaderEXT(device, m_vertexShader, nullptr);
            vkDestroyShaderEXT(device, m_fragmentShader, nullptr);

            m_allocator.destroyBuffer(m_sceneResource.bSceneInfo);
            m_allocator.destroyBuffer(m_sceneResource.bMeshes);
            m_allocator.destroyBuffer(m_sceneResource.bMaterials);
            m_allocator.destroyBuffer(m_sceneResource.bInstances);
            for (auto& gltfData : m_sceneResource.bGltfDatas) {
                m_allocator.destroyBuffer(gltfData);
            }
            for (auto& texture : m_textures) {
                m_allocator.destroyImage(texture);
            }

            m_gBuffers.deinit();
            m_stagingUploader.deinit();
            m_skySimple.deinit();
            m_tonemapper.deinit();
            m_samplerPool.deinit();

            // Cleanup acceleration structures
            for (auto& blas : m_blasAccel) {
                m_allocator.destroyAcceleration(blas);
            }
            m_allocator.destroyAcceleration(m_tlasAccel);
            vkDestroyPipelineLayout(device, m_rtPipelineLayout, nullptr);
            vkDestroyPipeline(device, m_rtPipeline, nullptr);
            m_rtDescPack.deinit();
            m_allocator.destroyBuffer(m_sbtBuffer);

            m_allocator.deinit();
        }

        //---------------------------------------------------------------------------------------------------------------
        // Rendering all UI elements, this includes the image of the GBuffer, the camera controls, and the sky parameters.
        // - Called every frame
        void onUIRender() override {
            namespace PE = nvgui::PropertyEditor;
            // Display the rendering GBuffer in the ImGui window ("Viewport")
            if (ImGui::Begin("Viewport")) {
                ImGui::Image(ImTextureID(m_gBuffers.getDescriptorSet(eImgTonemapped)), ImGui::GetContentRegionAvail());
            }
            ImGui::End();

            // Setting panel
            if (ImGui::Begin("Settings")) {
                // Ray tracing toggle
                ImGui::Checkbox("Use Ray Tracing", &m_useRayTracing);

                if (ImGui::CollapsingHeader("Camera")) {
                    nvgui::CameraWidget(m_camera_manip);
                }
                if (ImGui::CollapsingHeader("Environment")) {
                    ImGui::Checkbox("Use Sky", (bool*) &m_sceneResource.sceneInfo.useSky);
                    if (m_sceneResource.sceneInfo.useSky) {
                        nvgui::skySimpleParametersUI(m_sceneResource.sceneInfo.skySimpleParam);
                    }
                    else {
                        PE::begin();
                        PE::ColorEdit3("Background", (float*) &m_sceneResource.sceneInfo.backgroundColor);
                        PE::end();
                        // Light
                        PE::begin();
                        if (m_sceneResource.sceneInfo.punctualLights[0].type == shaderio::GltfLightType::ePoint || m_sceneResource.sceneInfo.punctualLights[0].type == shaderio::GltfLightType::eSpot) {
                            PE::DragFloat3("Light Position", glm::value_ptr(m_sceneResource.sceneInfo.punctualLights[0].position), 1.0f, -20.0f, 20.0f, "%.2f", ImGuiSliderFlags_None, "Position of the light");
                        }
                        if (m_sceneResource.sceneInfo.punctualLights[0].type == shaderio::GltfLightType::eDirectional || m_sceneResource.sceneInfo.punctualLights[0].type == shaderio::GltfLightType::eSpot) {
                            PE::SliderFloat3("Light Direction", glm::value_ptr(m_sceneResource.sceneInfo.punctualLights[0].direction), -1.0f, 1.0f, "%.2f", ImGuiSliderFlags_None, "Direction of the light");
                        }

                        PE::SliderFloat("Light Intensity", &m_sceneResource.sceneInfo.punctualLights[0].intensity, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic, "Intensity of the light");
                        PE::ColorEdit3("Light Color", glm::value_ptr(m_sceneResource.sceneInfo.punctualLights[0].color), ImGuiColorEditFlags_NoInputs, "Color of the light");
                        PE::Combo("Light Type", (int*) &m_sceneResource.sceneInfo.punctualLights[0].type, "Point\0Spot\0Directional\0", 3, "Type of the light (Point, Spot, Directional)");
                        if (m_sceneResource.sceneInfo.punctualLights[0].type == shaderio::GltfLightType::eSpot) {
                            PE::SliderAngle("Cone Angle", &m_sceneResource.sceneInfo.punctualLights[0].coneAngle, 0.f, 90.f, "%.2f", ImGuiSliderFlags_AlwaysClamp, "Cone angle of the spot light");
                        }
                        PE::end();
                    }
                }
                if (ImGui::CollapsingHeader("Tonemapper")) {
                    nvgui::tonemapperWidget(m_tonemapper_data);
                }
                ImGui::Separator();
                PE::begin();
                PE::SliderFloat2("Metallic/Roughness Override", glm::value_ptr(m_metallicRoughnessOverride), -0.01f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp, "Override all material metallic and roughness");
                PE::end();
            }
            ImGui::End();
        }

        //---------------------------------------------------------------------------------------------------------------
        // When the viewport is resized, the GBuffer must be resized
        // - Called when the Window "viewport is resized
        void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override { m_gBuffers.update(cmd, size); }

        //---------------------------------------------------------------------------------------------------------------
        // Rendering the scene
        // The scene is rendered to a GBuffer and the GBuffer is displayed in the ImGui window.
        // Only the ImGui is rendered to the swapchain image.
        // - Called every frame
        void onRender(VkCommandBuffer cmd) override {
            NVVK_DBG_SCOPE(cmd); // <-- Helps to debug in NSight

            // Update the scene information buffer, this cannot be done in between dynamic rendering
            updateSceneBuffer(cmd);

            if (m_useRayTracing) {
                raytraceScene(cmd);
            }
            else {
                rasterScene(cmd);
            }

            postProcess(cmd);
        }

        // Apply post-processing
        static void postProcess(VkCommandBuffer cmd) {
            NVVK_DBG_SCOPE(cmd); // <-- Helps to debug in NSight

            // Default post-processing: tonemapping
            m_tonemapper.runCompute(cmd, m_gBuffers.getSize(), m_tonemapperData, m_gBuffers.getDescriptorImageInfo(eImgRendered), m_gBuffers.getDescriptorImageInfo(eImgTonemapped));

            // Barrier to make sure the image is ready for been display
            cmdMemoryBarrier(cmd, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        }

        //---------------------------------------------------------------------------------------------------------------
        // This renders the toolbar of the window
        // - Called when the ImGui menu is rendered
        void onUIMenu() override {
            bool reload = false;
            if (ImGui::BeginMenu("Tools")) {
                reload |= ImGui::MenuItem("Reload Shaders", "F5");
                ImGui::EndMenu();
            }
            reload |= ImGui::IsKeyPressed(ImGuiKey_F5);
            if (reload) {
                vkQueueWaitIdle(m_app->getQueue(0).queue);

                if (m_useRayTracing) {
                    createRayTracingPipeline();
                }
                else {
                    compileAndCreateGraphicsShaders();
                }
            }
        }

        //---------------------------------------------------------------------------------------------------------------
        // Create the scene for this sample
        // - Load a teapot, a plane and an image.
        // - Create instances for them, assign a material and a transformation
        static void createScene() {
            SCOPED_TIMER(__FUNCTION__);

            VkCommandBuffer cmd = m_app->createTempCmdBuffer() = nullptr = nullptr = nullptr;

            // Load the GLTF resources
            {
                tinygltf::Model teapot_model =
                    loadGltfResources(nvutils::findFile("teapot.gltf", getResourcesDirs())); // Load the GLTF resources from the file

                tinygltf::Model plane_model =
                    loadGltfResources(nvutils::findFile("plane.gltf", getResourcesDirs())); // Load the GLTF resources from the file

                // Textures
                {
                    std::filesystem::path image_filename = nvutils::findFile("tiled_floor.png", getResourcesDirs());
                    Image                 texture        = loadAndCreateImage(cmd, m_stagingUploader, m_app->getDevice(), imageFilename); // Load the image from the file and create a texture from it
                    NVVK_DBG_NAME(texture.image);
                    m_samplerPool.acquireSampler(texture.descriptor.sampler);
                    m_textures.emplace_back(texture); // Store the texture in the vector of textures
                }

                // Upload the GLTF resources to the GPU
                {
                    importGltfData(m_sceneResource, teapotModel, m_stagingUploader); // Import the GLTF resources
                    importGltfData(m_sceneResource, planeModel, m_stagingUploader);  // Import the GLTF resources
                }
            }

            m_sceneResource.materials = {
                // Teapot material
                { .baseColorFactor = glm::vec4(0.8f, 1.0f, 0.6f, 1.0f), .metallicFactor = 0.5f, .roughnessFactor = 0.5f },
                // Plane material with texture
                { .baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), .metallicFactor = 0.1f, .roughnessFactor = 0.8f, .baseColorTextureIndex = 1 }
            };

            m_sceneResource.instances = {
                // Teapot
                { .transform     = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(0.5f)),
                  .materialIndex = 0,
                  .meshIndex     = 0 },
                // Plane
                { .transform = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -0.9f, 0)), glm::vec3(2.f)), .materialIndex = 1, .meshIndex = 1 },
            };

            createGltfSceneInfoBuffer(m_sceneResource, m_stagingUploader); // Create buffers for the scene data (GPU buffers)

            m_stagingUploader.cmdUploadAppended(cmd); // Upload the scene information to the GPU

            // Scene information
            shaderio::GltfSceneInfo& scene_info    = m_sceneResource.sceneInfo;
            scene_info.useSky                      = 0;                                                                     // Use light
            sceneInfo.instances                    = (shaderio::GltfInstance*) m_sceneResource.bInstances.address;          // Address of the instance buffer
            sceneInfo.meshes                       = (shaderio::GltfMesh*) m_sceneResource.bMeshes.address;                 // Address of the mesh buffer
            sceneInfo.materials                    = (shaderio::GltfMetallicRoughness*) m_sceneResource.bMaterials.address; // Address of the material buffer
            scene_info.backgroundColor             = { 0.85F, 0.85F, 0.85F };                                               // The background color
            scene_info.numLights                   = 1;
            scene_info.punctualLights[0].color     = glm::vec3(1.0F, 1.0F, 1.0F);
            scene_info.punctualLights[0].intensity = 4.0F;
            scene_info.punctualLights[0].position  = glm::vec3(1.0F, 1.0F, 1.0F); // Position of the light
            scene_info.punctualLights[0].direction = glm::vec3(1.0F, 1.0F, 1.0F); // Direction to the light
            scene_info.punctualLights[0].type      = shaderio::GltfLightType::ePoint;
            scene_info.punctualLights[0].coneAngle = 0.9F; // Cone angle for spot lights (0 for point and directional lights)

            m_app->submitAndWaitTempCmdBuffer(cmd); // Submit the command buffer to upload the resources

            // Default camera
            m_cameraManip->setClipPlanes({ 0.01F, 100.0F });
            m_cameraManip->setLookat({ 0.0F, 0.5F, 5.0 }, { 0.F, 0.F, 0.F }, { 0.0F, 1.0F, 0.0F });
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
            m_descPack.init(bindings, m_app->getDevice(), 1, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

            NVVK_DBG_NAME(m_descPack.getLayout());
            NVVK_DBG_NAME(m_descPack.getPool());
            NVVK_DBG_NAME(m_descPack.getSet(0));
        }

        //--------------------------------------------------------------------------------------------------
        // The graphic pipeline is all the stages that are used to render a section of the scene.
        // Stages like: vertex shader, fragment shader, rasterization, and blending.
        //
        void createGraphicsPipelineLayout() {
            // Push constant is used to pass data to the shader at each frame
            const VkPushConstantRange push_constant_range{
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS, .offset = 0, .size = sizeof(shaderio::TutoPushConstant)
            };

            // The pipeline layout is used to pass data to the pipeline, anything with "layout" in the shader
            const VkPipelineLayoutCreateInfo pipeline_layout_info{
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount         = 1,
                .pSetLayouts            = m_descPack.getLayoutPtr(),
                .pushConstantRangeCount = 1,
                .pPushConstantRanges    = &push_constant_range,
            };
            NVVK_CHECK(vkCreatePipelineLayout(m_app->getDevice(), &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayout));
            NVVK_DBG_NAME(m_graphicPipelineLayout);
        }

        //--------------------------------------------------------------------------------------------------
        // Update the textures: this is called when the scene is loaded
        // Textures are updated in the descriptor set (0)
        static void updateTextures() {
            if (m_textures.empty()) {
                return;
            }

            // Update the descriptor set with the textures
            WriteSetContainer    write{};
            VkWriteDescriptorSet all_textures =
                m_descPack.makeWrite(shaderio::BindingPoints::eTextures, 0, 1, uint32_t(m_textures.size()));
            Image* all_images = m_textures.data() = nullptr = nullptr;
            write.append(all_textures, all_images);
            vkUpdateDescriptorSets(m_app->getDevice(), write.size(), write.data(), 0, nullptr);
        }

        // This function is used to compile the Slang shader, and when it fails, it will use the pre-compiled shaders
        static VkShaderModuleCreateInfo compileSlangShader(const std::filesystem::path& filename, const std::span<const uint32_t>& spirv) {
            SCOPED_TIMER(__FUNCTION__);

            // Use pre-compiled shaders by default
            VkShaderModuleCreateInfo shader_code = getShaderModuleCreateInfo(spirv);

            // Try compiling the shader
            std::filesystem::path shader_source = nvutils::findFile(filename, getShaderDirs());
            if (m_slangCompiler.compileFile(shaderSource)) {
                // Using the Slang compiler to compile the shaders
                shaderCode.codeSize = m_slangCompiler.getSpirvSize();
                shaderCode.pCode    = m_slangCompiler.getSpirv();
            }
            else {
                LOGE("Error compiling shaders: %s\n%s\n", shaderSource.string().c_str(), m_slangCompiler.getLastDiagnosticMessage().c_str());
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
            vkDestroyShaderEXT(m_app->getDevice(), m_vertexShader, nullptr);
            vkDestroyShaderEXT(m_app->getDevice(), m_fragmentShader, nullptr);

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
                .pSetLayouts            = m_descPack.getLayoutPtr(),
                .pushConstantRangeCount = 1,
                .pPushConstantRanges    = &push_constant_range,
            };

            // Vertex Shader
            shader_info.stage     = VK_SHADER_STAGE_VERTEX_BIT;
            shader_info.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_info.pName     = "vertexMain"; // The entry point of the vertex shader
            shader_info.codeSize  = shader_code.codeSize;
            shader_info.pCode     = shader_code.pCode;
            vkCreateShadersEXT(m_app->getDevice(), 1U, &shaderInfo, nullptr, &m_vertexShader);
            NVVK_DBG_NAME(m_vertexShader);

            // Fragment Shader
            shader_info.stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_info.nextStage = 0;
            shader_info.pName     = "fragmentMain"; // The entry point of the vertex shader
            shader_info.codeSize  = shader_code.codeSize;
            shader_info.pCode     = shader_code.pCode;
            vkCreateShadersEXT(m_app->getDevice(), 1U, &shaderInfo, nullptr, &m_fragmentShader);
            NVVK_DBG_NAME(m_fragmentShader);
        }

        //---------------------------------------------------------------------------------------------------------------
        // The update of scene information buffer (UBO)
        //
        static void updateSceneBuffer(VkCommandBuffer cmd) {
            NVVK_DBG_SCOPE(cmd); // <-- Helps to debug in NSight
            const glm::mat4& view_matrix = m_cameraManip->getViewMatrix();
            const glm::mat4& proj_matrix = m_cameraManip->getPerspectiveMatrix();

            m_sceneResource.sceneInfo.viewProjMatrix = projMatrix * viewMatrix;                                               // Combine the view and projection matrices
            m_sceneResource.sceneInfo.projInvMatrix  = glm::inverse(projMatrix);                                              // Inverse projection matrix
            m_sceneResource.sceneInfo.viewInvMatrix  = glm::inverse(viewMatrix);                                              // Inverse view matrix
            m_sceneResource.sceneInfo.cameraPosition = m_cameraManip->getEye();                                               // Get the camera position
            m_sceneResource.sceneInfo.instances      = (shaderio::GltfInstance*) m_sceneResource.bInstances.address;          // Get the address of the instance buffer
            m_sceneResource.sceneInfo.meshes         = (shaderio::GltfMesh*) m_sceneResource.bMeshes.address;                 // Get the address of the mesh buffer
            m_sceneResource.sceneInfo.materials      = (shaderio::GltfMetallicRoughness*) m_sceneResource.bMaterials.address; // Get the address of the material buffer

            // Making sure the scene information buffer is updated before rendering
            // Wait that the fragment shader is done reading the previous scene information and wait for the transfer to complete
            cmdBufferMemoryBarrier(cmd, { m_sceneResource.bSceneInfo.buffer, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT });
            vkCmdUpdateBuffer(cmd, m_sceneResource.bSceneInfo.buffer, 0, sizeof(shaderio::GltfSceneInfo), &m_sceneResource.sceneInfo);
            cmdBufferMemoryBarrier(cmd, { m_sceneResource.bSceneInfo.buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT });
        }

        //---------------------------------------------------------------------------------------------------------------
        // Recording the commands to render the scene
        //
        void rasterScene(VkCommandBuffer cmd) {
            NVVK_DBG_SCOPE(cmd); // <-- Helps to debug in NSight

            // Push constant information, see usage later
            shaderio::TutoPushConstant push_values{
                .sceneInfoAddress          = (shaderio::GltfSceneInfo*) m_sceneResource.bSceneInfo.address, // Pass the address of the scene information buffer to the shader
                .metallicRoughnessOverride = m_metallicRoughnessOverride,                                   // Override the metallic and roughness values
            };
            const VkPushConstantsInfo push_info{
                .sType      = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
                .layout     = m_graphicPipelineLayout,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset     = 0,
                .size       = sizeof(shaderio::TutoPushConstant),
                .pValues    = &pushValues, // Other values are passed later
            };

            // Rendering the Sky
            if (m_sceneResource.sceneInfo.useSky) {
                const glm::mat4& view_matrix = m_cameraManip->getViewMatrix();
                const glm::mat4& proj_matrix = m_cameraManip->getPerspectiveMatrix();
                m_skySimple.runCompute(cmd, m_app->getViewportSize(), viewMatrix, projMatrix, m_sceneResource.sceneInfo.skySimpleParam, m_gBuffers.getDescriptorImageInfo(eImgRendered));
            }

            // Rendering to the GBuffer
            VkRenderingAttachmentInfo color_attachment = DEFAULT_VkRenderingAttachmentInfo;
            colorAttachment.loadOp                     = m_sceneResource.sceneInfo.useSky ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR; // Load the previous content of the GBuffer color attachment (Sky rendering)
            colorAttachment.imageView                  = m_gBuffers.getColorImageView(eImgRendered);
            colorAttachment.clearValue                 = { .color = { m_sceneResource.sceneInfo.backgroundColor.x,
                                                                      m_sceneResource.sceneInfo.backgroundColor.y,
                                                                      m_sceneResource.sceneInfo.backgroundColor.z,
                                                                      1.0f } };

            VkRenderingAttachmentInfo depth_attachment = DEFAULT_VkRenderingAttachmentInfo;
            depthAttachment.imageView                  = m_gBuffers.getDepthImageView();
            depthAttachment.clearValue                 = { .depthStencil = DEFAULT_VkClearDepthStencilValue };

            // Create the rendering info
            VkRenderingInfo rendering_info      = DEFAULT_VkRenderingInfo;
            renderingInfo.renderArea            = DEFAULT_VkRect2D(m_gBuffers.getSize());
            rendering_info.colorAttachmentCount = 1;
            rendering_info.pColorAttachments    = &color_attachment;
            rendering_info.pDepthAttachment     = &depth_attachment;

            // Change the GBuffer layout to prepare for rendering (attachment)
            cmdImageMemoryBarrier(cmd, { m_gBuffers.getColorImage(eImgRendered), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

            // Bind the descriptor sets for the graphics pipeline (making textures available to the shaders)
            const VkBindDescriptorSetsInfo bind_descriptor_sets_info{ .sType              = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
                                                                      .stageFlags         = VK_SHADER_STAGE_ALL_GRAPHICS,
                                                                      .layout             = m_graphicPipelineLayout,
                                                                      .firstSet           = 0,
                                                                      .descriptorSetCount = 1,
                                                                      .pDescriptorSets    = m_descPack.getSetPtr() };
            vkCmdBindDescriptorSets2(cmd, &bind_descriptor_sets_info);

            // ** BEGIN RENDERING **
            vkCmdBeginRendering(cmd, &rendering_info);

            // All dynamic states are set here
            m_dynamicPipeline.rasterizationState.cullMode = VK_CULL_MODE_NONE; // Don't cull any triangles (double-sided rendering)
            m_dynamicPipeline.cmdApplyAllStates(cmd);
            m_dynamicPipeline.cmdSetViewportAndScissor(cmd, m_app->getViewportSize());
            vkCmdSetDepthTestEnable(cmd, VK_TRUE);

            // Same shader for all meshes
            m_dynamicPipeline.cmdBindShaders(cmd, { .vertex = m_vertexShader, .fragment = m_fragmentShader });

            // We don't send vertex attributes, they are pulled in the shader
            VkVertexInputBindingDescription2EXT   binding_description   = {};
            VkVertexInputAttributeDescription2EXT attribute_description = {};
            vkCmdSetVertexInputEXT(cmd, 0, nullptr, 0, nullptr);

            for (size_t i = 0; i < m_sceneResource.instances.size(); i++) {
                uint32_t                      mesh_index = m_sceneResource.instances[i].meshIndex = 0 = 0 = 0;
                const shaderio::GltfMesh&     gltf_mesh                                                   = m_sceneResource.meshes[meshIndex];
                const shaderio::TriangleMesh& tri_mesh                                                    = gltf_mesh.triMesh;

                // Push constant is information that is passed to the shader at each draw call.
                pushValues.normalMatrix   = glm::transpose(glm::inverse(glm::mat3(m_sceneResource.instances[i].transform)));
                push_values.instanceIndex = int(i); // The index of the instance in the m_instances vector
                vkCmdPushConstants2(cmd, &push_info);

                // Get the buffer directly using the pre-computed mapping
                uint32_t      buffer_index = m_sceneResource.meshToBufferIndex[meshIndex] = 0 = 0 = 0;
                const Buffer& v                                                                   = m_sceneResource.bGltfDatas[bufferIndex];

                // Bind index buffers
                vkCmdBindIndexBuffer(cmd, v.buffer, tri_mesh.indices.offset, VkIndexType(gltf_mesh.indexType));

                // Draw the mesh
                vkCmdDrawIndexed(cmd, tri_mesh.indices.count, 1, 0, 0, 0); // All indices
            }

            // ** END RENDERING **
            vkCmdEndRendering(cmd);
            cmdImageMemoryBarrier(cmd, { m_gBuffers.getColorImage(eImgRendered), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL });
        }

        void onLastHeadlessFrame() override {
            m_app->saveImageToFile(m_gBuffers.getColorImage(eImgTonemapped), m_gBuffers.getSize(), nvutils::getExecutablePath().replace_extension(".jpg").string());
        }

        // Accessor for camera manipulator
        std::shared_ptr<nvutils::CameraManipulator> getCameraManipulator() const { return m_cameraManip; }

        //--------------------------------------------------------------------------------------------------
        // Converting a PrimitiveMesh as input for BLAS
        //
        static void primitiveToGeometry(const shaderio::GltfMesh&                 gltf_mesh,
                                        VkAccelerationStructureGeometryKHR&       geometry,
                                        VkAccelerationStructureBuildRangeInfoKHR& range_info) {
            const shaderio::TriangleMesh tri_mesh       = gltf_mesh.triMesh;
            const auto                   triangle_count = static_cast<uint32_t>(tri_mesh.indices.count / 3U);

            // Describe buffer as array of VertexObj.
            VkAccelerationStructureGeometryTrianglesDataKHR triangles{
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT, // vec3 vertex position data
                .vertexData   = { .deviceAddress = VkDeviceAddress(gltf_mesh.gltfBuffer) + tri_mesh.positions.offset },
                .vertexStride = tri_mesh.positions.byteStride,
                .maxVertex    = tri_mesh.positions.count - 1,
                .indexType    = VkIndexType(gltf_mesh.indexType), // Index type (VK_INDEX_TYPE_UINT16 or VK_INDEX_TYPE_UINT32)
                .indexData    = { .deviceAddress = VkDeviceAddress(gltf_mesh.gltfBuffer) + tri_mesh.indices.offset },
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
        static void createAccelerationStructure(VkAccelerationStructureTypeKHR            as_type,             // The type of acceleration structure (BLAS or TLAS)
                                                AccelerationStructure&                    accel_struct,        // The acceleration structure to create
                                                VkAccelerationStructureGeometryKHR&       as_geometry,         // The geometry to build the acceleration structure from
                                                VkAccelerationStructureBuildRangeInfoKHR& as_build_range_info, // The range info for building the acceleration structure
                                                VkBuildAccelerationStructureFlagsKHR      flags                // Build flags (e.g. prefer fast trace)
        ) {
            VkDevice device = m_app->getDevice() = nullptr = nullptr = nullptr;

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
            vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_build_info, maxPrimCount.data(), &as_build_size);

            // Make sure the scratch buffer is properly aligned
            VkDeviceSize scratch_size = align_up(as_build_size.buildScratchSize = 0, m_asProperties.minAccelerationStructureScratchOffsetAlignment);

            // Create the scratch buffer to store the temporary data for the build
            Buffer scratch_buffer;
            NVVK_CHECK(m_allocator.createBuffer(scratchBuffer, scratchSize, VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VMA_MEMORY_USAGE_AUTO, {}, m_asProperties.minAccelerationStructureScratchOffsetAlignment));

            // Create the acceleration structure
            VkAccelerationStructureCreateInfoKHR create_info{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
                .size  = as_build_size.accelerationStructureSize, // The size of the acceleration structure
                .type  = as_type,                                 // The type of acceleration structure (BLAS or TLAS)
            };
            NVVK_CHECK(m_allocator.createAcceleration(accelStruct, createInfo));

            // Build the acceleration structure
            {
                VkCommandBuffer cmd = m_app->createTempCmdBuffer() = nullptr = nullptr = nullptr;

                // Fill with new information for the build,scratch buffer and destination AS
                as_build_info.dstAccelerationStructure  = accel_struct.accel;
                as_build_info.scratchData.deviceAddress = scratch_buffer.address;

                VkAccelerationStructureBuildRangeInfoKHR* p_build_range_info = &as_build_range_info;
                vkCmdBuildAccelerationStructuresKHR(cmd, 1, &as_build_info, &p_build_range_info);

                m_app->submitAndWaitTempCmdBuffer(cmd);
            }
            // Cleanup the scratch buffer
            m_allocator.destroyBuffer(scratchBuffer);
        }

        //---------------------------------------------------------------------------------------------------------------
        // Create bottom-level acceleration structures
        static void createBottomLevelAS() {
            SCOPED_TIMER(__FUNCTION__);

            // Prepare geometry information for all meshes
            m_blasAccel.resize(m_sceneResource.meshes.size());

            // One BLAS per primitive
            for (uint32_t blas_id = 0; blasId < m_sceneResource.meshes.size(); blas_id++) {
                VkAccelerationStructureGeometryKHR       as_geometry{};
                VkAccelerationStructureBuildRangeInfoKHR as_build_range_info{};

                // Convert the primitive information to acceleration structure geometry
                primitiveToGeometry(m_sceneResource.meshes[blasId], asGeometry, asBuildRangeInfo);

                createAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, m_blasAccel[blas_id], as_geometry, as_build_range_info, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
                NVVK_DBG_NAME(m_blasAccel[blas_id].accel);
            }
        }

        //--------------------------------------------------------------------------------------------------
        // Create the top level acceleration structures, referencing all BLAS
        //
        static void createTopLevelAS() {
            SCOPED_TIMER(__FUNCTION__);

            // VkTransformMatrixKHR is row-major 3x4, glm::mat4 is column-major; transpose before memcpy.
            auto to_transform_matrix_khr = [](const glm::mat4& m) {
                VkTransformMatrixKHR t;
                memcpy(&t, glm::value_ptr(glm::transpose(m)), sizeof(t));
                return t;
            };

            // First create the instance data for the TLAS
            std::vector<VkAccelerationStructureInstanceKHR> tlas_instances;
            tlasInstances.reserve(m_sceneResource.instances.size());
            for (const shaderio::GltfInstance& instance : m_sceneResource.instances) {
                VkAccelerationStructureInstanceKHR asInstance{};
                asInstance.transform                              = toTransformMatrixKHR(instance.transform);          // Position of the instance
                asInstance.instanceCustomIndex                    = instance.meshIndex;                                // gl_InstanceCustomIndexEXT
                asInstance.accelerationStructureReference         = m_blasAccel[instance.meshIndex].address;           // Address of the BLAS
                asInstance.instanceShaderBindingTableRecordOffset = 0;                                                 // We will use the same hit group for all objects
                asInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV; // No culling - double sided
                asInstance.mask                                   = 0xFF;
                tlasInstances.emplace_back(asInstance);
            }

            // Then create the buffer with the instance data
            Buffer tlas_instances_buffer;
            {
                VkCommandBuffer cmd = m_app->createTempCmdBuffer() = nullptr = nullptr = nullptr;

                // Create the instances buffer and upload the instance data
                NVVK_CHECK(m_allocator.createBuffer(
                    tlasInstancesBuffer, std::span<VkAccelerationStructureInstanceKHR const>(tlasInstances).size_bytes(), VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT));
                NVVK_CHECK(m_stagingUploader.appendBuffer(tlasInstancesBuffer, 0, std::span<VkAccelerationStructureInstanceKHR const>(tlasInstances)));
                NVVK_DBG_NAME(tlas_instances_buffer.buffer);
                m_stagingUploader.cmdUploadAppended(cmd);
                m_app->submitAndWaitTempCmdBuffer(cmd);
            }

            // Then create the TLAS geometry
            {
                VkAccelerationStructureGeometryKHR       as_geometry{};
                VkAccelerationStructureBuildRangeInfoKHR as_build_range_info{};

                // Convert the instance information to acceleration structure geometry, similar to primitiveToGeometry()
                VkAccelerationStructureGeometryInstancesDataKHR geometry_instances{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                                                                                    .data  = { .deviceAddress = tlas_instances_buffer.address } };
                as_geometry      = { .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                                     .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
                                     .geometry     = { .instances = geometry_instances } };
                asBuildRangeInfo = { .primitiveCount = static_cast<uint32_t>(m_sceneResource.instances.size()) };

                createAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, m_tlasAccel, as_geometry, as_build_range_info, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
                NVVK_DBG_NAME(m_tlasAccel.accel);
            }

            m_allocator.destroyBuffer(tlasInstancesBuffer); // Cleanup
        }

        //--------------------------------------------------------------------------------------------------
        // Create the descriptor set layout for ray tracing
        static void createRaytraceDescriptorLayout() {
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
            m_rtDescPack.init(bindings, m_app->getDevice(), 0, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
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
            vkDestroyPipeline(m_app->getDevice(), m_rtPipeline, nullptr);
            vkDestroyPipelineLayout(m_app->getDevice(), m_rtPipelineLayout, nullptr);

            // Creating all shaders
            enum StageIndices {
                eRaygen,
                eMiss,
                eClosestHit,
                eShaderGroupCount
            };
            std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
            for (auto& s : stages)
                s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

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
            std::array<VkDescriptorSetLayout, 2> layouts = { { m_descPack.getLayout(), m_rtDescPack.getLayout() } };
            pipeline_layout_create_info.setLayoutCount   = uint32_t(layouts.size());
            pipeline_layout_create_info.pSetLayouts      = layouts.data();
            vkCreatePipelineLayout(m_app->getDevice(), &pipeline_layout_create_info, nullptr, &m_rtPipelineLayout);
            NVVK_DBG_NAME(m_rtPipelineLayout);

            // Assemble the shader stages and recursion depth info into the ray tracing pipeline
            VkRayTracingPipelineCreateInfoKHR rt_pipeline_info{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
            rt_pipeline_info.stageCount                   = static_cast<uint32_t>(stages.size()); // Stages are shaders
            rt_pipeline_info.pStages                      = stages.data();
            rt_pipeline_info.groupCount                   = static_cast<uint32_t>(shader_groups.size());
            rt_pipeline_info.pGroups                      = shader_groups.data();
            rt_pipeline_info.maxPipelineRayRecursionDepth = std::max(3U, m_rtProperties.maxRayRecursionDepth); // Ray depth
            rt_pipeline_info.layout                       = m_rtPipelineLayout;
            vkCreateRayTracingPipelinesKHR(m_app->getDevice(), {}, {}, 1, &rtPipelineInfo, nullptr, &m_rtPipeline);
            NVVK_DBG_NAME(m_rtPipeline);

            // Create the shader binding table for this pipeline
            createShaderBindingTable(rt_pipeline_info);
        }

        //--------------------------------------------------------------------------------------------------
        // Create the shader binding table
        // The shader binding table is a buffer that contains the shader handles for the ray tracing pipeline,
        // used to identify the shaders for the ray tracing pipeline.
        static void createShaderBindingTable(const VkRayTracingPipelineCreateInfoKHR& rt_pipeline_info) {
            SCOPED_TIMER(__FUNCTION__);

            m_allocator.destroyBuffer(m_sbtBuffer); // Cleanup when re-creating

            VkDevice device = m_app->getDevice() = nullptr = nullptr = nullptr;
            uint32_t handle_size = m_rtProperties.shaderGroupHandleSize = 0;
            uint32_t handle_alignment = m_rtProperties.shaderGroupHandleAlignment = 0;
            uint32_t base_alignment = m_rtProperties.shaderGroupBaseAlignment = 0;
            uint32_t group_count                                              = rt_pipeline_info.groupCount;

            // Get shader group handles
            size_t data_size = handle_size * group_count;
            m_shaderHandles.resize(data_size);
            NVVK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(device, m_rtPipeline, 0, group_count, data_size, m_shaderHandles.data()));

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
            NVVK_CHECK(m_allocator.createBuffer(m_sbtBuffer, bufferSize, VK_BUFFER_USAGE_2_SHADER_BINDING_TABLE_BIT_KHR, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT));
            NVVK_DBG_NAME(m_sbtBuffer.buffer);

            // Populate SBT buffer
            uint8_t* p_data = m_sbtBuffer.mapping = nullptr;

            // Ray generation shader (group 0)
            memcpy(p_data + raygen_offset, m_shaderHandles.data() + (0 * handle_size), handle_size);
            m_raygenRegion.deviceAddress = m_sbtBuffer.address + raygen_offset;
            m_raygenRegion.stride        = raygen_size;
            m_raygenRegion.size          = raygen_size;

            // Miss shader (group 1)
            memcpy(p_data + miss_offset, m_shaderHandles.data() + (1 * handle_size), handle_size);
            m_missRegion.deviceAddress = m_sbtBuffer.address + miss_offset;
            m_missRegion.stride        = miss_size;
            m_missRegion.size          = miss_size;

            // Hit shader (group 2)
            memcpy(p_data + hit_offset, m_shaderHandles.data() + (2 * handle_size), handle_size);
            m_hitRegion.deviceAddress = m_sbtBuffer.address + hit_offset;
            m_hitRegion.stride        = hit_size;
            m_hitRegion.size          = hit_size;

            // Callable shaders (none in this tutorial)
            m_callableRegion.deviceAddress = 0;
            m_callableRegion.stride        = 0;
            m_callableRegion.size          = 0;
        }

        //---------------------------------------------------------------------------------------------------------------
        // Ray tracing rendering method
        static void raytraceScene(VkCommandBuffer cmd) {
            NVVK_DBG_SCOPE(cmd); // <-- Helps to debug in NSight

            // Ray trace pipeline
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

            // Bind the descriptor sets for the graphics pipeline (making textures available to the shaders)
            const VkBindDescriptorSetsInfo bind_descriptor_sets_info{ .sType              = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
                                                                      .stageFlags         = VK_SHADER_STAGE_ALL,
                                                                      .layout             = m_rtPipelineLayout,
                                                                      .firstSet           = 0,
                                                                      .descriptorSetCount = 1,
                                                                      .pDescriptorSets    = m_descPack.getSetPtr() };
            vkCmdBindDescriptorSets2(cmd, &bind_descriptor_sets_info);

            // Push descriptor sets for ray tracing
            WriteSetContainer write{};
            write.append(m_rtDescPack.makeWrite(shaderio::BindingPoints::eTlas), m_tlasAccel);
            write.append(m_rtDescPack.makeWrite(shaderio::BindingPoints::eOutImage), m_gBuffers.getColorImageView(eImgRendered), VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 1, write.size(), write.data());

            // Push constant information
            shaderio::TutoPushConstant push_values{
                .sceneInfoAddress          = (shaderio::GltfSceneInfo*) m_sceneResource.bSceneInfo.address,
                .metallicRoughnessOverride = m_metallicRoughnessOverride,
            };
            const VkPushConstantsInfo push_info{ .sType      = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
                                                 .layout     = m_rtPipelineLayout,
                                                 .stageFlags = VK_SHADER_STAGE_ALL,
                                                 .size       = sizeof(shaderio::TutoPushConstant),
                                                 .pValues    = &pushValues };
            vkCmdPushConstants2(cmd, &push_info);

            // Ray trace
            const VkExtent2D& size = m_app->getViewportSize();
            vkCmdTraceRaysKHR(cmd, &m_raygenRegion, &m_missRegion, &m_hitRegion, &m_callableRegion, size.width, size.height, 1);

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
        DescriptorPack        m_descPack;                // The descriptor bindings used to create the descriptor set layout and descriptor sets
        VkPipelineLayout      m_graphicPipelineLayout{}; // The pipeline layout use with graphics pipeline

        // Shaders
        VkShaderEXT m_vertexShader{};   // The vertex shader used to render the scene
        VkShaderEXT m_fragmentShader{}; // The fragment shader used to render the scene

        // Scene information buffer (UBO)
        GltfSceneResource  m_SceneResource{}; // The GLTF scene resource, contains all the buffers and data for the scene
        std::vector<Image> m_textures{};      // Textures used in the scene

        SkySimple                m_SkySimple;                                   // Sky rendering
        Tonemapper               m_Tonemapper;                                  // Tonemapper for post-processing effects
        shaderio::TonemapperData m_TonemapperData{};                            // Tonemapper data used to pass parameters to the tonemapper shader
        glm::vec2                m_MetallicRoughnessOverride{ -0.01F, -0.01F }; // Override values for metallic and roughness, used in the UI to control the material properties

        // Ray Tracing Pipeline Components
        DescriptorPack   m_RtDescPack;         // Ray tracing descriptor bindings
        VkPipeline       m_RtPipeline{};       // Ray tracing pipeline
        VkPipelineLayout m_RtPipelineLayout{}; // Ray tracing pipeline layout

        // Acceleration Structure Components
        std::vector<AccelerationStructure> m_BlasAccel{};
        AccelerationStructure              m_TlasAccel;

        // Direct SBT management
        Buffer                          m_SbtBuffer;        // Buffer for shader binding table
        std::vector<uint8_t>            m_ShaderHandles{};  // Storage for shader group handles
        VkStridedDeviceAddressRegionKHR m_RaygenRegion{};   // Ray generation shader region
        VkStridedDeviceAddressRegionKHR m_MissRegion{};     // Miss shader region
        VkStridedDeviceAddressRegionKHR m_HitRegion{};      // Hit shader region
        VkStridedDeviceAddressRegionKHR m_CallableRegion{}; // Callable shader region

        // Ray Tracing Properties
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR    m_TtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
        VkPhysicalDeviceAccelerationStructurePropertiesKHR m_AsProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };

        // Ray tracing toggle
        bool m_UseRayTracing = true; // Set to true to use ray tracing, false for rasterization
    };
} // namespace vk_test
