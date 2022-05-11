# Shader Editor
This project is a WGSL online editor and compiler based on [WebGPU](https://gpuweb.github.io/gpuweb/)/[Emscripten](https://emscripten.org/) and [Dawn](https://dawn.googlesource.com/dawn).

Tested with Chrome Canary and Chromium 100.0.4868.0 with the `--enable-unsafe-webgpu` flag.

## Dependences
- Emscripten: version 3.1.3
- Node.js: version 14.18.2
- MongoDB: version 4.4.13

## Inspired by (and derived from) the following examples
- https://github.com/cwoffenden/hello-webgpu

## Demo
- [Shader Editor](https://shadereditor.kaust.edu.sa/)

- Now it only supports Chorme.
## How to run locally
```sh
$ git clone https://github.com/nanovis/shadertoy-webgpu.git
$ cd Shader-Editor/out
$ npm install
$ cd .. && make serve
# Access http://localhost:8080 on your web browser
```
## Todo:
- Syntax highlighting
- Compiling error log
- Shader sharing and publishing
- Register email verification
- Brower pages pagination 
- ...
