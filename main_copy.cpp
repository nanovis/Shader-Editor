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

WGPURenderPipeline pipeline;


WGPUBuffer vertBuf; // vertex buffer with triangle position and colours
WGPUBuffer indxBuf; // index buffer
WGPUBuffer timeBuf; // uniform buffer (containing the rotation angle)
WGPUBuffer resolutionBuf;  //uniform buffer
WGPUBuffer mouseBuf;
WGPUBindGroup bindGroup;
WGPUBindGroup texturebindGroup;  //bindgroup for textures

unsigned char* img_1,img_2,img_3,img_4;
int imgw_1=0,imgh_1=0,imgw_2=0,imgh_2=0,imgw_3=0,imgh_3=0,imgw_4=0,imgh_4=0;
std::string texture1_path="texture/London.jpg";
std::string texture2_path="texture/black.jpg";
std::string texture3_path="texture/black.jpg";
std::string texture4_path="texture/black.jpg";
WGPUTexture tex1,tex2,tex3,tex4; // Texture
WGPUSampler samplerTex;
WGPUTextureView texView1,texView2,texView3,texView4;
WGPUExtent3D texSize1 = {},texSize2 = {},texSize3 = {},texSize4 = {};
WGPUTextureDescriptor texDesc1 = {},texDesc2 = {},texDesc3 = {},texDesc4 = {};
WGPUTextureDataLayout texDataLayout1 = {},texDataLayout2 = {},texDataLayout3 = {},texDataLayout4 = {};
WGPUImageCopyTexture texCopy1 = {},texCopy2 = {},texCopy3 = {},texCopy4 = {};
glm::vec4 mouselocation=glm::vec4(2.0f,3.0f,0.0f,0.0f);


/**
 * Current rotation angle (in degrees, updated per frame).
 */
clock_t startTime,endTime;
float runtime = 0.0f;
glm::vec2 resolution=glm::vec2(800.0f,600.0f);

static char const triangle_vert_wgsl[] = R"(
	struct VertexIn {
		@location(0) aPos : vec2<f32>;
	};
	struct VertexOut {
		@builtin(position) Position : vec4<f32>;
	};
	@stage(vertex)
	fn main(input : VertexIn) -> VertexOut {
		var output : VertexOut;
		output.Position = vec4<f32>(input.aPos,0.0, 1.0);
		return output;
	}
)";

static char const triangle_frag_wgsl[] = R"(@group(0) @binding(0) var<uniform> Time : f32;
@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;
@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;
@group(1) @binding(0) var t_diffuse: texture_2d<f32>;
@group(1) @binding(1) var s_diffuse: sampler;

@stage(fragment)
fn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
  var uv:vec2<f32>=vec2<f32>(position.xy/Resolution);
  var n:f32=100.0;
  var d:f32=n*abs(sin(Time*0.1));
  d=d+(Resolution.x-d)*step(n-(30.0),d);
  return textureSample(t_diffuse, s_diffuse, floor(uv*d)/d);

})"; // fragment shader end

/*
[[group(0),binding(0)]] var<uniform> Time : f32;
[[group(0),binding(1)]] var<uniform> Resolution : vec2<f32>;
[[group(0),binding(2)]] var<uniform> Mouse : vec4<f32>;
[[stage(fragment)]]
fn main([[builtin(position)]] position: vec4<f32>) -> [[location(0)]] vec4<f32> {
  var uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);
  var col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));
  return vec4<f32>(col, 1.0);
}
*/
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



