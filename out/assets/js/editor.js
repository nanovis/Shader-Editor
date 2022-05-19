require.config({ paths: { vs: 'assets/monaco-editor/min/vs' } });
require(['vs/editor/editor.main'], function () {
    editor = monaco.editor.create(document.getElementById('textarea'), {
        value: '@stage(fragment)\nfn main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {\nvar uv: vec3<f32> =vec3<f32>(position.xyx/Resolution.xyx);\nvar col:vec3<f32> =0.5f+vec3<f32> ( 0.5*cos(uv+Time+vec3<f32>(0.0,2.0,4.0)));\nreturn vec4<f32>(col, 1.0);\n}',
        language: 'plaintext',
        theme:'vs-light',
        lineNumbers: 'on',
        roundedSelection: false,
        scrollBeyondLastLine: false,
        readOnly: false
    });  
    
});
 
function updatetextarea () {
    require(['vs/editor/editor.main'], function () {
        document.getElementById("code").value=editor.getValue()
    });
};  



