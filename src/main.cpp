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

// Window's width and height
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
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
        }

        void mainLoop() {
            // The application will run until either and error occrus or the
            // window is closed.
            while (!glfwWindowShouldClose(window)) {
                glfwPollEvents();
            }
            
        }

        void cleanup() {
            if (enable_validation_layers) {
                //DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
            }

            // Destroy Vulkan instance
            vkDestroyInstance(instance, nullptr);

            // Clean up GLFW resources
            glfwDestroyWindow(window);

            // Terminate GLFW
            glfwTerminate();
        }


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
            std::vector<std::string> missing_extensions;
            for (uint32_t i = 0; i < static_cast<uint32_t>(required_extensions.size()); i++) {
                auto it = supported_extensions.begin();
                while(it != supported_extensions.end()) {
                    if (std::strcmp(required_extensions[i], it->extensionName) == 0) {
                        break;
                    }
                    ++it;
                }
                if (it == supported_extensions.end()) {
                    missing_extensions.emplace_back(required_extensions[i]);
                }
            }
            if (missing_extensions.size() > 0) {
                std::cout << "missing extension(s) needed by GLFW:\n";
                for (const auto& extension : missing_extensions) {
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