#include "dm.h"

#define VOLK_IMPLEMENTATION
#define VK_NO_PROTOTYPES
#include <volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#define DM_FRAMES_IN_FLIGHT     3
#define DM_SWAPCHAIN_MAX_IMAGES 5
#define DM_SWAPCHAIN_FORMAT     VK_FORMAT_B8G8R8A8_SRGB
#define DM_DEPTH_FORMAT         VK_FORMAT_D32_SFLOAT

extern VkSurfaceKHR dm_window_create_vulkan_surface(dm_context *context, VkInstance instance);
extern const char** dm_window_get_vulkan_extensions(u32 *glfw_ext_count);

typedef struct dm_vulkan_gpu_t
{
    VkPhysicalDevice physical;
    VkDevice         device;

    VkQueue gfx_queue;
    u32     gfx_index;

    VkPhysicalDeviceFeatures   features;
    VkPhysicalDeviceProperties properties;
} dm_vulkan_gpu;

typedef struct dm_vulkan_surface_t
{
    VkSurfaceKHR surface;

    VkSurfaceCapabilitiesKHR capabilities;
} dm_vulkan_surface;

typedef struct dm_vulkan_swapchain_image_t
{
    VkImage     image;
    VkImageView view;
    VkSemaphore semaphore;
} dm_vulkan_swapchain_image;

typedef struct dm_vulkan_depth_image_t
{
    VkImage       image;
    VkImageView   view;
    VmaAllocation allocation;
} dm_vulkan_depth_image;

typedef struct dm_vulkan_swapchain_t
{
    VkSwapchainKHR swapchain;
    VkFormat       format, depth_format;

    dm_vulkan_swapchain_image images[DM_SWAPCHAIN_MAX_IMAGES];
    dm_vulkan_depth_image depth_image;

    u16 width, height;
    u32 count, index;
} dm_vulkan_swapchain;

typedef struct dm_vulkan_frame_data_t
{
    VkCommandPool   gfx_pool;
    VkCommandBuffer gfx_cmd;
    VkSemaphore     semaphore;
} dm_vulkan_frame_data;

typedef struct dm_vulkan_renderer_t
{
    VkInstance       instance;
    VmaAllocator     allocator ;

    dm_vulkan_gpu        gpu;
    dm_vulkan_surface    surface;
    dm_vulkan_swapchain  swapchain;
    dm_vulkan_frame_data frame_data[DM_FRAMES_IN_FLIGHT];

    VkCommandPool single_use_pool;

    VkSemaphore timeline_semaphore;
    u64         timeline_value;

    u32 frame_index;
} dm_vulkan_renderer;

#ifdef DM_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL dm_vk_debug_callback(
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

VkInstance dm_vulkan_create_instance()
{
    VkInstance instance = VK_NULL_HANDLE;

#ifdef DM_DEBUG
    const char* layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const u32 layer_count = 1;
#endif

    char extensions[10][512] = {
#ifdef DM_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };

#ifdef DM_DEBUG
    u32 ext_count = 1;
#else
    u32 ext_count = 0;
#endif

    u32 window_ext_count;
    const char** window_exts = dm_window_get_vulkan_extensions(&window_ext_count);
    for(u32 i=0; i<window_ext_count; i++)
    {
        strcpy(extensions[i+ext_count], window_exts[i]);
    }
    ext_count += window_ext_count;

    const char* ext_ptr[10];
    for(u32 i=0; i<ext_count; i++)
    {
        ext_ptr[i] = extensions[i];
    }
    const char** exts = ext_ptr;

#ifdef DM_DEBUG
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
        .pfnUserCallback=dm_vk_debug_callback
    };
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
#ifdef DM_DEBUG
        .enabledLayerCount=layer_count,
        .ppEnabledLayerNames=layers,
        .pNext=&dbg_info
#endif
    };

    if(vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) return VK_NULL_HANDLE;
    volkLoadInstance(instance);

    return instance;
}

bool dm_vulkan_check_physical_device(VkPhysicalDevice physical)
{
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
    vkGetPhysicalDeviceFeatures2(physical, &supported_features2);

    if(!v14_supported.pushDescriptor)    { LOG_ERROR("Push descriptor not supported");     return false; }
    if(!v13_supported.dynamicRendering)  { LOG_ERROR("Dynamic rendering not supported");   return false; }
    if(!v13_supported.synchronization2)  { LOG_ERROR("Synchronization2 not supported");    return false; }
    if(!v12_supported.timelineSemaphore) { LOG_ERROR("Timeline semaphores not supported"); return false; }

    return true;
}

