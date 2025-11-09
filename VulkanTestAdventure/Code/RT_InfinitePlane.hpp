#include "shaderio.h"

// Pre-compiled shaders
#include "../_autogen/sky_simple.slang.h"
#include "../_autogen/tonemapper.slang.h"
#include "../_autogen/infinite_plane.slang.h"

// Common base class (see 02_basic)
#include "RtBase.hpp"

namespace vk_test {
    //class Rt12InfinitePlane : public RtBase {

    //public:
    //    Rt12InfinitePlane()           = default;
    //    ~Rt12InfinitePlane() override = default;

    //    //-------------------------------------------------------------------------------
    //    // Override virtual methods from RtBase
    //    //-------------------------------------------------------------------------------

    //    void onUIRender() override {
    //        bool modified = false;

    //        if (ImGui::Begin("Settings")) {
    //            // Add toggle for infinite plane
    //            modified |= ImGui::Checkbox("Enable Infinite Plane", (bool*) &m_pushValues.planeEnabled);
    //            modified |= ImGui::SliderFloat2("Metallic/Roughness", glm::value_ptr(m_metallicRoughnessOverride), 0.f, 1.f);
    //            modified |= ImGui::ColorEdit3("Color", glm::value_ptr(m_pushValues.planeColor));
    //            modified |= ImGui::SliderFloat("Height", &m_pushValues.planeHeight, -5.f, 2.f);
    //        }
    //        ImGui::End();

    //        modified |= RtBase::renderUI();

    //        if (modified)
    //            resetFrame(); // Reset frame count
    //    }

    //    void createScene() override {
    //        SCOPED_TIMER(__FUNCTION__);

    //        VkCommandBuffer cmd = m_app->createTempCmdBuffer();

    //        m_metallicRoughnessOverride = glm::vec2{ 0.4f, 0.1f };

    //        // Load the GLTF resources
    //        {
    //            tinygltf::Model teapotModel =
    //                nvsamples::loadGltfResources(nvutils::findFile("teapot.gltf", nvsamples::getResourcesDirs())); // Load the GLTF resources from the file
    //            nvsamples::importGltfData(m_sceneResource, teapotModel, m_stagingUploader, false);
    //        }

    //        // Create materials
    //        m_sceneResource.materials = {
    //            { .baseColorFactor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f), .metallicFactor = 0.5f, .roughnessFactor = 0.5f },
    //        };

    //        // Make instances of the meshes
    //        m_sceneResource.instances = {
    //            { .transform = glm::mat4(1), .materialIndex = 0, .meshIndex = 0 }, // Teapot
    //        };

    //        // By default, set the plane to the bottom of the mesh
    //        m_pushValues.planeHeight = -1.67f;

    //        // Create buffers for the scene data (GPU buffers)
    //        nvsamples::createGltfSceneInfoBuffer(m_sceneResource, m_stagingUploader);

    //        m_stagingUploader.cmdUploadAppended(cmd); // Upload the resources
    //        m_app->submitAndWaitTempCmdBuffer(cmd);   // Submit the command buffer to upload the resources

    //        // Set the camera
    //        m_cameraManip->setLookat({ 8.11, 2.415, 10.66498 }, { 0.114, -1.2, -0.03517 }, { 0, 1, 0 });
    //    }

    //    void createRayTracingPipeline() override {
    //        SCOPED_TIMER(__FUNCTION__);

    //        // For re-creation
    //        destroyRayTracingPipeline();

    //        // Compile shader, and if failed, use pre-compiled shaders
    //        VkShaderModuleCreateInfo shaderCode = compileSlangShader("infinite_plane.slang", infinite_plane_slang);

    //        // Creating all shaders
    //        enum StageIndices {
    //            eRaygen,
    //            eMiss,
    //            eClosestHit,
    //            eShaderGroupCount
    //        };
    //        std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
    //        for (auto& s : stages)
    //            s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    //        stages[eRaygen].pNext     = &shaderCode;
    //        stages[eRaygen].pName     = "rgenMain";
    //        stages[eRaygen].stage     = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    //        stages[eMiss].pNext       = &shaderCode;
    //        stages[eMiss].pName       = "rmissMain";
    //        stages[eMiss].stage       = VK_SHADER_STAGE_MISS_BIT_KHR;
    //        stages[eClosestHit].pNext = &shaderCode;
    //        stages[eClosestHit].pName = "rchitMain";
    //        stages[eClosestHit].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    //        // Shader groups
    //        VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
    //        group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    //        group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    //        group.generalShader      = VK_SHADER_UNUSED_KHR;
    //        group.intersectionShader = VK_SHADER_UNUSED_KHR;

    //        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    //        // Raygen
    //        group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    //        group.generalShader = eRaygen;
    //        shaderGroups.push_back(group);

    //        // Miss
    //        group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    //        group.generalShader = eMiss;
    //        shaderGroups.push_back(group);

    //        // closest hit shader
    //        group.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    //        group.generalShader    = VK_SHADER_UNUSED_KHR;
    //        group.closestHitShader = eClosestHit;
    //        shaderGroups.push_back(group);

    //        // Create the ray tracing pipeline
    //        VkRayTracingPipelineCreateInfoKHR rtPipelineInfo = createRayTracingPipelineCreateInfo(stages, shaderGroups);
    //        vkCreateRayTracingPipelinesKHR(m_app->getDevice(), {}, {}, 1, &rtPipelineInfo, nullptr, &m_rtPipeline);
    //        NVVK_DBG_NAME(m_rtPipeline);

    //        // Creating the SBT
    //        createShaderBindingTable(rtPipelineInfo);
    //    }

    //    void raytraceScene(VkCommandBuffer cmd) override {
    //        updateFrame();
    //        if (m_pushValues.frame >= m_maxFrames)
    //            return;
    //        // Normal ray tracing
    //        RtBase::raytraceScene(cmd);
    //    }

    //    // Frame management functions
    //    void resetFrame() { m_pushValues.frame = -1; }

    //    void updateFrame() {
    //        static glm::mat4 refCamMatrix;
    //        static float     refFov{ m_cameraManip->getFov() };

    //        const auto& m   = m_cameraManip->getViewMatrix();
    //        const auto  fov = m_cameraManip->getFov();

    //        if (refCamMatrix != m || refFov != fov) {
    //            resetFrame();
    //            refCamMatrix = m;
    //            refFov       = fov;
    //        }
    //        m_pushValues.frame = std::min(++m_pushValues.frame, m_maxFrames); // Increment frame count, but limit it to maxFrames
    //    }

    //    // Override onResize to reset frame
    //    void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override {
    //        resetFrame();
    //        RtBase::onResize(cmd, size);
    //    }

    //private:
    //    int m_frame     = 0;
    //    int m_maxDepth  = 5;
    //    int m_maxFrames = 10000; // Maximum number of frames for accumulation
    //};
} // namespace vk_test
