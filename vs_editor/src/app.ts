import type { LanguageId } from './register';
import type { ScopeName, TextMateGrammar, ScopeNameInfo } from './providers';

// Recall we are using MonacoWebpackPlugin. According to the
// monaco-editor-webpack-plugin docs, we must use:
//
// import * as monaco from 'monaco-editor/esm/vs/editor/editor.api';
//
// instead of
//
// import * as monaco from 'monaco-editor';
//
// because we are shipping only a subset of the languages.
import * as monaco from 'monaco-editor/esm/vs/editor/editor.api';
import { createOnigScanner, createOnigString, loadWASM } from 'vscode-oniguruma';
import { SimpleLanguageInfoProvider } from './providers';
import { registerLanguages } from './register';
import { rehydrateRegexps } from './configuration';
import VsCodeDarkTheme from './vs-light-plus-theme';
import { startValidationServer } from './autocomplete/validator';
import { SyntaxCheck } from './syntax';
//import { addHoverText } from './hover_text';

interface DemoScopeNameInfo extends ScopeNameInfo {
  path: string;
}

main('wgsl');

declare global {
  interface Window { MonacoEnvironment: any, editor: monaco.editor.IStandaloneCodeEditor }
}

self.MonacoEnvironment = {
  getWorkerUrl: function () {
    return './assets/js/vs_editor/editor.worker.bundle.js';
  }
}

async function main(language: LanguageId) {
  const languages: monaco.languages.ILanguageExtensionPoint[] = [
    {
      id: 'python',
      extensions: [
        '.py',
        '.rpy',
        '.pyw',
        '.cpy',
        '.gyp',
        '.gypi',
        '.pyi',
        '.ipy',
        '.bzl',
        '.cconf',
        '.cinc',
        '.mcconf',
        '.sky',
        '.td',
        '.tw',
      ],
      aliases: ['Python', 'py'],
      filenames: ['Snakefile', 'BUILD', 'BUCK', 'TARGETS'],
      firstLine: '^#!\\s*/?.*\\bpython[0-9.-]*\\b',
    },
    {
      id: 'wgsl',
      extensions: [
        '.wgsl',
        '.WGSL',
      ],
      aliases: ['WGSL', 'wgsl'],
      firstLine: '^#!\\s*/?.*\\bwgsl[0-9.-]*\\b',
    },
  ];
  const grammars: { [scopeName: string]: DemoScopeNameInfo } = {
    'source.python': {
      language: 'python',
      path: 'MagicPython.tmLanguage.json',
    },
    'source.wgsl': {
      language: 'wgsl',
      path: 'WGSL.tmLanguage.json',
    },
  };

  const fetchGrammar = async (scopeName: ScopeName): Promise<TextMateGrammar> => {
    const { path } = grammars[scopeName];
    const uri = `/assets/static/langs/${path}`;
    const response = await fetch(uri);
    const grammar = await response.text();
    const type = path.endsWith('.json') ? 'json' : 'plist';
    return { type, grammar };
  };

  const fetchConfiguration = async (
    language: LanguageId,
  ): Promise<monaco.languages.LanguageConfiguration> => {
    const uri = `/assets/static/configurations/${language}.json`;
    const response = await fetch(uri);
    const rawConfiguration = await response.text();
    return rehydrateRegexps(rawConfiguration);
  };

  const data: ArrayBuffer | Response = await loadVSCodeOnigurumWASM();
  loadWASM(data);
  const onigLib = Promise.resolve({
    createOnigScanner,
    createOnigString,
  });

  const provider = new SimpleLanguageInfoProvider({
    grammars,
    fetchGrammar,
    configurations: languages.map((language) => language.id),
    fetchConfiguration,
    theme: VsCodeDarkTheme,
    onigLib,
    monaco,
  });
  registerLanguages(
    languages,
    (language: LanguageId) => provider.fetchLanguageInfo(language),
    monaco,
  );

  const element = document.getElementById('textarea');
  if (element == null) {
    throw Error(`could not find element #textarea`);
  }

  const value = (document.getElementById('code') as HTMLInputElement).value === "" ? '@stage(fragment)\nfn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {\nvar uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);\nvar col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));\nreturn vec4<f32>(col, 1.0);\n}' : (document.getElementById('code') as HTMLInputElement).value;
  window.editor = monaco.editor.create(element, {
    value,
    language,
    theme: 'vs-light',
    lineNumbers: 'on',
    roundedSelection: false,
    scrollBeyondLastLine: false,
    readOnly: false
  });
  provider.injectCSS();

  startValidationServer();
  window.editor.onDidChangeModelContent(() => SyntaxCheck());
  //addHoverText();
}

// Taken from https://github.com/microsoft/vscode/blob/829230a5a83768a3494ebbc61144e7cde9105c73/src/vs/workbench/services/textMate/browser/textMateService.ts#L33-L40
async function loadVSCodeOnigurumWASM(): Promise<Response | ArrayBuffer> {
  const response = await fetch('assets/static/vs_editor/onig.wasm');
  const contentType = response.headers.get('content-type');
  if (contentType === 'application/wasm') {
    return response;
  }

  // Using the response directly only works if the server sets the MIME type 'application/wasm'.
  // Otherwise, a TypeError is thrown when using the streaming compiler.
  // We therefore use the non-streaming compiler :(.
  return await response.arrayBuffer();
}

export function updatetextarea() {
  (document.getElementById("code") as HTMLInputElement)!.value = window.editor.getValue();
}; 