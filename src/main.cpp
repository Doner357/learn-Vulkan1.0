// GLFW will include its own definitions and automatically
// load the Vulkan header with it.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>

// Window's width and height
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

// Manully load Create Debug Utils Messenger function since it's a extension function.
VkResult CreateDebugUtilsMessengerEXT(
        VkInstance                                instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks*              pAllocator,
        VkDebugUtilsMessengerEXT*                 pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT"
    );
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
// Manully load Destroy Debug Utils Messenger function
void DestroyDebugUtilsMessengerEXT(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     debugMessenger,
    const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool isComplete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;   // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images) 
    std::vector<VkSurfaceFormatKHR> formats;        // Surface formats (pixel format, color space)
    std::vector<VkPresentModeKHR>   present_modes;  // Available presentation modes
};


class HelloTriangleApplication {
    public:
        void run() {
            initWindow();
            initVulkan();
            mainLoop();
            cleanup();
        }

    private:
        // Members
        GLFWwindow*              window;          // GLFW provided window

        VkInstance               instance;        // The instance of Vulkan library
        VkDebugUtilsMessengerEXT debug_messenger; // Manually-handled debug messenger
        VkSurfaceKHR             surface;         // Surface extension for window, which is optional

        // For Physical Device
        VkPhysicalDevice         physical_device = VK_NULL_HANDLE; // Physical device like graphic card

        // For Logical Device
        VkDevice                 device;          // Logical Device. Important!
        VkQueue                  graphics_queue;  // Queues along with logical device (graphics)
        VkQueue                  present_queue;   // Queues along with logical device (surface presentation)

        // For swapchain
        VkSwapchainKHR           swapchain;                 // Swapchain handle
        std::vector<VkImage>     swapchain_images;          // The images in swapchain
        VkFormat                 swapchain_images_format;   // The format of swapchain images
        VkExtent2D               swapchain_extent;          // The extent info of swapchain images
        std::vector<VkImageView> swapchain_image_views;     // Image View objects for swapchain images

        // For pipeline
        VkPipelineLayout pipeline_layout;

        // Initialize GLFW window
        void initWindow() {
            // Initialize GLFW library
            glfwInit();
            // Hint GLFW not to create an OpenGL context
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            // Tell GLFW window the window is unresizable
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            // Create the actuall window
            window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        }

        void initVulkan() {
            createInstance();
            setupDebugMessenger();
            createSurface();
            pickPhysicalDevice();
            createLogicalDevice();
            createSwapchain();
            createImageViews();
            createGraphicsPipeline();
        }

        void mainLoop() {
            // The application will run until either and error occrus or the
            // window is closed.
            while (!glfwWindowShouldClose(window)) {
                glfwPollEvents();
            }
            
        }

        void cleanup() {

            for (auto image_view : swapchain_image_views) {
                vkDestroyImageView(device, image_view, nullptr);
            }
            vkDestroySwapchainKHR(device, swapchain, nullptr);

            vkDestroyDevice(device, nullptr);

            if (enable_validation_layers) {
                DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
            }

            vkDestroySurfaceKHR(instance, surface, nullptr);
            // Destroy Vulkan instance
            vkDestroyInstance(instance, nullptr);

            // Clean up GLFW resources
            glfwDestroyWindow(window);

            // Terminate GLFW
            glfwTerminate();
        }


