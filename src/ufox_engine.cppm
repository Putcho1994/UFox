//
// Created by Puwiwad on 05.10.2025.
//
module;
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include <fstream>
#ifdef USE_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#else
#include <GLFW/glfw3.h>
#endif
export module ufox_engine;
import ufox_lib;
import ufox_graphic_device;
import ufox_windowing;



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
        createInfo.enableSynchronization2 = true;
        createInfo.commandPoolCreateFlags = vk::CommandPoolCreateFlags{
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
            vk::CommandPoolCreateFlagBits::eTransient
        };

        return createInfo;
    }

class Engine {
public:
    Engine() = default;

    ~Engine() {
        // Wait for all GPU operations to complete before destroying resources
        if (gpuResources.device) {
            gpuResources.device->waitIdle();
        }

    }

    void Init() {
        InitializeGPU();
    }

    void Run() {
        window->Draw();
    }

private:
    void InitializeGPU() {
        gpuCreateInfo = CreateGraphicDeviceInfo();

        gpuResources.context.emplace();
        gpuResources.instance.emplace(gpu::vulkan::MakeInstance(gpuResources, gpuCreateInfo));
        gpuResources.physicalDevice.emplace(gpu::vulkan::PickBestPhysicalDevice(gpuResources));

        window.emplace(gpuResources, "UFox", vk::Extent2D{800, 800});

        gpuResources.queueFamilyIndices.emplace(window->getQueueFamilyIndices());
        gpuResources.device.emplace(gpu::vulkan::MakeDevice(gpuResources, gpuCreateInfo));
        gpuResources.commandPool.emplace(gpu::vulkan::MakeCommandPool(gpuResources));

        gpuResources.graphicsQueue.emplace(gpu::vulkan::MakeGraphicsQueue(gpuResources));
        gpuResources.presentQueue.emplace(gpu::vulkan::MakePresentQueue(gpuResources));

        window->initResources();

    }

    gpu::vulkan::GraphicDeviceCreateInfo        gpuCreateInfo{};
    gpu::vulkan::GPUResources                   gpuResources{};
    std::optional<windowing::UFoxWindow>        window{};
};

}