static WGPUTexture createTexture(unsigned char* data, unsigned int w, unsigned int h,WGPUExtent3D &texSize, WGPUTextureDescriptor &texDesc, WGPUTextureDataLayout &texDataLayout ,WGPUImageCopyTexture &texCopy) {


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

	WGPUBindGroupLayoutEntry mouselEntry = {};
	mouselEntry.binding = 2;
	mouselEntry.visibility = WGPUShaderStage_Fragment;
	mouselEntry.buffer = buf;


	WGPUBindGroupLayoutEntry* allBgLayoutEntries = new WGPUBindGroupLayoutEntry[5];
	allBgLayoutEntries[0] = timelEntry;
	allBgLayoutEntries[1] = resolutionlEntry;
	allBgLayoutEntries[2] = mouselEntry;

	WGPUBindGroupLayoutDescriptor bglDesc = {};
	bglDesc.entryCount = 3;  
	bglDesc.entries = allBgLayoutEntries;
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

	tex1 = createTexture(img_1, imgw_1, imgh_1,texSize1,texDesc1,texDataLayout1,texCopy1);
	WGPUTextureViewDescriptor texViewDesc = {};
	texViewDesc.dimension = WGPUTextureViewDimension_2D;
	texViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
	texViewDesc.mipLevelCount=1;
	texViewDesc.arrayLayerCount=1;
	texView1 = wgpuTextureCreateView(tex1, &texViewDesc);

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
	bglTexEntry.binding = 0;
	bglTexEntry.visibility = WGPUShaderStage_Fragment;
	bglTexEntry.texture = texLayout;

	WGPUBindGroupLayoutEntry bglSamplerEntry = {};
	bglSamplerEntry.binding = 1;
	bglSamplerEntry.visibility = WGPUShaderStage_Fragment;
	bglSamplerEntry.sampler = samplerLayout;


	WGPUBindGroupLayoutEntry* textureBgLayoutEntries = new WGPUBindGroupLayoutEntry[2];
	textureBgLayoutEntries[0] = bglTexEntry;
	textureBgLayoutEntries[1] = bglSamplerEntry;

	WGPUBindGroupLayoutDescriptor texturebglDesc = {};
	texturebglDesc.entryCount = 2;  
	texturebglDesc.entries = textureBgLayoutEntries;
	WGPUBindGroupLayout texturebindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &texturebglDesc);
	
	WGPUBindGroupLayout* bindGroupLayouts= new WGPUBindGroupLayout[2];
	bindGroupLayouts[0]=bindGroupLayout;
	bindGroupLayouts[1]=texturebindGroupLayout;


	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 2;
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
	mouseBuf = createBuffer(&mouselocation, sizeof(mouselocation), WGPUBufferUsage_Uniform);
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

	WGPUBindGroupEntry mouseEntry = {};
	mouseEntry.binding = 2;
	mouseEntry.buffer = mouseBuf;
	mouseEntry.size = sizeof(mouselocation);



	WGPUBindGroupEntry* uniformBgEntries = new WGPUBindGroupEntry[3];
	uniformBgEntries[0] = timeEntry;
	uniformBgEntries[1] = resolutionEntry;
	uniformBgEntries[2] = mouseEntry;

	WGPUBindGroupDescriptor uniformbgDesc = {};
	uniformbgDesc.layout = bindGroupLayout;
	uniformbgDesc.entryCount = 3;   
	uniformbgDesc.entries = uniformBgEntries;

	bindGroup = wgpuDeviceCreateBindGroup(device, &uniformbgDesc);

	WGPUBindGroupEntry bgTexEntry = {};
	bgTexEntry.binding = 0;
	bgTexEntry.textureView = texView1;

	WGPUBindGroupEntry bgSamplerEntry = {};
	bgSamplerEntry.binding = 1;
	bgSamplerEntry.sampler = samplerTex;

	WGPUBindGroupEntry* textureBgEntries = new WGPUBindGroupEntry[2];
	textureBgEntries[0] = bgTexEntry;
	textureBgEntries[1] = bgSamplerEntry;

	WGPUBindGroupDescriptor texturebgDesc = {};
	texturebgDesc.layout = texturebindGroupLayout;//
	texturebgDesc.entryCount = 2;   
	texturebgDesc.entries = textureBgEntries;

	texturebindGroup = wgpuDeviceCreateBindGroup(device, &texturebgDesc);


	// last bit of clean-up
	wgpuBindGroupLayoutRelease(bindGroupLayout);
	wgpuBindGroupLayoutRelease(texturebindGroupLayout);
}
EM_JS(void, jsprint, ( float x,float y), {
  console.log(x,y);
});
EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
  /*printf("%s, screen: (%ld,%ld), client: (%ld,%ld),%s%s%s%s button: %hu, buttons: %hu, movement: (%ld,%ld), canvas: (%ld,%ld), timestamp: %lf\n",
    emscripten_event_type_to_string(eventType), e->screenX, e->screenY, e->clientX, e->clientY,
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
    e->button, e->buttons, e->movementX, e->movementY, e->canvasX, e->canvasY,
    e->timestamp);*/
	mouselocation[0]=e->clientX;
	mouselocation[1]=e->clientY;
  return 0;
}
EM_BOOL mouse_click_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
  /*printf("%s, screen: (%ld,%ld), client: (%ld,%ld),%s%s%s%s button: %hu, buttons: %hu, movement: (%ld,%ld), canvas: (%ld,%ld), timestamp: %lf\n",
    emscripten_event_type_to_string(eventType), e->screenX, e->screenY, e->clientX, e->clientY,
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
    e->button, e->buttons, e->movementX, e->movementY, e->canvasX, e->canvasY,
    e->timestamp);*/
	mouselocation[2]=e->clientX;
	mouselocation[3]=e->clientY;
  return 0;
}
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
  /*printf("%s, key: \"%s\", code: \"%s\", location: %lu,%s%s%s%s repeat: %d, locale: \"%s\", char: \"%s\", charCode: %lu, keyCode: %lu, which: %lu, timestamp: %lf\n",
    emscripten_event_type_to_string(eventType), e->key, e->code, e->location, 
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
    e->repeat, e->locale, e->charValue, e->charCode, e->keyCode, e->which,
    e->timestamp);*/
	keypress=0;
  if (eventType == EMSCRIPTEN_EVENT_KEYPRESS) {
    keypress=e->which;
  }

  return 0;
}
static bool redraw() {
	EMSCRIPTEN_RESULT ret = emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, mouse_callback);
	ret = emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, mouse_click_callback);
	ret = emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
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
	runtime = (float)(endTime - startTime) / (CLOCKS_PER_SEC);
	
	wgpuQueueWriteBuffer(queue, timeBuf,0, &runtime, sizeof(runtime));
	wgpuQueueWriteBuffer(queue, resolutionBuf,0, &resolution, sizeof(resolution));
	wgpuQueueWriteBuffer(queue, mouseBuf,0, &mouselocation, sizeof(mouselocation));
	
	wgpuQueueWriteTexture(queue, &texCopy1, img_1, imgh_1 * imgw_1 * 4, &texDataLayout1, &texSize1);
	// draw the triangle (comment these five lines to simply clear the screen)
	wgpuRenderPassEncoderSetPipeline(pass, pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, 0);
	wgpuRenderPassEncoderSetBindGroup(pass, 1, texturebindGroup, 0, 0);
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
void image_init(std::string image_path,unsigned char*& img, int &imgw, int &imgh)
{
		SDL_Surface *image;
		int flags=IMG_INIT_JPG|IMG_INIT_PNG;
		int initted=IMG_Init(flags);
		image=IMG_Load((char*)image_path.data());
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
	image_init(texture1_path,img_1,imgw_1,imgh_1);
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
			wgpuBindGroupRelease(texturebindGroup);
			wgpuBufferRelease(resolutionBuf);
			wgpuBufferRelease(timeBuf);
			wgpuBufferRelease(mouseBuf);
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
