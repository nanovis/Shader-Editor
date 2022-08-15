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
#include<math.h>
#include<vector>

WGPUDevice device;
WGPUQueue queue;
WGPUSwapChain swapchain;

WGPURenderPipeline pipeline;


WGPUBuffer vertBuf; // vertex buffer with triangle position and colours
WGPUBuffer indxBuf; // index buffer
WGPUBuffer timeBuf; // uniform buffer (containing the rotation angle)
WGPUBuffer resolutionBuf;  //uniform buffer
WGPUBuffer mouseBuf;
WGPUBuffer keypressBuf; 
WGPUBuffer date1Buf;
WGPUBuffer date2Buf;
WGPUBuffer positionBuf;
WGPUBuffer positionDinoBuf;
WGPUBuffer randomBuf;
WGPUBuffer randomArrayBuf;
WGPUBindGroup bindGroup;
WGPUBindGroup texturebindGroup;  //bindgroup for textures


unsigned char* img_1;
unsigned char* img_2;
unsigned char* img_3;
unsigned char* img_4;
int imgw_1=0,imgh_1=0,imgw_2=0,imgh_2=0,imgw_3=0,imgh_3=0,imgw_4=0,imgh_4=0;
WGPUTexture tex1,tex2,tex3,tex4; // Texture
WGPUSampler samplerTex;
WGPUTextureView texView1,texView2,texView3,texView4;
WGPUExtent3D texSize1 = {},texSize2 = {},texSize3 = {},texSize4 = {};
WGPUTextureDescriptor texDesc1 = {},texDesc2 = {},texDesc3 = {},texDesc4 = {};
WGPUTextureDataLayout texDataLayout1 = {},texDataLayout2 = {},texDataLayout3 = {},texDataLayout4 = {};
WGPUImageCopyTexture texCopy1 = {},texCopy2 = {},texCopy3 = {},texCopy4 = {};
glm::vec4 mouselocation=glm::vec4(2.0f,3.0f,0.0f,0.0f);
int date1[3];
int date2[3];
glm::vec4 randomArray[25];
float position[2]={0.0f,300.0f};
float position_dino[2]={50.0f,600.0f};
int keypress=100; //ascii
int mouseflag=0;
float random_num=0.0f;
int w_press=0; 

/**
 * Current rotation angle (in degrees, updated per frame).
 */
clock_t startTime,endTime;
bool press=false;
int pressflag=0;
float runtime = 0.0f;
glm::vec2 resolution=glm::vec2(800.0f,600.0f);

static char const triangle_vert_wgsl[] = R"(
	struct VertexIn {
		@location(0) aPos : vec2<f32>,
	}
	struct VertexOut {
		@builtin(position) Position : vec4<f32>,
	}
	@vertex
	fn main(input : VertexIn) -> VertexOut {
		var output : VertexOut;
		output.Position = vec4<f32>(input.aPos,0.0, 1.0);
		return output;
	}
)";

static char const triangle_frag_wgsl[] = R"(@group(0) @binding(0) var<uniform> Time : f32;
@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;
@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;
@group(0) @binding(3) var<uniform> Date1 : vec3<i32>;
@group(0) @binding(4) var<uniform> Date2 : vec3<i32>;
@group(0) @binding(5) var<uniform> Key : i32;
@group(0) @binding(6) var<uniform> Position : vec2<f32>;
@group(0) @binding(7) var<uniform> Random : f32;
@group(0) @binding(8) var<uniform> randomarray: array<vec4<f32>,25>;
@group(0) @binding(9) var<uniform> Position_dino : vec2<f32>;
@group(1) @binding(0) var texture1: texture_2d<f32>;
@group(1) @binding(1) var texture2: texture_2d<f32>;
@group(1) @binding(2) var texture3: texture_2d<f32>;
@group(1) @binding(3) var texture4: texture_2d<f32>;
@group(1) @binding(4) var sampler_: sampler;
@fragment
fn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
  let width = 800.0;
  let height = 600.0;
  let xFact = 1.0 / (width / 2.0);
  let yFact = 1.0 / (height / 2.0);
  let aspect = width/height;
  let PI = 3.141592653589793238; 
  
  var light: vec3<f32> = vec3<f32>(0.0, 0.0, 5.0);
  var camera: vec3<f32> = vec3<f32>(0.0, 0.0, 1.0);
  var cameraUp: vec3<f32> = vec3<f32>(0.0, 1.0, 0.0);
  var viewDirection: vec3<f32> = vec3<f32>(0.0, 0.0, -1.0);

  var plane1: Object;
  plane1.color = vec3<f32>(0.5, 0.5, 0.5);
  var plane1_trans: Transformation;
  plane1_trans.translation = vec3<f32>(0.0, 1.0, -2.0);
  plane1_trans.rotation = vec3<f32>(-0.55*PI, -0.0*PI, 0.0*PI);
  plane1_trans.scale = vec3<f32>(5.0, 5.0, 1.0);
  plane1.transformations = plane1_trans;

  var sphere1: Object;
  sphere1.color = vec3<f32>(1.0, 0.0, 0.0);
  var sphere1_trans: Transformation;
  sphere1_trans.translation = vec3<f32>(0.0, 0.0, -5.0);
  sphere1_trans.rotation = vec3<f32>(0.0, 0.0, 0.0);
  sphere1_trans.scale = vec3<f32>(0.75, 0.75, 0.75);
  sphere1.transformations = sphere1_trans;

  var sphere2: Object;
  sphere2.color = vec3<f32>(0.0, 1.0, 0.0);
  var sphere2_trans: Transformation;
  sphere2_trans.translation = vec3<f32>(-1.0, 0.0, -3.0);
  sphere2_trans.rotation = vec3<f32>(0.0, 0.0, 0.0);
  sphere2_trans.scale = vec3<f32>(0.5, 0.5, 0.5);
  sphere2.transformations = sphere2_trans;

  var cone1: Object;
  cone1.color = vec3<f32>(0.0, 0.0, 1.0);
  var cone1_trans: Transformation;
  cone1_trans.translation = vec3<f32>(0.0, 0.0, -3.0);
  cone1_trans.rotation = vec3<f32>(-PI/2.0, -PI, 0.0);
  cone1_trans.scale = vec3<f32>(0.5, 0.5, 0.5);
  cone1.transformations = cone1_trans;

  let normX = (position.x * xFact) - 1.0;
  let normY = (position.y * yFact) - 1.0;

  let alignment = normalize(viewDirection - camera);

  let u = normalize(cross(alignment, cameraUp));
  let v = normalize(cross(u, alignment)) / aspect;

  let screenCenter = camera + alignment;
  let screenWorldCoordinate = screenCenter + (u * normX) + (v * normY);

  var cameraRay: Ray;
  cameraRay.point1 = camera;
  cameraRay.point2 = screenWorldCoordinate;
  cameraRay.ab = cameraRay.point2 - cameraRay.point1;
  
  let objectCount: i32 = 4;
  var colors: array<ColorPoint, 4>;

  colors[0] = get_sphere(cameraRay, sphere1, light);
  colors[1] = get_sphere(cameraRay, sphere2, light);
  colors[2] = get_cone(cameraRay, cone1, light);
  colors[3] = get_plane(cameraRay, plane1, light);
  
  var min_idx: i32 = 0;
  var min_dist: f32 = colors[0].distance;
  for (var i: i32 = 1; i < objectCount; i++) {
    if (colors[i].distance < min_dist) {
      min_dist = colors[i].distance;
      min_idx = i;
    }
  }
  return colors[min_idx].color;
}

