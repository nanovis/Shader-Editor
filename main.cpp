#include "webgpu.h"
#include <fstream>
#include <string.h>
#include<iostream>
#include<vector>
#include<string>
#include "SDL_image.h"
#include "SDL.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <emscripten/html5.h>
#include<emscripten/emscripten.h>
#include"Camera.h"
#include"CameraController.h"
#include "window.h"
#include<ctime>

WGPUDevice device;
WGPUQueue queue;
WGPUSwapChain swapchain;
unsigned char* img=new unsigned char[256*256*4];
int imgw=0,imgh=0;
WGPURenderPipeline pipeline;
WGPUBuffer vertBuf; // vertex buffer with triangle position and colours
WGPUBuffer indxBuf; // index buffer
WGPUBuffer timeBuf; // uniform buffer (containing the rotation angle)
WGPUBuffer resolutionBuf;  //uniform buffer
WGPUBindGroup bindGroup;

WGPUTexture tex; // Texture
WGPUSampler samplerTex;
WGPUTextureView texView;
WGPUExtent3D texSize = {};
WGPUTextureDescriptor texDesc = {};
WGPUTextureDataLayout texDataLayout = {};
WGPUImageCopyTexture texCopy = {};


/**
 * Current rotation angle (in degrees, updated per frame).
 */
clock_t startTime,endTime;
float runtime = 0.0f;
glm::vec2 resolution=glm::vec2(800.0f,600.0f);

static char const triangle_vert_wgsl[] = R"(
	struct VertexIn {
		[[location(0)]] aPos : vec2<f32>;
	};
	struct VertexOut {
		[[builtin(position)]] Position : vec4<f32>;
	};
	[[stage(vertex)]]
	fn main(input : VertexIn) -> VertexOut {
		var output : VertexOut;
		output.Position = vec4<f32>(input.aPos,0.0, 1.0);
		return output;
	}
)";

static char const triangle_frag_wgsl[] = R"(
[[group(0),binding(0)]] var<uniform> Time : f32;
[[group(0),binding(1)]] var<uniform> Resolution : vec2<f32>;
[[stage(fragment)]]
fn main([[builtin(position)]] position: vec4<f32>) -> [[location(0)]] vec4<f32> {
  var uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);
  var col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));
  return vec4<f32>(col, 1.0);
}
)"; 

static WGPUShaderModule createShader(const char* const code, const char* label = nullptr) {
	WGPUShaderModuleWGSLDescriptor wgsl = {};
	wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	wgsl.source = code;
	WGPUShaderModuleDescriptor desc = {};
	desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl);
	desc.label = label;
	return wgpuDeviceCreateShaderModule(device, &desc);
}

/**=
 * Helper to create a buffer.
 *
 * \param[in] data pointer to the start of the raw data
 * \param[in] size number of bytes in \a data
 * \param[in] usage type of buffer
 */
static WGPUBuffer createBuffer(const void* data, size_t size, WGPUBufferUsage usage) {
	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | usage;
	desc.size  = size;
	WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);
	wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
	return buffer;
}



static WGPUTexture createTexture(unsigned char* data, unsigned int w, unsigned int h) {


	texSize.depthOrArrayLayers = 1;
	texSize.height = h;
	texSize.width = w;

	texDesc.sampleCount = 1;
	texDesc.mipLevelCount = 1;
	texDesc.dimension = WGPUTextureDimension_2D;
	texDesc.size = texSize;
	texDesc.usage =   WGPUTextureUsage_CopyDst | 0x00000004; //WGPUTextureUsage_Sampled
	texDesc.format = WGPUTextureFormat_RGBA8Unorm;


	texDataLayout.offset = 0;
	texDataLayout.bytesPerRow = 4 * w;
	texDataLayout.rowsPerImage = h;
	texCopy.texture = wgpuDeviceCreateTexture(device, &texDesc);
	wgpuQueueWriteTexture(queue, &texCopy, data, w * h*4, &texDataLayout, &texSize);
	return texCopy.texture;
}



