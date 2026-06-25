#include <stdio.h>

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "GLFW/glfw3.h"

//#define CLOG_IMPLEMENTATION
#include "clog/clog.h"

#define LOG_DEBUG(...) DBG(__VA_ARGS__)
#define LOG_TRACE(...) TRC(__VA_ARGS__)
#define LOG_INFO(...)  INF(__VA_ARGS__)
#define LOG_WARN(...)  WRN(__VA_ARGS__)
#define LOG_ERROR(...) ERR(__VA_ARGS__)
#define LOG_FATAL(...) FTL(__VA_ARGS__)

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#ifndef NDEBUG
#define DEBUG
#endif

#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data)
{
    switch(severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_TRACE("%s", callback_data->pMessage);
            return VK_TRUE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO("%s", callback_data->pMessage);
            return VK_TRUE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARN("%s", callback_data->pMessage);
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR("%s", callback_data->pMessage);
            return VK_FALSE;

        default: return VK_FALSE;
    }
}
#endif

VkSurfaceKHR window_create_vulkan_surface(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if(glfwCreateWindowSurface(instance, window, NULL, &surface)==VK_SUCCESS) return surface;

    LOG_ERROR("glfwCreateWindowSurface failed");
    return VK_NULL_HANDLE;
}

VkInstance create_instance()
{
    VkInstance instance = VK_NULL_HANDLE;

#ifdef DEBUG
    const char* layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const u32 layer_count = 1;
#endif

    char extentions[10][512] = {
#ifdef DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };

#ifdef DEBUG
    u32 ext_count = 1;
#else
    u32 ext_count = 0;
#endif

    u32 glfw_ext_count;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    for(u32 i=0; i<glfw_ext_count+1; i++)
    {
        strcpy(extentions[i+1], glfw_extensions[i]);
    }
    ext_count += glfw_ext_count;

    const char* ext_ptr[10];
    for(u32 i=0; i<ext_count; i++)
    {
        ext_ptr[i] = extentions[i];
    }
    const char** exts = ext_ptr;

#ifdef DEBUG
    VkDebugUtilsMessageTypeFlagBitsEXT types = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    VkDebugUtilsMessageSeverityFlagsEXT severity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT dbg_info = {
        .sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageType=types,
        .messageSeverity=severity,
        .pfnUserCallback=vk_debug_callback
    };
#else
    const u32 ext_count = 2;
#endif

    VkApplicationInfo app_info = {
        .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion=VK_API_VERSION_1_4,
        .applicationVersion=VK_MAKE_VERSION(0, 1, 0),
        .engineVersion=VK_MAKE_VERSION(0, 1, 0),
        .pEngineName="DarkMatter",
        .pApplicationName="TestApp"
    };

    VkInstanceCreateInfo create_info = {
        .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo=&app_info,
        .enabledExtensionCount=ext_count,
        .ppEnabledExtensionNames=exts,
#ifdef DEBUG
        .enabledLayerCount=layer_count,
        .ppEnabledLayerNames=layers,
        .pNext=&dbg_info
#endif
    };

    if(vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) return VK_NULL_HANDLE;
    volkLoadInstance(instance);

    return instance;
}

VkPhysicalDevice create_physical_device(VkInstance instance)
{
    VkPhysicalDevice device = VK_NULL_HANDLE;

    u32 count;
    VkPhysicalDevice devices[10] = { 0 };

    vkEnumeratePhysicalDevices(instance, &count, NULL);
    vkEnumeratePhysicalDevices(instance, &count, devices);

    VkPhysicalDeviceProperties props;

    for(u32 i=0; i<count; i++)
    {
        vkGetPhysicalDeviceProperties(devices[i], &props);

        if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            device = devices[i];
            break;
        }
    }
    if(device == VK_NULL_HANDLE)
    {
        LOG_WARN("No dedicated gpu found, looking for integrated gpu...");


        for(u32 i=0; i<count; i++)
        {
            vkGetPhysicalDeviceProperties(devices[i], &props);

            if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                device = devices[i];
                break;
            }
        }
    }

    if(device == VK_NULL_HANDLE) return VK_NULL_HANDLE;

    LOG_INFO("GPU: %s", props.deviceName);

    return device;
}