fn get_sphere(cameraRay: Ray,
              object: Object, 
              light: vec3<f32>) -> ColorPoint {
  let KA = 0.3;
  let KD = 0.7;
  let PI = 3.141592653589793238; 

  let back_ray = transform_ray(cameraRay, object.transformations, false);
  let vhat = normalize(back_ray.ab);
  var output: ColorPoint;
  output.color = vec4<f32>(0.0,0.0,0.0,1.0);
  output.distance = pow(10.0,10.0);

  let b = 2.0 * dot(back_ray.point1, vhat); 
  let c = dot(back_ray.point1, back_ray.point1) - 1.0;
  let disc = b * b - 4.0 * c;
  if (disc > 0.0) {
    let numSQRT = sqrt(disc);
    let t1 = (-b + numSQRT) / 2.0;
    let t2 = (-b - numSQRT) / 2.0;
    
    if ((t1 < 0.0) || (t2 < 0.0)) {
      return output;
    } else {
      var poi: vec3<f32> = vec3<f32>(0.0,0.0,0.0);
      if (t1 < t2) {
        poi = back_ray.point1 + (vhat * t1);
      } else {
        poi = back_ray.point1 + (vhat * t2);
      }
      let int_point = transform_vec(poi, object.transformations, true);
      let object_origin: vec3<f32> = vec3<f32>(0.0, 0.0, 0.0);
      let new_object_origin = transform_vec(object_origin, object.transformations, true);
      let localNormal = normalize(int_point - new_object_origin);

      let lightDirection:vec3<f32> = normalize(light - poi);
      
      let angle: f32 = acos(dot(localNormal, lightDirection));
      var intensity: f32;
      if (angle > PI/2.0) {
        intensity = 0.0;
      } else {
        intensity = (1.0 - (angle / (PI / 2.0)));
      }
      
      output.color = vec4<f32>(calculate_pixel(object.color, intensity), 1.0);
      output.distance = distance(int_point, cameraRay.point2);
      return output;
    }
  } else {
    return output;
  }
}

