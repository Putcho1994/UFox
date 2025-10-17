module;
#if defined( _WIN32 )
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined( __linux__ )
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined( __APPLE__ )
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <string>
#include <optional>
#include <SDL3/SDL_vulkan.h>
#include <map>
#include <set>
#include <GLFW/glfw3.h>

export module ufox_graphic;

import ufox_lib;
import ufox_windowing;

export namespace ufox::gpu::vulkan {

    std::vector<std::string> GetDeviceExtensions() {
        return {
            vk::KHRSwapchainExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRSynchronization2ExtensionName,
            vk::KHRCreateRenderpass2ExtensionName
        };
    }

    std::vector<std::string> GetInstanceExtensions()
    {
        std::vector<std::string> extensions;
        extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME );
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
        extensions.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_METAL_EXT )
        extensions.push_back( VK_EXT_METAL_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_VI_NN )
        extensions.push_back( VK_NN_VI_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
        extensions.push_back( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
        extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XCB_KHR )
        extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
        extensions.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
        extensions.push_back( VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME );
#endif
        return extensions;
    }

    struct GraphicDeviceCreateInfo {
        vk::ApplicationInfo appInfo{};
        std::vector<std::string> instanceExtensions{};
        std::vector<std::string> deviceExtensions{};
        bool enableDynamicRendering{true};
        bool enableExtendedDynamicState{true};
        vk::CommandPoolCreateFlags commandPoolCreateFlags{};
    };

    [[nodiscard]] vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    [[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::Extent2D extent, const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return surfaceCapabilities.currentExtent;
        }

        return {
            std::clamp<uint32_t>(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp<uint32_t>(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        };
    }



    auto ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
        const auto preferredFormat = std::find_if(formats.begin(), formats.end(),
            [](const auto& format) {
                return format.format == vk::Format::eB8G8R8A8Srgb &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            });

        if (preferredFormat == formats.end()) {
            throw std::runtime_error("Failed to find suitable surface format");
        }
        return *preferredFormat;
    }
    bool AreExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available) {
        for (const auto* req : required) {
            bool found = false;
            for (const auto& avail : available) {
                if (strcmp(req, avail.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                debug::log(debug::LogLevel::WARNING, "Missing extension: {}", req);
                return false;
            }
        }
        return true;
    }

    uint32_t ClampSurfaceImageCount( const uint32_t desiredImageCount, const uint32_t minImageCount, const uint32_t maxImageCount )
    {
        uint32_t imageCount = ( std::max )( desiredImageCount, minImageCount );
        // Some drivers report maxImageCount as 0, so only clamp to max if it is valid.
        if ( maxImageCount > 0 )
        {
            imageCount = ( std::min )( imageCount, maxImageCount );
        }
        return imageCount;
    }

    auto ConfigureQueueSharingMode(vk::SwapchainCreateInfoKHR& createInfo, const QueueFamilyIndices& indices) {
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                     .setQueueFamilyIndexCount(2)
                     .setPQueueFamilyIndices(queueFamilyIndices);
        } else {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }
    }

    vk::raii::Instance MakeInstance(const vk::raii::Context&        context, const GraphicDeviceCreateInfo& info) {
        std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();

        std::vector<char const *> enabledExtensions;
        enabledExtensions.reserve( info.instanceExtensions.size() );
        for ( auto const & ext : info.instanceExtensions )
        {
            enabledExtensions.push_back( ext.data() );
        }

        if (!AreExtensionsSupported(enabledExtensions, availableExtensions))
            throw std::runtime_error("Required instance extensions are missing");

        vk::InstanceCreateInfo createInfo{};
        createInfo.setPApplicationInfo(&info.appInfo)
                .setEnabledExtensionCount(static_cast<uint32_t>(enabledExtensions.size()))
                .setPpEnabledExtensionNames(enabledExtensions.data());

        return {context, createInfo};
    }

    vk::raii::Instance MakeInstance(const GPUResources& gpu, const GraphicDeviceCreateInfo& info) {
        return MakeInstance(*gpu.context, info);
    }

    vk::raii::PhysicalDevice PickBestPhysicalDevice(const vk::raii::Instance& instance) {
        auto devices = vk::raii::PhysicalDevices(instance);
        if (devices.empty())
            throw std::runtime_error( "failed to find GPUs with Vulkan support!" );

        std::multimap<uint32_t, vk::raii::PhysicalDevice> candidates;

        for (const auto& device : devices) {
            auto deviceProperties = device.getProperties();
            auto deviceFeatures = device.getFeatures();

            // Skip if doesn't meet basic requirements
            if (deviceProperties.apiVersion < PhysicalDeviceRequirements::MINIMUM_API_VERSION ||
                !deviceFeatures.geometryShader){
                continue;
                }

            uint32_t score = 0;

            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu){
                score += PhysicalDeviceRequirements::DISCRETE_GPU_SCORE_BONUS;
            }

            // Safely add scores with overflow check
            if (score <= UINT32_MAX - deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic){
                score += deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic;
            }

            if (score <= UINT32_MAX - deviceProperties.limits.maxImageDimension2D){
                score += deviceProperties.limits.maxImageDimension2D;
            }

            candidates.insert(std::make_pair(score, device));
        }

        if (candidates.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        return std::move(candidates.rbegin()->second);
    }

    vk::raii::PhysicalDevice PickBestPhysicalDevice(const GPUResources& gpu) {
        return PickBestPhysicalDevice(*gpu.instance);
    }

    QueueFamilyIndices FindGraphicsAndPresentQueueFamilyIndex(const vk::raii::PhysicalDevice& physicalDevice, const SurfaceData& surfaceData) {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        assert(queueFamilyProperties.size() < (std::numeric_limits<uint32_t>::max)());

        // get the first index into queueFamiliyProperties which supports graphics
        auto graphicsQueueFamilyProperty =
          std::find_if( queueFamilyProperties.begin(),
                        queueFamilyProperties.end(),
                        []( vk::QueueFamilyProperties const & qfp ) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics;});
        assert( graphicsQueueFamilyProperty != queueFamilyProperties.end());

        auto graphicsQueueFamilyIndex = static_cast<uint32_t>( std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
        if (physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, *surfaceData.surface))
        {
            return {graphicsQueueFamilyIndex,graphicsQueueFamilyIndex};  // the first graphicsQueueFamilyIndex does also support presents
        }

        // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++ )
        {
            if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics &&
                physicalDevice.getSurfaceSupportKHR( static_cast<uint32_t>(i), *surfaceData.surface) ) {
                return {static_cast<uint32_t>(i), static_cast<uint32_t>(i)};
                }
        }

        // there's nothing like a single family index that supports both graphics and present -> look for an other
        // family index that supports present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++ )
        {
            if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surfaceData.surface)){
                return { graphicsQueueFamilyIndex, static_cast<uint32_t>( i ) };
            }
        }

        throw std::runtime_error( "Could not find queues for both graphics or present -> terminating" );
    }

    QueueFamilyIndices FindGraphicsAndPresentQueueFamilyIndex(const GPUResources& gpu, const SurfaceData& surfaceData) {
        return FindGraphicsAndPresentQueueFamilyIndex(*gpu.physicalDevice, surfaceData);
    }

    vk::raii::Device MakeDevice(const vk::raii::PhysicalDevice&       physicalDevice,
                                   const uint32_t                     queueFamilyIndex,
                                   const std::vector<std::string>&    extensions             = {},
                                   const vk::PhysicalDeviceFeatures*  physicalDeviceFeatures = nullptr,
                                   const void*                        pNext                  = nullptr )
    {
        std::vector<char const *> enabledExtensions;
        enabledExtensions.reserve( extensions.size() );
        for ( auto const & ext : extensions )
        {
            enabledExtensions.push_back( ext.data() );
        }

        float                     queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo( vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority );
        vk::DeviceCreateInfo      deviceCreateInfo( vk::DeviceCreateFlags(), deviceQueueCreateInfo, {}, enabledExtensions, physicalDeviceFeatures, pNext );
        return {physicalDevice, deviceCreateInfo};
    }

    vk::raii::Device MakeDevice(const GPUResources& gpu, const GraphicDeviceCreateInfo& info) {

        auto features = gpu.physicalDevice->getFeatures2();
        vk::PhysicalDeviceVulkan13Features vulkan13Features;
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures;
        vulkan13Features.dynamicRendering = info.enableDynamicRendering ? vk::True : vk::False;
        extendedDynamicStateFeatures.extendedDynamicState = info.enableExtendedDynamicState ? vk::True : vk::False;
        vulkan13Features.pNext = &extendedDynamicStateFeatures;
        features.pNext = &vulkan13Features;

        return MakeDevice(*gpu.physicalDevice, gpu.queueFamilyIndices->graphicsFamily, info.deviceExtensions, nullptr, features);
    }

    vk::raii::CommandPool MakeCommandPool(const vk::raii::Device& device, const QueueFamilyIndices& queueFamilyIndices,
        const vk::CommandPoolCreateFlags& flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient) {
        vk::CommandPoolCreateInfo createInfo{};
        createInfo.setQueueFamilyIndex(queueFamilyIndices.graphicsFamily)
                  .setFlags(flags);

        return {device, createInfo};
    }

    vk::raii::CommandPool MakeCommandPool(const GPUResources& gpu,
        const vk::CommandPoolCreateFlags& flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient) {
        return MakeCommandPool(*gpu.device, *gpu.queueFamilyIndices, flags);
    }

    vk::raii::CommandPool MakeCommandPool(const GPUResources& gpu, const GraphicDeviceCreateInfo& info) {
        return MakeCommandPool(*gpu.device, *gpu.queueFamilyIndices, info.commandPoolCreateFlags);
    }

    vk::raii::Queue MakeGraphicsQueue(const vk::raii::Device& device, const QueueFamilyIndices& queueFamilyIndices) {
        ufox::debug::log(debug::LogLevel::INFO, "Retrieving graphics queue for queue family index: {}", queueFamilyIndices.graphicsFamily);

        return {device, queueFamilyIndices.graphicsFamily, 0};
    }

    vk::raii::Queue MakeGraphicsQueue(const GPUResources& gpu) {
        return MakeGraphicsQueue(*gpu.device, *gpu.queueFamilyIndices);
    }

    vk::raii::Queue MakePresentQueue(const vk::raii::Device& device, const QueueFamilyIndices& queueFamilyIndices) {

        return {device, queueFamilyIndices.presentFamily, 0};
    }

    vk::raii::Queue MakePresentQueue(const GPUResources& gpu) {
        return MakePresentQueue(*gpu.device, *gpu.queueFamilyIndices);
    }

    SwapchainData MakeSwapchainData(const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface,
        const vk::Extent2D& extent, vk::ImageUsageFlags usage, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex) {
        SwapchainData resource{};
        vk::SurfaceCapabilitiesKHR surfaceCapabilities  = physicalDevice.getSurfaceCapabilitiesKHR( surface );
        vk::SurfaceFormatKHR surfaceFormat              = ChooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
        resource.colorFormat                            = surfaceFormat.format;
        vk::Extent2D swapchainExtent                    = ChooseSwapExtent(extent, surfaceCapabilities);
        vk::PresentModeKHR presentMode                  = ChooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface));
        vk::SurfaceTransformFlagBitsKHR preTransform    =  surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surfaceCapabilities.currentTransform;
        vk::CompositeAlphaFlagBitsKHR compositeAlpha    = surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied  ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
            :  surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
            : surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit ? vk::CompositeAlphaFlagBitsKHR::eInherit: vk::CompositeAlphaFlagBitsKHR::eOpaque;

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.setSurface(*surface)
            .setMinImageCount(ClampSurfaceImageCount(3u, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount))
            .setImageFormat(resource.colorFormat )
            .setImageColorSpace(surfaceFormat.colorSpace) // Use colorSpace from surfaceFormat
            .setImageExtent(swapchainExtent)
            .setImageArrayLayers(1)
            .setImageUsage(usage)
            .setPreTransform(preTransform)
            .setCompositeAlpha(compositeAlpha)
            .setPresentMode(presentMode)
            .setClipped(true);

        if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
            uint32_t queueFamilyIndices[2] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setPQueueFamilyIndices(queueFamilyIndices);
        }
        else {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }

        resource.swapChain.emplace(device, createInfo);

        resource.images = resource.swapChain->getImages();

        resource.imageViews.reserve( resource.images.size() );
        vk::ImageViewCreateInfo imageViewCreateInfo( {}, {}, vk::ImageViewType::e2D, resource.colorFormat, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );
        for ( auto image : resource.images )
        {
            imageViewCreateInfo.image = image;
            resource.imageViews.emplace_back( device, imageViewCreateInfo );
        }

        return resource;
    }

    SwapchainData MakeSwapchainData(const GPUResources& gpu, const SurfaceData& surfaceData, vk::ImageUsageFlags usage) {
        return MakeSwapchainData(*gpu.physicalDevice, *gpu.device, *surfaceData.surface, surfaceData.extent,
            usage, gpu.queueFamilyIndices->graphicsFamily, gpu.queueFamilyIndices->presentFamily);
    }
}