VkPhysicalDevice dm_vulkan_create_physical_device(VkInstance instance)
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

    if(!dm_vulkan_check_physical_device(device))
    {
        LOG_ERROR("Device is not adequate.");
        return VK_NULL_HANDLE;
    }

    LOG_INFO("GPU: %s", props.deviceName);

    return device;
}

u32 dm_vulkan_find_graphics_queue(VkPhysicalDevice physical, VkSurfaceKHR surface)
{
    u32 queue_count, index;
    VkQueueFamilyProperties props[10] = { 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_count, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_count, props);

    index = UINT32_MAX;
    bool found = false;
    VkBool32 has_present = VK_FALSE;
    for(u32 i=0; i<queue_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &has_present);
        if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && has_present)
        {
            index = i;
            found = true;
            break;
        }
    }
    if(!found) LOG_ERROR("Could not find graphics queue"); 

    return index;
}

VkDevice dm_vulkan_create_device(VkInstance instance, VkPhysicalDevice physical_device, VkSurfaceKHR surface, u32 gfx_index)
{
    VkDevice device = VK_NULL_HANDLE;

    float priorities[] = { 1.f };

    VkDeviceQueueCreateInfo gfx_info = {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount=1,
        .queueFamilyIndex=gfx_index,
        .pQueuePriorities=priorities
    };

    // features
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
#ifdef DM_DEBUG
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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
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

dm_vulkan_gpu dm_vulkan_create_gpu(VkInstance instance, dm_vulkan_surface surface)
{
    dm_vulkan_gpu gpu = { 0 };

    VkPhysicalDevice physical = dm_vulkan_create_physical_device(instance);
    if(physical == VK_NULL_HANDLE) { LOG_ERROR("Could not create physical device."); return gpu; }

    u32 gfx_index = dm_vulkan_find_graphics_queue(physical, surface.surface);
    if(gfx_index == UINT32_MAX) { LOG_ERROR("Could not find graphics queue."); return gpu; }

    VkDevice device = dm_vulkan_create_device(instance, physical, surface.surface, gfx_index);
    if(device == VK_NULL_HANDLE) { LOG_ERROR("Could not create device."); return gpu; }

    gpu.physical  = physical;
    gpu.device    = device;
    gpu.gfx_index = gfx_index;

    vkGetPhysicalDeviceFeatures(physical, &gpu.features);
    vkGetPhysicalDeviceProperties(physical, &gpu.properties);

    return gpu;
}

VmaAllocator create_vma_allocator(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device)
{
    VmaAllocator allocator = VK_NULL_HANDLE;

    VmaVulkanFunctions functions = { 0 };

    VmaAllocatorCreateInfo info = {
        .instance=instance,
        .physicalDevice=physical_device,
        .device=device,
        .flags=VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .vulkanApiVersion=VK_API_VERSION_1_4,
        .pVulkanFunctions=&functions
    };

    if(vmaImportVulkanFunctionsFromVolk(&info, &functions) != VK_SUCCESS) { LOG_ERROR("vmaImportVulkanFunctionsFromVolk failed"); return VK_NULL_HANDLE; }

    if(vmaCreateAllocator(&info, &allocator)==VK_SUCCESS) return allocator;

    LOG_ERROR("vmaCreateAllocator failed");
    return VK_NULL_HANDLE;
}

VkSwapchainKHR dm_vulkan_create_vk_swap(dm_vulkan_gpu gpu, dm_vulkan_surface surface)
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    VkSwapchainCreateInfoKHR info = {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface=surface.surface,
        .imageFormat=DM_SWAPCHAIN_FORMAT,
        .imageColorSpace=VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .minImageCount=surface.capabilities.minImageCount,
        .imageExtent.width=surface.capabilities.currentExtent.width,
        .imageExtent.height=surface.capabilities.currentExtent.height,
        .imageArrayLayers=1,
        .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=VK_PRESENT_MODE_FIFO_KHR,
    };

    if(vkCreateSwapchainKHR(gpu.device, &info, NULL, &swapchain) != VK_SUCCESS) 
    { 
        LOG_ERROR("vkCreateSwapchainKHR failed"); 
        return VK_NULL_HANDLE; 
    }

    return swapchain;
}

