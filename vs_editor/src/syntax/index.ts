/// <reference path="../../node_modules/@webgpu/types/dist/index.d.ts" />
import * as monaco from 'monaco-editor/esm/vs/editor/editor.api';



const MarkerSeverity = {
  hint: 1,
  info: 2,
  warning: 4,
  error: 8
}


export async function SyntaxCheck() {
  const adapter = await window.navigator.gpu.requestAdapter();
  if (!adapter) {
    console.error('[Syntax Checker] GPU Not Found')
    return;
  }
  const device = await adapter.requestDevice();

  /*device.addEventListener('error', () => {
    return;
  })*/

  // First Matrix

  const firstMatrix = new Float32Array([
    2 /* rows */,
    4 /* columns */,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8
  ]);

  const gpuBufferFirstMatrix = device.createBuffer({
    mappedAtCreation: true,
    size: firstMatrix.byteLength,
    usage: GPUBufferUsage.STORAGE
  });
  const arrayBufferFirstMatrix = gpuBufferFirstMatrix.getMappedRange();
  new Float32Array(arrayBufferFirstMatrix).set(firstMatrix);
  gpuBufferFirstMatrix.unmap();

  // Second Matrix

  const secondMatrix = new Float32Array([
    4 /* rows */,
    2 /* columns */,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8
  ]);

  const gpuBufferSecondMatrix = device.createBuffer({
    mappedAtCreation: true,
    size: secondMatrix.byteLength,
    usage: GPUBufferUsage.STORAGE
  });
  const arrayBufferSecondMatrix = gpuBufferSecondMatrix.getMappedRange();
  new Float32Array(arrayBufferSecondMatrix).set(secondMatrix);
  gpuBufferSecondMatrix.unmap();

  // Result Matrix

  const resultMatrixBufferSize =
    Float32Array.BYTES_PER_ELEMENT * (2 + firstMatrix[0] * secondMatrix[1]);
  const resultMatrixBuffer = device.createBuffer({
    size: resultMatrixBufferSize,
    usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC
  });

  // Bind group layout and bind group

  const bindGroupLayout = device.createBindGroupLayout({
    entries: [
      {
        binding: 0,
        visibility: GPUShaderStage.COMPUTE,
        buffer: {
          type: "read-only-storage"
        }
      },
      {
        binding: 1,
        visibility: GPUShaderStage.COMPUTE,
        buffer: {
          type: "read-only-storage"
        }
      },
      {
        binding: 2,
        visibility: GPUShaderStage.COMPUTE,
        buffer: {
          type: "storage"
        }
      }
    ]
  });

  const bindGroup = device.createBindGroup({
    layout: bindGroupLayout,
    entries: [
      {
        binding: 0,
        resource: {
          buffer: gpuBufferFirstMatrix
        }
      },
      {
        binding: 1,
        resource: {
          buffer: gpuBufferSecondMatrix
        }
      },
      {
        binding: 2,
        resource: {
          buffer: resultMatrixBuffer
        }
      }
    ]
  });


  const shaderModule = device.createShaderModule({
    code: "@group(0) @binding(0) var<uniform> Time : f32;\n@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;\n@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;\n@group(0) @binding(3) var<uniform> Date1 : vec3<i32>;\n@group(0) @binding(4) var<uniform> Date2 : vec3<i32>;\n@group(0) @binding(5) var<uniform> Key : i32;\n@group(0) @binding(6) var<uniform> Position : vec2<f32>;\n@group(0) @binding(7) var<uniform> Random : f32;\n@group(0) @binding(8) var<uniform> randomarray: array<vec4<f32>,25>;\n@group(0) @binding(9) var<uniform> Position_dino : vec2<f32>;\@group(1) @binding(0) var texture1: texture_2d<f32>;\n@group(1) @binding(1) var texture2: texture_2d<f32>;\n@group(1) @binding(2) var texture3: texture_2d<f32>;\n@group(1) @binding(3) var texture4: texture_2d<f32>;\n@group(1) @binding(4) var sampler_: sampler;\n"+window.editor.getValue(),
  });

  const comments = window.editor.getValue().split('\n').map((e, index) => {
    if (e.includes('//syntax-check disable')) return {
      id: index,
      value: e
    };
  }).filter((el) => el?.value !== undefined);

  const compInfo = await shaderModule.compilationInfo();

  const errorMsgs = compInfo.messages.map(e => {
    return {
      startLineNumber: e.lineNum,
      startColumn: e.linePos,
      endLineNumber: e.lineNum,
      endColumn: e.linePos + (e.length - 1),
      message: e.message,
      severity: MarkerSeverity[e.type as keyof typeof MarkerSeverity]
    }
  });

  const finalArr = errorMsgs.map((err) => {
    const t = comments.flatMap((a) => {
      if ((a?.id! + 1) === err.startLineNumber) {
        return undefined;
      } else return err;
    });
    return t;
  }).filter(e => !e.includes(undefined)).flatMap(o => o);

  const commentedErrorMsgs = [...new Set(finalArr) as any];

  if (window.editor.getModel() !== null) {
    monaco.editor.setModelMarkers(window.editor.getModel()!, "owner", (commentedErrorMsgs.length === 0 ? errorMsgs : commentedErrorMsgs));
  }
  const computePipeline = device.createComputePipeline({
    layout: device.createPipelineLayout({
      bindGroupLayouts: [bindGroupLayout]
    }),
    compute: {
      module: shaderModule,
      entryPoint: "main"
    }
  });

  // Commands submission

  const commandEncoder = device.createCommandEncoder();

  const passEncoder = commandEncoder.beginComputePass();
  passEncoder.setPipeline(computePipeline);
  passEncoder.setBindGroup(0, bindGroup);
  const workgroupCountX = Math.ceil(firstMatrix[0] / 8);
  const workgroupCountY = Math.ceil(secondMatrix[1] / 8);
  passEncoder.dispatchWorkgroups(workgroupCountX, workgroupCountY);
  passEncoder.end();

  // Get a GPU buffer for reading in an unmapped state.
  const gpuReadBuffer = device.createBuffer({
    size: resultMatrixBufferSize,
    usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
  });

  // Encode commands for copying buffer to buffer.
  commandEncoder.copyBufferToBuffer(
    resultMatrixBuffer /* source buffer */,
    0 /* source offset */,
    gpuReadBuffer /* destination buffer */,
    0 /* destination offset */,
    resultMatrixBufferSize /* size */
  );

  // Submit GPU commands.
  const gpuCommands = commandEncoder.finish();
  device.queue.submit([gpuCommands]);

  // Read buffer.
  //await gpuReadBuffer.mapAsync(GPUMapMode.READ);
  //const arrayBuffer = gpuReadBuffer.getMappedRange();
}

export function getErrors() {
  const errorLog = monaco.editor.getModelMarkers({ owner: 'owner' }).filter(e => e.severity === 8);

  if (errorLog.length !== 0) {
    (document.getElementById("compile_btn") as HTMLButtonElement).disabled = true;
  } else {
    (document.getElementById("compile_btn") as HTMLButtonElement).disabled = false;
  }
}