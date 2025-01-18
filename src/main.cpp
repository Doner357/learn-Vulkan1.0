// GLFW will include its own definitions and automatically
// load the Vulkan header with it.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <stb/stb_image.h>
#include <tinyobj/tiny_obj_loader.h>


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
#include <array>
#include <chrono>
#include <unordered_map>

#include "headers/camera.hpp"

// Window's width and height
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

// Models path
const std::string MODEL_PATH = "models/viking_room/viking_room.obj";
const std::string TEXTRUE_PATH = "models/viking_room/viking_room.png";

// How many in-flight frames
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE1_EXTENSION_NAME
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


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 tex_coord;

    // Same as providing some parameters of glVertexAttribIPointer
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding   = 0;                              // The index of the binding in the array of bindings.
        binding_description.stride    = sizeof(Vertex);                 // The number of bytes from one entry to the next
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;    // Could be per vertex or per instance

        return binding_description;
    }

    // Same as providing some parameters of glVertexAttribIPointer
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
        
        // For vertex position
        attribute_descriptions[0].binding  = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset   = offsetof(Vertex, pos);
        // For vertex color
        attribute_descriptions[1].binding  = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset   = offsetof(Vertex, color);
        // For texture coordinates
        attribute_descriptions[2].binding  = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset   = offsetof(Vertex, tex_coord);

        return attribute_descriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
    }
};

// Custom hash function for Vertex struct
namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                    (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                    (hash<glm::vec2>()(vertex.tex_coord) << 1);
        }
    };
}

// Structure for model-view-projection matrix
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


class HelloTriangleApplication {
    public:
        void run() {
            initWindow();
            initVulkan();
            initCamera();
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

        // For render passes & descriptor set layout & pipeline
        VkRenderPass          render_pass;
        VkPipelineLayout      pipeline_layout;
        VkDescriptorSetLayout descriptor_set_layout;
        VkPipeline            graphics_pipeline;

        // For command pool
        VkCommandPool   command_pool;

        // For command buffer
        std::vector<VkCommandBuffer> command_buffers;

        // For the swapchain's framebuffers
        std::vector<VkFramebuffer> swapchain_framebuffers;

        // For sync objects
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence>     inflight_fences;

        // Addtional check to guarantee the recreation of swapchain is needed.
        bool framebuffer_resized = false;

        // Current rendering frame
        uint32_t current_frame = 0;

        // Vertices for rectangle
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        std::vector<Vertex> vertices;
        // Indices for rectangle
        VkBuffer index_buffer;
        VkDeviceMemory index_memory;
        std::vector<uint32_t> indices;

        // Uniform buffer for transformation matrix
        // Use multiple buffer to prevent the in-flight frame read the
        // buffer which is still read by previous frame's draw command.
        std::vector<VkBuffer>       uniform_buffers;
        std::vector<VkDeviceMemory> uniform_buffers_memory;
        std::vector<void*>          uniform_buffers_mapped;

        // Descriptor Pool for uniform buffers
        VkDescriptorPool descriptor_pool;
        std::vector<VkDescriptorSet> descriptor_sets;

        // Image objects
        uint32_t       mip_levels;
        VkImage        texture_image;
        VkDeviceMemory texture_image_memroy;
        VkImageView    texture_image_view;
        VkSampler      texture_sampler;

        // Depth attachment
        VkImage        depth_image;
        VkDeviceMemory depth_image_memory;
        VkImageView    depth_image_view;

        // Camera
        lvk::ArcballCamera camera;


        // Time
        float current_time = 0;
        float previous_time = 0;
        float delta_time = 0;


        // Initialize GLFW window
        void initWindow() {
            // Initialize GLFW library
            glfwInit();
            // Hint GLFW not to create an OpenGL context
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            // Tell GLFW window the window is unresizable
            //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            // Create the actuall window
            window = glfwCreateWindow(WIDTH, HEIGHT, reinterpret_cast<const char*>(u8"Learn-Vulkan"), nullptr, nullptr);
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
            glfwSetScrollCallback(window, scroll_callback);
            glfwSetCursorPosCallback(window, mouse_callback);
        }

        // GLFW call back for resize window
        static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
            app->framebuffer_resized = true;
        }

