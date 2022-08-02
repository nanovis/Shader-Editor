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
    console.log('[Syntax Checker] GPU Not Found')
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
    code: window.editor.getValue(),
  });

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

  if (window.editor.getModel() !== null) {
    monaco.editor.setModelMarkers(window.editor.getModel()!, "owner", errorMsgs);
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
  await gpuReadBuffer.mapAsync(GPUMapMode.READ);
  const arrayBuffer = gpuReadBuffer.getMappedRange();
}
