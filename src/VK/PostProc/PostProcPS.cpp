// AMD AMDUtils code
//
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <iterator>

#include "Misc/threadpool.h"

#include "base/DynamicBufferRing.h"
#include "base/StaticBufferPool.h"
#include "base/ResourceViewHeaps.h"
#include "base/ShaderCompilerHelper.h"
#include "base/UploadHeap.h"
#include "base/Texture.h"


#include "PostProcPS.h"

namespace CAULDRON_VK
{
    void PostProcPS::OnCreate(
        Device* pDevice,
        VkRenderPass renderPass,
        const std::string &shaderFilename,
        StaticBufferPool *pStaticBufferPool,
        DynamicBufferRing *pDynamicBufferRing,
        VkDescriptorSetLayout descriptorSetLayout,
        VkPipelineColorBlendStateCreateInfo *pBlendDesc,
        VkSampleCountFlagBits sampleDescCount
    )
    {
        m_pDevice = pDevice;

        float vertices[] = {
            -1,  1,  1,  0, 0,
             3,  1,  1,  2, 0,
            -1, -3,  1,  0, 2,
        };
        pStaticBufferPool->AllocBuffer(3, 5 * sizeof(float), vertices, &m_verticesView);

        // Create the vertex shader
        static const char* vertexShader =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec3 pos;\n"
            "layout (location = 1) in vec2 inTexCoord;\n"
            "layout (location = 0) out vec2 outTexCoord;\n"
            "void main() {\n"
            "   outTexCoord = inTexCoord;\n"
            "   gl_Position = vec4(pos, 1.0f);\n"
            "}\n";

        //GetThreadPool()->Add_Job([this]()
        {
            VkResult res;

            /////////////////////////////////////////////
            // vertex input state

            VkVertexInputBindingDescription vi_binding = {};
            vi_binding.binding = 0;
            vi_binding.stride = sizeof(float) * 5;
            vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            VkVertexInputAttributeDescription vi_attrs[] =
            {
                { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
                { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3 },
            };

            // Compile shaders
            //
            DefineList attributeDefines;

            VkPipelineShaderStageCreateInfo m_vertexShader;
            res = VKCompileFromString(m_pDevice->GetDevice(), SST_GLSL, VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", &attributeDefines, &m_vertexShader);
            assert(res == VK_SUCCESS);

            VkPipelineShaderStageCreateInfo m_fragmentShader;
            res = VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, shaderFilename.c_str(), "main", &attributeDefines, &m_fragmentShader);
            assert(res == VK_SUCCESS);

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { m_vertexShader, m_fragmentShader };

            // Create pipeline layout
            //

            VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
            pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pPipelineLayoutCreateInfo.pNext = nullptr;
            pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
            pPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
            pPipelineLayoutCreateInfo.setLayoutCount = 1;
            pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

            res = vkCreatePipelineLayout(pDevice->GetDevice(), &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
            assert(res == VK_SUCCESS);

            // Create pipeline
            //

            // input assembly state and layout

            VkPipelineVertexInputStateCreateInfo vi = {};
            vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vi.pNext = nullptr;
            vi.flags = 0;
            vi.vertexBindingDescriptionCount = 1;
            vi.pVertexBindingDescriptions = &vi_binding;
            vi.vertexAttributeDescriptionCount = std::size(vi_attrs);
            vi.pVertexAttributeDescriptions = vi_attrs;

            VkPipelineInputAssemblyStateCreateInfo ia;
            ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            ia.pNext = nullptr;
            ia.flags = 0;
            ia.primitiveRestartEnable = VK_FALSE;
            ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // rasterizer state

            VkPipelineRasterizationStateCreateInfo rs;
            rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rs.pNext = nullptr;
            rs.flags = 0;
            rs.polygonMode = VK_POLYGON_MODE_FILL;
            rs.cullMode = VK_CULL_MODE_NONE;
            rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rs.depthClampEnable = VK_FALSE;
            rs.rasterizerDiscardEnable = VK_FALSE;
            rs.depthBiasEnable = VK_FALSE;
            rs.depthBiasConstantFactor = 0;
            rs.depthBiasClamp = 0;
            rs.depthBiasSlopeFactor = 0;
            rs.lineWidth = 1.0f;

            VkPipelineColorBlendAttachmentState att_state[1];
            att_state[0].colorWriteMask = 0xf;
            att_state[0].blendEnable = VK_FALSE;
            att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
            att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
            att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

            // Color blend state

            VkPipelineColorBlendStateCreateInfo cb;
            cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cb.flags = 0;
            cb.pNext = nullptr;
            cb.attachmentCount = 1;
            cb.pAttachments = att_state;
            cb.logicOpEnable = VK_FALSE;
            cb.logicOp = VK_LOGIC_OP_NO_OP;
            cb.blendConstants[0] = 1.0f;
            cb.blendConstants[1] = 1.0f;
            cb.blendConstants[2] = 1.0f;
            cb.blendConstants[3] = 1.0f;

            std::vector<VkDynamicState> dynamicStateEnables = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
                VK_DYNAMIC_STATE_BLEND_CONSTANTS
            };
            VkPipelineDynamicStateCreateInfo dynamicState = {};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pNext = nullptr;
            dynamicState.pDynamicStates = dynamicStateEnables.data();
            dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

            // view port state

            VkPipelineViewportStateCreateInfo vp = {};
            vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vp.pNext = nullptr;
            vp.flags = 0;
            vp.viewportCount = 1;
            vp.scissorCount = 1;
            vp.pScissors = nullptr;
            vp.pViewports = nullptr;

            // depth stencil state

            VkPipelineDepthStencilStateCreateInfo ds;
            ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            ds.pNext = nullptr;
            ds.flags = 0;
            ds.depthTestEnable = VK_FALSE;
            ds.depthWriteEnable = VK_FALSE;
            ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            ds.depthBoundsTestEnable = VK_FALSE;
            ds.stencilTestEnable = VK_FALSE;
            ds.back.failOp = VK_STENCIL_OP_KEEP;
            ds.back.passOp = VK_STENCIL_OP_KEEP;
            ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
            ds.back.compareMask = 0;
            ds.back.reference = 0;
            ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
            ds.back.writeMask = 0;
            ds.minDepthBounds = 0;
            ds.maxDepthBounds = 0;
            ds.stencilTestEnable = VK_FALSE;
            ds.front = ds.back;

            // multi sample state

            VkPipelineMultisampleStateCreateInfo ms;
            ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            ms.pNext = nullptr;
            ms.flags = 0;
            ms.pSampleMask = nullptr;
            ms.rasterizationSamples = sampleDescCount;
            ms.sampleShadingEnable = VK_FALSE;
            ms.alphaToCoverageEnable = VK_FALSE;
            ms.alphaToOneEnable = VK_FALSE;
            ms.minSampleShading = 0.0;

            // create pipeline

            VkGraphicsPipelineCreateInfo pipeline = {};
            pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline.pNext = nullptr;
            pipeline.layout = m_pipelineLayout;
            pipeline.basePipelineHandle = VK_NULL_HANDLE;
            pipeline.basePipelineIndex = 0;
            pipeline.flags = 0;
            pipeline.pVertexInputState = &vi;
            pipeline.pInputAssemblyState = &ia;
            pipeline.pRasterizationState = &rs;
            pipeline.pColorBlendState = (pBlendDesc == nullptr) ? &cb : pBlendDesc;
            pipeline.pTessellationState = nullptr;
            pipeline.pMultisampleState = &ms;
            pipeline.pDynamicState = &dynamicState;
            pipeline.pViewportState = &vp;
            pipeline.pDepthStencilState = &ds;
            pipeline.pStages = shaderStages.data();
            pipeline.stageCount = (uint32_t)shaderStages.size();
            pipeline.renderPass = renderPass;
            pipeline.subpass = 0;

            res = vkCreateGraphicsPipelines(pDevice->GetDevice(), pDevice->GetPipelineCache(), 1, &pipeline, nullptr, &m_pipeline);
            assert(res == VK_SUCCESS);
        }
        //);
    }

    void PostProcPS::OnDestroy()
    {
        vkDestroyPipeline(m_pDevice->GetDevice(), m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_pDevice->GetDevice(), m_pipelineLayout, nullptr);
    }

    void PostProcPS::Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo constantBuffer, VkDescriptorSet descriptorSet)
    {
        if (m_pipeline == VK_NULL_HANDLE)
            return;

        // Bind vertices
        //
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_verticesView.buffer, &m_verticesView.offset);

        // Bind Descriptor sets
        //
        VkDescriptorSet descritorSets[1] = { descriptorSet };
        int numUniformOffsets = 1;
        if (constantBuffer.buffer == nullptr)
        {
            numUniformOffsets = 0;
        }
        uint32_t uniformOffsets[1] = { (uint32_t)constantBuffer.offset };
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descritorSets, numUniformOffsets, uniformOffsets);

        // Bind Pipeline
        //
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Draw
        //
        vkCmdDraw(cmd_buf, 3, 1, 0, 0);
    }
}
