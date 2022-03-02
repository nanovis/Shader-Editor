#include "webgpu.h"
#include <fstream>
#include <string.h>
#include<iostream>
#include<vector>
#include<string>
#include "SDL_image.h"
#include "SDL.h"
#include <glm/glm.hpp>
#include <emscripten/html5.h>
#include<emscripten/emscripten.h>
#include"Camera.h"
#include"CameraController.h"
#include "window.h"

WGPUDevice device;
WGPUQueue queue;
WGPUSwapChain swapchain;
unsigned char* img=new unsigned char[256*256*4];
int imgw=0,imgh=0;
CameraController cameraController=CameraController(0.2);
Camera camera=Camera();
glm::mat4 model_matrix=glm::mat4(1.0f);
glm::mat4 transform_matrix=glm::mat4(1.0f);
WGPURenderPipeline pipeline;
WGPUBuffer vertBuf; // vertex buffer with triangle position and colours
WGPUBuffer indxBuf; // index buffer
WGPUBuffer uRotBuf; // uniform buffer (containing the rotation angle)
WGPUBuffer matrixBuf;  //uniform buffer
WGPUBindGroup bindGroup;
WGPUBindGroup bindGroup_camera; //bindgroup for view transformation
bool keypress_lock=false;
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
float rotDeg = 0.0f;

static char const triangle_vert_wgsl[] = R"(
	struct CameraUniform {
    view_proj: mat4x4<f32>;
	};
	[[group(1), binding(0)]] var<uniform> camera: CameraUniform;
	struct VertexIn {
		[[location(0)]] aPos : vec3<f32>;
		[[location(1)]] aCol : vec2<f32>;
	};
	struct VertexOut {
		[[location(0)]] vCol : vec2<f32>;
		[[builtin(position)]] Position : vec4<f32>;
	};
	struct Rotation {
		[[location(0)]] degs : f32;
	};
	[[group(0),binding(0)]] var<uniform> uRot : Rotation;
	[[stage(vertex)]]
	fn main(input : VertexIn) -> VertexOut {
		var rads : f32 = radians(uRot.degs);
		var cosA : f32 = cos(rads);
		var sinA : f32 = sin(rads);
		var rot : mat4x4<f32> = mat4x4<f32>(
			vec4<f32>( cosA, sinA, 0.0,0.0),
			vec4<f32>(-sinA, cosA, 0.0,0.0),
			vec4<f32>( 0.0,  0.0,  1.0,0.0),
			vec4<f32>( 0.0,  0.0,  0.0,1.0));
		var output : VertexOut;
		output.Position = camera.view_proj * rot*vec4<f32>(input.aPos, 1.0);
		output.vCol = input.aCol;
		return output;
	}
)";

static char const triangle_frag_wgsl[] = R"(
	[[group(0), binding(1)]]
	var t_diffuse: texture_2d<f32>;
	[[group(0), binding(2)]]
	var s_diffuse: sampler;
	[[stage(fragment)]]
	fn main([[location(0)]] tex : vec2<f32>) -> [[location(0)]] vec4<f32> {
		return textureSample(t_diffuse, s_diffuse, tex);
	}
)"; 


/**
 * Helper to create a shader from SPIR-V IR.
 *
 * \param[in] code shader source (output using the \c -V \c -x options in \c glslangValidator)
 * \param[in] size size of \a code in bytes
 * \param[in] label optional shader name
 */
EM_JS(void, jsprint, (const char* str), {
  console.log(UTF8ToString(str));
  //alert('hai');
});
EM_JS(void, jsprintfloat, (float a), {
  console.log(a);
  //alert('hai');
});
EM_JS(void, jsprintmatrix, (float a,float b,float c,float d), {
console.log([a,b,c,d]);


});
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
  /*printf("%s, key: \"%s\", code: \"%s\", location: %lu,%s%s%s%s repeat: %d, locale: \"%s\", char: \"%s\", charCode: %lu, keyCode: %lu, which: %lu, timestamp: %lf\n",
    emscripten_event_type_to_string(eventType), e->key, e->code, e->location, 
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
    e->repeat, e->locale, e->charValue, e->charCode, e->keyCode, e->which,
    e->timestamp);
  */
