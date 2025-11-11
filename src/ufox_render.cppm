//
// Created by b-boy on 11.11.2025.
//
module;

#include <vulkan/vulkan_raii.hpp>

export module ufox_render;

import ufox_lib;

export namespace ufox::render {

    void RenderArea(const vk::raii::CommandBuffer& cmb, const windowing::WindowResource& window, const vk::Rect2D& rect, const vk::ClearColorValue& clearColor) {
        if (rect.extent.width < 1 || rect.extent.height < 1) return;

        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(window.swapchainResource->getCurrentImageView())
                       .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                       .setLoadOp(vk::AttachmentLoadOp::eClear)
                       .setStoreOp(vk::AttachmentStoreOp::eStore)
                       .setClearValue(clearColor);

        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea(rect)
                     .setLayerCount(1)
                     .setColorAttachmentCount(1)
                     .setPColorAttachments(&colorAttachment);
        cmb.beginRendering(renderingInfo);





        cmb.endRendering();
    }

}
