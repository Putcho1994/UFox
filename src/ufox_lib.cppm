
module;

#include <utility>
#include <vector>
#include <optional>
#include <iostream>
#include <string>
#include <format>
#include <functional>
#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#ifdef USE_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#else
#include <GLFW/glfw3.h>
#endif
export module ufox_lib;

export namespace ufox {
    namespace utilities {
        size_t GenerateUniqueID(const std::vector<glm::vec4>& fields) {
            std::hash<float> floatHasher;
            size_t hash = 0;
            auto combineHash = [&hash](size_t value) {
                hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            auto combineFloat = [&combineHash, &floatHasher](float value) {
                float normalized = std::round(value * 1000000.0f) / 1000000.0f;
                combineHash(floatHasher(normalized));
            };
            for (const auto& vec : fields) {
                combineFloat(vec.r);
                combineFloat(vec.g);
                combineFloat(vec.b);
                combineFloat(vec.a);
            }
            return hash;
        }

        // Hash a vector of floats
        size_t GenerateUniqueID(const std::vector<float>& fields) {
            std::hash<float> floatHasher;
            size_t hash = 0;
            auto combineHash = [&hash](size_t value) {
                hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            auto combineFloat = [&combineHash, &floatHasher](float value) {
                float normalized = std::round(value * 1000000.0f) / 1000000.0f;
                combineHash(floatHasher(normalized));
            };
            for (float value : fields) {
                combineFloat(value);
            }
            return hash;
        }
    }
    namespace debug {
        enum class LogLevel {
            eInfo,
            eWarning,
            eError
        };

        template<typename... Args>
        void log(LogLevel level, const std::string& format_str, Args&&... args) {
            std::string level_str;
            switch (level) {
                case LogLevel::eInfo:    level_str = "INFO"; break;
                case LogLevel::eWarning: level_str = "WARNING"; break;
                case LogLevel::eError:   level_str = "ERROR"; break;
            }
            std::string message = std::vformat(format_str, std::make_format_args(args...));
            std::cout << std::format("[{}] {}\n", level_str, message);
        }
    }

    namespace windowing {

        struct WindowResource {
#ifdef USE_SDL
            using WindowHandle = SDL_Window;
            using DestroyFunc = decltype(&SDL_DestroyWindow);
#else
            using WindowHandle = GLFWwindow;
            using DestroyFunc = decltype(&glfwDestroyWindow);
#endif

            explicit WindowResource(WindowHandle* wnd)
                : handle{wnd, DestroyFunc{}} {}

            WindowResource(WindowResource&& other) noexcept
                : handle{nullptr, DestroyFunc{}} {
                std::swap(handle, other.handle);
            }
            WindowResource(const WindowResource&) = delete;
            ~WindowResource() noexcept = default;

            std::unique_ptr<WindowHandle, DestroyFunc> handle;


            // Getters
            [[nodiscard]] WindowHandle* getHandle() const { return handle.get(); }
        };


WindowResource CreateWindow( std::string const & windowName, vk::Extent2D const & extent ) {
#ifdef USE_SDL

        struct sdlContext {
            sdlContext() {
                if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
                    debug::log(debug::LogLevel::eError, "Failed to initialize SDL: {}", SDL_GetError());
                    throw std::runtime_error("Failed to initialize SDL");
                }

            }
            ~sdlContext() { SDL_Quit(); }
        };
        static auto sdlCtx = sdlContext();
        (void)sdlCtx;

        // SDL_DisplayID primary = SDL_GetPrimaryDisplay();
        // SDL_Rect usableBounds{};
        // SDL_GetDisplayUsableBounds(primary, &usableBounds);

        Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
        SDL_Window* window = SDL_CreateWindow(windowName.c_str(), static_cast<int>(extent.width), static_cast<int>(extent.height), flags);
        if (!window) {
            throw std::runtime_error("Failed to create SDL window");
        }

        //SDL_SetWindowPosition(window, 2, 32);
        debug::log(debug::LogLevel::eInfo, "Created SDL window: {} ({}x{})", windowName, static_cast<int>(extent.width), static_cast<int>(extent.height));
        return WindowResource{window};
#else

        struct glfwContext {
            glfwContext() {
                glfwInit();
                glfwSetErrorCallback([](int error, const char* msg) {
                    debug::log(debug::LogLevel::eError, "GLFW error ({}): {}", error, msg);
                });
            }
            ~glfwContext() { glfwTerminate(); }
        };
        static auto glfwCtx = glfwContext();
        (void)glfwCtx;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height), windowName.c_str(), nullptr, nullptr);
        if (!window) {
            debug::log(debug::LogLevel::eError, "Failed to create GLFW window: {}", windowName);
            throw std::runtime_error("Failed to create GLFW window");
        }
        debug::log(debug::LogLevel::eInfo, "Created GLFW window: {} ({}x{})", windowName, static_cast<int>(extent.width), static_cast<int>(extent.height));
        return WindowResource{window};
#endif
    }


    }