if (keypress_lock==false)
  {
	  if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (!strcmp(e->key, "w") )) {
	  cameraController.process_events(1);
	  cameraController.update_camera(camera);
	  camera.updatetransformMatrix(model_matrix);
	  keypress_lock=true;
  	}
    if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (!strcmp(e->key, "a") )) {
	  cameraController.process_events(3);
	  cameraController.update_camera(camera);
	  camera.updatetransformMatrix(model_matrix);
	  keypress_lock=true;
 	 }
    if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (!strcmp(e->key, "s") )) {
	  cameraController.process_events(2);
	  cameraController.update_camera(camera);
	  camera.updatetransformMatrix(model_matrix);
	  keypress_lock=true;

 	 }
    if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (!strcmp(e->key, "d") )) {
	  cameraController.process_events(4);
	  cameraController.update_camera(camera);
	  camera.updatetransformMatrix(model_matrix);
	  keypress_lock=true;

 	 }
  }

  return 0;

  }
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
	// compile shaders
	// NOTE: these are now the WGSL shaders (tested with Dawn and Chrome Canary)
	WGPUShaderModule vertMod = createShader(triangle_vert_wgsl);
	WGPUShaderModule fragMod = createShader(triangle_frag_wgsl);
	
	WGPUBufferBindingLayout buf = {};
	buf.type = WGPUBufferBindingType_Uniform;

	// bind group layout (used by both the pipeline layout and uniform bind group, released at the end of this function)
	WGPUBindGroupLayoutEntry bglEntry = {};
	bglEntry.binding = 0;
	bglEntry.visibility = WGPUShaderStage_Vertex;
	bglEntry.buffer = buf;

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
	bglTexEntry.binding = 1;
	bglTexEntry.visibility = WGPUShaderStage_Fragment;
	bglTexEntry.texture = texLayout;

	WGPUBindGroupLayoutEntry bglSamplerEntry = {};
	bglSamplerEntry.binding = 2;
	bglSamplerEntry.visibility = WGPUShaderStage_Fragment;
	bglSamplerEntry.sampler = samplerLayout;

	WGPUBindGroupLayoutEntry* allBgLayoutEntries = new WGPUBindGroupLayoutEntry[3];
	allBgLayoutEntries[0] = bglEntry;
	allBgLayoutEntries[1] = bglTexEntry;
	allBgLayoutEntries[2] = bglSamplerEntry;

	WGPUBindGroupLayoutDescriptor bglDesc = {};
	bglDesc.entryCount = 3;  
	bglDesc.entries = allBgLayoutEntries;
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
	
	
	//create camera bindgroup
	WGPUBindGroupLayoutEntry bglcameraEntry = {};
	bglcameraEntry.binding = 0;
	bglcameraEntry.visibility = WGPUShaderStage_Vertex;
	bglcameraEntry.buffer = buf;

	WGPUBindGroupLayoutDescriptor bglcameraDesc = {};
	bglcameraDesc.entryCount = 1;  
	bglcameraDesc.entries = &bglcameraEntry;
	WGPUBindGroupLayout camerabindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglcameraDesc);


	WGPUBindGroupLayout* bindGroupLayouts= new WGPUBindGroupLayout[2];
	bindGroupLayouts[0]=bindGroupLayout;
	bindGroupLayouts[1]=camerabindGroupLayout;

	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 2;
	layoutDesc.bindGroupLayouts = bindGroupLayouts;
	WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

	// describe buffer layouts
	WGPUVertexAttribute vertAttrs[2] = {};
	vertAttrs[0].format = WGPUVertexFormat_Float32x3;
	vertAttrs[0].offset = 0;
	vertAttrs[0].shaderLocation = 0;
	vertAttrs[1].format = WGPUVertexFormat_Float32x2;
	vertAttrs[1].offset = 3 * sizeof(float);
	vertAttrs[1].shaderLocation = 1;
	WGPUVertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.arrayStride = 5 * sizeof(float);
	vertexBufferLayout.attributeCount = 2;
	vertexBufferLayout.attributes = vertAttrs;

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
	desc.vertex.buffers = &vertexBufferLayout;

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
		-0.0868241f, 0.49240386f, 0.0f, 0.4131759f, 0.00759614f,//A
		 -0.49513406f, 0.06958647f, 0.0f, 0.0048659444f, 0.43041354f,  // B
		-0.21918549f, -0.44939706f, 0.0f, 0.28081453f, 0.949397f,  // C
		0.35966998f, -0.3473291f, 0.0f,0.85967f, 0.84732914f,//D
		0.44147372f, 0.2347359f, 0.0f,0.9414737f, 0.2652641f//E
	};
	uint16_t const indxData[] = {
		0, 1, 4,
    	1, 2, 4,
    	2, 3, 4,
		0 // padding (better way of doing this?)
	};
	vertBuf = createBuffer(vertData, sizeof(vertData), WGPUBufferUsage_Vertex);
	indxBuf = createBuffer(indxData, sizeof(indxData), WGPUBufferUsage_Index);

	// create the uniform bind group (note 'rotDeg' is copied here, not bound in any way)
	uRotBuf = createBuffer(&rotDeg, sizeof(rotDeg), WGPUBufferUsage_Uniform);
	matrixBuf = createBuffer(&transform_matrix, sizeof(transform_matrix), WGPUBufferUsage_Uniform);
	WGPUBindGroupEntry bgEntry = {};
	bgEntry.binding = 0;
	bgEntry.buffer = uRotBuf;
	bgEntry.offset = 0;
	bgEntry.size = sizeof(rotDeg);

	WGPUBindGroupEntry bgTexEntry = {};
	bgTexEntry.binding = 1;
	bgTexEntry.textureView = texView;

	WGPUBindGroupEntry bgSamplerEntry = {};
	bgSamplerEntry.binding = 2;
	bgSamplerEntry.sampler = samplerTex;

	WGPUBindGroupEntry* allBgEntries = new WGPUBindGroupEntry[3];
	allBgEntries[0] = bgEntry;
	allBgEntries[1] = bgTexEntry;
	allBgEntries[2] = bgSamplerEntry;

	WGPUBindGroupDescriptor bgDesc = {};
	bgDesc.layout = bindGroupLayout;
	bgDesc.entryCount = 3;   
	bgDesc.entries = allBgEntries;

	bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

	WGPUBindGroupEntry bgcameraEntry = {};
	bgcameraEntry.binding = 0;
	bgcameraEntry.buffer = matrixBuf;
	bgcameraEntry.offset = 0;
	bgcameraEntry.size = sizeof(transform_matrix);

	WGPUBindGroupEntry* allcameraBgEntries = new WGPUBindGroupEntry[1];
	allcameraBgEntries[0] = bgcameraEntry;

	WGPUBindGroupDescriptor bgcameraDesc = {};
	bgcameraDesc.layout = camerabindGroupLayout;
	bgcameraDesc.entryCount = 1;  
	bgcameraDesc.entries = allcameraBgEntries;

	bindGroup_camera = wgpuDeviceCreateBindGroup(device, &bgcameraDesc);
	// last bit of clean-up
	wgpuBindGroupLayoutRelease(bindGroupLayout);
	wgpuBindGroupLayoutRelease(camerabindGroupLayout);
}
static bool redraw() {
	EMSCRIPTEN_RESULT ret = emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
	transform_matrix=camera.transformMatrix;

	keypress_lock=false;
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(swapchain);			// create textureView

	WGPURenderPassColorAttachment colorDesc = {};
	colorDesc.view    = backBufView;
	colorDesc.loadOp  = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;
	colorDesc.clearColor.r = 0.3f;
	colorDesc.clearColor.g = 0.3f;
	colorDesc.clearColor.b = 0.3f;
	colorDesc.clearColor.a = 1.0f;

	WGPURenderPassDescriptor renderPass = {};
	renderPass.colorAttachmentCount = 1;
	renderPass.colorAttachments = &colorDesc;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);			// create encoder
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass);	// create pass

	// update the rotation
	rotDeg += 0.1f;
	
	wgpuQueueWriteBuffer(queue, uRotBuf, 0, &rotDeg, sizeof(rotDeg));
	wgpuQueueWriteBuffer(queue, matrixBuf,0, &transform_matrix, sizeof(transform_matrix));
	wgpuQueueWriteTexture(queue, &texCopy, img, imgh * imgw * 4, &texDataLayout, &texSize);
	// draw the triangle (comment these five lines to simply clear the screen)
	wgpuRenderPassEncoderSetPipeline(pass, pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, 0);
	wgpuRenderPassEncoderSetBindGroup(pass, 1, bindGroup_camera, 0, 0);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertBuf, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderSetIndexBuffer(pass, indxBuf, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderDrawIndexed(pass, 9, 1, 0, 0, 0);

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
			window::loop(wHnd, redraw);
		#ifndef __EMSCRIPTEN__
			wgpuBindGroupRelease(bindGroup);
			wgpuBufferRelease(uRotBuf);
			wgpuBufferRelease(matrixBuf);
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
