# Shader Editor
This project is a WGSL online editor and compiler based on [WebGPU](https://gpuweb.github.io/gpuweb/)/[Emscripten](https://emscripten.org/) and [Dawn](https://dawn.googlesource.com/dawn). Users could write shaders to do the pixel-level operations. During each frame, different input uniforms are fed into shaders to utilise mouse, time and texture information. After logging in, users could save shaders and upload textures.

Tested with Chrome Dev (version 103.0.5060.13) with the `--enable-unsafe-webgpu` flag.

## Dependences
- Emscripten: version 3.1.10
- Node.js: version 14.18.2
- MongoDB: version 4.4.13

## Inspired by (and derived from) the following examples
- https://github.com/cwoffenden/hello-webgpu

## Demo
- [Shader Editor](https://shadereditor.kaust.edu.sa/) - Now it only supports Chorme.
## How to run locally
```sh
$ git clone https://github.com/nanovis/Shader-Editor.git
$ cd Shader-Editor/out
$ npm install
$ cd ../vs_editor
$ npm install
$ cd .. && make serve
```
Access http://localhost:8080 on [Chorme Dev](https://www.google.com/chrome/dev/) and access `chrome://flags` to enable `--enable-unsafe-webgpu`.
## Todo:
- Provide an interface for users to create uniform buffers
- Syntax highlighting
- Compiling error log
- Shaders' distributed version control
- Simultaneous editing by multiple people
- Shader sharing and publishing
- Register email verification
- Brower pages pagination 
- ...
## License
Shader Editor is licensed under the MIT License, see [LICENSE](https://github.com/nanovis/Shader-Editor/blob/main/LICENSE) for more information.
