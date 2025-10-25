//
// Created by Puwiwad on 05.10.2025.
//
module;
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

#include "glm/gtc/constants.hpp"

#ifdef USE_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#else
#include <GLFW/glfw3.h>
#endif

export module ufox_windowing;

import ufox_lib;
import ufox_graphic_device;
import ufox_gui_renderer;


export namespace ufox::windowing {
    class UFoxWindow {
    public:
        UFoxWindow(const gpu::vulkan::GPUResources& gpu_,std::string const& windowName, vk::Extent2D const& extent_)
            : gpu(gpu_), windowResource{CreateWindow(windowName, extent_)} {
            surfaceResource.emplace(*gpu.instance, *windowResource);
#ifdef USE_SDL
            SDL_AddEventWatch(framebufferResizeCallbackSDL, this);
#else
            glfwSetWindowUserPointer(windowResource->getHandle(), this);
            glfwSetFramebufferSizeCallback(windowResource->getHandle(), framebufferResizeCallbackGLFW);
            glfwSetWindowIconifyCallback(windowResource->getHandle(), windowIconifyCallbackGLFW);
#endif
        }
        // Resources should be destroyed in reverse order of creation
        ~UFoxWindow() {
            pauseRendering = true;
            if (swapchainResource) {
                swapchainResource->Clear();
            }
#ifdef USE_SDL
            SDL_RemoveEventWatch(framebufferResizeCallbackSDL, this);
#endif

        }
        void initResources() {
            swapchainResource.emplace(gpu::vulkan::MakeSwapchainResource(gpu, *surfaceResource, vk::ImageUsageFlagBits::eColorAttachment));
            frameResource.emplace(gpu::vulkan::MakeFrameResource(gpu, vk::FenceCreateFlagBits::eSignaled));

            std::vector<char> shaderCode = ReadFile("res/shaders/test.slang.spv");
            guiRenderer.emplace(gpu, shaderCode, getSwapChainImageCount(), getPipelineRenderingCreateInfo());
        }

        void Draw() {
#ifdef USE_SDL
            SDL_Event event;
            bool running = true;
            while (running) {
                while (SDL_PollEvent(&event)) {
                    switch (event.type) {
                        case SDL_EVENT_QUIT: {
                            pauseRendering = true;
                            running = false;
                            break;
                        }

                        case SDL_EVENT_WINDOW_MINIMIZED: {
                            pauseRendering = true;
                            break;
                        }
                        case SDL_EVENT_WINDOW_RESTORED: {
                            pauseRendering = false;
                            break;
                        }
                        default: {}
                    }

                }
                drawFrame();
            }
#else
            while (!glfwWindowShouldClose(windowResource->getHandle())) {
                drawFrame();
                glfwPollEvents();
            }
#endif
        }

        [[nodiscard]] gpu::vulkan::QueueFamilyIndices getQueueFamilyIndices() const {
            return gpu::vulkan::FindGraphicsAndPresentQueueFamilyIndex(gpu, *surfaceResource);
        }

        [[nodiscard]] vk::PipelineRenderingCreateInfo getPipelineRenderingCreateInfo() const {
            vk::PipelineRenderingCreateInfo renderingInfo{};
            renderingInfo
            .setColorAttachmentCount(1)
            .setPColorAttachmentFormats(&swapchainResource->colorFormat);

            return renderingInfo;
        }

        static std::vector<char> ReadFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            std::vector<char> buffer(file.tellg());

            file.seekg(0, std::ios::beg);
            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

            file.close();

            return buffer;
        }

        // Getter for windowResource
        [[nodiscard]] const WindowResource& getWindowResource() const {
            return *windowResource;
        }

        [[nodiscard]] vk::Extent2D getSwapchainExtent() const {
            return swapchainResource->extent;
        }

        [[nodiscard]] uint32_t getSwapChainImageCount() const {
            return swapchainResource->getImageCount();
        }


        [[nodiscard]] uint32_t getCurrentImageIndex() const {
            return swapchainResource->currentImageIndex;
        }



    private:
        void recreateSwapchain() {
            // Handle window minimization
            int width = 0, height = 0;
#ifdef USE_SDL
            SDL_GetWindowSize(windowResource->getHandle(), &width, &height);
#else
            glfwGetFramebufferSize(windowResource->getHandle(), &width, &height);

#endif
            pauseRendering = width == 0 || height == 0;

            if (pauseRendering) return;

            gpu.device->waitIdle();
            surfaceResource->extent = vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            swapchainResource->Clear();
            gpu::vulkan::ReMakeSwapchainResource(*swapchainResource, gpu, *surfaceResource, vk::ImageUsageFlagBits::eColorAttachment);
        }

        void recreateSwapchain(int width, int height) {

            pauseRendering = width == 0 || height == 0;

            if (pauseRendering) return;

            gpu.device->waitIdle();
            surfaceResource->extent = vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            swapchainResource->Clear();
            gpu::vulkan::ReMakeSwapchainResource(*swapchainResource, gpu, *surfaceResource, vk::ImageUsageFlagBits::eColorAttachment);
        }

