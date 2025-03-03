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


#include <algorithm>
#include <cassert>

#include "Instance.h"
#include "InstanceProperties.h"

namespace CAULDRON_VK
{
    VkInstance CreateInstance(VkApplicationInfo app_info, InstanceProperties *pIP)
    {
        VkInstance instance;

        //populate list from enabled extensions
        void *pNext = NULL;
        std::vector<const char *> instance_layer_names;
        std::vector<const char *> instance_extension_names;
        pIP->GetExtensionNamesAndConfigs(&instance_layer_names, &instance_extension_names);

        // Add 2 more thatn should aways be present in windows
        #ifdef _WIN32
        instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        #endif
        instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        instance_extension_names.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

        // do create the instance
        VkInstanceCreateInfo inst_info = {};
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = pNext;
        inst_info.flags = 0;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledLayerCount = (uint32_t)instance_layer_names.size();
        inst_info.ppEnabledLayerNames = (uint32_t)instance_layer_names.size() ? instance_layer_names.data() : NULL;
        inst_info.enabledExtensionCount = (uint32_t)instance_extension_names.size();
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();           
        VkResult res = vkCreateInstance(&inst_info, NULL, &instance);
        assert(res == VK_SUCCESS);

        return instance;
    }

    void DestroyInstance(VkInstance instance)
    {
        vkDestroyInstance(instance, nullptr);
    }
}