fn get_cone(cameraRay: Ray,
            object: Object,
            light: vec3<f32>) -> ColorPoint {
  let EPSILON: f32 = pow(10.0, -21.0);
  let PI = 3.141592653589793238;

  let back_ray = transform_ray(cameraRay, object.transformations, false);

  let p = back_ray.point1;
  let v = normalize(back_ray.ab);

  var output: ColorPoint;
  output.color = vec4<f32>(0.0,0.0,0.0,1.0);
  output.distance = pow(10.0,10.0);

  let a = pow(v[0], 2.0) + pow(v[1], 2.0) - pow(v[2], 2.0);
  let b = 2.0 * (p[0]*v[0] + p[1]*v[1] - p[2]*v[2]);
  let c = pow(p[0], 2.0) + pow(p[1], 2.0) - pow(p[2], 2.0);
  
  let disc = b * b - 4.0 * a * c;
  var t: array<f32, 3>;
  var t_validity: array<bool, 3>;
  var poi: array<vec3<f32>, 3>;
  if (disc > 0.0) {
    let numSQRT = sqrt(disc);
    if (numSQRT > 0.0) {
      t[0] = (-b + numSQRT) / (2.0 * a);
      t[1] = (-b - numSQRT) / (2.0 * a);
      
      poi[0] = back_ray.point1 + v * t[0];
      poi[1] = back_ray.point1 + v * t[1];
      if ((t[0] > 0.0) && (poi[0][2] > 0.0) && (poi[0][2] < 1.0)) {
        t_validity[0] = true;
      } else {
        t_validity[0] = false;
        t[0] = pow(10.0, 10.0);
      }

      if ((t[1] > 0.0) && (poi[1][2] > 0.0) && (poi[1][2] < 1.0)) {
        t_validity[1] = true;
      } else {
        t_validity[1] = false;
        t[1] = pow(10.0, 10.0);
      }
    } else {
      t_validity[0] = false;
      t_validity[1] = false;
      t[0] = pow(10.0, 10.0);
      t[1] = pow(10.0, 10.0);
    }
  }

  if (abs(v[2]) < EPSILON) {
    t_validity[2] = false;
    t[2] = pow(10.0, 10.0);
  } else {  
    t[2] = (back_ray.point1[2] - 1.0) / - v[2];
    poi[2] = back_ray.point1 + t[2] * v;
    
    if ((t[2] > 0.0) && (sqrt(pow(poi[2][0], 2.0) + pow(poi[2][1], 2.0)) < 1.0)) {
      t_validity[2] = true;
    } else {
      t_validity[2] = false;
      t[2] = pow(10.0, 10.0);
    }           
  }
  
  if ((!t_validity[0]) && (!t_validity[1]) && (!t_validity[2])) {
    return output;  
  }

  var min_idx: i32 = 0;
  var min_t: f32 = t[0];
  for (var i: i32 = 1; i < 3; i++) {
    if (t[i] < min_t) {
      min_t = t[i];
      min_idx = i;
    }
  }

  let valid_poi: vec3<f32> = poi[min_idx];
  if (min_idx < 2) { 
    let local_org = vec3<f32>(0.0, 0.0, 0.0);
    let global_org = transform_vec(local_org, object.transformations, true);

    let tX = valid_poi[0];
    let tY = valid_poi[1];
    let tZ = -sqrt(pow(tX, 2.0) + pow(tY, 2.0));
    let org_normal = vec3<f32>(tX, tY, tZ);
    let normal = normalize(transform_vec(org_normal, object.transformations, true) - global_org);
    
    let int_point = transform_vec(valid_poi, object.transformations, true);   
    let lightDirection:vec3<f32> = normalize(light - int_point);
    
    let angle: f32 = acos(dot(normal, lightDirection));
    var intensity: f32;
    if (angle > PI/2.0) {
      intensity = 0.0;
    } else {
      intensity = (1.0 - (angle / (PI / 2.0)));
    }

    output.color = vec4<f32>(calculate_pixel(object.color, intensity), 1.0);
    output.distance = distance(int_point, cameraRay.point2);
  } else {
    if (abs(v[2]) > 0.0) {
      if (sqrt(pow(valid_poi[0], 2.0) + pow(valid_poi[1], 2.0)) < 1.0) {
        let int_point = transform_vec(valid_poi, object.transformations, true);  
        let lightDirection:vec3<f32> = normalize(light - int_point);     
        
        let local_org = vec3<f32>(0.0, 0.0, 0.0);    
        var normal = vec3<f32>(0.0, 0.0, 1.0);

        let global_org = transform_vec(local_org, object.transformations, true);
        normal = normalize(transform_vec(normal, object.transformations, true) - global_org);
        
        let angle: f32 = acos(dot(normal, lightDirection));
        var intensity: f32;
        if (angle > PI/2.0) {
          intensity = 0.0;
        } else {
          intensity = (1.0 - (angle / (PI / 2.0)));
        }

        output.color = vec4<f32>(calculate_pixel(object.color, intensity), 1.0);
        output.distance = distance(int_point, cameraRay.point2);        
      }    
    } 
  }
  return output;
}

fn get_plane(cameraRay: Ray,
            object: Object,
            light: vec3<f32>) -> ColorPoint {
  let EPSILON: f32 = pow(10.0, -21.0);
  let PI = 3.141592653589793238;

  var output: ColorPoint;
  output.color = vec4<f32>(0.2,0.2,0.2,1.0);
  output.distance = pow(10.0,10.0);

  let back_ray = transform_ray(cameraRay, object.transformations, false);
  let k = normalize(back_ray.ab);

  let t = back_ray.point1[2] / (-k[2]);
  if (t > 0.0) {
    let u = back_ray.point1[0] + t * k[0];
    let v = back_ray.point1[1] + t * k[1];

    if (abs(u) < 1.0 && abs(v) < 1.0) {
      let poi = back_ray.point1 + t*k;
      let int_point = transform_vec(poi, object.transformations, true);

      let local_org = vec3<f32>(0.0, 0.0, 0.0);
      let global_org = transform_vec(local_org, object.transformations, true);
      let normal_vector = vec3<f32>(0.0, 0.0, -1.0);
      let normal = normalize(transform_vec(normal_vector, object.transformations, true) - global_org);
           
      let lightDirection:vec3<f32> = normalize(light - int_point);
      let angle: f32 = acos(dot(normal, lightDirection));
      var intensity: f32;
      if (angle > PI/2.0) {
        intensity = 0.0;
      } else {
        intensity = (1.0 - (angle / (PI / 2.0)));
      }

      output.color = vec4<f32>(calculate_pixel(object.color, intensity), 1.0);
      output.distance = distance(int_point, cameraRay.point2);     
    }
  }

  return output;
}