VkDevice create_device(VkInstance instance, VkPhysicalDevice physical_device, VkSurfaceKHR surface, u32* gfx_index)
{
    VkDevice device = VK_NULL_HANDLE;

    u32 queue_count;
    VkQueueFamilyProperties props[10] = { 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, props);

    *gfx_index = UINT32_MAX;
    bool found = false;
    VkBool32 has_present = VK_FALSE;
    for(u32 i=0; i<queue_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &has_present);
        if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && has_present)
        {
            *gfx_index = i;
            found = true;
            break;
        }
    }
    if(!found) { LOG_ERROR("Could not find graphics queue"); return VK_NULL_HANDLE; }

    float priorities[] = { 1.f };

    VkDeviceQueueCreateInfo gfx_info = {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount=1,
        .queueFamilyIndex=*gfx_index,
        .pQueuePriorities=priorities
    };

    // features
    VkPhysicalDeviceVulkan14Features v14_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES
    };
    VkPhysicalDeviceVulkan13Features v13_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext=&v14_supported
    };
    VkPhysicalDeviceVulkan12Features v12_supported = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext=&v13_supported
    };
    VkPhysicalDeviceFeatures2 supported_features2 = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext=&v12_supported
    };
    vkGetPhysicalDeviceFeatures2(physical_device, &supported_features2);

    if(!v14_supported.pushDescriptor)    { LOG_ERROR("Push descriptor not supported"); return device; }
    if(!v13_supported.dynamicRendering)  { LOG_ERROR("Dynamic rendering not supported"); return device; }
    if(!v13_supported.synchronization2)  { LOG_ERROR("Synchronization2 not supported"); return device; }
    if(!v12_supported.timelineSemaphore) { LOG_ERROR("Timeline semaphores not supported"); return device; }

    VkPhysicalDeviceVulkan14Features v14_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pushDescriptor=1,
    };
    VkPhysicalDeviceVulkan13Features v13_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext=&v14_features,
        .dynamicRendering=1,
        .synchronization2=1
    };
    VkPhysicalDeviceVulkan12Features v12_features = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext=&v13_features,
        .bufferDeviceAddress=1,
#ifdef DEBUG
        .bufferDeviceAddressCaptureReplay=1,
#endif
        .scalarBlockLayout=1,
        .timelineSemaphore=1
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext=&v12_features,
        .features.shaderInt64=1
    };

    const char* extensions[256] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    //
    VkDeviceCreateInfo create_info = {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount=1,
        .pQueueCreateInfos=&gfx_info,
        .pNext=&features2,
        .enabledExtensionCount=1,
        .ppEnabledExtensionNames=extensions
    };

    if(vkCreateDevice(physical_device, &create_info, NULL, &device) == VK_SUCCESS) return device;

    LOG_ERROR("vkCreateDevice failed");
    return VK_NULL_HANDLE;
}

int main(void){
    printf("Hello world!\n");

    if(!glfwInit()) { LOG_FATAL("glfwInit failed"); return 1; }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(960, 540, "test", NULL, NULL);

    if(volkInitialize() != VK_SUCCESS) { LOG_FATAL("volkInitialize failed"); return 1; }

    VkInstance instance = create_instance();
    if(instance == VK_NULL_HANDLE) { LOG_FATAL("VkInstance is NULL"); return 1; }

    VkPhysicalDevice physical_device = create_physical_device(instance);
    if(physical_device == VK_NULL_HANDLE) { LOG_FATAL("VkPhysicalDevice is NULL"); return 1; }

    VkSurfaceKHR surface = window_create_vulkan_surface(instance, window);
    if(surface == VK_NULL_HANDLE) { LOG_FATAL("VkSurfaceKHR is NULL"); return 1; }

    u32 gfx_index;
    VkDevice device = create_device(instance, physical_device, surface, &gfx_index);
    if(device == VK_NULL_HANDLE) { LOG_FATAL("VkDevice is NULL"); return 1; }
    volkLoadDevice(device);

    if(instance == VK_NULL_HANDLE) { LOG_FATAL("VkInstance is invalid"); return 1; }

    // main loop
#if 1 
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
#endif

    // shutdown
    volkFinalize();
    //glfwDestroyWindow(window);

    return 0;
}