u32 dm_vulkan_get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, VkImage* images)
{
    u32 count = UINT32_MAX;

    if(vkGetSwapchainImagesKHR(device, swapchain, &count, NULL) != VK_SUCCESS)   return UINT32_MAX;
    if(vkGetSwapchainImagesKHR(device, swapchain, &count, images) != VK_SUCCESS) return UINT32_MAX;
    if(count >= DM_SWAPCHAIN_MAX_IMAGES) { LOG_ERROR("Swapchain image count greater than compile time max."); return UINT32_MAX; }

    return count;
}

dm_vulkan_swapchain_image dm_vulkan_create_swapchain_image(VkDevice device, VkImage* images, u32 index)
{
    dm_vulkan_swapchain_image image = { 0 };

    VkImageView view = VK_NULL_HANDLE;

    VkImageViewCreateInfo info = { 
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image=images[index],
        .format=DM_SWAPCHAIN_FORMAT,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    if(vkCreateImageView(device, &info, NULL, &view) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateImageView failed");
        return image;
    }

    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphore_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    if(vkCreateSemaphore(device, &semaphore_info, NULL, &semaphore) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateSemaphore failed");
        return image;
    }

    image.image     = images[index];
    image.view      = view;
    image.semaphore = semaphore;

    return image;
}

dm_vulkan_depth_image dm_vulkan_create_depth_image(dm_vulkan_gpu gpu, VmaAllocator allocator, dm_vulkan_surface surface)
{
    dm_vulkan_depth_image image = { 0 };

    VkImage       vk_image = VK_NULL_HANDLE;
    VkImageView   vk_view = VK_NULL_HANDLE;
    VmaAllocation vk_alloc = VK_NULL_HANDLE;

    VkImageCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .format=DM_DEPTH_FORMAT,
        .imageType=VK_IMAGE_TYPE_2D,
        .usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .tiling=VK_IMAGE_TILING_OPTIMAL,
        .samples=VK_SAMPLE_COUNT_1_BIT,
        .extent.width=surface.capabilities.currentExtent.width,
        .extent.height=surface.capabilities.currentExtent.height,
        .extent.depth=1,
        .mipLevels=1,
        .arrayLayers=1
    };

    VmaAllocationCreateInfo alloc_info = {
        .flags=VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage=VMA_MEMORY_USAGE_AUTO
    };

    if(vmaCreateImage(allocator, &info, &alloc_info, &vk_image, &vk_alloc, NULL) != VK_SUCCESS)
    {
        LOG_ERROR("vmaCreateImage failed");
        return image;
    }

    VkImageViewCreateInfo depth_view_info = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image=vk_image,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=DM_DEPTH_FORMAT,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };

    if(vkCreateImageView(gpu.device, &depth_view_info, NULL, &vk_view) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateImageView failed");
        return image;
    }

    image.image      = vk_image;
    image.view       = vk_view;
    image.allocation = vk_alloc;

    return image;
}

dm_vulkan_swapchain dm_vulkan_create_swapchain(dm_vulkan_gpu gpu, dm_vulkan_surface surface, VmaAllocator allocator)
{
    dm_vulkan_swapchain swapchain = { 0 };

    VkSwapchainKHR vk_swap = dm_vulkan_create_vk_swap(gpu, surface);
    if(vk_swap == VK_NULL_HANDLE) 
    { 
        LOG_ERROR("Could not create vulkan swapchain.");
        return swapchain; 
    }

    VkImage vk_images[DM_SWAPCHAIN_MAX_IMAGES] = { 0 };

    u32 image_count = dm_vulkan_get_swapchain_images(gpu.device, vk_swap, vk_images);
    if(image_count == UINT32_MAX) 
    { 
        LOG_ERROR("Could not get swapchain images.");
        return swapchain; 
    }

    dm_vulkan_swapchain_image images[DM_SWAPCHAIN_MAX_IMAGES] = { 0 };
    for(u32 i=0; i<image_count; i++)
    {
        images[i] = dm_vulkan_create_swapchain_image(gpu.device, vk_images, i);
        if(images[i].image == VK_NULL_HANDLE) 
        { 
            LOG_ERROR("Could not create swapchain image.");
            return swapchain; 
        }
    }

    dm_vulkan_depth_image depth_image = dm_vulkan_create_depth_image(gpu, allocator, surface);
    if(depth_image.image == VK_NULL_HANDLE) 
    { 
        LOG_ERROR("Could not create depth image.");
        return swapchain; 
    }

    // assign
    swapchain.swapchain    = vk_swap;
    swapchain.format       = DM_SWAPCHAIN_FORMAT;
    swapchain.depth_format = DM_DEPTH_FORMAT;
    swapchain.width        = surface.capabilities.currentExtent.width;
    swapchain.height       = surface.capabilities.currentExtent.height;

    for(u32 i=0; i<image_count; i++)
    {
        swapchain.images[i] = images[i];
    }
    swapchain.count     = image_count;

    swapchain.depth_image = depth_image;

    return swapchain;
}

