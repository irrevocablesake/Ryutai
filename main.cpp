#define VOLK_IMPLEMENTATION
#include<volk/volk.h>

#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/quaternion.hpp>

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include<iostream>
#include<vector>
#include<array>

class Renderer {

	public:
		//validation functions
		void validateResult( VkResult result, std::string message = "ERROR!") {
			if (result != VK_SUCCESS) {
				std::cerr << "ERROR: " << message << std::endl;
				exit(result);
			}
		}

		void validateResult(bool result, std::string message = "ERROR!") {
			if (!result) {
				std::cerr << "ERROR " << message << std::endl;
				exit(result);
			}
		}

		void validateSwapchain( VkResult result ) {
			if (result < VK_SUCCESS ) {
				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					deviceIF.logical.swapchainConfiguration.updateSwapchain = true;
					return;
				}

				std::cerr << "ERROR: Swapchain Validation Failed" << std::endl;
				exit(result);
			}
		}

		//vulkan functions
		void setupLibraries() {
			validateResult(SDL_Init( SDL_INIT_VIDEO ));
			validateResult(SDL_Vulkan_LoadLibrary(NULL));
			volkInitialize();
		}

		void setupInstance() {
			VkApplicationInfo applicationInfo{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Rytutai",
				.apiVersion = VK_API_VERSION_1_4
			};

			extensionsIF.instance.extensions = SDL_Vulkan_GetInstanceExtensions( &extensionsIF.instance.count );

			VkInstanceCreateInfo instanceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &applicationInfo,
				.enabledExtensionCount = extensionsIF.instance.count,
				.ppEnabledExtensionNames = extensionsIF.instance.extensions
			};

			validateResult( vkCreateInstance(&instanceCreateInfo, nullptr, &windowIF.vkInstance ));
			volkLoadInstance(windowIF.vkInstance);
		}

		void pickPhysicalDeviceAndQueueIndex() {
			validateResult(vkEnumeratePhysicalDevices( windowIF.vkInstance, &deviceIF.physical.count, nullptr ));

			deviceIF.physical.devices.resize(deviceIF.physical.count);
			deviceIF.queue.properties.resize(deviceIF.physical.count);

			validateResult( vkEnumeratePhysicalDevices( windowIF.vkInstance, &deviceIF.physical.count, deviceIF.physical.devices.data() ) );

			for (uint32_t i = 0; i < deviceIF.physical.count; i++) {
				VkPhysicalDeviceProperties2 properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
				};

				vkGetPhysicalDeviceProperties2(deviceIF.physical.devices[i], &properties);
				deviceIF.physical.properties.push_back(properties);

				uint32_t queueFamiliesCount;
				vkGetPhysicalDeviceQueueFamilyProperties2(deviceIF.physical.devices[i], &queueFamiliesCount, nullptr);
				deviceIF.queue.properties[i].resize(queueFamiliesCount);

				for (uint32_t j = 0; j < queueFamiliesCount; j++) {
					deviceIF.queue.properties[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
				}

				vkGetPhysicalDeviceQueueFamilyProperties2(deviceIF.physical.devices[i], &queueFamiliesCount, deviceIF.queue.properties[i].data());
			}

			for (uint32_t i = 0; i < deviceIF.physical.count; i++) {

				bool found = false;
				for (uint32_t j = 0; j < deviceIF.queue.properties[i].size(); j++) {
					
					bool cond1 = deviceIF.physical.properties[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
					bool cond2 = deviceIF.queue.properties[i][j].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
					if (cond1 && cond2 ) {
						deviceIF.physical.index = i;
						deviceIF.physical.device = deviceIF.physical.devices[i];

						deviceIF.queue.index = j;

						found = true;
						break;
					}
				}
				
				if (found) {
					break;
				}
			}

			validateResult( SDL_Vulkan_GetPresentationSupport( windowIF.vkInstance, deviceIF.physical.device, deviceIF.queue.index ));
		}

		void setupLogicalDevice() {

			float priorities = 1.0;

			VkDeviceQueueCreateInfo queueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = deviceIF.queue.index,
				.queueCount = 1,
				.pQueuePriorities = &priorities
			};

			VkPhysicalDeviceVulkan11Features enabledVk11Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
				.shaderDrawParameters = VK_TRUE
			};

			VkPhysicalDeviceVulkan13Features enabledVk13Features{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
				.pNext = &enabledVk11Features,
				.synchronization2 = VK_TRUE,
				.dynamicRendering = VK_TRUE,
			};

			VkDeviceCreateInfo deviceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = &enabledVk13Features,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &queueCreateInfo,
				.enabledExtensionCount = static_cast< uint32_t >( extensionsIF.device.extensions.size() ),
				.ppEnabledExtensionNames = extensionsIF.device.extensions.data()
			};

			validateResult(vkCreateDevice(deviceIF.physical.device, &deviceCreateInfo, nullptr, &deviceIF.logical.device));
			
			volkLoadDevice(deviceIF.logical.device);
			vkGetDeviceQueue(deviceIF.logical.device, deviceIF.queue.index, 0, &deviceIF.queue.queue );
		}

		void setupVMA() {
			VmaVulkanFunctions vkFunctions{
				.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
				.vkGetDeviceProcAddr = vkGetDeviceProcAddr
			};

			VmaAllocatorCreateInfo allocatorCreateInfo{
				.physicalDevice = deviceIF.physical.device,
				.device = deviceIF.logical.device,
				.pVulkanFunctions = &vkFunctions,
				.instance = windowIF.vkInstance
			};

			validateResult( vmaCreateAllocator( &allocatorCreateInfo, &vmaAllocator ) );
		}
		
		void setupWindowAndSurface(){
			windowIF.window = SDL_CreateWindow(
				windowIF.windowName.c_str(),
				windowIF.windowWidth,
				windowIF.windowHeight,
				windowIF.windowFlags
			);

			validateResult( SDL_GetWindowSize( windowIF.window, &windowIF.dimensions.x, &windowIF.dimensions.y  ) );

			validateResult( SDL_Vulkan_CreateSurface( windowIF.window, windowIF.vkInstance, nullptr, &windowIF.surface )  );
			validateResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( deviceIF.physical.device, windowIF.surface, &windowIF.surfaceCapabilities ));
		}

		void setupSwapchain() {
			deviceIF.logical.swapchainConfiguration.swapchainCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = windowIF.surface,
				.minImageCount = windowIF.surfaceCapabilities.minImageCount,
				.imageFormat = deviceIF.logical.swapchainConfiguration.imageFormat,
				.imageColorSpace = deviceIF.logical.swapchainConfiguration.imageColorSpace,
				.imageExtent = {
					.width = windowIF.surfaceCapabilities.currentExtent.width,
					.height = windowIF.surfaceCapabilities.currentExtent.height
				},
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = VK_PRESENT_MODE_FIFO_KHR
			};

			validateResult( vkCreateSwapchainKHR( deviceIF.logical.device, &deviceIF.logical.swapchainConfiguration.swapchainCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchain ));

			validateResult( vkGetSwapchainImagesKHR( deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, nullptr  ) );
			deviceIF.logical.swapchainConfiguration.swapchainImages.resize(deviceIF.logical.swapchainConfiguration.imageCount);
			deviceIF.logical.swapchainConfiguration.swapchainImageViews.resize(deviceIF.logical.swapchainConfiguration.imageCount);

			validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, deviceIF.logical.swapchainConfiguration.swapchainImages.data()));

			for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
				VkImageViewCreateInfo imageViewCreateInfo{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = deviceIF.logical.swapchainConfiguration.swapchainImages[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = deviceIF.logical.swapchainConfiguration.imageFormat,
					.subresourceRange {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					}
				};

				validateResult( vkCreateImageView( deviceIF.logical.device, &imageViewCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchainImageViews[i]));
			}
		}

		void setupSync2() {
			VkSemaphoreCreateInfo semaphoreCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			};

			VkFenceCreateInfo fenceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};

			for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.maxFramesInFlight; i++) {
				validateResult(vkCreateFence( deviceIF.logical.device, &fenceCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.fences[i]));
				validateResult(vkCreateSemaphore(deviceIF.logical.device, &semaphoreCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.presentSemaphore[i]));
			}

			deviceIF.logical.swapchainConfiguration.renderSemaphore.resize(deviceIF.logical.swapchainConfiguration.imageCount);
			for ( VkSemaphore& semaphore : deviceIF.logical.swapchainConfiguration.renderSemaphore ) {
				validateResult(vkCreateSemaphore(deviceIF.logical.device, &semaphoreCreateInfo, nullptr, &semaphore));
			}
		}

		void setupCommandBuffers() {
			VkCommandPoolCreateInfo commandPoolCreateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = deviceIF.queue.index
			};

			validateResult( vkCreateCommandPool( deviceIF.logical.device, &commandPoolCreateInfo, nullptr, &deviceIF.logical.commandPool ) );
		
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = deviceIF.logical.commandPool,
				.commandBufferCount = deviceIF.logical.swapchainConfiguration.maxFramesInFlight
			};

			validateResult( vkAllocateCommandBuffers( deviceIF.logical.device, &commandBufferAllocateInfo, deviceIF.logical.swapchainConfiguration.commandBuffers.data()  ) );
		}

		void loadAndCompileShaders() {
			slang::createGlobalSession(slangGlobalSession.writeRef());

			auto slangTargets{
				std::to_array< slang::TargetDesc >({{
					.format{SLANG_SPIRV},
					.profile{slangGlobalSession->findProfile("spirv_1_4")}
				}})
			};

			auto slangOptions{
				std::to_array < slang::CompilerOptionEntry>({{
					slang::CompilerOptionName::EmitSpirvDirectly,
					{
						slang::CompilerOptionValueKind::Int, 1
					}
				}})
			};

			slang::SessionDesc slangSessionDesc{
				.targets{
					slangTargets.data()
				},
				.targetCount{
					SlangInt(slangTargets.size())
				},
				.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
				.compilerOptionEntries{
					slangOptions.data()
				},
				.compilerOptionEntryCount{
					uint32_t(slangOptions.size())
				}
			};

			Slang::ComPtr< slang::ISession > slangSession;
			slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

			Slang::ComPtr< slang::IModule > slangModule{
				slangSession->loadModuleFromSource("Modern Vulkan SLANG Shader", "assets/shaders/shader.slang", nullptr, nullptr)
			};

			Slang::ComPtr< ISlangBlob > spirv;
			slangModule->getTargetCode(0, spirv.writeRef());

			VkShaderModuleCreateInfo shaderModuleCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = spirv->getBufferSize(),
				.pCode = (uint32_t*)spirv->getBufferPointer()
			};

			validateResult(vkCreateShaderModule(deviceIF.logical.device, &shaderModuleCreateInfo, nullptr, &shaderModule));

		}

		struct FieldState {
			VkImage image;
			VkImageView imageView;
			VmaAllocation allocation;
			VkSampler sampler;
			VkDescriptorImageInfo descriptor;

			VkClearColorValue clearColor;
		};

		void clearImage( FieldState &fieldState, VkClearColorValue &clearValue ) {
			VkFenceCreateInfo fenceOneTimeCreateInfo{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
			};
			VkFence fenceOneTime;
			validateResult(vkCreateFence(deviceIF.logical.device, &fenceOneTimeCreateInfo, nullptr, &fenceOneTime));

			VkCommandBuffer commandBufferOneTime;
			VkCommandBufferAllocateInfo commandBufferOneTimeAllocationInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = deviceIF.logical.commandPool,
				.commandBufferCount = 1
			};

			validateResult(vkAllocateCommandBuffers(deviceIF.logical.device, &commandBufferOneTimeAllocationInfo, &commandBufferOneTime));

			VkCommandBufferBeginInfo commandBufferOneTimeBeginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			};
			validateResult(vkBeginCommandBuffer(commandBufferOneTime, &commandBufferOneTimeBeginInfo));

			VkImageMemoryBarrier2 barrierLayoutTransition{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
				.srcAccessMask = VK_ACCESS_2_NONE,
				.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.image = fieldState.image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};

			VkDependencyInfo barrierDependencyInfo{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrierLayoutTransition
			};

			vkCmdPipelineBarrier2(commandBufferOneTime, &barrierDependencyInfo);

			VkImageSubresourceRange range{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			};
			vkCmdClearColorImage(commandBufferOneTime, fieldState.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);

			VkImageMemoryBarrier2 barrierLayoutTransition1{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = fieldState.image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};

			VkDependencyInfo barrierDependencyInfo1{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrierLayoutTransition1
			};

			vkCmdPipelineBarrier2(commandBufferOneTime, &barrierDependencyInfo1);

			validateResult(vkEndCommandBuffer(commandBufferOneTime));

			VkSubmitInfo oneTimeSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.commandBufferCount = 1,
				.pCommandBuffers = &commandBufferOneTime
			};

			validateResult(vkQueueSubmit(deviceIF.queue.queue, 1, &oneTimeSubmitInfo, fenceOneTime));
			validateResult(vkWaitForFences(deviceIF.logical.device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
		}

		FieldState generateField( VkClearColorValue clearValue ) {

			FieldState fieldState;

			VkImageCreateInfo textureImageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = deviceIF.logical.swapchainConfiguration.imageFormat,
				.extent{
					.width = windowIF.surfaceCapabilities.currentExtent.width,
					.height = windowIF.surfaceCapabilities.currentExtent.height,
					.depth = 1
				},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};

			VmaAllocationCreateInfo textureImageAllocationCreateInfo{
				.usage = VMA_MEMORY_USAGE_AUTO
			};
			validateResult(vmaCreateImage(vmaAllocator, &textureImageCreateInfo, &textureImageAllocationCreateInfo, &fieldState.image, &fieldState.allocation, nullptr));

			VkImageViewCreateInfo textureImageViewCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = fieldState.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = textureImageCreateInfo.format,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				}
			};

			validateResult(vkCreateImageView(deviceIF.logical.device, &textureImageViewCreateInfo, nullptr, &fieldState.imageView));

			clearImage( fieldState, clearValue );

			VkSamplerCreateInfo samplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.maxAnisotropy = 8.0f,
				.maxLod = 1
			};

			validateResult(vkCreateSampler(deviceIF.logical.device, &samplerCreateInfo, nullptr, &fieldState.sampler));

			fieldState.descriptor = {
				.sampler = fieldState.sampler,
				.imageView = fieldState.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			return fieldState;
		}

		void generateFields() {
			FieldState vectorFieldA = generateField(VkClearColorValue{ 1.0, 0.0, 0.0 });
			FieldState vectorFieldB = generateField(VkClearColorValue{ 0.0, 1.0, 0.0 });

			fields.push_back(vectorFieldA);
			fields.push_back(vectorFieldB);

			uploadFields();
		}

		void uploadFields() {

			descriptorSets.resize(fields.size());

			VkDescriptorPoolSize poolSize{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = static_cast< uint32_t>( fields.size())
			};

			VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = static_cast<uint32_t>(fields.size()),
				.poolSizeCount = 1,
				.pPoolSizes = &poolSize
			};

			validateResult( vkCreateDescriptorPool( deviceIF.logical.device, &descriptorPoolCreateInfo, nullptr, &descriptorPool  ));
	
			VkDescriptorSetLayoutBinding binding{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			};
			
			VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = 1,
				.pBindings = &binding
			};

			validateResult( vkCreateDescriptorSetLayout( deviceIF.logical.device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout ) );
		
			descriptorSetLayouts = std::vector<VkDescriptorSetLayout>(fields.size(), descriptorSetLayout);
			VkDescriptorSetAllocateInfo descriptorAllocateInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = descriptorPool,
				.descriptorSetCount = static_cast< uint32_t>(fields.size()),
				.pSetLayouts = descriptorSetLayouts.data()
			};

			validateResult( vkAllocateDescriptorSets( deviceIF.logical.device, &descriptorAllocateInfo, descriptorSets.data()));
		
			for (uint32_t index = 0; index < fields.size(); index++) {
				VkWriteDescriptorSet writeDesriptorSet{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = descriptorSets[ index ],
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &fields[index].descriptor
				};

				vkUpdateDescriptorSets(deviceIF.logical.device, 1, &writeDesriptorSet, 0, nullptr);
			}
		}

		void setupPipeline() {
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = 1,
				.pSetLayouts = &descriptorSetLayout
			};

			validateResult( vkCreatePipelineLayout( deviceIF.logical.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout  ) );
		
			VkPipelineVertexInputStateCreateInfo vertexInputState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.vertexBindingDescriptionCount = 0,
				.pVertexBindingDescriptions= nullptr,
				.vertexAttributeDescriptionCount = 0,
				.pVertexAttributeDescriptions = nullptr,
			};

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
			};

			std::vector< VkPipelineShaderStageCreateInfo > shaderStages{
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = shaderModule,
					.pName = "main"
				},
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = shaderModule,
					.pName = "main"
				}
			};

			std::vector< VkDynamicState > dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = 2,
				.pDynamicStates = dynamicStates.data()
			};

			VkPipelineViewportStateCreateInfo viewportState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				.viewportCount = 1,
				.scissorCount = 1
			};

			VkPipelineDepthStencilStateCreateInfo depthStencilState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_FALSE,
				.depthWriteEnable = VK_FALSE
			};

			VkPipelineRenderingCreateInfo renderingCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &deviceIF.logical.swapchainConfiguration.imageFormat,
			};

			VkPipelineColorBlendAttachmentState blendAttachment{
				.colorWriteMask = 0xF
			};

			VkPipelineColorBlendStateCreateInfo colorBlendState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &blendAttachment
			};

			VkPipelineRasterizationStateCreateInfo rasterizationState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.cullMode = VK_CULL_MODE_BACK_BIT,
				.frontFace = VK_FRONT_FACE_CLOCKWISE,
				.lineWidth = 1.0f
			};

			VkPipelineMultisampleStateCreateInfo multisampleState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
			};

			VkGraphicsPipelineCreateInfo pipelineCreateInfo{
				.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				.pNext = &renderingCreateInfo,
				.stageCount = 2,
				.pStages = shaderStages.data(),
				.pVertexInputState = &vertexInputState,
				.pInputAssemblyState = &inputAssemblyState,
				.pViewportState = &viewportState,
				.pRasterizationState = &rasterizationState,
				.pMultisampleState = &multisampleState,
				.pDepthStencilState = &depthStencilState,
				.pColorBlendState = &colorBlendState,
				.pDynamicState = &dynamicState,
				.layout = pipelineLayout
			};

			validateResult(vkCreateGraphicsPipelines( deviceIF.logical.device, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline  ));
		}

		void recreateSwapchain() {
			deviceIF.logical.swapchainConfiguration.updateSwapchain = false;
			vkDeviceWaitIdle(deviceIF.logical.device);
			validateResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceIF.physical.device, windowIF.surface, &windowIF.surfaceCapabilities), "Failed to Get Surface Capabilities");
			deviceIF.logical.swapchainConfiguration.swapchainCreateInfo.oldSwapchain = deviceIF.logical.swapchainConfiguration.swapchain;
			deviceIF.logical.swapchainConfiguration.swapchainCreateInfo.imageExtent = { .width = static_cast<uint32_t>(windowIF.dimensions.x), .height = static_cast<uint32_t>(windowIF.dimensions.y) };
			validateResult(vkCreateSwapchainKHR(deviceIF.logical.device, &deviceIF.logical.swapchainConfiguration.swapchainCreateInfo, nullptr, &deviceIF.logical.swapchainConfiguration.swapchain), "Failed to Create Swap chain");
			for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
				vkDestroyImageView(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchainImageViews[i], nullptr);
			}
			validateResult(vkGetSwapchainImagesKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, nullptr), "Failed To Create Swap Chain Images");
			deviceIF.logical.swapchainConfiguration.swapchainImages.resize(deviceIF.logical.swapchainConfiguration.imageCount);
			validateResult(vkGetSwapchainImagesKHR( deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, &deviceIF.logical.swapchainConfiguration.imageCount, deviceIF.logical.swapchainConfiguration.swapchainImages.data()), "Failed To Get Swap Chain Images");
			deviceIF.logical.swapchainConfiguration.swapchainImageViews.resize(deviceIF.logical.swapchainConfiguration.imageCount);
			for (uint32_t i = 0; i < deviceIF.logical.swapchainConfiguration.imageCount; i++) {
				VkImageViewCreateInfo viewCI{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = deviceIF.logical.swapchainConfiguration.swapchainImages[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = deviceIF.logical.swapchainConfiguration.imageFormat,
					.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
				};
				validateResult(vkCreateImageView(deviceIF.logical.device, &viewCI, nullptr, &deviceIF.logical.swapchainConfiguration.swapchainImageViews[i]), "Failed To Create Image View");
			}
			vkDestroySwapchainKHR(deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchainCreateInfo.oldSwapchain, nullptr);
		}

		void animate() {

			bool quit{ false };
			while (!quit) {

				SDL_Event event;
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_EVENT_QUIT) {
						quit = true;
					}

					if (event.type == SDL_EVENT_WINDOW_RESIZED) {
						deviceIF.logical.swapchainConfiguration.updateSwapchain = true;
					}
				}

				validateResult(vkWaitForFences(deviceIF.logical.device, 1, &deviceIF.logical.swapchainConfiguration.fences[frameIndex], true, UINT64_MAX));
				validateResult( vkResetFences( deviceIF.logical.device, 1, &deviceIF.logical.swapchainConfiguration.fences[frameIndex]));
			
				validateSwapchain(vkAcquireNextImageKHR( deviceIF.logical.device, deviceIF.logical.swapchainConfiguration.swapchain, UINT64_MAX, deviceIF.logical.swapchainConfiguration.presentSemaphore[frameIndex], VK_NULL_HANDLE, &imageIndex ));
			
				auto commandBuffer = deviceIF.logical.swapchainConfiguration.commandBuffers[frameIndex];
				validateResult( vkResetCommandBuffer( commandBuffer, 0 ));

				VkCommandBufferBeginInfo commandBufferBeginInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
				};

				validateResult( vkBeginCommandBuffer( commandBuffer, &commandBufferBeginInfo ) );
			
				VkViewport viewport{
				.width = float(windowIF.dimensions.x),
				.height = float(windowIF.dimensions.y),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
				};

				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				VkRect2D scissor{
					.extent {
						.width = static_cast<uint32_t>(windowIF.dimensions.x),
						.height = static_cast<uint32_t>(windowIF.dimensions.y),
					}
				};
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

				VkRenderingAttachmentInfo colorAttachmentInfo{
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = fields[(simulationIndex + 1) % 2].imageView,
					.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.clearValue{
						windowIF.clearColor
					}
				};

				VkRenderingInfo renderingInfo{
					.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
					.renderArea{
						.extent{
							.width = static_cast<uint32_t>(windowIF.dimensions.x),
							.height = static_cast<uint32_t>(windowIF.dimensions.y)
						},
					},
					.layerCount = 1,
					.colorAttachmentCount = 1,
					.pColorAttachments = &colorAttachmentInfo
				};

				VkImageMemoryBarrier2 transitionSwapchainImageToGetTextureData{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
						.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
						.srcAccessMask = VK_ACCESS_2_NONE,
						.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
						.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						.image = deviceIF.logical.swapchainConfiguration.swapchainImages[imageIndex],
						.subresourceRange {
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.levelCount = 1,
							.layerCount = 1
						}
				};

				VkDependencyInfo transitionSwapchainImageToGetTextureDataDependencyInfo{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = 1,
					.pImageMemoryBarriers = &transitionSwapchainImageToGetTextureData
				};

				vkCmdPipelineBarrier2(commandBuffer, &transitionSwapchainImageToGetTextureDataDependencyInfo);

				VkImageMemoryBarrier2 transitionAttachementfromReadtoGetWritten{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

					.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
					.srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,

					.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,

					.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,

					.image = fields[(simulationIndex + 1) % 2].image,

					.subresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					}
				};

				VkDependencyInfo transitionAttachementfromReadtoGetWrittenDependencyInfo{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = 1,
					.pImageMemoryBarriers = &transitionAttachementfromReadtoGetWritten
				};

				vkCmdPipelineBarrier2(commandBuffer, &transitionAttachementfromReadtoGetWrittenDependencyInfo);

				/*VkDescriptorImageInfo imageInfo{
					.sampler = fields[simulationIndex].sampler,
					.imageView = fields[simulationIndex].imageView,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				VkWriteDescriptorSet write{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = descriptorSet,
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageInfo
				};

				vkUpdateDescriptorSets(deviceIF.logical.device, 1, &write, 0, nullptr);*/

				vkCmdBeginRendering(commandBuffer, &renderingInfo);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[simulationIndex], 0, nullptr);
				vkCmdDraw(commandBuffer, 3, 1, 0, 0 );

				vkCmdEndRendering(commandBuffer);

				VkImageMemoryBarrier2 transitionAttachmentFromWriteToRead{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.image = fields[(simulationIndex + 1) % 2].image,
					.subresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					}
				};
				VkDependencyInfo transitionAttachmentFromWriteToReadDependencyInfo{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = 1,
					.pImageMemoryBarriers = &transitionAttachmentFromWriteToRead
				};
				vkCmdPipelineBarrier2(commandBuffer, &transitionAttachmentFromWriteToReadDependencyInfo);

				VkImageCopy copyRegion{
					.srcSubresource{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.layerCount = 1
					},
					.dstSubresource{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.layerCount = 1
					},
					.extent{
						.width = static_cast<uint32_t>(windowIF.dimensions.x),
						.height = static_cast<uint32_t>(windowIF.dimensions.y),
						.depth = 1
					}
				};

				vkCmdCopyImage(
					commandBuffer,
					fields[(simulationIndex + 1) % 2].image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					deviceIF.logical.swapchainConfiguration.swapchainImages[imageIndex],
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion
				);

				VkImageMemoryBarrier2 transitionAttachmentFromGettingReadToSampling{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

					.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,

					.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
					.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,

					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

					.image = fields[(simulationIndex + 1) % 2].image,

					.subresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					}
				};

				VkDependencyInfo transitionAttachmentFromGettingReadToSamplingDependencyInfo{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = 1,
					.pImageMemoryBarriers = &transitionAttachmentFromGettingReadToSampling
				};

				vkCmdPipelineBarrier2(commandBuffer, &transitionAttachmentFromGettingReadToSamplingDependencyInfo);

				VkImageMemoryBarrier2 barrierPresent{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
					.dstAccessMask = VK_ACCESS_2_NONE,
					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.image = deviceIF.logical.swapchainConfiguration.swapchainImages[imageIndex],
					.subresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					}
				};

				VkDependencyInfo barrierPresentDependencyInfo12{
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = 1,
					.pImageMemoryBarriers = &barrierPresent
				};
				vkCmdPipelineBarrier2(commandBuffer, &barrierPresentDependencyInfo12);

				vkEndCommandBuffer(commandBuffer);

				VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				VkSubmitInfo submitInfo{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &deviceIF.logical.swapchainConfiguration.presentSemaphore[frameIndex],
					.pWaitDstStageMask = &waitStages,
					.commandBufferCount = 1,
					.pCommandBuffers = &commandBuffer,
					.signalSemaphoreCount = 1,
					.pSignalSemaphores = &deviceIF.logical.swapchainConfiguration.renderSemaphore[imageIndex],
				};
				validateResult(vkQueueSubmit(deviceIF.queue.queue, 1, &submitInfo, deviceIF.logical.swapchainConfiguration.fences[frameIndex]), "Failed to Submit Queue");

				frameIndex = (frameIndex + 1) % deviceIF.logical.swapchainConfiguration.maxFramesInFlight;
				simulationIndex = (simulationIndex + 1) % 2;

				VkPresentInfoKHR presentInfo{
					.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &deviceIF.logical.swapchainConfiguration.renderSemaphore[imageIndex],
					.swapchainCount = 1,
					.pSwapchains = &deviceIF.logical.swapchainConfiguration.swapchain,
					.pImageIndices = &imageIndex
				};
				validateResult(vkQueuePresentKHR(deviceIF.queue.queue, &presentInfo), "Failed To Present Queue");

				if (deviceIF.logical.swapchainConfiguration.updateSwapchain) {
					recreateSwapchain();
				}

				//vkQueueWaitIdle( deviceIF.queue.queue );
				//vkDeviceWaitIdle(deviceIF.logical.device);

				SDL_Delay(1000);
			}
		}

		void setup() {
			setupLibraries();
			setupInstance();
			pickPhysicalDeviceAndQueueIndex();
			setupLogicalDevice();
			setupVMA();
			setupWindowAndSurface();
			setupSwapchain();
			setupSync2();
			setupCommandBuffers();
			loadAndCompileShaders();
			generateFields();
			setupPipeline();
			animate();
		}

	private:
		struct ExtensionsIF {
			struct Instance {
				uint32_t count;
				const char* const* extensions;
			} instance;

			struct Device {
				const std::vector< const char* > extensions{
					VK_KHR_SWAPCHAIN_EXTENSION_NAME
				};
			} device;
		} extensionsIF;

		struct WindowIF {
			VkInstance vkInstance;

			SDL_Window* window;
			std::string windowName = "Ryutai";
			int windowWidth = 100;
			int windowHeight = 100;
			SDL_WindowFlags windowFlags{
				SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN
			};

			glm::ivec2 dimensions{};

			VkSurfaceKHR surface;
			VkSurfaceCapabilitiesKHR surfaceCapabilities;

			VkClearColorValue clearColor{ 0.0, 0.0, 0.0 };
		} windowIF;

		struct DeviceIF {
			struct physical {
				uint32_t count;
				std::vector< VkPhysicalDevice > devices;
				std::vector< VkPhysicalDeviceProperties2 > properties;
				uint32_t index;
				VkPhysicalDevice device;
			} physical;

			struct Queue {
				VkQueue queue;
				std::vector < std::vector< VkQueueFamilyProperties2 > > properties;
				uint32_t index;
			} queue;

			struct Logical {
				VkDevice device;
				VkCommandPool commandPool;

				struct SwapchainConfiguration {
					VkSwapchainCreateInfoKHR swapchainCreateInfo;
					VkSwapchainKHR swapchain;
					std::vector<VkImage> swapchainImages;
					std::vector<VkImageView> swapchainImageViews;
					VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
					VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;

					bool updateSwapchain{ false };
					uint32_t imageCount;

					static constexpr uint32_t maxFramesInFlight{ 2 };
					std::array< VkFence, maxFramesInFlight > fences;
					std::array< VkSemaphore, maxFramesInFlight > presentSemaphore;
					std::vector< VkSemaphore > renderSemaphore;
					std::array< VkCommandBuffer, maxFramesInFlight > commandBuffers{};
				} swapchainConfiguration;
			} logical;
		} deviceIF;

		VmaAllocator vmaAllocator;

		Slang::ComPtr< slang::IGlobalSession > slangGlobalSession;
		VkShaderModule shaderModule{};

		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector< VkDescriptorSet > descriptorSets;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		int frameIndex = 0;
		uint32_t imageIndex = 0;
		int simulationIndex = 0;

		std::vector<FieldState> fields;
};

class Ryutai {
	public:
		void start() {
			renderer.setup();
		}

	private:
		Renderer renderer;
};

int main() {
	Ryutai fluidSimulation{};
	fluidSimulation.start();

	return 0;
}