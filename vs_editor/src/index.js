import { wireTmGrammars } from 'monaco-editor-textmate'
import { loadWASM } from 'onigasm' // peer dependency of 'monaco-textmate'
import { Registry } from 'monaco-textmate' // peer dependency
import * as monaco from 'monaco-editor/esm/vs/editor/editor.api';

self.MonacoEnvironment = {
	getWorkerUrl: function (moduleId, label) {
		if (label === 'typescript' || label === 'javascript') {
			return './assets/js/vs_editor/ts.worker.bundle.js';
		}
		return './assets/js/vs_editor/editor.worker.bundle.js';
	}
};

async () => {
	await loadWASM(`vs_editor/src/static/onigasm.wasm`) // See https://www.npmjs.com/package/onigasm#light-it-up
}

const registry = new Registry({
	getGrammarDefinition: async (scopeName) => {
		return {
			format: 'json',
			content: await (await fetch(`assets/langs/wgsl.tmGrammar.json`)).text()
		}
	}
});

monaco.languages.register({ id: "wgsl" });


const grammars = new Map();
grammars.set('wgsl', 'source.wgsl');

//Theme to work with TextMate Scopes
async () => {
	monaco.editor.defineTheme('vs-code-theme-converted', await (await fetch('assets/css/vs_theme_light.json')).json());
}

const editor = monaco.editor.create(document.getElementById('textarea'), {
	value: '@stage(fragment)\nfn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {\nvar uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);\nvar col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));\nreturn vec4<f32>(col, 1.0);\n}',
	language: 'wgsl', // this won't work out of the box, see below for more info,
	theme: 'vs-code-theme-converted', // very important, see comment above
	lineNumbers: 'on',
	roundedSelection: false,
	scrollBeyondLastLine: false,
	readOnly: false
});

async () => {
	await wireTmGrammars(monaco, registry, grammars, editor)
}