void dm_vulkan_destroy_swapchain(dm_vulkan_swapchain* swapchain, dm_vulkan_gpu gpu, VmaAllocator allocator)
{
    vkDeviceWaitIdle(gpu.device);

    for(u32 i=0; i<swapchain->count; i++)
    {
        vkDestroyImageView(gpu.device, swapchain->images[i].view, NULL);
        vkDestroySemaphore(gpu.device, swapchain->images[i].semaphore, NULL);
    }

    vkDestroySwapchainKHR(gpu.device, swapchain->swapchain, NULL);

    dm_vulkan_depth_image depth_image = swapchain->depth_image;
    vkDestroyImageView(gpu.device, depth_image.view, NULL);
    vmaDestroyImage(allocator, depth_image.image, depth_image.allocation);
}

dm_vulkan_frame_data dm_vulkan_create_frame_data(dm_vulkan_gpu gpu)
{
    dm_vulkan_frame_data data = { 0 };

    VkCommandPool pool    = VK_NULL_HANDLE;
    VkCommandBuffer cmd   = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo pool_info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex=gpu.gfx_index
    };

    if(vkCreateCommandPool(gpu.device, &pool_info, NULL, &pool) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateCommandPool");
        return data;
    }

    VkCommandBufferAllocateInfo cmd_info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool=pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount=1
    };

    if(vkAllocateCommandBuffers(gpu.device, &cmd_info, &cmd) != VK_SUCCESS)
    {
        LOG_ERROR("vkAllocateCommandBuffers failed");
        return data;
    }

    VkSemaphoreCreateInfo semaphore_info = { 
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    if(vkCreateSemaphore(gpu.device, &semaphore_info, NULL, &semaphore) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateSemaphore failed");
        return data;
    }

    // assign
    data.gfx_pool  = pool;
    data.gfx_cmd   = cmd;
    data.semaphore = semaphore;

    return data;
}

VkCommandPool dm_vulkan_create_single_use_pool(dm_vulkan_gpu gpu)
{
    VkCommandPool pool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex=gpu.gfx_index
    };

    if(vkCreateCommandPool(gpu.device, &info, NULL, &pool) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateCommandPool");
        return VK_NULL_HANDLE;
    }

    return pool;
}

VkSemaphore dm_vulkan_create_timeline_semaphore(dm_vulkan_gpu gpu, u64 value)
{
    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkSemaphoreTypeCreateInfo type_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue=value
    };
    VkSemaphoreCreateInfo info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=&type_info
    };
    if(vkCreateSemaphore(gpu.device, &info, NULL, &semaphore) != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateSemaphore failed");
        return VK_NULL_HANDLE;
    }

    return semaphore;
}

