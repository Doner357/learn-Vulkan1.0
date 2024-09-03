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
        GLFWwindow* window;
        VkInstance instance;

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
            // Create vulkan instance which connects application and the Vulkan library.
            createInstance();
        }

        void mainLoop() {
            // The application will run until either and error occrus or the
            // window is closed.
            while (!glfwWindowShouldClose(window)) {
                glfwPollEvents();
            }
            
        }

        void cleanup() {
            // Destroy Vulkan instance
            vkDestroyInstance(instance, nullptr);

            // Clean up GLFW resources
            glfwDestroyWindow(window);

            // Terminate GLFW
            glfwTerminate();
        }


        void createInstance() {
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

            // GLFW has a handy built-in function that returns the extensions it needs
            // to do that which we can pass to the struct.
            uint32_t glfw_extension_count = 0;
            const char** glfw_extensions;

            glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
            std::cout << "GLFW required extensions:\n";
            for (uint32_t i = 0; i < glfw_extension_count; i++) {
                std::cout << '\t' << glfw_extensions[i] << '\n';
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
            for (uint32_t i = 0; i < glfw_extension_count; i++) {
                auto it = supported_extensions.begin();
                while(it != supported_extensions.end()) {
                    if (std::strcmp(glfw_extensions[i], (*it).extensionName) == 0) {
                        break;
                    }
                    ++it;
                }
                if (it == supported_extensions.end()) {
                    missing_extensions.emplace_back(glfw_extensions[i]);
                }
            }
            if (missing_extensions.size() > 0) {
                std::cout << "missing extension(s) needed by GLFW:\n";
                for (const auto& extension : missing_extensions) {
                    std::cout << '\t' << extension << '\n';
                }
                throw std::runtime_error("exist missing extension(s)!");
            }

            create_info.enabledExtensionCount   = glfw_extension_count; // Number of extensions
            create_info.ppEnabledExtensionNames = glfw_extensions;      // What kind of extensions
            create_info.enabledLayerCount       = 0;

            // Create and check the instance
            if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }
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