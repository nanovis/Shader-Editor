//@stage(fragment)
fn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
var uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);
var col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));
return vec4<f32>(col, 1.0);
}