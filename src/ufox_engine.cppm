//
// Created by Puwiwad on 05.10.2025.
//
module;
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
export module ufox_engine;
import ufox_lib;
import ufox_windowing;
import ufox_graphic;

export namespace ufox {
    gpu::vulkan::GraphicDeviceCreateInfo CreateGraphicDeviceInfo() {


        gpu::vulkan::GraphicDeviceCreateInfo createInfo{};
        createInfo.appInfo.pApplicationName = "UFox";
        createInfo.appInfo.applicationVersion = vk::makeVersion(1, 0, 0);
        createInfo.appInfo.pEngineName = "UFox Engine";
        createInfo.appInfo.engineVersion = vk::makeVersion(1, 0, 0);
        createInfo.appInfo.apiVersion = vk::ApiVersion14;
        createInfo.instanceExtensions = gpu::vulkan::GetInstanceExtensions();
        createInfo.deviceExtensions = gpu::vulkan::GetDeviceExtensions();
        createInfo.enableDynamicRendering = true;
        createInfo.enableExtendedDynamicState = true;
        createInfo.commandPoolCreateFlags = vk::CommandPoolCreateFlags{
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
            vk::CommandPoolCreateFlagBits::eTransient
        };






        return createInfo;
    }

    class Engine {
    public:
        Engine() = default;
        ~Engine() = default;

        void Init() {

            InitializeGPU();
        }

        void Run() {
            bool isRunning = true;
            SDL_Event event;

            // while (isRunning) {
            //     while (SDL_PollEvent(&event)) {
            //         isRunning = HandleEvent(event);
            //     }
            // }
        }

    private:
        void InitializeGPU() {
            gpuCreateInfo = CreateGraphicDeviceInfo();
            resources.context.emplace();
            resources.instance.emplace(gpu::vulkan::MakeInstance(resources,gpuCreateInfo));
            resources.physicalDevice.emplace(gpu::vulkan::PickBestPhysicalDevice(resources));
            surfaceData.emplace(*resources.instance, gpuCreateInfo.appInfo.pApplicationName, vk::Extent2D{800, 800});
            resources.queueFamilyIndices.emplace(gpu::vulkan::FindGraphicsAndPresentQueueFamilyIndex(resources, *surfaceData)) ;
            resources.device.emplace(gpu::vulkan::MakeDevice(resources, gpuCreateInfo));
            resources.commandPool.emplace(gpu::vulkan::MakeCommandPool(resources));
            resources.graphicsQueue.emplace(gpu::vulkan::MakeGraphicsQueue(resources));
            resources.presentQueue.emplace(gpu::vulkan::MakePresentQueue(resources));
            swapchainData.emplace(gpu::vulkan::MakeSwapchainData(resources, *surfaceData, vk::ImageUsageFlagBits::eColorAttachment));
            ufox::debug::log(debug::LogLevel::INFO, "hi");
        }

        // bool HandleEvent(const SDL_Event &event) {
        //     switch (event.type) {
        //         case SDL_EVENT_QUIT:
        //             return false;
        //         case SDL_EVENT_WINDOW_RESIZED:
        //             windowing::sdl::UpdatePanel(resources.mainWindow);
        //             break;
        //         default: ;
        //     }
        //     return true;
        // }

        static void HandleError(const std::exception &e) {
            std::cerr << "Engine error: " << e.what() << std::endl;
            throw;
        }

        gpu::vulkan::GraphicDeviceCreateInfo gpuCreateInfo{};
        GPUResources resources{};
        std::optional<gpu::vulkan::SurfaceData> surfaceData{};
        std::optional<gpu::vulkan::SwapchainData> swapchainData{};
    };
}