fn calculate_pixel(color: vec3<f32>, intensity: f32) -> vec3<f32> {
  let KA = 0.3;
  let KD = 0.7;
  return intensity*(KA + color * KD);
}

fn transform(transformation: Transformation,
             direction: bool) -> mat4x4<f32> {
  var translation: vec3<f32> = transformation.translation;
  var rotation: vec3<f32> = transformation.rotation;
  var scale: vec3<f32> = transformation.scale;
  let id: mat4x4<f32> = mat4x4<f32>(1.0, 0.0, 0.0, 0.0,
                                    0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 1.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0);
  var translationMatrix: mat4x4<f32>;
  var rotationMatrixX: mat4x4<f32>;
  var rotationMatrixY: mat4x4<f32>;
  var rotationMatrixZ: mat4x4<f32>;
  var scaleMatrix: mat4x4<f32>;
  
  translationMatrix = id;
  rotationMatrixX = id;
  rotationMatrixY = id;
  rotationMatrixZ = id;
  scaleMatrix = id;
  
  translationMatrix[0][3] = translation[0];
  translationMatrix[1][3] = translation[1];
  translationMatrix[2][3] = translation[2];
  rotationMatrixZ[0][0] = cos(rotation[2]);
  rotationMatrixZ[0][1] = -sin(rotation[2]);
  rotationMatrixZ[1][0] = sin(rotation[2]);
  rotationMatrixZ[1][1] = cos(rotation[2]);
  rotationMatrixY[0][0] = cos(rotation[1]);
  rotationMatrixY[0][2] = sin(rotation[1]);
  rotationMatrixY[2][0] = -sin(rotation[1]);
  rotationMatrixY[2][2] = cos(rotation[1]);
  rotationMatrixX[1][1] = cos(rotation[0]);
  rotationMatrixX[1][2] = -sin(rotation[0]);
  rotationMatrixX[2][1] = sin(rotation[0]);
  rotationMatrixX[2][2] = cos(rotation[0]);
  scaleMatrix[0][0] = scale[0];
  scaleMatrix[1][1] = scale[1];
  scaleMatrix[2][2] = scale[2];

  var result: mat4x4<f32> =
      rotationMatrixX *
      rotationMatrixY *
      rotationMatrixZ *
      scaleMatrix *
      translationMatrix;
  if (direction) {
    return result;
  } else {
    return inverse(result);
  }
}
fn transform_ray(input: Ray, 
                 transformation: Transformation,
                 direction: bool) -> Ray {
  var output: Ray;
  output.point1 = transform_vec(input.point1, transformation, direction);
  output.point2 = transform_vec(input.point2, transformation, direction);
  output.ab = output.point2 - output.point1;

  return output;
}

fn transform_vec(input: vec3<f32>, 
                 transformation: Transformation,
                 direction: bool) -> vec3<f32> {
  var output: vec3<f32>;
  var trans_matrix: mat4x4<f32> = transform(transformation, direction);

  var result_mat: vec4<f32> = trans_matrix * vec4<f32>(input, 1.0);
  output = vec3<f32>(result_mat[0], result_mat[1], result_mat[2]);

  return output;
}