//
bool dm_renderer_init(dm_context* context)
{
    LOG_INFO("Initializing Vulkan backend...");

    VkInstance instance    = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;

    dm_vulkan_gpu gpu = { 0 };
    dm_vulkan_surface surface = { 0 };
    dm_vulkan_swapchain swapchain = { 0 };
    dm_vulkan_frame_data frame_data[DM_FRAMES_IN_FLIGHT] = { 0 };

    VkCommandPool single_use_pool    = VK_NULL_HANDLE;
    VkSemaphore   timeline_semaphore = VK_NULL_HANDLE;
    u64           timeline_value     = DM_FRAMES_IN_FLIGHT - 1;
    
    //
    if(volkInitialize() != VK_SUCCESS) return false;

    instance = dm_vulkan_create_instance();
    if(instance == VK_NULL_HANDLE) return false;

    surface.surface = dm_window_create_vulkan_surface(context, instance); 
    if(surface.surface == VK_NULL_HANDLE) { LOG_ERROR("Could not create Vulkan surface."); return false; }
    gpu = dm_vulkan_create_gpu(instance, surface);
    if(gpu.device == VK_NULL_HANDLE) { LOG_ERROR("Creating Vulkan GPU failed"); return false; }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.physical, surface.surface, &surface.capabilities);

    volkLoadDevice(gpu.device);

    vkGetDeviceQueue(gpu.device, gpu.gfx_index, 0, &gpu.gfx_queue);

    allocator = create_vma_allocator(instance, gpu.physical, gpu.device);
    if(allocator == VK_NULL_HANDLE) return false; 

    swapchain = dm_vulkan_create_swapchain(gpu, surface, allocator);
    if(swapchain.swapchain == VK_NULL_HANDLE) { LOG_ERROR("Could not create swapchain."); return false; }

    for(u32 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        frame_data[i] = dm_vulkan_create_frame_data(gpu);
        if(frame_data[i].gfx_pool == VK_NULL_HANDLE)
        {
            LOG_ERROR("Could not create frame data for frame %u", i);
            return false;
        }
    }

    single_use_pool = dm_vulkan_create_single_use_pool(gpu);
    if(single_use_pool == VK_NULL_HANDLE) { LOG_ERROR("Could not create single use pool."); return false; }

    // timeline semaphore
    timeline_semaphore = dm_vulkan_create_timeline_semaphore(gpu, timeline_value);
    if(timeline_semaphore == VK_NULL_HANDLE) { LOG_ERROR("Could not create timeline semaphore."); return false; }

    // assign
    dm_vulkan_renderer* renderer = dm_arena_alloc(&context->arena, sizeof(dm_vulkan_renderer), &context->renderer.offset);

    renderer->instance = instance;
    renderer->allocator = allocator;
    renderer->gpu = gpu;
    renderer->surface = surface;
    renderer->swapchain = swapchain;
    for(u32 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        renderer->frame_data[i] = frame_data[i];
    }
    renderer->single_use_pool = single_use_pool;
    renderer->timeline_semaphore = timeline_semaphore;
    renderer->timeline_value = timeline_value;

    return true;
}

void dm_renderer_shutdown(dm_context* context)
{
    dm_vulkan_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_surface surface = renderer->surface;

    vkDeviceWaitIdle(gpu.device);

    vkDestroyCommandPool(gpu.device, renderer->single_use_pool, NULL);
    for(u32 i=0; i<DM_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyCommandPool(gpu.device, renderer->frame_data[i].gfx_pool, NULL);
        vkDestroySemaphore(gpu.device, renderer->frame_data[i].semaphore, NULL);
    }

    dm_vulkan_destroy_swapchain(&renderer->swapchain, gpu, renderer->allocator);

    vkDestroySemaphore(gpu.device, renderer->timeline_semaphore, NULL);

    vkDestroySurfaceKHR(renderer->instance, surface.surface, NULL);
    vkDestroyDevice(gpu.device, NULL);
    vmaDestroyAllocator(renderer->allocator);
    vkDestroyInstance(renderer->instance, NULL);

    volkFinalize();
}

