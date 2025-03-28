#include "app.hpp"
#include "lve_device.hpp"
#include "lve_model.hpp"
#include "lve_pipeline.hpp"
#include "lve_swap_chain.hpp"
#include "lve_window.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

namespace lve {
    App::App() {
        loadModels();
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }

    App::~App() {
        vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
    }

    void App::run() {
        while (!lveWindow.shouldClose()) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(lveDevice.device());
    }


    std::vector<LveModel::Vertex> App::Sierpinski_trig(std::vector<LveModel::Vertex> verts) {
        std::vector<LveModel::Vertex> outverts;

        for (int i = 0; i < verts.size(); i+=3) {
            LveModel::Vertex v1 = verts[i];
            LveModel::Vertex v2 = verts[i+1];
            LveModel::Vertex v3 = verts[i+2];

            LveModel::Vertex v12 = {(v1.position+v2.position) * 0.5f, (v1.color+v3.color) * 0.5f};
            LveModel::Vertex v13 = {(v1.position+v3.position) * 0.5f, (v2.color+v3.color) * 0.5f};
            LveModel::Vertex v23 = {(v2.position+v3.position) * 0.5f, (v1.color+v2.color) * 0.5f};

            outverts.push_back(v1);
            outverts.push_back(v12);
            outverts.push_back(v13);

            outverts.push_back(v2);
            outverts.push_back(v12);
            outverts.push_back(v23);

            outverts.push_back(v3);
            outverts.push_back(v13);
            outverts.push_back(v23);

            outverts.push_back(v12); 
            outverts.push_back(v13); 
            outverts.push_back(v23);
        }
        return outverts;
    }

    std::vector<LveModel::Vertex> App::Sierpinski(int iter) {
        std::vector<LveModel::Vertex> verts {
            {{0.0f, -0.8f}, {1.0f,0.0f,0.0f}},
            {{0.8f, 0.8f}, {0.0f,1.0f,0.0f}},
            {{-0.8f, 0.8f}, {0.0f,0.0f,1.0f}}
        };

        for (int i = 0; i < iter; i++) {
            verts = Sierpinski_trig(verts);
        }

        return verts;
    }

    void App::loadModels() {
        std::vector<LveModel::Vertex> vertices = Sierpinski(6); 

        lveModel = std::make_unique<LveModel>(lveDevice, vertices);
    }

    void App::createPipelineLayout() {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if(vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }


    void App::createPipeline() {
        auto pipelineConfig = LvePipeline::defaultPipelineConfigInfo(lveSwapChain.width(), lveSwapChain.height());
        pipelineConfig.renderPass = lveSwapChain.getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        lvePipeline = std::make_unique<LvePipeline>(
            lveDevice, 
            "shaders/simple_shader.vert.spv", 
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }


    void App::createCommandBuffers(){
        commandBuffers.resize(lveSwapChain.imageCount());
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = lveDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (int i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = lveSwapChain.getRenderPass();
            renderPassInfo.framebuffer = lveSwapChain.getFrameBuffer(i);

            renderPassInfo.renderArea.offset = {0,0};
            renderPassInfo.renderArea.extent = lveSwapChain.getSwapChainExtent();

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            lvePipeline->bind(commandBuffers[i]);
            lveModel->bind(commandBuffers[i]);
            lveModel->draw(commandBuffers[i]);

            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to end recording command buffer!");
            }
        }
    }

    void App::drawFrame(){
        uint32_t imageIndex;
        auto result = lveSwapChain.acquireNextImage(&imageIndex);

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        result = lveSwapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

    }
}