fn inverse(m: mat4x4<f32>) -> mat4x4<f32> {
  var inv: mat4x4<f32>;
  var out: mat4x4<f32>;
  inv[0][0] = m[1][1]  * m[2][2] * m[3][3] - 
            m[1][1]  * m[2][3] * m[3][2] - 
            m[2][1]  * m[1][2]  * m[3][3] + 
            m[2][1]  * m[1][3]  * m[3][2] +
            m[3][1] * m[1][2]  * m[2][3] - 
            m[3][1] * m[1][3]  * m[2][2];
  inv[1][0] = -m[1][0]  * m[2][2] * m[3][3] + 
            m[1][0]  * m[2][3] * m[3][2] + 
            m[2][0]  * m[1][2]  * m[3][3] - 
            m[2][0]  * m[1][3]  * m[3][2] - 
            m[3][0] * m[1][2]  * m[2][3] + 
            m[3][0] * m[1][3]  * m[2][2];
  inv[2][0] = m[1][0]  * m[2][1] * m[3][3] - 
            m[1][0]  * m[2][3] * m[3][1] - 
            m[2][0]  * m[1][1] * m[3][3] + 
            m[2][0]  * m[1][3] * m[3][1] + 
            m[3][0] * m[1][1] * m[2][3] - 
            m[3][0] * m[1][3] * m[2][1];
  inv[3][0] = -m[1][0]  * m[2][1] * m[3][2] + 
              m[1][0]  * m[2][2] * m[3][1] +
              m[2][0]  * m[1][1] * m[3][2] - 
              m[2][0]  * m[1][2] * m[3][1] - 
              m[3][0] * m[1][1] * m[2][2] + 
              m[3][0] * m[1][2] * m[2][1];
  inv[0][1] = -m[0][1]  * m[2][2] * m[3][3] + 
            m[0][1]  * m[2][3] * m[3][2] + 
            m[2][1]  * m[0][2] * m[3][3] - 
            m[2][1]  * m[0][3] * m[3][2] - 
            m[3][1] * m[0][2] * m[2][3] + 
            m[3][1] * m[0][3] * m[2][2];
  inv[1][1] = m[0][0]  * m[2][2] * m[3][3] - 
            m[0][0]  * m[2][3] * m[3][2] - 
            m[2][0]  * m[0][2] * m[3][3] + 
            m[2][0]  * m[0][3] * m[3][2] + 
            m[3][0] * m[0][2] * m[2][3] - 
            m[3][0] * m[0][3] * m[2][2];
  inv[2][1] = -m[0][0]  * m[2][1] * m[3][3] + 
            m[0][0]  * m[2][3] * m[3][1] + 
            m[2][0]  * m[0][1] * m[3][3] - 
            m[2][0]  * m[0][3] * m[3][1] - 
            m[3][0] * m[0][1] * m[2][3] + 
            m[3][0] * m[0][3] * m[2][1];
  inv[3][1] = m[0][0]  * m[2][1] * m[3][2] - 
            m[0][0]  * m[2][2] * m[3][1] - 
            m[2][0]  * m[0][1] * m[3][2] + 
            m[2][0]  * m[0][2] * m[3][1] + 
            m[3][0] * m[0][1] * m[2][2] - 
            m[3][0] * m[0][2] * m[2][1];
  inv[0][2] = m[0][1]  * m[1][2] * m[3][3] - 
            m[0][1]  * m[1][3] * m[3][2] - 
            m[1][1]  * m[0][2] * m[3][3] + 
            m[1][1]  * m[0][3] * m[3][2] + 
            m[3][1] * m[0][2] * m[1][3] - 
            m[3][1] * m[0][3] * m[1][2];
  inv[1][2] = -m[0][0]  * m[1][2] * m[3][3] + 
            m[0][0]  * m[1][3] * m[3][2] + 
            m[1][0]  * m[0][2] * m[3][3] - 
            m[1][0]  * m[0][3] * m[3][2] - 
            m[3][0] * m[0][2] * m[1][3] + 
            m[3][0] * m[0][3] * m[1][2];
  inv[2][2] = m[0][0]  * m[1][1] * m[3][3] - 
            m[0][0]  * m[1][3] * m[3][1] - 
            m[1][0]  * m[0][1] * m[3][3] + 
            m[1][0]  * m[0][3] * m[3][1] + 
            m[3][0] * m[0][1] * m[1][3] - 
            m[3][0] * m[0][3] * m[1][1];
  inv[3][2] = -m[0][0]  * m[1][1] * m[3][2] + 
              m[0][0]  * m[1][2] * m[3][1] + 
              m[1][0]  * m[0][1] * m[3][2] - 
              m[1][0]  * m[0][2] * m[3][1] - 
              m[3][0] * m[0][1] * m[1][2] + 
              m[3][0] * m[0][2] * m[1][1];
  inv[0][3] = -m[0][1] * m[1][2] * m[2][3] + 
            m[0][1] * m[1][3] * m[2][2] + 
            m[1][1] * m[0][2] * m[2][3] - 
            m[1][1] * m[0][3] * m[2][2] - 
            m[2][1] * m[0][2] * m[1][3] + 
            m[2][1] * m[0][3] * m[1][2];
  inv[1][3] = m[0][0] * m[1][2] * m[2][3] - 
            m[0][0] * m[1][3] * m[2][2] - 
            m[1][0] * m[0][2] * m[2][3] + 
            m[1][0] * m[0][3] * m[2][2] + 
            m[2][0] * m[0][2] * m[1][3] - 
            m[2][0] * m[0][3] * m[1][2];
  inv[2][3] = -m[0][0] * m[1][1] * m[2][3] + 
              m[0][0] * m[1][3] * m[2][1] + 
              m[1][0] * m[0][1] * m[2][3] - 
              m[1][0] * m[0][3] * m[2][1] - 
              m[2][0] * m[0][1] * m[1][3] + 
              m[2][0] * m[0][3] * m[1][1];
  inv[3][3] = m[0][0] * m[1][1] * m[2][2] - 
            m[0][0] * m[1][2] * m[2][1] - 
            m[1][0] * m[0][1] * m[2][2] + 
            m[1][0] * m[0][2] * m[2][1] + 
            m[2][0] * m[0][1] * m[1][2] - 
            m[2][0] * m[0][2] * m[1][1];
  var det: f32 = m[0][0] * inv[0][0] + m[0][1] * inv[1][0] + m[0][2] * inv[2][0] + m[0][3] * inv[3][0];
  det = 1.0 / det;
  for (var i:i32 = 0; i < 4; i++) {
    for (var j:i32; j<4; j++) {
      out[i][j] = inv[i][j] * det;
    }
  }
  return out;
}
struct Ray {
  point1: vec3<f32>,
  point2: vec3<f32>,
  ab: vec3<f32>
}
struct ColorPoint {
  color: vec4<f32>,
  distance: f32,
}
struct Transformation {
  translation: vec3<f32>,
  rotation: vec3<f32>, 
  scale: vec3<f32>
}
struct Object {
  color: vec3<f32>,
  transformations: Transformation
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

	WGPUBindGroupLayoutEntry date1lEntry = {};
	date1lEntry.binding = 3;
	date1lEntry.visibility = WGPUShaderStage_Fragment;
	date1lEntry.buffer = buf;

	WGPUBindGroupLayoutEntry date2lEntry = {};
	date2lEntry.binding = 4;
	date2lEntry.visibility = WGPUShaderStage_Fragment;
	date2lEntry.buffer = buf;
	
	WGPUBindGroupLayoutEntry keylEntry = {};
	keylEntry.binding = 5;
	keylEntry.visibility = WGPUShaderStage_Fragment;
	keylEntry.buffer = buf;

	WGPUBindGroupLayoutEntry positionlEntry = {};
	positionlEntry.binding = 6;
	positionlEntry.visibility = WGPUShaderStage_Fragment;
	positionlEntry.buffer = buf;