/**
 * Draws using the above pipeline and buffers.
 */
static void createPipelineAndBuffers() {

	WGPUShaderModule vertMod = createShader(triangle_vert_wgsl);
	WGPUShaderModule fragMod = createShader(triangle_frag_wgsl);
	
	WGPUBufferBindingLayout buf = {};
	buf.type = WGPUBufferBindingType_Uniform;

	// bind group layout (used by both the pipeline layout and uniform bind group, released at the end of this function)
	WGPUBindGroupLayoutEntry timelEntry = {};
	timelEntry.binding = 0;
	timelEntry.visibility = WGPUShaderStage_Fragment;
	timelEntry.buffer = buf;

	WGPUBindGroupLayoutEntry resolutionlEntry = {};
	resolutionlEntry.binding = 1;
	resolutionlEntry.visibility = WGPUShaderStage_Fragment;
	resolutionlEntry.buffer = buf;

	tex = createTexture(img, imgw, imgh);
	WGPUTextureViewDescriptor texViewDesc = {};
	texViewDesc.dimension = WGPUTextureViewDimension_2D;
	texViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
	texViewDesc.mipLevelCount=1;
	texViewDesc.arrayLayerCount=1;
	texView = wgpuTextureCreateView(tex, &texViewDesc);

	WGPUSamplerDescriptor samplerDesc = {};
	samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
	samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
	samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
	samplerDesc.magFilter = WGPUFilterMode_Linear;
	samplerDesc.minFilter = WGPUFilterMode_Nearest;
	samplerDesc.mipmapFilter = WGPUFilterMode_Nearest;
	samplerDesc.lodMaxClamp = 32;
	samplerDesc.lodMinClamp = 0;
	samplerDesc.compare = WGPUCompareFunction_Undefined;
	samplerDesc.maxAnisotropy = 1;
	
	samplerTex = wgpuDeviceCreateSampler(device, &samplerDesc);

	WGPUSamplerBindingLayout samplerLayout = {};
	samplerLayout.type = WGPUSamplerBindingType_Filtering;

	WGPUTextureBindingLayout texLayout = {};
	texLayout.sampleType = WGPUTextureSampleType_Float;
	texLayout.viewDimension = WGPUTextureViewDimension_2D;//here
	texLayout.multisampled = false;

	WGPUBindGroupLayoutEntry bglTexEntry = {};
	bglTexEntry.binding = 2;
	bglTexEntry.visibility = WGPUShaderStage_Fragment;
	bglTexEntry.texture = texLayout;

	WGPUBindGroupLayoutEntry bglSamplerEntry = {};
	bglSamplerEntry.binding = 3;
	bglSamplerEntry.visibility = WGPUShaderStage_Fragment;
	bglSamplerEntry.sampler = samplerLayout;

	WGPUBindGroupLayoutEntry* allBgLayoutEntries = new WGPUBindGroupLayoutEntry[4];
	allBgLayoutEntries[0] = timelEntry;
	allBgLayoutEntries[1] = resolutionlEntry;
	allBgLayoutEntries[2] = bglTexEntry;
	allBgLayoutEntries[3] = bglSamplerEntry;

	WGPUBindGroupLayoutDescriptor bglDesc = {};
	bglDesc.entryCount = 4;  
	bglDesc.entries = allBgLayoutEntries;
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
	
	WGPUBindGroupLayout* bindGroupLayouts= new WGPUBindGroupLayout[0];
	bindGroupLayouts[0]=bindGroupLayout;


	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = bindGroupLayouts;
	WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

	// describe buffer layouts
	WGPUVertexAttribute vertAttrs[1] = {};
	vertAttrs[0].format = WGPUVertexFormat_Float32x2;
	vertAttrs[0].offset = 0;
	vertAttrs[0].shaderLocation = 0;
	WGPUVertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.arrayStride = 2 * sizeof(float);
	vertexBufferLayout.attributeCount = 1;
	vertexBufferLayout.stepMode =WGPUVertexStepMode_Vertex;
	vertexBufferLayout.attributes = vertAttrs;


	WGPUVertexBufferLayout* vertexBufferLayouts=new WGPUVertexBufferLayout[1];
	vertexBufferLayouts[0]=vertexBufferLayout;


	// Fragment state
	WGPUBlendState blend = {};
	blend.color.operation = WGPUBlendOperation_Add;
	blend.color.srcFactor = WGPUBlendFactor_One;
	blend.color.dstFactor = WGPUBlendFactor_One;
	blend.alpha.operation = WGPUBlendOperation_Add;
	blend.alpha.srcFactor = WGPUBlendFactor_One;
	blend.alpha.dstFactor = WGPUBlendFactor_One;

	WGPUColorTargetState colorTarget = {};
	colorTarget.format = webgpu::getSwapChainFormat(device);
	colorTarget.blend = &blend;
	colorTarget.writeMask = WGPUColorWriteMask_All;

	WGPUFragmentState fragment = {};
	fragment.module = fragMod;
	fragment.entryPoint = "main";
	fragment.targetCount = 1;
	fragment.targets = &colorTarget;

	WGPURenderPipelineDescriptor desc = {};
	desc.fragment = &fragment;

	// Other state
	desc.layout = pipelineLayout;
	desc.depthStencil = nullptr;

	desc.vertex.module = vertMod;
	desc.vertex.entryPoint = "main";
	desc.vertex.bufferCount = 1;
	desc.vertex.buffers = vertexBufferLayouts;

	desc.multisample.count = 1;
	desc.multisample.mask = 0xFFFFFFFF;
	desc.multisample.alphaToCoverageEnabled = false;

	desc.primitive.frontFace = WGPUFrontFace_CCW;
	desc.primitive.cullMode = WGPUCullMode_None;
	desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

	pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);

	// partial clean-up (just move to the end, no?)
	wgpuPipelineLayoutRelease(pipelineLayout);

	wgpuShaderModuleRelease(fragMod);
	wgpuShaderModuleRelease(vertMod);

	// create the buffers (x, y,z, texture_x,texture_y)
	float const vertData[] = {
		-1.0f, 1.0f, //A
		1.0f, 1.0f,   // B
		-1.0f, -1.0f,  // C
		1.0f, -1.0f //D

	};
	uint16_t const indxData[] = {
		0, 2, 1,
    	1, 2, 3
	};
	vertBuf = createBuffer(vertData, sizeof(vertData), WGPUBufferUsage_Vertex);
	indxBuf = createBuffer(indxData, sizeof(indxData), WGPUBufferUsage_Index);

	timeBuf = createBuffer(&runtime, sizeof(runtime), WGPUBufferUsage_Uniform);
	resolutionBuf = createBuffer(&resolution, sizeof(resolution), WGPUBufferUsage_Uniform);
	WGPUBindGroupEntry timeEntry = {};
	timeEntry.binding = 0;
	timeEntry.buffer = timeBuf;
	timeEntry.offset = 0;
	timeEntry.size = sizeof(runtime);
	
	WGPUBindGroupEntry resolutionEntry = {};
	resolutionEntry.binding = 1;
	resolutionEntry.buffer = resolutionBuf;
	//bgEntry.offset = 0;
	resolutionEntry.size = sizeof(resolution);

	WGPUBindGroupEntry bgTexEntry = {};
	bgTexEntry.binding = 2;
	bgTexEntry.textureView = texView;

	WGPUBindGroupEntry bgSamplerEntry = {};
	bgSamplerEntry.binding = 3;
	bgSamplerEntry.sampler = samplerTex;

	WGPUBindGroupEntry* allBgEntries = new WGPUBindGroupEntry[4];
	allBgEntries[0] = timeEntry;
	allBgEntries[1] = resolutionEntry;
	allBgEntries[2] = bgTexEntry;
	allBgEntries[3] = bgSamplerEntry;

	WGPUBindGroupDescriptor bgDesc = {};
	bgDesc.layout = bindGroupLayout;
	bgDesc.entryCount = 4;   
	bgDesc.entries = allBgEntries;

	bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

	// last bit of clean-up
	wgpuBindGroupLayoutRelease(bindGroupLayout);
}
static bool redraw() {
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(swapchain);			// create textureView

	WGPURenderPassColorAttachment colorDesc = {};
	colorDesc.view    = backBufView;
	colorDesc.loadOp  = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;
	colorDesc.clearColor.r = 0.0f;
	colorDesc.clearColor.g = 0.0f;
	colorDesc.clearColor.b = 0.0f;
	colorDesc.clearColor.a = 0.0f;

	WGPURenderPassDescriptor renderPass = {};
	renderPass.colorAttachmentCount = 1;
	renderPass.colorAttachments = &colorDesc;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);			// create encoder
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass);	// create pass

	// update the time 
	endTime = clock();
	runtime = (float)(endTime - startTime) / CLOCKS_PER_SEC;
	
	wgpuQueueWriteBuffer(queue, timeBuf,0, &runtime, sizeof(runtime));
	wgpuQueueWriteBuffer(queue, resolutionBuf,0, &resolution, sizeof(resolution));
	wgpuQueueWriteTexture(queue, &texCopy, img, imgh * imgw * 4, &texDataLayout, &texSize);
	// draw the triangle (comment these five lines to simply clear the screen)
	wgpuRenderPassEncoderSetPipeline(pass, pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, 0);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertBuf, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderSetIndexBuffer(pass, indxBuf, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderDrawIndexed(pass, 6, 1, 0, 0, 0);

	wgpuRenderPassEncoderEndPass(pass);
	wgpuRenderPassEncoderRelease(pass);														// release pass
	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);				// create commands
	wgpuCommandEncoderRelease(encoder);														// release encoder

	wgpuQueueSubmit(queue, 1, &commands);
	wgpuCommandBufferRelease(commands);
