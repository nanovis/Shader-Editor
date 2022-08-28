//by KGSP student Mohammed Alnasser
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
}