	WGPUBindGroupLayoutEntry randomlEntry = {};
	randomlEntry.binding = 7;
	randomlEntry.visibility = WGPUShaderStage_Fragment;
	randomlEntry.buffer = buf;

	WGPUBindGroupLayoutEntry randomArraylEntry = {};
	randomArraylEntry.binding = 8;
	randomArraylEntry.visibility = WGPUShaderStage_Fragment;
	randomArraylEntry.buffer = buf;

	WGPUBindGroupLayoutEntry positiondinolEntry = {};
	positiondinolEntry.binding = 9;
	positiondinolEntry.visibility = WGPUShaderStage_Fragment;
	positiondinolEntry.buffer = buf;


	WGPUBindGroupLayoutEntry* allBgLayoutEntries = new WGPUBindGroupLayoutEntry[10];
	allBgLayoutEntries[0] = timelEntry;
	allBgLayoutEntries[1] = resolutionlEntry;
	allBgLayoutEntries[2] = mouselEntry;
	allBgLayoutEntries[3] = date1lEntry;
	allBgLayoutEntries[4] = date2lEntry;
	allBgLayoutEntries[5] = keylEntry;
	allBgLayoutEntries[6] = positionlEntry;
	allBgLayoutEntries[7] = randomlEntry;
	allBgLayoutEntries[8] = randomArraylEntry;
	allBgLayoutEntries[9] = positiondinolEntry;
	WGPUBindGroupLayoutDescriptor bglDesc = {};
	bglDesc.entryCount = 10;  
	bglDesc.entries = allBgLayoutEntries;
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

	tex1 = createTexture(img_1, imgw_1, imgh_1,texSize1,texDesc1,texDataLayout1,texCopy1);
	tex2 = createTexture(img_2, imgw_2, imgh_2,texSize2,texDesc2,texDataLayout2,texCopy2);
	tex3 = createTexture(img_3, imgw_3, imgh_3,texSize3,texDesc3,texDataLayout3,texCopy3);
	tex4 = createTexture(img_4, imgw_4, imgh_4,texSize4,texDesc4,texDataLayout4,texCopy4);

	WGPUTextureViewDescriptor texViewDesc = {};
	texViewDesc.dimension = WGPUTextureViewDimension_2D;
	texViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
	texViewDesc.mipLevelCount=1;
	texViewDesc.arrayLayerCount=1;
	texView1 = wgpuTextureCreateView(tex1, &texViewDesc);
	texView2 = wgpuTextureCreateView(tex2, &texViewDesc);
	texView3 = wgpuTextureCreateView(tex3, &texViewDesc);
	texView4 = wgpuTextureCreateView(tex4, &texViewDesc);

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

	WGPUBindGroupLayoutEntry bglTexEntry_1 = {};
	bglTexEntry_1.binding = 0;
	bglTexEntry_1.visibility = WGPUShaderStage_Fragment;
	bglTexEntry_1.texture = texLayout;

	WGPUBindGroupLayoutEntry bglTexEntry_2 = {};
	bglTexEntry_2.binding = 1;
	bglTexEntry_2.visibility = WGPUShaderStage_Fragment;
	bglTexEntry_2.texture = texLayout;

	WGPUBindGroupLayoutEntry bglTexEntry_3 = {};
	bglTexEntry_3.binding = 2;
	bglTexEntry_3.visibility = WGPUShaderStage_Fragment;
	bglTexEntry_3.texture = texLayout;

	WGPUBindGroupLayoutEntry bglTexEntry_4 = {};
	bglTexEntry_4.binding = 3;
	bglTexEntry_4.visibility = WGPUShaderStage_Fragment;
	bglTexEntry_4.texture = texLayout;

	WGPUBindGroupLayoutEntry bglSamplerEntry = {};
	bglSamplerEntry.binding = 4;
	bglSamplerEntry.visibility = WGPUShaderStage_Fragment;
	bglSamplerEntry.sampler = samplerLayout;


	WGPUBindGroupLayoutEntry* textureBgLayoutEntries = new WGPUBindGroupLayoutEntry[5];
	textureBgLayoutEntries[0] = bglTexEntry_1;
	textureBgLayoutEntries[1] = bglTexEntry_2;
	textureBgLayoutEntries[2] = bglTexEntry_3;
	textureBgLayoutEntries[3] = bglTexEntry_4;
	textureBgLayoutEntries[4] = bglSamplerEntry;

	WGPUBindGroupLayoutDescriptor texturebglDesc = {};
	texturebglDesc.entryCount = 5;  
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
	date1Buf = createBuffer(&date1, sizeof(date1), WGPUBufferUsage_Uniform);
	date2Buf = createBuffer(&date2, sizeof(date2), WGPUBufferUsage_Uniform);
	keypressBuf= createBuffer(&keypress,sizeof(keypress), WGPUBufferUsage_Uniform);
	positionBuf= createBuffer(&position,sizeof(position), WGPUBufferUsage_Uniform);
	randomBuf= createBuffer(&random_num,sizeof(random_num), WGPUBufferUsage_Uniform);
	randomArrayBuf= createBuffer(&randomArray,sizeof(randomArray), WGPUBufferUsage_Uniform);
	positionDinoBuf = createBuffer(&position_dino,sizeof(position_dino), WGPUBufferUsage_Uniform);
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