#ifndef __EMSCRIPTEN__
	wgpuSwapChainPresent(swapchain);
#endif
	wgpuTextureViewRelease(backBufView);													// release textureView

	return true;
}
void image_init()
{
		SDL_Surface *image;
		int flags=IMG_INIT_JPG|IMG_INIT_PNG;
		int initted=IMG_Init(flags);
		image=IMG_Load("happytree.jpg");
		imgw=image->w;
		imgh=image->h;
		img=new unsigned char[imgw * imgh*4];
		SDL_LockSurface(image);
		Uint8 r,g,b;
		Uint32* pixel = (Uint32*)image->pixels;
		for (int i=0;i<imgh;i++)
		{
			for (int j=0;j<imgw;j++)
			{
				SDL_GetRGB(pixel[i*imgw+j],image->format,&r,&g,&b);
				int idx = (i*imgh+j) * 4;
				img[idx] = int(r);
				img[idx + 1] = int(g);
				img[idx + 2] = int(b);
				img[idx + 3] = 255;
			}
		}
		SDL_UnlockSurface(image);
		IMG_Quit();
}


extern "C" int __main__(int /*argc*/, char* /*argv*/[]) {
	image_init();
	if (window::Handle wHnd = window::create()) {
		if ((device = webgpu::create(wHnd))) {
			queue = wgpuDeviceGetQueue(device);
			swapchain = webgpu::createSwapChain(device);
			createPipelineAndBuffers();
			window::show(wHnd);
			startTime = clock();
			window::loop(wHnd, redraw);
		#ifndef __EMSCRIPTEN__
			wgpuBindGroupRelease(bindGroup);
			wgpuBufferRelease(resolutionBuf);
			wgpuBufferRelease(timeBuf);
			wgpuBufferRelease(indxBuf);
			wgpuBufferRelease(vertBuf);
			wgpuRenderPipelineRelease(pipeline);
			wgpuSwapChainRelease(swapchain);
			wgpuQueueRelease(queue);
			wgpuDeviceRelease(device);
		#endif
		}
	#ifndef __EMSCRIPTEN__
		window::destroy(wHnd);
	#endif
	}
	return 0;
}
