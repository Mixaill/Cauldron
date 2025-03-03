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

#include <cassert>
#include <vector>

#include <vulkan/vulkan.h>

#include "Instance.h"
#include "InstanceProperties.h"
#include "DeviceProperties.h"
#include "validationExt.h"

namespace CAULDRON_VK
{
    static PFN_vkCreateDebugReportCallbackEXT g_vkCreateDebugReportCallbackEXT = NULL;
    static PFN_vkDebugReportMessageEXT g_vkDebugReportMessageEXT = NULL;
    static PFN_vkDestroyDebugReportCallbackEXT g_vkDestroyDebugReportCallbackEXT = NULL;
    static VkDebugReportCallbackEXT g_DebugReportCallback = NULL;

    static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData)
    {
        #ifdef _WIN32
        OutputDebugStringA(pMessage);
        OutputDebugStringA("\n");
		#else
		#warning "TODO: implement debug messages for Linux"
        #endif
        return VK_FALSE;
    }

    VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT };
    VkValidationFeaturesEXT features = {};

    const char instanceExtensionName[] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    const char instanceLayerName[] = "VK_LAYER_LUNARG_standard_validation";

    bool ValidationCheckExtensions(InstanceProperties *pIP)
    {
        bool res = pIP->Add(instanceLayerName, instanceExtensionName);
        return res;
    }

    bool ValidationCheckExtensions(DeviceProperties *pDP, void **pNext)
    {
        bool res = pDP->Add(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        if (res)
        {
            features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            features.pNext = *pNext;
            features.enabledValidationFeatureCount = 1;
            features.pEnabledValidationFeatures = enables;

            *pNext = &features;
        }
        return res;
    }

    void ValidationOnCreate(VkInstance instance)
    {
        g_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        g_vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
        g_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        assert(g_vkCreateDebugReportCallbackEXT);
        assert(g_vkDebugReportMessageEXT);
        assert(g_vkDestroyDebugReportCallbackEXT);

        VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
        debugReportCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debugReportCallbackInfo.pfnCallback = MyDebugReportCallback;
        VkResult res = g_vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, nullptr, &g_DebugReportCallback);
        assert(res == VK_SUCCESS);
    }

    void ValidationOnDestroy(VkInstance instance)
    {
        // It should happen after destroing device, before destroying instance.
        if (g_DebugReportCallback)
        {
            g_vkDestroyDebugReportCallbackEXT(instance, g_DebugReportCallback, nullptr);
            g_DebugReportCallback = nullptr;
        }
    }
}