	WGPUBindGroupEntry date1Entry = {};
	date1Entry.binding = 3;
	date1Entry.buffer = date1Buf;
	date1Entry.size = sizeof(date1);

	WGPUBindGroupEntry date2Entry = {};
	date2Entry.binding = 4;
	date2Entry.buffer = date2Buf;
	date2Entry.size = sizeof(date2);

	WGPUBindGroupEntry keypressEntry = {};
	keypressEntry.binding = 5;
	keypressEntry.buffer = keypressBuf;
	keypressEntry.size = sizeof(keypress);

	WGPUBindGroupEntry positionEntry = {};
	positionEntry.binding = 6;
	positionEntry.buffer = positionBuf;
	positionEntry.size = sizeof(position);

	WGPUBindGroupEntry randomEntry = {};
	randomEntry.binding = 7;
	randomEntry.buffer = randomBuf;
	randomEntry.size = sizeof(random_num);

	WGPUBindGroupEntry randomArrayEntry = {};
	randomArrayEntry.binding = 8;
	randomArrayEntry.buffer = randomArrayBuf;
	randomArrayEntry.size = sizeof(randomArray);

	WGPUBindGroupEntry positiondinoEntry = {};
	positiondinoEntry.binding = 9;
	positiondinoEntry.buffer = positionDinoBuf;
	positiondinoEntry.size = sizeof(position_dino);

	WGPUBindGroupEntry* uniformBgEntries = new WGPUBindGroupEntry[10];
	uniformBgEntries[0] = timeEntry;
	uniformBgEntries[1] = resolutionEntry;
	uniformBgEntries[2] = mouseEntry;
	uniformBgEntries[3] = date1Entry;
	uniformBgEntries[4] = date2Entry;
	uniformBgEntries[5] = keypressEntry;
	uniformBgEntries[6] = positionEntry;
	uniformBgEntries[7] = randomEntry;
	uniformBgEntries[8] = randomArrayEntry;
	uniformBgEntries[9] = positiondinoEntry;

	WGPUBindGroupDescriptor uniformbgDesc = {};
	uniformbgDesc.layout = bindGroupLayout;
	uniformbgDesc.entryCount = 10;   
	uniformbgDesc.entries = uniformBgEntries;

	bindGroup = wgpuDeviceCreateBindGroup(device, &uniformbgDesc);

	WGPUBindGroupEntry bgTexEntry_1 = {};
	bgTexEntry_1.binding = 0;
	bgTexEntry_1.textureView = texView1;
	WGPUBindGroupEntry bgTexEntry_2 = {};
	bgTexEntry_2.binding = 1;
	bgTexEntry_2.textureView = texView2;
	WGPUBindGroupEntry bgTexEntry_3 = {};
	bgTexEntry_3.binding = 2;
	bgTexEntry_3.textureView = texView3;
	WGPUBindGroupEntry bgTexEntry_4 = {};
	bgTexEntry_4.binding = 3;
	bgTexEntry_4.textureView = texView4;

	WGPUBindGroupEntry bgSamplerEntry = {};
	bgSamplerEntry.binding = 4;
	bgSamplerEntry.sampler = samplerTex;

	WGPUBindGroupEntry* textureBgEntries = new WGPUBindGroupEntry[5];
	textureBgEntries[0] = bgTexEntry_1;
	textureBgEntries[1] = bgTexEntry_2;
	textureBgEntries[2] = bgTexEntry_3;
	textureBgEntries[3] = bgTexEntry_4;
	textureBgEntries[4] = bgSamplerEntry;

	WGPUBindGroupDescriptor texturebgDesc = {};
	texturebgDesc.layout = texturebindGroupLayout;//
	texturebgDesc.entryCount = 5;   
	texturebgDesc.entries = textureBgEntries;

	texturebindGroup = wgpuDeviceCreateBindGroup(device, &texturebgDesc);


