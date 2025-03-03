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
#pragma once

#include "base/Device.h"
#include "base/StaticBufferPool.h"
#include "base/ResourceViewHeaps.h"
#include "base/DynamicBufferRing.h"
#include "GLTFTexturesAndBuffers.h"
#include "widgets/WireframeBox.h"

#include <DirectXMath.h>

namespace CAULDRON_VK
{
    class GltfBBoxPass
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            VkSampleCountFlagBits sampleCount);

        void OnDestroy();
        void Draw(VkCommandBuffer cmd_buf, XMMATRIX cameraViewProjMatrix);
    private:
        GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;
        WireframeBox m_wireframeBox;
    };
}
