var cmd=require('node-cmd');
const express = require('express')
const bodyParser=require("body-parser")
var fs = require('fs')
const app = express()
const port = 8000
app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())
app.use(express.static(__dirname))
app.use(express.static(".."))
app.get('/', (req, res) => {
  res.sendFile("index.html")
})
app.post('/replace', (req, res) => {
    fragment_code=req.body.story.replace(/\r?\n/g,'')
    fragment_code="static char const triangle_frag_wgsl[] = R\"("+fragment_code+")\"; // fragment shader end"
    fs.readFile('/Users/jdg/Documents/GitHub/shadertoy-webgpu/main.cpp',function(err,data){
        if(err) throw err;
        else
        {
            code=data.toString().replace(/static char const triangle_frag_wgsl.*?\/\/ fragment shader end/,fragment_code)
            fs.writeFile('/Users/jdg/Documents/GitHub/shadertoy-webgpu/main.cpp',code,function(err){
                if(err) throw err;
                cmd.run("cd /Users/jdg/Documents/GitHub/shadertoy-webgpu && make",function(err,data)
                {
                    if(err) throw err;
                    else{res.redirect("/")}
                })
                
            })
        }
    })
  })

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})