	// last bit of clean-up
	wgpuBindGroupLayoutRelease(bindGroupLayout);
	wgpuBindGroupLayoutRelease(texturebindGroupLayout);
}
EM_JS(void, jsprint, ( float x), {
  console.log(x);
});
EM_JS(void, say, (const char* str), {
  console.log( UTF8ToString(str));
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

  if (eventType == EMSCRIPTEN_EVENT_KEYPRESS&& (!strcmp(e->key, "w")|| e->which == 119)&&press==false)
  {
	keypress=e->which;
	press=true;
	w_press=30;
  }
  if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && press==false) {
    keypress=e->which;
	press=true;
  }
  if(position[0]<0.0f){position[0]=0.0f;}
  if(position[0]>800.0f){position[0]=800.0f;}
  if(position[1]<0.0f){position[1]=0.0f;}
  if(position[1]>600.0f){position[1]=600.0f;}
  if(position_dino[0]<0.0f){position_dino[0]=0.0f;}
  if(position_dino[0]>800.0f){position_dino[0]=800.0f;}
  if(position_dino[1]<0.0f){position_dino[1]=0.0f;}
  if(position_dino[1]>600.0f){position_dino[1]=600.0f;}
  return 0;
}
static bool redraw() {
	random_num=emscripten_random();
	EMSCRIPTEN_RESULT ret;
	if (mouseflag%10==0)
	{
		ret = emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, mouse_callback);
		ret = emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, mouse_click_callback);
		mouseflag++;
	}
	if(press==true)
	{
		pressflag++;
	}
	if(pressflag%10==0)
	{
		pressflag=0;
		keypress=0;
		press=false;
	}
	if(w_press>0)
	{
		position[1]-=5.0f;
		position_dino[1]-=5.0f;
		w_press-=1;
	}
	else
	{
		position[1]+=5.0f;
		position_dino[1]+=5.0f;
		if(position_dino[1]>600.0f)
		{
			position_dino[1]=600.0f;
		}
	}
	ret = emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
	// update the time 
	endTime = clock();
	runtime = (float)(endTime - startTime) / (CLOCKS_PER_SEC);
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(swapchain);			// create textureView

	WGPURenderPassColorAttachment colorDesc = {};
	colorDesc.view    = backBufView;
	colorDesc.loadOp  = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;
	colorDesc.clearValue.r = 0.0f;
	colorDesc.clearValue.g = 0.0f;
	colorDesc.clearValue.b = 0.0f;
	colorDesc.clearValue.a = 0.0f;

	WGPURenderPassDescriptor renderPass = {};
	renderPass.colorAttachmentCount = 1;
	renderPass.colorAttachments = &colorDesc;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);			// create encoder
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass);	// create pass

	

	//update the date

	time_t now = time(0);
	tm *ltm = localtime(&now);
	date1[0]=1900+ ltm->tm_year;
	date1[1]=1+ ltm->tm_mon;
	date1[2]=ltm->tm_mday;

	date2[0]=ltm->tm_hour;
	date2[1]=ltm->tm_min;
	date2[2]=ltm->tm_sec;

	
	wgpuQueueWriteBuffer(queue, timeBuf,0, &runtime, sizeof(runtime));
	wgpuQueueWriteBuffer(queue, resolutionBuf,0, &resolution, sizeof(resolution));
	wgpuQueueWriteBuffer(queue, mouseBuf,0, &mouselocation, sizeof(mouselocation));
	wgpuQueueWriteBuffer(queue, date1Buf,0, &date1, sizeof(date1));
	wgpuQueueWriteBuffer(queue, date2Buf,0, &date2, sizeof(date2));
	wgpuQueueWriteBuffer(queue, keypressBuf,0, &keypress, sizeof(keypress));
	wgpuQueueWriteBuffer(queue, positionBuf,0, &position, sizeof(position));
	wgpuQueueWriteBuffer(queue, randomBuf,0, &random_num, sizeof(random_num));
	wgpuQueueWriteBuffer(queue, randomArrayBuf,0, &randomArray, sizeof(randomArray));
	wgpuQueueWriteBuffer(queue, positionDinoBuf,0, &position_dino, sizeof(position_dino));
	wgpuQueueWriteTexture(queue, &texCopy1, img_1, imgh_1 * imgw_1 * 4, &texDataLayout1, &texSize1);
	wgpuQueueWriteTexture(queue, &texCopy2, img_2, imgh_2 * imgw_2 * 4, &texDataLayout2, &texSize2);
	wgpuQueueWriteTexture(queue, &texCopy3, img_3, imgh_3 * imgw_3 * 4, &texDataLayout3, &texSize3);
	wgpuQueueWriteTexture(queue, &texCopy4, img_4, imgh_4 * imgw_4 * 4, &texDataLayout4, &texSize4);
	// draw the triangle (comment these five lines to simply clear the screen)
	wgpuRenderPassEncoderSetPipeline(pass, pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, 0);
	wgpuRenderPassEncoderSetBindGroup(pass, 1, texturebindGroup, 0, 0);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertBuf, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderSetIndexBuffer(pass, indxBuf, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
	wgpuRenderPassEncoderDrawIndexed(pass, 6, 1, 0, 0, 0);

	wgpuRenderPassEncoderEnd(pass);
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
void load_images(SDL_Surface *image, int imgw,int imgh,unsigned char*& img )
{
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
}
void image_init()
{		
		SDL_Surface *image;
		image=IMG_Load("out/texture/admin_black.jpg");//texture1
		imgw_1=image->w;
		imgh_1=image->h;
		img_1=new unsigned char[imgw_1 * imgh_1*4];
		load_images(image,imgw_1,imgh_1,img_1);

		image=IMG_Load("out/texture/admin_black.jpg");//texture2
		imgw_2=image->w;
		imgh_2=image->h;
		img_2=new unsigned char[imgw_2 * imgh_2*4];
		load_images(image,imgw_2,imgh_2,img_2);

		image=IMG_Load("out/texture/admin_black.jpg");//texture3
		imgw_3=image->w;
		imgh_3=image->h;
		img_3=new unsigned char[imgw_3 * imgh_3*4];
		load_images(image,imgw_3,imgh_3,img_3);

		image=IMG_Load("out/texture/admin_black.jpg");//texture4
		imgw_4=image->w;
		imgh_4=image->h;

		img_4=new unsigned char[imgw_4 * imgh_4*4];
		load_images(image,imgw_4,imgh_4,img_4);

}


extern "C" int __main__(int /*argc*/, char* /*argv*/[]) {
	int flags=IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF;
	int initted=IMG_Init(flags);
	image_init();
	for (int i=0;i<100;i++)
	{
		int t=i/4;
		int index=i%4;
		randomArray[t][index]=emscripten_random();
	}
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
			wgpuBufferRelease(dateBuf);
			wgpuBufferRelease(mouseBuf);
			wgpuBufferRelease(indxBuf);
			wgpuBufferRelease(vertBuf);
			wgpuBufferRelease(keypressBuf);
			wgpuBufferRelease(positionBuf);
			wgpuBufferRelease(randomBuf);
			wgpuBufferRelease(randomArrayBuf);
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
