module;

#include <vector>
#include <optional>
#include <iostream>
#include <string>
#include <format>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include "SDL3/SDL_video.h"
export module ufox_lib;

export namespace ufox {
    namespace debug {
        enum class LogLevel {
            INFO,
            WARNING,
            ERROR
        };

        // Moved source_location parameter to the beginning
        template<typename... Args>
        void log(LogLevel level,
                const std::string& format_str,
                Args&&... args) {
            std::string level_str;
            switch (level) {
                case LogLevel::INFO:    level_str = "INFO"; break;
                case LogLevel::WARNING: level_str = "WARNING"; break;
                case LogLevel::ERROR:   level_str = "ERROR"; break;
            }

            // Format the message using std::format
            std::string message = std::vformat(format_str, std::make_format_args(args...));

            std::cout << std::format("[{}] {}\n",
                level_str,
                message);
        }
    }

    struct Panel {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        std::vector<Panel*> rows;
        std::vector<Panel*> columns;

        void addRowChild(Panel* child) {
            rows.push_back(child);
        }

        void addColumnChild(Panel* child) {
            columns.push_back(child);
        }

        void print() const {
            debug::log(debug::LogLevel::INFO, "Panel: x={}, y={}, width={}, height={}", x, y, width, height);
        }
    };

    namespace windowing::sdl {

        struct SDLException : std::runtime_error {explicit SDLException(const std::string& msg) : std::runtime_error(msg + ": " + SDL_GetError()) {}};

        enum class WindowFlag : Uint32 {
            None = 0,
            Borderless = SDL_WINDOW_BORDERLESS,
            Resizable = SDL_WINDOW_RESIZABLE,
            Maximized = SDL_WINDOW_MAXIMIZED,
            Minimized = SDL_WINDOW_MINIMIZED,
            Vulkan = SDL_WINDOW_VULKAN,
            OpenGL = SDL_WINDOW_OPENGL,
            Fullscreen = SDL_WINDOW_FULLSCREEN,

        };

        inline WindowFlag operator|(WindowFlag a, WindowFlag b) {
            return static_cast<WindowFlag>(static_cast<Uint32>(a) | static_cast<Uint32>(b));
        }
        inline WindowFlag operator|=(WindowFlag& a, WindowFlag b) {
            a = a | b;
            return a;
        }

        struct UFoxWindowCreateInfo {
            std::string title;
            WindowFlag flags;
        };

    }

    namespace windowing {
        struct UFoxWindowData
        {
            UFoxWindowData( GLFWwindow * wnd, std::string const & name, vk::Extent2D const & extent ) : handle{ wnd, glfwDestroyWindow }, name{ name }, extent{ extent } {}
            UFoxWindowData( UFoxWindowData && other ) noexcept : handle{nullptr, glfwDestroyWindow}, name{}, extent{}
            {
                std::swap( handle, other.handle );
                std::swap( name, other.name );
                std::swap( extent, other.extent );
            }
            UFoxWindowData( const UFoxWindowData & ) = delete;
            ~UFoxWindowData() noexcept = default;

            std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> handle;
            std::string  name;
            vk::Extent2D extent;
        };

        UFoxWindowData CreateWindow( std::string const & windowName, vk::Extent2D const & extent )
        {
            struct glfwContext
            {
                glfwContext()
                {
                    glfwInit();
                    glfwSetErrorCallback( []( int error, const char * msg ) { std::cerr << "glfw: " << "(" << error << ") " << msg << std::endl; } );
                }

                ~glfwContext()
                {
                    glfwTerminate();
                }
            };

            static auto glfwCtx = glfwContext();
            (void)glfwCtx;

            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
            GLFWwindow * window = glfwCreateWindow( extent.width, extent.height, windowName.c_str(), nullptr, nullptr );
            return UFoxWindowData( window, windowName, extent );
        }
    }

namespace gpu::vulkan {
        struct PhysicalDeviceRequirements {
            static constexpr uint32_t MINIMUM_API_VERSION = vk::ApiVersion14;
            static constexpr uint32_t DISCRETE_GPU_SCORE_BONUS = 1000;
        };

        struct SurfaceData
        {
            SurfaceData( vk::raii::Instance const & instance, std::string const & windowName, vk::Extent2D const & extent_ )
              : extent( extent_ ), window( windowing::CreateWindow( windowName, extent ) )
            {
                VkSurfaceKHR _surface;
                VkResult     err = glfwCreateWindowSurface( *instance, window.handle.get(), nullptr, &_surface );
                if ( err != VK_SUCCESS )
                    throw std::runtime_error( "Failed to create window!" );
                surface.emplace(instance, _surface);
            }

            vk::Extent2D                                extent;
            windowing::UFoxWindowData                   window;
            std::optional<vk::raii::SurfaceKHR>         surface{};
        };

        struct SwapchainData {
            vk::Format                                  colorFormat;
            std::optional<vk::raii::SwapchainKHR>       swapChain{};
            std::vector<vk::Image>                      images;
            std::vector<vk::raii::ImageView>            imageViews;
        };

        struct QueueFamilyIndices {
            uint32_t                                    graphicsFamily{0};
            uint32_t                                    presentFamily{0};
        };
    }

    struct GPUResources {
        std::optional<vk::raii::Context>                context{};
        std::optional<vk::raii::Instance>               instance{};

        std::optional<vk::raii::PhysicalDevice>         physicalDevice{};
        std::optional<vk::raii::Device>                 device{};
        std::optional<gpu::vulkan::QueueFamilyIndices>  queueFamilyIndices{};
        std::optional<vk::raii::CommandPool>            commandPool{};
        std::optional<vk::raii::Queue>                  graphicsQueue{};
        std::optional<vk::raii::Queue>                  presentQueue{};

    };
}