namespace gpu::vulkan {

        uint32_t FindMemoryType(const vk::PhysicalDeviceMemoryProperties &memoryProperties, uint32_t typeBits,
                            vk::MemoryPropertyFlags requirementsMask){
            auto typeIndex = static_cast<uint32_t>(~0);
            for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++ )
            {
                if ( typeBits & 1 && ( memoryProperties.memoryTypes[i].propertyFlags & requirementsMask ) == requirementsMask )
                {
                    typeIndex = i;
                    break;
                }
                typeBits >>= 1;
            }
            assert( typeIndex != static_cast<uint32_t>(~0));
            return typeIndex;
        }

        vk::raii::DeviceMemory AllocateDeviceMemory( vk::raii::Device const &                   device,
                                                           vk::PhysicalDeviceMemoryProperties const & memoryProperties,
                                                           vk::MemoryRequirements const &             memoryRequirements,
                                                           vk::MemoryPropertyFlags                    memoryPropertyFlags )
        {
            uint32_t               memoryTypeIndex = FindMemoryType( memoryProperties, memoryRequirements.memoryTypeBits, memoryPropertyFlags );
            vk::MemoryAllocateInfo memoryAllocateInfo( memoryRequirements.size, memoryTypeIndex );
            return {device, memoryAllocateInfo};
        }


        struct PhysicalDeviceRequirements {
            static constexpr uint32_t MINIMUM_API_VERSION = vk::ApiVersion14;
            static constexpr uint32_t DISCRETE_GPU_SCORE_BONUS = 1000;
        };

        struct GraphicDeviceCreateInfo {
            vk::ApplicationInfo appInfo{};
            std::vector<std::string> instanceExtensions{};
            std::vector<std::string> deviceExtensions{};
            bool enableDynamicRendering{true};
            bool enableExtendedDynamicState{true};
            bool enableSynchronization2{true};
            vk::CommandPoolCreateFlags commandPoolCreateFlags{};

            // Getters/Setters
            [[nodiscard]] vk::ApplicationInfo getAppInfo() const { return appInfo; }
            void setAppInfo(const vk::ApplicationInfo& info) { appInfo = info; }
            [[nodiscard]] std::vector<std::string> getInstanceExtensions() const { return instanceExtensions; }
            void setInstanceExtensions(const std::vector<std::string>& exts) { instanceExtensions = exts; }
            [[nodiscard]] std::vector<std::string> getDeviceExtensions() const { return deviceExtensions; }
            void setDeviceExtensions(const std::vector<std::string>& exts) { deviceExtensions = exts; }
            [[nodiscard]] bool getEnableDynamicRendering() const { return enableDynamicRendering; }
            void setEnableDynamicRendering(bool enable) { enableDynamicRendering = enable; }
            [[nodiscard]] bool getEnableExtendedDynamicState() const { return enableExtendedDynamicState; }
            void setEnableExtendedDynamicState(bool enable) { enableExtendedDynamicState = enable; }
            [[nodiscard]] bool getEnableSynchronization2() const { return enableSynchronization2; }
            void setEnableSynchronization2(bool enable) { enableSynchronization2 = enable; }
            [[nodiscard]] vk::CommandPoolCreateFlags getCommandPoolCreateFlags() const { return commandPoolCreateFlags; }
            void setCommandPoolCreateFlags(vk::CommandPoolCreateFlags flags) { commandPoolCreateFlags = flags; }
        };

        struct SurfaceResource
        {
            SurfaceResource( vk::raii::Instance const & instance, const windowing::WindowResource& window )
            {
                VkSurfaceKHR _surface;
#ifdef USE_SDL
                if ( !SDL_Vulkan_CreateSurface( window.handle.get(), *instance, nullptr, &_surface )) {
                    throw std::runtime_error( "Failed to create window surface!" );
                }
#else
                if (const VkResult err = glfwCreateWindowSurface( *instance, window.getHandle(), nullptr, &_surface ); err != VK_SUCCESS )
                    throw std::runtime_error( "Failed to create window!" );

                int width = 0, height = 0;
                glfwGetFramebufferSize(window.getHandle(), &width, &height);
                extent = vk::Extent2D{static_cast<uint32_t> (width), static_cast<uint32_t> (height)};
#endif


                surface.emplace(instance, _surface);
            }

            vk::Extent2D                                extent;
            std::optional<vk::raii::SurfaceKHR>         surface{};
        };

        struct SwapchainResource {
            vk::Format                                  colorFormat;
            std::optional<vk::raii::SwapchainKHR>       swapChain{};
            std::vector<vk::Image>                      images;
            std::vector<vk::raii::ImageView>            imageViews;
            vk::Extent2D                                extent{0,0};
            std::vector<vk::raii::Semaphore>            renderFinishedSemaphores{};
            uint32_t                                    currentImageIndex{0};

            [[nodiscard]] const vk::Image& getCurrentImage() const {
                return images[currentImageIndex];
            }

            [[nodiscard]] vk::ImageView getCurrentImageView() const {
                return *imageViews[currentImageIndex]; // Dereference to get the underlying VkImageView
            }

            const vk::raii::Semaphore& getCurrentRenderFinishedSemaphore()  {
                return renderFinishedSemaphores[currentImageIndex];
            }

            [[nodiscard]] uint32_t getImageCount() const {
                return static_cast<uint32_t>(images.size());
            }

            void Clear() {
                imageViews.clear();
                renderFinishedSemaphores.clear();
                swapChain.reset();
            }
        };

        struct FrameResource {
            static constexpr int                        MAX_FRAMES_IN_FLIGHT = 2;
            std::vector<vk::raii::CommandBuffer>        commandBuffers{};
            std::vector<vk::raii::Semaphore>            presentCompleteSemaphores{};
            std::vector<vk::raii::Fence>                drawFences{};
            uint32_t                                    currentFrameIndex{0};

            const vk::raii::Fence& getCurrentDrawFence()  {
                return drawFences[currentFrameIndex];
            }

            const vk::raii::Semaphore& getCurrentPresentCompleteSemaphore() {
                return presentCompleteSemaphores[currentFrameIndex];
            }

            const vk::raii::CommandBuffer& getCurrentCommandBuffer()  {
                return commandBuffers[currentFrameIndex];
            }

            void ContinueNextFrame() {
                currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        };

        struct QueueFamilyIndices {
            uint32_t                                    graphicsFamily{0};
            uint32_t                                    presentFamily{0};
        };

        struct GPUResources {
            std::optional<vk::raii::Context>            context{};
            std::optional<vk::raii::Instance>           instance{};

            std::optional<vk::raii::PhysicalDevice>     physicalDevice{};
            std::optional<vk::raii::Device>             device{};
            std::optional<QueueFamilyIndices>           queueFamilyIndices{};

            std::optional<vk::raii::CommandPool>        commandPool{};

            std::optional<vk::raii::Queue>              graphicsQueue{};
            std::optional<vk::raii::Queue>              presentQueue{};
        };

        struct Buffer {
            Buffer( vk::raii::PhysicalDevice const & physicalDevice,
                                vk::raii::Device const &         device,
                                vk::DeviceSize                   size,
                                vk::BufferUsageFlags             usage,
                                vk::MemoryPropertyFlags          propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent ) {
                vk::BufferCreateInfo bufferInfo{};
                bufferInfo
                .setSize( size )
                .setUsage( usage );
                data.emplace(device, bufferInfo);

                memory = AllocateDeviceMemory( device, physicalDevice.getMemoryProperties(), data->getMemoryRequirements(), propertyFlags );
                data->bindMemory( *memory,0 );
            }

            Buffer(const GPUResources& gpu, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent ) :
                Buffer(gpu.physicalDevice.value(), gpu.device.value(), size, usage, propertyFlags){}


            Buffer() = default;


            std::optional<vk::raii::DeviceMemory>       memory{nullptr};
            std::optional<vk::raii::Buffer>             data{nullptr};
        };

        struct RemapableBuffer {
            std::optional<Buffer>                       buffer{};
            std::optional<void*>                        mapped{nullptr};
        };
    }


    namespace gui {
        struct Vertex {
            glm::vec2 position;
            glm::vec3 color;
            glm::vec2 uv;



            static constexpr  vk::VertexInputBindingDescription getBindingDescription(uint32_t binding) {
                vk::VertexInputBindingDescription bindingDescription{};
                bindingDescription.setBinding(binding).setStride(sizeof(Vertex)).setInputRate(vk::VertexInputRate::eVertex);
                return bindingDescription;
            }

            static constexpr  std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions(uint32_t binding) {
                std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
                attributeDescriptions[0].setBinding(binding).setLocation(0).setFormat(vk::Format::eR32G32Sfloat).setOffset(0);
                attributeDescriptions[1].setBinding(binding).setLocation(1).setFormat(vk::Format::eR32G32B32A32Sfloat).setOffset(offsetof(Vertex, color));
                attributeDescriptions[2].setBinding(binding).setLocation(2).setFormat(vk::Format::eR32G32Sfloat).setOffset(offsetof(Vertex, uv));
                return attributeDescriptions;
            }
        };

        struct UniformBufferObject {
          alignas(16)  glm::mat4 model;
          alignas(16)  glm::mat4 view;
          alignas(16)  glm::mat4 proj;
        };

        constexpr Vertex Geometries[] {
            {{0.0f, 0.0f,}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // Top-left
             {{1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // Top-right
             {{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // Bottom-right
             {{0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // Bottom-left
    };

        constexpr uint16_t Indices[] {
            0, 1, 2, 2, 3, 0,
    };



        // GUIStyle: Visual properties for GUI elements
        struct Style {
            glm::vec4 backgroundColor = {0.5f, 0.5f, 0.5f, 1.0f};
            glm::vec4 borderTopColor = {0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 borderRightColor = {0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 borderBottomColor = {0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 borderLeftColor = {0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 borderThickness = {1.0f, 1.0f, 1.0f, 1.0f}; // Top, right, bottom, left
            glm::vec4 cornerRadius = {10.0f, 10.0f, 10.0f, 10.0f}; // Top-left, top-right, bottom-left, bottom-right

            [[nodiscard]] size_t generateUniqueID() const {
                return utilities::GenerateUniqueID({
                    backgroundColor,
                    borderTopColor,
                    borderRightColor,
                    borderBottomColor,
                    borderLeftColor,
                    borderThickness,
                    cornerRadius
                });
            }
        };

        struct StyleBuffer {
            size_t                  hashUID;
            Style                   content;
            gpu::vulkan::Buffer     buffer{};
        };

        constexpr vk::DeviceSize GUI_VERTEX_BUFFER_SIZE = sizeof(Vertex);
        //constexpr vk::DeviceSize GUI_TRANSFORM_MATRIX_SIZE = sizeof(GUITransformMatrix);
        constexpr vk::DeviceSize GUI_RECT_MESH_BUFFER_SIZE = sizeof(Geometries);
        constexpr vk::DeviceSize GUI_INDEX_BUFFER_SIZE = sizeof(Indices);
        constexpr vk::DeviceSize GUI_STYLE_BUFFER_SIZE = sizeof(Style);


    }
}