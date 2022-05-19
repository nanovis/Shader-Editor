require.config({ paths: { vs: '/assets/monaco-editor/min/vs' } });
require(['vs/editor/editor.main'], function () {
    editor = monaco.editor.create(document.getElementById('textarea'), {
        value: document.getElementById("code").value,
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