        static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
            // make sure the viewport matches the new window dimensions; note that width and 
            // height will be significantly larger than specified on retina displays.
            glViewport(0, 0, width, height);
        }

        // glfw: whenever the mouse scroll wheel scrolls, this callback is called
        static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
            auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
            app->camera.radiusMove(static_cast<float>(yoffset));
        }

        static void mouse_callback(GLFWwindow *window, double x_pos, double y_pos) {
            static bool   drag_first_time = true;
            static double pre_x_position  = 0;
            static double pre_y_position  = 0;

            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {

                if (drag_first_time) {
                    pre_x_position  = x_pos;
                    pre_y_position  = y_pos;
                    drag_first_time = false;
                }
                float delta_x = pre_x_position - x_pos;
                float delta_y = pre_y_position - y_pos;

                auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));

                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    app->camera.centerMove(app->delta_time * delta_x, app->delta_time * -delta_y);
                }
                else {
                    app->camera.spherePosMove(delta_x, delta_y);
                }

                pre_x_position = x_pos;
                pre_y_position = y_pos;
            }
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
                drag_first_time = true;
            }
            
        }


        void initCamera() {
            camera = lvk::ArcballCamera(
                glm::vec3(0.0f, 0.0f, 2.0f),   // Camera position
                glm::vec3(0.0f, 0.0f, 0.0f),   // Look at the origin
                glm::vec3(0.0f, 1.0f, 0.0f),   // Up vector
                45.0f,                         // Field of view in degrees
                static_cast<float>(swapchain_extent.width) / swapchain_extent.height, // Aspect ratio
                0.1f,                          // Near plane
                100.0f,                        // Far plane
                15.0f,
                0.5f
            );
        }


        void initVulkan() {
            createInstance();
            setupDebugMessenger();
            createSurface();
            pickPhysicalDevice();
            createLogicalDevice();
            createSwapchain();
            createImageViews();
            createRenderPass();
            createDescriptorLayout();
            createGraphicsPipeline();
            createCommandPool();
            createDepthResorces();
            createFramebuffers();
            createTextureImage();
            createTextureImageView();
            createTextureSampler();
            loadModel();
            createVertexBuffer();
            createIndexBuffer();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
            createCommandBuffers();
            createSyncObjects();
        }


        void mainLoop() {
            // The application will run until either and error occrus or the
            // window is closed.
            while (!glfwWindowShouldClose(window)) {
                updateClock();
                glfwPollEvents();
                drawFrame();
            }
            
            // Wait for all the operations run on device are complete.
            vkDeviceWaitIdle(device);
        }

        // Update current time and delta time
        void updateClock() {
            current_time = static_cast<float>(glfwGetTime());
            delta_time = current_time - previous_time;
            previous_time = current_time;
        }


        // Cleanup objects for swapchain
        void cleanupSwapchain() {
            vkDestroyImageView(device, depth_image_view, nullptr);
            vkDestroyImage(device, depth_image, nullptr);
            vkFreeMemory(device, depth_image_memory, nullptr);

            for (auto framebuffer : swapchain_framebuffers) {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
            
            for (auto image_view : swapchain_image_views) {
                vkDestroyImageView(device, image_view, nullptr);
            }

            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        void cleanup() {
            cleanupSwapchain();

            vkDestroySampler(device, texture_sampler, nullptr);
            vkDestroyImageView(device, texture_image_view, nullptr);
            vkDestroyImage(device, texture_image, nullptr);
            vkFreeMemory(device, texture_image_memroy, nullptr);

            vkDestroyBuffer(device, index_buffer, nullptr);
            vkFreeMemory(device, index_memory, nullptr);

            vkDestroyBuffer(device, vertex_buffer, nullptr);
            vkFreeMemory(device, vertex_buffer_memory, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyBuffer(device, uniform_buffers[i], nullptr);
                vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
            }

            vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

            vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
                vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
                vkDestroyFence(device, inflight_fences[i], nullptr);                
            }

            vkDestroyCommandPool(device, command_pool, nullptr);

            vkDestroyPipeline(device, graphics_pipeline, nullptr);
            vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            vkDestroyRenderPass(device, render_pass, nullptr);

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
            app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pApplicationName   = "Hello Triangle";
            app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            app_info.pEngineName        = "No Engine";
            app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
            app_info.apiVersion         = VK_API_VERSION_1_0;

            // Necessary, tells the Vulkan drive which global extensions and
            // validation layers want to use.
            VkInstanceCreateInfo create_info{};
            create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            create_info.pApplicationInfo = &app_info;
            
            // Get and print required extensions
            auto required_extensions = getRequiredExtensions();
            std::cout << "required extensions:\n";
            for (auto extension : required_extensions) {
                std::cout << '\t' << extension << '\n';
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

            VkPhysicalDeviceFeatures support_features;
            vkGetPhysicalDeviceFeatures(device, &support_features);

            return indices.isComplete() && extensions_supported && swapchain_adequate && support_features.samplerAnisotropy;
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
            device_features.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo create_info{};
            create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.pQueueCreateInfos       = queue_create_infos.data();
            create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
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
            uint32_t queue_family_indices[] = {
                indices.graphics_family.value(),
                indices.present_family.value()
            };

            if (indices.graphics_family != indices.present_family) {
                create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = 2;
                create_info.pQueueFamilyIndices   = queue_family_indices;
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
                swapchain_image_views[i] = createImageView(swapchain_images[i], swapchain_images_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            }

        }


        //////////////////////////////////////////////////////////////////
        // Render Passes
        //////////////////////////////////////////////////////////////////
        void createRenderPass() {
            // -- Color Attachment descriptions --
            VkAttachmentDescription color_attachment{};
            color_attachment.format         = swapchain_images_format;

            // Set the number of samples for multisampling
            color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;

            // Operations for the start and the end of rendering
            // Note: loadOP, storeOP are for color attachments.
            //       stencilLoadOp, stencilStoreOp are for stencil attachments.
            color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            // Set how to treat the color attachment of the start and the end of render pass.
            // Note that vulkan's textures and framebuffers are represented by "VkImage" object,
            // So you have to set the image layout type for the image.
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


            // -- Depth Attachment descriptions --
            VkAttachmentDescription depth_attachment{};
            depth_attachment.format         = findDepthFormat();
            depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
            depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


            // -- Subpasses and attachments references --
            // -- Color attachment --
            VkAttachmentReference color_attachment_ref{};
            // Specify which attachment to reference by its index in the attachment descriptions array.
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // -- Depth attachment --
            VkAttachmentReference depth_attachment_ref{};
            depth_attachment_ref.attachment = 1;
            depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            // The index of the attachment in this array is directly referenced from the
            // fragment shader with the "layout(location = 0) out vec4" outColor directive!
            subpass.pColorAttachments       = &color_attachment_ref;
            subpass.pDepthStencilAttachment = &depth_attachment_ref;

            
            // Deal with subpass dependencies
            VkSubpassDependency dependency{};
            // Specify by giving the index of subpass
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicit subpass before or after the render pass
            dependency.dstSubpass = 0;
            // Set up the current subpass which state should wait for the previous one
            dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            // Set up which state the current subpass should start waiting,
            // so the subpass can execute the state before this state.
            dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


            // -- Render pass --
            std::array<VkAttachmentDescription, 2> attachments = {
                color_attachment,
                depth_attachment
            };
            VkRenderPassCreateInfo render_pass_info{};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            render_pass_info.pAttachments    = attachments.data();
            render_pass_info.subpassCount    = 1;
            render_pass_info.pSubpasses      = &subpass;
            render_pass_info.dependencyCount = 1;
            render_pass_info.pDependencies   = &dependency;

            if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            } 
        }


        //////////////////////////////////////////////////////////////////
        // Dexcriptor Layout for Uniform Buffer
        //////////////////////////////////////////////////////////////////
        void createDescriptorLayout() {
            VkDescriptorSetLayoutBinding ubo_layout_binding{};
            ubo_layout_binding.binding            = 0; // Same as binding index in shader
            ubo_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo_layout_binding.descriptorCount    = 1;
            ubo_layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
            ubo_layout_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding sampler_layout_binding{};
            sampler_layout_binding.binding            = 1;
            sampler_layout_binding.descriptorCount    = 1;
            sampler_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sampler_layout_binding.pImmutableSamplers = nullptr;
            sampler_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

            std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
            layout_info.pBindings    = bindings.data();

            if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
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


            // ---- Fixed States ----

            // -- Vertex layout --
            // Tell Vulkan the pattern of veticies (similar to glVertexAttribIPointer in OpenGL,
            // but you have to provide the vertex array info by passing descriptions).
            auto binding_description = Vertex::getBindingDescription();
            auto attribute_description = Vertex::getAttributeDescriptions();
            VkPipelineVertexInputStateCreateInfo vertex_input_info{};
            vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_info.vertexBindingDescriptionCount   = 1;
            vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
            vertex_input_info.pVertexBindingDescriptions      = &binding_description;
            vertex_input_info.pVertexAttributeDescriptions    = attribute_description.data();


            // -- Vertex Assembly --
            // The assembly state in pipeline (similar to the "GL_TRIANGLES" part in glDrawArray(...);)
            VkPipelineInputAssemblyStateCreateInfo input_assembly{};
            input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly.primitiveRestartEnable = VK_FALSE;


            // -- Viewport --
            
            // We use dynamic state here, which config viewport and scissor in command buffer.
            /*
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
            */

            // -- Dynamic States for Viewport and Scissor --
            std::vector<VkDynamicState> dynamic_states = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            VkPipelineDynamicStateCreateInfo dynamic_state{};
            dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
            dynamic_state.pDynamicStates    = dynamic_states.data();

            VkPipelineViewportStateCreateInfo viewport_state{};
            viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state.viewportCount = 1;
            // viewport_state.pViewports = &viewport; // We use dynamic state here
            viewport_state.scissorCount  = 1;
            // viewport_state.pScissors  = &scissor;  // We use dynamic state here


            // -- Rasterizer -- (Set Depth Testing, Face Culling, etc...)
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            // If set to VK_TRUE, fragments that are beyond the near and far
            // planes are clamped to them as opposed to discarding them.
            rasterizer.depthClampEnable = VK_FALSE;
            // If set to VK_TRUE, then geometry never passes through the
            // rasterizer stage. This basically disables any output to the framebuffer.
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            // How fragments are generated for geometry.
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            // How wide the line should be, thicker than 1.0 requires GPU features
            rasterizer.lineWidth = 1.0f;
            // Face culling setting
            rasterizer.cullMode  = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
            VkPipelineDepthStencilStateCreateInfo depth_stencil{};
            depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil.depthTestEnable  = VK_TRUE;
            depth_stencil.depthWriteEnable = VK_TRUE;
            // Specifies the comparison that is performed to keep or discard fragments
            depth_stencil.depthCompareOp        = VK_COMPARE_OP_LESS;
            depth_stencil.depthBoundsTestEnable = VK_FALSE;
            depth_stencil.minDepthBounds        = 0.0f; // Optional
            depth_stencil.maxDepthBounds        = 1.0f; // Optional
            // Not using there
            depth_stencil.stencilTestEnable     = VK_FALSE;
            depth_stencil.front = {}; // Optional
            depth_stencil.back  = {}; // Optional


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
            color_blending.logicOpEnable     = VK_FALSE;
            color_blending.logicOp           = VK_LOGIC_OP_COPY; // Optional
            color_blending.attachmentCount   = 1;
            color_blending.pAttachments      = &color_blend_attachment;
            color_blending.blendConstants[0] = 0.0f; // R, Optional
            color_blending.blendConstants[1] = 0.0f; // G, Optional
            color_blending.blendConstants[2] = 0.0f; // B, Optional
            color_blending.blendConstants[3] = 0.0f; // A, Optional


            // -- Pipeline Layout (uniform in shader) --
            VkPipelineLayoutCreateInfo pipeline_layout_info{};
            pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount         = 1;
            pipeline_layout_info.pSetLayouts            = &descriptor_set_layout;
            pipeline_layout_info.pushConstantRangeCount = 0;        // Optional
            pipeline_layout_info.pPushConstantRanges    = nullptr;  // Optional

            if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }

            
            // -- Graphics pipeline --
            VkGraphicsPipelineCreateInfo pipeline_info{};
            pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            // Programable states
            pipeline_info.stageCount          = 2;
            pipeline_info.pStages             = shader_stages;
            // Fixed states
            pipeline_info.pVertexInputState   = &vertex_input_info;
            pipeline_info.pInputAssemblyState = &input_assembly;
            pipeline_info.pViewportState      = &viewport_state;
            pipeline_info.pRasterizationState = &rasterizer;
            pipeline_info.pMultisampleState   = &multisampling;
            pipeline_info.pDepthStencilState  = &depth_stencil;
            pipeline_info.pColorBlendState    = &color_blending;
            pipeline_info.pDynamicState       = &dynamic_state;
            // Pipeline layout (uniform)
            pipeline_info.layout              = pipeline_layout;
            // Render pass
            pipeline_info.renderPass          = render_pass;
            pipeline_info.subpass             = 0;
            // Base pipeline
            pipeline_info.basePipelineHandle = VK_NULL_HANDLE;  // Optional
            pipeline_info.basePipelineIndex  = -1;              // Optional

            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_FALSE) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
            

            // Since the shader code has been stored in the pipeline, free the module.
            vkDestroyShaderModule(device, vert_shader_module, nullptr);
            vkDestroyShaderModule(device, frag_shader_module, nullptr);
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


        //////////////////////////////////////////////////////////////////
        // Swapchain's Framebuffers
        //////////////////////////////////////////////////////////////////
        void createFramebuffers() {
            swapchain_framebuffers.resize(swapchain_image_views.size());

            // Iterate all the image views and create framebuffers from them
            for (size_t i = 0; i < swapchain_image_views.size(); i++) {
                std::array<VkImageView, 2> attachments = {
                    swapchain_image_views[i],
                    depth_image_view
                };

                VkFramebufferCreateInfo framebuffer_info{};
                framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_info.renderPass      = render_pass;
                framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebuffer_info.pAttachments    = attachments.data();
                framebuffer_info.width           = swapchain_extent.width;
                framebuffer_info.height          = swapchain_extent.height;
                framebuffer_info.layers          = 1;

                if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }


        //////////////////////////////////////////////////////////////////
        // Command Pool
        //////////////////////////////////////////////////////////////////
        void createCommandPool() {
            QueueFamilyIndices queue_family_indices = findQueueFamilies(physical_device);

            // Command pools are created base on the type of queue family
            VkCommandPoolCreateInfo pool_info{};
            pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

            if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }
        }


        //////////////////////////////////////////////////////////////////
        // Buffer Utilities
        //////////////////////////////////////////////////////////////////
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& buffer_memory
        ) {
            VkBufferCreateInfo buffer_info{};
            buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size        = size;
            buffer_info.usage       = usage;                        // Something like binding buffer to specific type in OpenGL, but allow multiple types.
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    // The mode to share with different queue

            if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }

            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

            // Allocate memory for vertex buffer
            VkMemoryAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(device, buffer, buffer_memory, 0);
        } 

        uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) {
            VkPhysicalDeviceMemoryProperties mem_properties;
            vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

            for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
                if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }

            throw std::runtime_error("failed to find suitable memory type!");
        }

        void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
            VkCommandBuffer command_buffer = beginSingleTimeCommands();

            VkBufferCopy copy_region;
            copy_region.srcOffset = 0;  // Optional
            copy_region.dstOffset = 0;  // Optional
            copy_region.size      = size;
            vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

            endSingleTimeCommands(command_buffer);
        }


        //////////////////////////////////////////////////////////////////
        // Depth Attachment Resources
        //////////////////////////////////////////////////////////////////
        void createDepthResorces() {
            VkFormat depth_format = findDepthFormat();
            createImage(
                swapchain_extent.width, swapchain_extent.height, 1,
                depth_format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depth_image,
                depth_image_memory
            );
            depth_image_view = createImageView(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

            // We don't need to explicitly transition the layout of the image to
            // a depth attachment because we'll take care of this in the render pass.
            // transitionImageLayout(depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
        }

        VkFormat findSupportedFormat(
            const std::vector<VkFormat>& candidates,
            VkImageTiling tiling,
            VkFormatFeatureFlags
            features
        ) {
            for (VkFormat format : candidates) {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

                if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                    return format;
                }
                else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                    return format;
                }
            }
            // If none of the candidate formats support the desired usage,
            // then we can either return a special value or simply throw an exception
            throw std::runtime_error("failed to find supported format!");
        }

        VkFormat findDepthFormat() {
            return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        }

        bool hasStencilComponent(VkFormat format) {
            return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
        }


        //////////////////////////////////////////////////////////////////
        // Texture Image (Depend Command buffer and so on buffer pool)
        //////////////////////////////////////////////////////////////////
        void createTextureImage() {
            int tex_width = 0, tex_height = 0, tex_channels = 0;
            stbi_uc* pixels = stbi_load(TEXTRUE_PATH.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
            VkDeviceSize image_size = tex_width * tex_height * 4;

            if (!pixels) {
                throw std::runtime_error("failed to load texture image!");
            }

            // Calculate the mipmap levels for texture
            mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;


            // Start copying image to buffer
            VkBuffer staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            createBuffer(
                image_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                staging_buffer,
                staging_buffer_memory
            );

            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(image_size));
            vkUnmapMemory(device, staging_buffer_memory);

            stbi_image_free(pixels);

            createImage(
                tex_width, tex_height, mip_levels, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                texture_image,
                texture_image_memroy
            );

            transitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
            copyBufferToImage(staging_buffer, texture_image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));

            /*
            transitionImageLayout(
                texture_image,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                mip_levels
            );
            */

            // Generate mipmaps
            generateMipmaps(texture_image, VK_FORMAT_R8G8B8A8_SRGB, tex_width, tex_height, mip_levels);

            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_buffer_memory, nullptr);
        }

        void createImage(
            uint32_t width, uint32_t height, uint32_t mip_levels, VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& image_memory
        ) {
            // Create image object
            VkImageCreateInfo image_info{};
            image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_info.imageType     = VK_IMAGE_TYPE_2D;
            image_info.extent.width  = width;
            image_info.extent.height = height;
            image_info.extent.depth  = 1;
            image_info.mipLevels     = mip_levels;
            image_info.arrayLayers   = 1;
            image_info.format        = format;
            image_info.tiling        = tiling;
            image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_info.usage         = usage;
            image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
            image_info.flags         = 0; // No additional flag (like sparse images)
            
            if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image!");
            }

            // Create image memory
            VkMemoryRequirements mem_requirements;
            vkGetImageMemoryRequirements(device, image, &mem_requirements);

            VkMemoryAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex = findMemoryType(
                                            mem_requirements.memoryTypeBits,
                                            properties
                                        );

            if (vkAllocateMemory(device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate image memroy!");
            }

            vkBindImageMemory(device, image, image_memory, 0);
        }

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) {
            VkCommandBuffer command_buffer = beginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = old_layout;
            barrier.newLayout = new_layout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = mip_levels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;

            VkPipelineStageFlags source_stage;
            VkPipelineStageFlags destination_stage;

            if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                command_buffer,
                source_stage, destination_stage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            endSingleTimeCommands(command_buffer);
        }

        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint16_t height) {
            VkCommandBuffer command_buffer = beginSingleTimeCommands();

            VkBufferImageCopy region{};
            region.bufferOffset      = 0;
            region.bufferRowLength   = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel       = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount     = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                width,
                height,
                1
            };

            // Copy data in buffer into image
            vkCmdCopyBufferToImage(
                command_buffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            endSingleTimeCommands(command_buffer);
        }

        void generateMipmaps(VkImage image, VkFormat image_format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels) {
            // Check if the image format supports linear blitting
            VkFormatProperties format_properties;
            vkGetPhysicalDeviceFormatProperties(physical_device, image_format, &format_properties);
            if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            VkCommandBuffer command_buffer = beginSingleTimeCommands();
            
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mip_width  = tex_width;
            int32_t mip_height = tex_height;

            for (uint32_t i = 1; i < mip_levels; i++) {

                // Transfer layout of previous mipmap level image to src layout
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(
                    command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                // Blit image from previous mipmap level to current mipmap level
                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mip_width, mip_height, 1 };
                blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel       = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount     = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
                blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel       = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount     = 1;

                vkCmdBlitImage(
                    command_buffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR
                );

                // Transfer layout of previous mipmap level image to shader read layout
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                if (mip_width > 1) mip_width /= 2;
                if (mip_height > 1) mip_height /=2;
            }

            barrier.subresourceRange.baseMipLevel = mip_levels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            endSingleTimeCommands(command_buffer);
        }


        //////////////////////////////////////////////////////////////////
        // Image view
        //////////////////////////////////////////////////////////////////
        void createTextureImageView() {
            texture_image_view = createImageView(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels);
        }

        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image    = image;
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format   = format;
            view_info.subresourceRange.aspectMask     = aspect_flags;
            view_info.subresourceRange.baseMipLevel   = 0;
            view_info.subresourceRange.levelCount     = mip_levels;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount     = 1;
            // Left out the explicit viewInfo.components initialization,
            // because VK_COMPONENT_SWIZZLE_IDENTITY is defined as 0 anyway.

            VkImageView image_view;
            if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }

            return image_view;
        }


        //////////////////////////////////////////////////////////////////
        // Texture Sampler
        //////////////////////////////////////////////////////////////////
        void createTextureSampler() {
            VkSamplerCreateInfo sampler_info{};
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            // Settings for mipmap sampling
            sampler_info.magFilter  = VK_FILTER_LINEAR;
            sampler_info.minFilter  = VK_FILTER_LINEAR;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_info.minLod     = 0.0f; // Optional
            sampler_info.maxLod     = static_cast<float>(mip_levels);
            sampler_info.mipLodBias = 0.0f; // Optional
            // Settings for sampling outside of the range
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            // Anisotropy (Hell Yeah~)
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(physical_device, &properties);
            sampler_info.anisotropyEnable = VK_TRUE;
            sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

            sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            // If this set to true, the coordinates will within [0, tex_width] and [0, tex_height]
            // instead of [0.0, 1.0)
            sampler_info.unnormalizedCoordinates = VK_FALSE;

            // If a comparison function is enabled, then texels will first be compared to a value,
            // and the result of that comparison is used in filtering operations.
            sampler_info.compareEnable = VK_FALSE;
            sampler_info.compareOp     = VK_COMPARE_OP_ALWAYS;

            if (vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS) {
                throw std::runtime_error("faled to create texture sampler!");
            }
        }



        //////////////////////////////////////////////////////////////////
        // Vertex buffer
        //////////////////////////////////////////////////////////////////
        void loadModel() {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, & shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<Vertex, uint32_t> unique_vertices{};

            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex{};

                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.tex_coord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                    };

                    vertex.color = { 1.0f, 1.0f, 1.0f };

                    if (unique_vertices.count(vertex) == 0) {
                        unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(unique_vertices[vertex]);
                }
            }
        }



        //////////////////////////////////////////////////////////////////
        // Vertex buffer
        //////////////////////////////////////////////////////////////////
        void createVertexBuffer() {
            VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

            // Host visible staging buffer to temporarlly store the vertices data
            VkBuffer       staging_buffer;
            VkDeviceMemory staging_buffer_memory;
            createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                // Note that the coherent is nessesary to let memory mapping always matches the content of the allocated memory.
                // Or you can choose to explicitly call the memory flash function.
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                staging_buffer,
                staging_buffer_memory
            );

            // Map and fill the staging buffer
            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
            memcpy(data, vertices.data(), buffer_size);
            vkUnmapMemory(device, staging_buffer_memory);


            // Create vertex buffer which is locate in the local memory to optimize the read operation for GPU.
            createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // This generally not able to us vkMapMemory
                vertex_buffer,
                vertex_buffer_memory
            );

            copyBuffer(staging_buffer, vertex_buffer, buffer_size);

            
            // Clean up staging buffer
            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_buffer_memory, nullptr);
        }


        //////////////////////////////////////////////////////////////////
        // Index buffer
        //////////////////////////////////////////////////////////////////
        void createIndexBuffer() {
            VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

            VkBuffer staging_buffer;
            VkDeviceMemory staging_memory;
            createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                staging_buffer,
                staging_memory
            );

            void* data;
            vkMapMemory(device, staging_memory, 0, buffer_size, 0, &data);
            memcpy(data, indices.data(), static_cast<size_t>(buffer_size));
            vkUnmapMemory(device, staging_memory);

            createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                index_buffer,
                index_memory
            );

            copyBuffer(staging_buffer, index_buffer, buffer_size);

            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_memory, nullptr);
        }


        //////////////////////////////////////////////////////////////////
        // Uniform buffers
        //////////////////////////////////////////////////////////////////
        void createUniformBuffers() {
            VkDeviceSize buffer_size = sizeof(UniformBufferObject);

            uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
            uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
            uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(
                    buffer_size,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    uniform_buffers[i],
                    uniform_buffers_memory[i]
                );

                vkMapMemory(device, uniform_buffers_memory[i], 0, buffer_size, 0, &uniform_buffers_mapped[i]);

            }
        }


        //////////////////////////////////////////////////////////////////
        // Descriptor Pool
        //////////////////////////////////////////////////////////////////
        void createDescriptorPool() {
            std::array<VkDescriptorPoolSize, 2> pool_sizes{};
            pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            VkDescriptorPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
            pool_info.pPoolSizes    = pool_sizes.data();
            pool_info.maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor pool!");
            }
        }


        //////////////////////////////////////////////////////////////////
        // Descriptor Sets
        //////////////////////////////////////////////////////////////////
        void createDescriptorSets() {
            std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool     = descriptor_pool;
            alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            alloc_info.pSetLayouts        = layouts.data(); // Expects an array of layout matching the number of sets.

            descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
            if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                VkDescriptorBufferInfo buffer_info{};
                buffer_info.buffer = uniform_buffers[i];
                buffer_info.offset = 0;
                buffer_info.range  = sizeof(UniformBufferObject);

                VkDescriptorImageInfo image_info{};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView   = texture_image_view;
                image_info.sampler     = texture_sampler;

                std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
                descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[0].dstSet           = descriptor_sets[i];
                descriptor_writes[0].dstBinding       = 0;
                descriptor_writes[0].dstArrayElement  = 0; // For array descriptors
                descriptor_writes[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_writes[0].descriptorCount  = 1; // How many array elements you want to update
                descriptor_writes[0].pBufferInfo      = &buffer_info; // |
                descriptor_writes[0].pImageInfo       = nullptr;      // --> Three choose one
                descriptor_writes[0].pTexelBufferView = nullptr;      // |

                descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_writes[1].dstSet          = descriptor_sets[i];
                descriptor_writes[1].dstBinding      = 1;
                descriptor_writes[1].dstArrayElement = 0;
                descriptor_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_writes[1].descriptorCount = 1;
                descriptor_writes[1].pImageInfo      = &image_info;

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
            }
        }


        //////////////////////////////////////////////////////////////////
        // Command Buffer
        //////////////////////////////////////////////////////////////////
        void createCommandBuffers() {
            command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool        = command_pool;
            alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

            if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }

        // Note that if the command buffer was already recorded once, then a call to
        // vkBeginCommandBuffer will implicitly reset it (since ). It's not possible to append
        // commands to a buffer at a later time.
        void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) {
            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags            = 0;        // Optional
            begin_info.pInheritanceInfo = nullptr;  // Optional, only relevant for secondary command buffers.

            if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            
            // Start drawing command by beginning the render pass
            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass  = render_pass;
            render_pass_info.framebuffer = swapchain_framebuffers[image_index];
            // Define the size of render area
            render_pass_info.renderArea.offset = {0, 0};
            render_pass_info.renderArea.extent = swapchain_extent;
            // Set up clear value
            std::array<VkClearValue, 2> clear_values{};
            clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clear_values[1].depthStencil = {1.0f, 0};
            render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
            render_pass_info.pClearValues    = clear_values.data();

            // Begin the render pass
            // The last parameter controls how the drawing
            // commands within the render pass will be provided.
            vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            // Bind the graphics pipeline
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

            VkBuffer vertex_buffers[] = {vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            // Bind the index buffer, only one index buffer is allowed
            vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = static_cast<float>(swapchain_extent.height);
            viewport.width    = static_cast<float>(swapchain_extent.width);
            viewport.height   = -static_cast<float>(swapchain_extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapchain_extent;
            vkCmdSetScissor(command_buffer, 0, 1, &scissor);

            // Bind the uniform buffer
            vkCmdBindDescriptorSets(
                command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_layout,
                0,
                1,
                &descriptor_sets[current_frame],
                0,
                nullptr
            );
            // Do draw call
            // Use draw index this time
            vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            // vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            
            // End the render pass
            vkCmdEndRenderPass(command_buffer);

            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        VkCommandBuffer beginSingleTimeCommands() {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandPool        = command_pool;
            alloc_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(command_buffer, &begin_info);

            return command_buffer;
        }

        void endSingleTimeCommands(VkCommandBuffer command_buffer) {
            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;

            vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphics_queue);

            vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
        }


        //////////////////////////////////////////////////////////////////
        // Sync Objects
        //////////////////////////////////////////////////////////////////
        void createSyncObjects() {
            image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
            render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inflight_fences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphore_info{};
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            // This fence should be initialized as signaled so the 
            // first wait for the available swapchain image won't 
            // be stuck.
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i])
                    != VK_SUCCESS ||
                    vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i])
                    != VK_SUCCESS ||
                    vkCreateFence(device, &fence_info, nullptr, &inflight_fences[i])
                    != VK_SUCCESS) {
                        throw std::runtime_error("failed to create semaphore!");
                    }                
            }

        }


        //////////////////////////////////////////////////////////////////
        // Draw Frame
        //////////////////////////////////////////////////////////////////
        void drawFrame() {
            // Wait until the previous frame has finished.
            vkWaitForFences(device, 1, &inflight_fences[current_frame], VK_TRUE, UINT64_MAX);

            uint32_t image_index;
            // The result of requirement about next image should be store to determine
            // whether to recreate the swapchain.
            VkResult result = vkAcquireNextImageKHR(
                device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index
            );

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreateSwapchain();
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("failed to acquire swpachain image!");
            }

            // Unsignaled the state of the fence
            // Note that this must be placed after requiring the swapchain image,
            // or you might encounter dead lock because of recreation of swapchain,
            // and the fence isn't signaled.
            vkResetFences(device, 1, &inflight_fences[current_frame]);

            // Update the uniform buffer
            updateUniformBuffer(current_frame);

            // Reset command buffer to ensure it is able to be recorded.
            vkResetCommandBuffer(command_buffers[current_frame], 0);

            // Record the command
            recordCommandBuffer(command_buffers[current_frame], image_index);

            // Submit the command buffer
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            // Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
            VkSemaphore wait_semaphores[]      = {image_available_semaphores[current_frame]};
            VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submit_info.waitSemaphoreCount     = 1;
            submit_info.pWaitSemaphores        = wait_semaphores;
            submit_info.pWaitDstStageMask      = wait_stages;

            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffers[current_frame];

            // The semaphores to be signaled once command buffers have finished execution
            VkSemaphore signal_semaphores[]  = {render_finished_semaphores[current_frame]};
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = signal_semaphores;

            if (vkQueueSubmit(graphics_queue, 1, &submit_info, inflight_fences[current_frame]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }


            // Submitting the result back to the swapchain to have it eventually show up on the screen.
            VkPresentInfoKHR present_info{};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores    = signal_semaphores;
            VkSwapchainKHR swapchains[]     = {swapchain};
            present_info.swapchainCount     = 1;
            present_info.pSwapchains        = swapchains;
            present_info.pImageIndices      = &image_index;
            // This allows you to specify an array of VkResult values to
            // check for every individual swap chain if presentation was successful.
            present_info.pResults           = nullptr; // Optional

            result = vkQueuePresentKHR(present_queue, &present_info);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
                recreateSwapchain();
                framebuffer_resized = false;
            }
            else if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to present swapchain image!");
            }

            // Update current frame index
            current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        }


        //////////////////////////////////////////////////////////////////
        // Uniform buffer updating
        //////////////////////////////////////////////////////////////////
        void updateUniformBuffer(uint32_t current_frame) {
            /*
            static auto start_time = std::chrono::high_resolution_clock::now();

            auto acurrent_time = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(
                            acurrent_time - start_time
                        ).count();
            */
            
            UniformBufferObject ubo{};
            ubo.model = glm::mat4(1.0f);
            ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            ubo.view = camera.getViewMat();
            ubo.proj = camera.getProjectionMat();
            //ubo.proj[1][1] *= -1;   // Flip the y axis since the y axis point down in Vulkan.
            memcpy(uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
        }


        //////////////////////////////////////////////////////////////////
        // Swapchain Recreation
        //////////////////////////////////////////////////////////////////
        void recreateSwapchain() {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device);

            cleanupSwapchain();

            createSwapchain();
            createImageViews();
            createDepthResorces();
            createFramebuffers();
            camera.setAspect(static_cast<float>(swapchain_extent.width) / swapchain_extent.height);
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