require.config({ stubModules: ['json', 'text'], paths: { vs: '/assets/monaco-editor/min/vs', langs: '/assets/langs' } });

require(['vs/editor/editor.main'], async function () {
    monaco.languages.register({ id: 'wgsl' });

    const langConf = await (fetch("/assets/langs/wgsl_syntax.json").then(r => r.json()).then(d => d));
    monaco.languages.setMonarchTokensProvider('wgsl', {
        tokenizer: {
            root: Object.values(langConf).map(({ comment, name, match }) => [new RegExp(match), name.split('.wgsl')[0]]),
        }
    });

    //TODO: Add Syntax Coloring and Predictve Text.


    monaco.editor.create(document.getElementById('textarea'), {
        value: '@stage(fragment)\nfn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {\nvar uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);\nvar col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));\nreturn vec4<f32>(col, 1.0);\n}',
        language: 'wgsl',
        theme: 'vs-light',
        lineNumbers: 'on',
        roundedSelection: false,
        scrollBeyondLastLine: false,
        readOnly: false
    });
});

function updatetextarea() {
    require(['vs/editor/editor.main'], function () {
        document.getElementById("code").value = monaco.editor.getValue()
    });
};