bool dm_renderer_begin_frame(dm_context* context)
{
    dm_vulkan_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_swapchain swapchain = renderer->swapchain;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];

    u64 wait_value = ++renderer->timeline_value;
    wait_value -= DM_FRAMES_IN_FLIGHT;
    VkSemaphoreWaitInfo wait_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount=1,
        .pSemaphores=&renderer->timeline_semaphore,
        .pValues=&wait_value
    };
    vkWaitSemaphores(gpu.device, &wait_info, UINT64_MAX);

    vkResetCommandPool(gpu.device, frame_data.gfx_pool, 0);

    VkResult vr = vkAcquireNextImageKHR(gpu.device, swapchain.swapchain, UINT64_MAX, frame_data.semaphore, VK_NULL_HANDLE, &swapchain.index);
    if(0)
    {
        LOG_ERROR("vkAcquireNextImageKHR failed");
        return false;
    }
    dm_vulkan_swapchain_image image = swapchain.images[swapchain.index];

    VkCommandBufferBeginInfo cmd_begin = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(frame_data.gfx_cmd, &cmd_begin);

    VkImageMemoryBarrier2 barriers[] = {
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask=0,
            .dstStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image=image.image,
            .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount=1,
            .subresourceRange.layerCount=1
        },
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask=VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask=VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask=VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask=VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout=VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .image=swapchain.depth_image.image,
            .subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.levelCount=1,
            .subresourceRange.layerCount=1
        }
    };
    VkDependencyInfo dep_info = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=2,
        .pImageMemoryBarriers=barriers
    };
    vkCmdPipelineBarrier2(frame_data.gfx_cmd, &dep_info);

    // attachments
    VkRenderingAttachmentInfo color_info = {
        .sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .imageView=image.view,
        .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color.float32[0] = 0.5f,
        .clearValue.color.float32[1] = 0.2f,
        .clearValue.color.float32[2] = 0.2f,
        .clearValue.color.float32[3] = 1.f,
    };

    VkRenderingAttachmentInfo depth_info = {
        .sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageLayout=VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .imageView=swapchain.depth_image.view,
        .loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.depthStencil.depth = 1.f
    };
    VkRenderingInfo render_info = {
        .sType=VK_STRUCTURE_TYPE_RENDERING_INFO,
        .colorAttachmentCount=1,
        .pColorAttachments=&color_info,
        .pDepthAttachment=&depth_info,
        .layerCount=1,
        .renderArea.extent.width=swapchain.width,
        .renderArea.extent.height=swapchain.height
    };
    vkCmdBeginRendering(frame_data.gfx_cmd, &render_info);

    renderer->swapchain = swapchain;
    
    return true;
}

bool dm_renderer_end_frame(dm_context* context)
{
    dm_vulkan_renderer *renderer = dm_arena_get_ptr(context->arena, context->renderer.offset);

    dm_vulkan_gpu gpu = renderer->gpu;
    dm_vulkan_frame_data frame_data = renderer->frame_data[renderer->frame_index];
    dm_vulkan_swapchain_image image = renderer->swapchain.images[renderer->swapchain.index];

    vkCmdEndRendering(frame_data.gfx_cmd);

    VkImageMemoryBarrier2 present_barrier = {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask=VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask=0,
        .oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image=image.image,
        .subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.layerCount=1,
        .subresourceRange.levelCount=1
    };
    VkDependencyInfo present_dep_info = {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount=1,
        .pImageMemoryBarriers=&present_barrier
    };
    vkCmdPipelineBarrier2(frame_data.gfx_cmd, &present_dep_info);

    vkEndCommandBuffer(frame_data.gfx_cmd);

    VkSemaphoreSubmitInfo semaphore_wait_info = {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore=frame_data.semaphore,
        .stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
    };

    VkSemaphoreSubmitInfo signal_semaphores[] = {
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore=image.semaphore,
            .stageMask=VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        },
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore=renderer->timeline_semaphore,
            .stageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .value=renderer->timeline_value
        },
    };

    VkCommandBufferSubmitInfo gfx_cmd_submit = {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer=frame_data.gfx_cmd
    };

    VkSubmitInfo2 submit = {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount=1,
        .pCommandBufferInfos=&gfx_cmd_submit,
        .waitSemaphoreInfoCount=1,
        .pWaitSemaphoreInfos=&semaphore_wait_info,
        .signalSemaphoreInfoCount=2,
        .pSignalSemaphoreInfos=signal_semaphores
    };
    vkQueueSubmit2(gpu.gfx_queue, 1, &submit, NULL);

    VkPresentInfoKHR present_info = {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount=1,
        .pSwapchains=&renderer->swapchain.swapchain,
        .waitSemaphoreCount=1,
        .pWaitSemaphores=&image.semaphore,
        .pImageIndices=&renderer->swapchain.index
    };

    vkQueuePresentKHR(gpu.gfx_queue, &present_info);

    //
    renderer->frame_index++;
    renderer->frame_index %= DM_FRAMES_IN_FLIGHT;

    return true;
}