        void drawFrame() {
            while ( vk::Result::eTimeout == gpu.device->waitForFences( *frameResource->getCurrentDrawFence(), vk::True, UINT64_MAX ) )
                ;

            if (pauseRendering) return;

            auto [result, imageIndex] = swapchainResource->swapChain->acquireNextImage(UINT64_MAX, frameResource->getCurrentPresentCompleteSemaphore(), nullptr);
            swapchainResource->currentImageIndex = imageIndex;
            gpu.device->resetFences(*frameResource->getCurrentDrawFence());
            if (result == vk::Result::eErrorOutOfDateKHR) {
                ufox::debug::log(debug::LogLevel::eInfo, "Recreating swapchain due to acquireNextImage failure");
                recreateSwapchain();
                return;
            }
            if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
                throw std::runtime_error("Failed to acquire swapchain image");
            }
            const vk::raii::CommandBuffer& cmb = frameResource->getCurrentCommandBuffer();
            cmb.reset();
            recordCommandBuffer(cmb);


            vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
            vk::SubmitInfo submitInfo{};
            submitInfo.setWaitSemaphoreCount(1)
                      .setPWaitSemaphores(&*frameResource->getCurrentPresentCompleteSemaphore())
                      .setPWaitDstStageMask(&waitDestinationStageMask)
                      .setCommandBufferCount(1)
                      .setPCommandBuffers(&*cmb)
                      .setSignalSemaphoreCount(1)
                      .setPSignalSemaphores(&*swapchainResource->getCurrentRenderFinishedSemaphore());
            gpu.graphicsQueue->submit(submitInfo, frameResource->getCurrentDrawFence());
            vk::PresentInfoKHR presentInfo{};
            presentInfo.setWaitSemaphoreCount(1)
                       .setPWaitSemaphores(&*swapchainResource->getCurrentRenderFinishedSemaphore())
                       .setSwapchainCount(1)
                       .setPSwapchains(&**swapchainResource->swapChain)
                       .setPImageIndices(&swapchainResource->currentImageIndex);

            result = gpu.presentQueue->presentKHR(presentInfo);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
                framebufferResized = false;
                recreateSwapchain();
                return;
            }
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to present swapchain image");
            }
            frameResource->ContinueNextFrame();
        }


        void recordCommandBuffer(const vk::raii::CommandBuffer& cmb) const {
            std::array<vk::ClearValue, 2> clearValues{};
            clearValues[0].color        = vk::ClearColorValue{0.2f, 0.2f, 0.2f,1.0f};
            clearValues[1].depthStencil = vk::ClearDepthStencilValue{0.0f, 0};
            cmb.begin({});
            vk::ImageSubresourceRange range{};
            range.aspectMask     = vk::ImageAspectFlagBits::eColor;
            range.baseMipLevel   = 0;
            range.levelCount     = VK_REMAINING_MIP_LEVELS;
            range.baseArrayLayer = 0;
            range.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            gpu::vulkan::TransitionImageLayout(
                cmb, swapchainResource->getCurrentImage(),
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput ,
                {},
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,range
                );
            vk::RenderingAttachmentInfo colorAttachment{};
            colorAttachment.setImageView(swapchainResource->getCurrentImageView())
                           .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                           .setLoadOp(vk::AttachmentLoadOp::eClear)
                           .setStoreOp(vk::AttachmentStoreOp::eStore)
                           .setClearValue(clearValues[0]);
            vk::Rect2D renderArea{};
            renderArea.offset = vk::Offset2D{0, 0};
            renderArea.extent = swapchainResource->extent;
            vk::RenderingInfo renderingInfo{};
            renderingInfo.setRenderArea(renderArea)
                         .setLayerCount(1)
                         .setColorAttachmentCount(1)
                         .setPColorAttachments(&colorAttachment);
            cmb.beginRendering(renderingInfo);

            vk::Extent2D extent = getSwapchainExtent();
            uint32_t imageIndex = getCurrentImageIndex();
            guiRenderer->UpdateRect(extent, imageIndex);
            guiRenderer->DrawRect(cmb, extent, imageIndex);

            cmb.endRendering();
            gpu::vulkan::TransitionImageLayout(cmb, swapchainResource->getCurrentImage(),
                vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, range);
            cmb.end();
        }

#ifdef USE_SDL
        static bool framebufferResizeCallbackSDL(void* userData, SDL_Event* event) {
            if (event->type == SDL_EVENT_WINDOW_RESIZED) {
                SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
                auto* app = static_cast<UFoxWindow*>(userData);
                int width, height;
                SDL_GetWindowSize(win, &width, &height);
                app->framebufferResized = true;
                app->recreateSwapchain(width, height);
                app->drawFrame();
            }
            return true;
        }
#else
        static void framebufferResizeCallbackGLFW(GLFWwindow* window, int width, int height) {
            auto app = static_cast<UFoxWindow*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
            app->recreateSwapchain(width, height);
            app->drawFrame();
        }

        static void windowIconifyCallbackGLFW(GLFWwindow* window, int iconified) {
            auto app = static_cast<UFoxWindow*>(glfwGetWindowUserPointer(window));
            app->pauseRendering = iconified != GLFW_FALSE;
        }
#endif

        const gpu::vulkan::GPUResources&                gpu;
        std::optional<WindowResource>                   windowResource;
        std::optional<gpu::vulkan::SurfaceResource>     surfaceResource{};
        std::optional<gpu::vulkan::SwapchainResource>   swapchainResource{};
        std::optional<gpu::vulkan::FrameResource>       frameResource{};

        std::optional<gui::Renderer>                    guiRenderer{};

        bool framebufferResized = false;
        bool pauseRendering = false;
    };
}