        //////////////////////////////////////////////////////////////////
        // Instance, Debug Utils Messenger, and also window surface
        //////////////////////////////////////////////////////////////////
        void createInstance() {
            if (enable_validation_layers && !checkValidationLayerSupport()) {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            // Optional, provide appication info may optimize the program.
            VkApplicationInfo app_info{};
            app_info.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pApplicationName = "Hello Triangle";
            app_info.apiVersion       = VK_MAKE_VERSION(1, 0, 0);
            app_info.pEngineName      = "No Engine";
            app_info.engineVersion    = VK_MAKE_VERSION(1, 0, 0);
            app_info.apiVersion       = VK_API_VERSION_1_0;

            // Necessary, tells the Vulkan drive which global extensions and
            // validation layers want to use.
            VkInstanceCreateInfo create_info{};
            create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            create_info.pApplicationInfo = &app_info;
            
            // Get and print required extensions
            auto required_extensions = getRequiredExtensions();
            std::cout << "required extensions:\n";
            for (uint32_t i = 0; i < static_cast<uint32_t>(required_extensions.size()); i++) {
                std::cout << '\t' << required_extensions[i] << '\n';
            }
            std::cout << '\n';

            // Enumerate the supported extensions in current environment
            uint32_t supported_extensions_count = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &supported_extensions_count, nullptr);
            std::vector<VkExtensionProperties> supported_extensions(supported_extensions_count);
            vkEnumerateInstanceExtensionProperties(nullptr, &supported_extensions_count, supported_extensions.data());
            
            std::cout << "available extensions:\n";
            for (const auto& extension : supported_extensions) {
                std::cout << '\t' << extension.extensionName << '\n';
            }
            std::cout << '\n';

            // Check if all the extensions needed by GLFW is supported.
            std::set<std::string> check_required_extensions(
                required_extensions.begin(), required_extensions.end()
            );
            for (const auto& extension : supported_extensions) {
                check_required_extensions.erase(extension.extensionName);
            }
            if (!check_required_extensions.empty()) {
                std::cout << "missing extension(s) needed by GLFW:\n";
                for (const auto& extension : check_required_extensions) {
                    std::cout << '\t' << extension << '\n';
                }
                throw std::runtime_error("exist missing extension(s)!");
            }

            create_info.enabledExtensionCount   = static_cast<uint32_t>(required_extensions.size()); // Number of extensions
            create_info.ppEnabledExtensionNames = required_extensions.data();                        // What kind of extensions
            
            // Add layer info if it's enable
            VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
            if (enable_validation_layers) {
                create_info.enabledLayerCount   = static_cast<uint32_t>(validation_layers.size());
                create_info.ppEnabledLayerNames = validation_layers.data();

                // Special debug utils messenger for Create instance and Destroy instance functions.
                populateDebugMessengerCreateInfo(debug_create_info);
                create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
            }
            else {
                create_info.enabledLayerCount = 0;

                create_info.pNext = nullptr;
            }

            // Create and check the instance
            if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }
        }

        std::vector<const char*> getRequiredExtensions() {
            // GLFW has a handy built-in function that returns the extensions it needs
            // to do that which we can pass to the struct.
            uint32_t glfw_extension_count = 0;
            const char** glfw_extensions;
            glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

            std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

            if (enable_validation_layers) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
            create_info = {};
            create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            create_info.pfnUserCallback = debugCallBack;
            create_info.pUserData       = nullptr;
        }

        void setupDebugMessenger() {
            if (!enable_validation_layers) {
                return;
            }

            VkDebugUtilsMessengerCreateInfoEXT create_info;
            populateDebugMessengerCreateInfo(create_info);

            if (CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
                throw std::runtime_error("failed to set up debug messenger!");
            }
        }

        // Check if all the required layers are available
        bool checkValidationLayerSupport() {
            uint32_t layer_count;
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

            std::vector<VkLayerProperties> available_layers(layer_count);
            vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

            for (const char* layer_name : validation_layers) {
                bool layer_found = false;

                for (const auto& layer_properties : available_layers) {
                    if (std::strcmp(layer_name, layer_properties.layerName) == 0) {
                        layer_found = true;
                        break;
                    }
                }

                if (!layer_found) {
                    return false;
                }
            }

            return true;
        }

        // Customized Debug Callback
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
            VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
            VkDebugUtilsMessageTypeFlagsEXT             message_type,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallback_date,
            void*                                       pUser_date
        ) {
            std::cerr << "validation layer: " << pCallback_date->pMessage << std::endl;

            return VK_FALSE;
        }

        // Get window's surface handle from glfw
        void createSurface() {
            if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("failed to create window surface!");
            }
        }


        //////////////////////////////////////////////////////////////////
        // Physical device
        //////////////////////////////////////////////////////////////////
        void pickPhysicalDevice() {
            uint32_t device_count = 0;
            vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

            if (device_count == 0) {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(device_count);
            vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

            // Check if any of the physical devices meet the requirements
            // that we will add to that function.
            for (const auto& device : devices) {
                if (isDeviceSuitable(device)) {
                    physical_device = device;
                    break;
                }
            }

            if (physical_device == VK_NULL_HANDLE) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        }

        // Check if the device is suitable
        bool isDeviceSuitable(VkPhysicalDevice device) {
            QueueFamilyIndices indices = findQueueFamilies(device);

            bool extensions_supported = checkDeviceExtensionSupport(device);

            bool swapchain_adequate = false;
            if (extensions_supported) {
                SwapchainSupportDetails swapchain_support = querySwapchainSupport(device);
                swapchain_adequate = 
                    !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
            }

            return indices.isComplete() && extensions_supported && swapchain_adequate;
        }

        // Check if the given device support required extensions.
        bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extension_count = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

            std::vector<VkExtensionProperties> available_extensions(extension_count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

            std::set<std::string> get_required_extensions(device_extensions.begin(), device_extensions.end());

            for (const auto& extension : available_extensions) {
                get_required_extensions.erase(extension.extensionName);
            }

            return get_required_extensions.empty();
        }

        // Find suitable queue family
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
            QueueFamilyIndices indices;
            // Logic to find queue family indices to populate struct with
            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

            std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

            int i = 0;
            for (const auto& queue_family : queue_families) {
                if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphics_family = i;
                }

                // Check if queue family support presentation
                VkBool32 present_support = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

                if (present_support) {
                    indices.present_family = i;
                }

                if (indices.isComplete()) {
                    break;
                }

                i++;
            }

            return indices;
        }

        SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) {
            SwapchainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

            uint32_t format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

            if (format_count != 0) {
                details.formats.resize(format_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
            }

            uint32_t present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

            if (present_mode_count != 0) {
                details.present_modes.resize(present_mode_count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
            }

            return details;
        }


        //////////////////////////////////////////////////////////////////
        // Logical device
        //////////////////////////////////////////////////////////////////
        void createLogicalDevice() {
            QueueFamilyIndices indices = findQueueFamilies(physical_device);

            std::vector<VkDeviceQueueCreateInfo>  queue_create_infos;
            std::set<uint32_t> unique_queue_families = {
                indices.graphics_family.value(),
                indices.present_family.value()
            };

            // influence the scheduling of command buffer execution using floating 
            // point numbers between 0.0 and 1.0.
            float queue_priority = 1.0f;
            for (uint32_t queue_family : unique_queue_families) {
                VkDeviceQueueCreateInfo queue_create_info{};
                queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = queue_family;
                queue_create_info.queueCount       = 1;
                queue_create_info.pQueuePriorities = &queue_priority;
                queue_create_infos.push_back(queue_create_info);
            }

            // No need any feature for now
            VkPhysicalDeviceFeatures device_features{};

            VkDeviceCreateInfo create_info{};
            create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.pQueueCreateInfos       = queue_create_infos.data();
            create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());;
            create_info.pEnabledFeatures        = &device_features;
            create_info.enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size());
            create_info.ppEnabledExtensionNames = device_extensions.data();

            if (enable_validation_layers) {
                create_info.enabledLayerCount   = static_cast<uint32_t>(validation_layers.size());
                create_info.ppEnabledLayerNames = validation_layers.data();
            }
            else {
                create_info.enabledLayerCount = 0;
            }

            if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }

            // Get queues' handle
            vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
            vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
        }


        //////////////////////////////////////////////////////////////////
        // Swapchain (Swap Chain)
        //////////////////////////////////////////////////////////////////
        void createSwapchain() {
            SwapchainSupportDetails swapchain_support = querySwapchainSupport(physical_device);

            VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
            VkPresentModeKHR   present_mode   = chooseSwapPresentMode(swapchain_support.present_modes);
            VkExtent2D         extent         = chooseSwapExtent(swapchain_support.capabilities);

            uint32_t image_count = swapchain_support.capabilities.minImageCount + 1; // May get only one image if don't add 1
            if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
                image_count = swapchain_support.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR create_info{};
            create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.surface          = surface;
            // Swapchain images
            create_info.minImageCount    = image_count;
            create_info.imageFormat      = surface_format.format;
            create_info.imageColorSpace  = surface_format.colorSpace;
            create_info.imageExtent      = extent;
            create_info.imageArrayLayers = 1;
            create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(physical_device);
            uint32_t QueueFamilyIndices[] = {
                indices.graphics_family.value(),
                indices.present_family.value()
            };

            if (indices.graphics_family != indices.present_family) {
                create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = 2;
                create_info.pQueueFamilyIndices   = QueueFamilyIndices;
            }
            else {
                create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                create_info.queueFamilyIndexCount = 0;       // Optional
                create_info.pQueueFamilyIndices   = nullptr; // Optioanl
            }

            // What kind of transform should apply (for example, rotate 90 degrees).
            create_info.preTransform = swapchain_support.capabilities.currentTransform;
            
            // If the alpha channel should be used for blending with other windows in the window system.
            create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Opaque (ignore alpha)

            create_info.presentMode = present_mode;
            create_info.clipped     = VK_TRUE;  // We don't care about the color of pixels that are obscured.

            create_info.oldSwapchain = VK_NULL_HANDLE; // Just empty now

            if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swapchain!");
            }

            // Get the handle of images in the swapchain
            vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
            swapchain_images.resize(image_count);
            vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());

            swapchain_images_format = surface_format.format;
            swapchain_extent        = extent;
        }

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
            for (const auto& available_format : available_formats) {
                if (
                    available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                ) {
                    return available_format;
                }
            }

            return available_formats[0];
        }

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
            for (const auto& available_present_mode : available_present_modes) {
                if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return available_present_mode;
                }
            }
            
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            }
            else {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actual_extent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actual_extent.width = std::clamp(
                    actual_extent.width,
                    capabilities.minImageExtent.width,
                    capabilities.maxImageExtent.width
                );
                actual_extent.height = std::clamp(
                    actual_extent.height,
                    capabilities.minImageExtent.height,
                    capabilities.maxImageExtent.height
                );

                return actual_extent;
            }
        }

        void createImageViews() {
            swapchain_image_views.resize(swapchain_images.size());

            for(size_t i = 0; i < swapchain_images.size(); i++) {
                VkImageViewCreateInfo create_info{};
                create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                create_info.image    = swapchain_images[i];
                create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;   // How to treat the image
                create_info.format   = swapchain_images_format;

                // Channels swizzle
                create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                // "subresourceRange" field describes what the image's purpose is and which part of the image should be accessed.
                create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                create_info.subresourceRange.baseMipLevel   = 0;
                create_info.subresourceRange.levelCount     = 1;
                create_info.subresourceRange.baseArrayLayer = 0;
                create_info.subresourceRange.layerCount     = 1;

                if (vkCreateImageView(device, &create_info, nullptr, &swapchain_image_views[i])) {
                    throw std::runtime_error("failed to create image views!");
                }
            }

        }


        //////////////////////////////////////////////////////////////////
        // Graphics Pipelines
        //////////////////////////////////////////////////////////////////
        void createGraphicsPipeline() {
            // -- Programable Shaders --
            auto vert_shader_code = readFile("shaders/shader.vert.spv");
            auto frag_shader_code = readFile("shaders/shader.frag.spv");

            VkShaderModule vert_shader_module = createShaderModule(vert_shader_code);
            VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

            // Since the shader code has been stored in the pipeline, free the module.
            vkDestroyShaderModule(device, vert_shader_module, nullptr);
            vkDestroyShaderModule(device, frag_shader_module, nullptr);

            VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
            vert_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            // Telling Vulkan in which pipeline stage the shader is going to be used.
            vert_shader_stage_info.stage               = VK_SHADER_STAGE_VERTEX_BIT;
            vert_shader_stage_info.module              = vert_shader_module;
            // Where is the start function entry, "main" function in this case.
            vert_shader_stage_info.pName               = "main";
            // Configured at pipeline creation by specifying different values for the constants used in it.
            vert_shader_stage_info.pSpecializationInfo = nullptr; // nullptr means no such constant

            VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
            frag_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_shader_stage_info.stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_shader_stage_info.module              = frag_shader_module;
            frag_shader_stage_info.pName               = "main";
            frag_shader_stage_info.pSpecializationInfo = nullptr;

            VkPipelineShaderStageCreateInfo shader_stages[] = {
                vert_shader_stage_info,
                frag_shader_stage_info
            };


            // -- Vertex layout --
            // Tell Vulkan the pattern of veticies (similar to glVertexAttribIPointer in OpenGL)
            VkPipelineVertexInputStateCreateInfo vertex_input_info{};
            vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_info.vertexBindingDescriptionCount   = 0;
            vertex_input_info.pVertexAttributeDescriptions    = nullptr; // Optional
            vertex_input_info.vertexAttributeDescriptionCount = 0;
            vertex_input_info.pVertexAttributeDescriptions    = nullptr; // Optional


            // -- Vertex Assembly --
            // The assembly state in pipeline (similar to the "GL_TRIANGLES" part in glDrawArray(...);)
            VkPipelineInputAssemblyStateCreateInfo input_assembly{};
            input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly.primitiveRestartEnable = VK_FALSE;


            // -- Viewport --
            // Create view port for framebuffer
            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = static_cast<float>(swapchain_extent.width);
            viewport.height   = static_cast<float>(swapchain_extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            // Scissor define in which regions pixels will actually be stored.
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapchain_extent;

            VkPipelineViewportStateCreateInfo viewport_state{};
            viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state.viewportCount = 1;
            viewport_state.pViewports    = &viewport;
            viewport_state.scissorCount  = 1;
            viewport_state.pScissors     = &scissor;


            // -- Rasterizer -- (Set Depth Testing, Face Culling, etc...)
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            // If set to VK_TRUE, fragments that are beyond the near and far
            // planes are clamped to them as opposed to discarding them.
            rasterizer.depthBiasEnable = VK_FALSE;
            // If set to VK_TRUE, then geometry never passes through the
            // rasterizer stage. This basically disables any output to the framebuffer.
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            // How fragments are generated for geometry.
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            // How wide the line should be, thicker than 1.0 requires GPU features
            rasterizer.lineWidth = 1.0f;
            // Face culling setting
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            // Alter the value of depth depends on constant value
            rasterizer.depthBiasEnable         = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
            rasterizer.depthBiasClamp          = 0.0;   // Optional
            rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional


            // -- Multisampling --
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable   = VK_FALSE;
            multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading      = 1.0f;     // Optional
            multisampling.pSampleMask           = nullptr;  // Optional
            multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
            multisampling.alphaToOneEnable      = VK_FALSE; // Optional


            // -- Depth and stencil testing --
            // Leave it nullptr

            // -- Color Blending --
            // Blending setting for each attachment
            VkPipelineColorBlendAttachmentState color_blend_attachment{};
            color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                    VK_COLOR_COMPONENT_G_BIT |
                                                    VK_COLOR_COMPONENT_B_BIT |
                                                    VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment.blendEnable         = VK_FALSE;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;            // Optional
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;  // Optional
            color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;       // Optional
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
            color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;       // Optional
            // The above set up act like following pseudocode
            /*
            if (blendEnable) {
                finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
                finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
            }
            else {
                finalColor = newColor;
            }
            finalColor = finalColor & colorWriteMask;
            */

            // Global blending setting / blending state configuration
            VkPipelineColorBlendStateCreateInfo color_blending{};
            color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blending.logicOpEnable = VK_FALSE;
            color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
            color_blending.attachmentCount = 1;
            color_blending.pAttachments = &color_blend_attachment;
            color_blending.blendConstants[0] = 0.0f; // R, Optional
            color_blending.blendConstants[1] = 0.0f; // G, Optional
            color_blending.blendConstants[2] = 0.0f; // B, Optional
            color_blending.blendConstants[3] = 0.0f; // A, Optional


            // -- Pipeline Layout (uniform in shader) --
            VkPipelineLayoutCreateInfo pipeline_layout_info{};
            pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount         = 0;        // Optional
            pipeline_layout_info.pSetLayouts            = nullptr;  // Optional
            pipeline_layout_info.pushConstantRangeCount = 0;        // Optional
            pipeline_layout_info.pPushConstantRanges    = nullptr;  // Optional

            if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        }

        // Read \SPIR-V shader byte codes
        static std::vector<char> readFile(const std::string& filename) {
            // "std::ios::ate" means read file from the end so we can easily know the size of file.
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error(std::string("failed to open file: ") + filename);
            }

            size_t file_size = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(file_size);

            std::cout << "The size of SPIR-V shader \""
                      << filename << '\"' << ": " << file_size << " bytes\n";

            // Seek back to the start of file then read file.
            file.seekg(0);
            file.read(buffer.data(), file_size);

            file.close();
            return buffer;
        }

        VkShaderModule createShaderModule(const std::vector<char>& code) {
            VkShaderModuleCreateInfo create_info {};
            create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = code.size();
            create_info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shader_module;
            if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }

            return shader_module;
        }
};


int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}