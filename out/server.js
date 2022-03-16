var cmd=require('node-cmd');
const express = require('express')
const bodyParser=require("body-parser")
var hbs = require('hbs');
var fs = require('fs')
const app = express()
const port = 8000
app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())
app.use(express.static(__dirname))
app.use(express.static(".."))
app.set('view engine', 'html');
app.engine('html', hbs.__express);
app.get('/', (req, res) => {

  res.render('index.html')
})
app.get('/new', (req, res) => {

  res.render(__dirname+"/new.html")
})
app.get('/signin', (req, res) => {

  res.render(__dirname+"/signin.html")
})
app.get('/signup', (req, res) => {
  res.render(__dirname+"/signup.html")
})
app.post('/compile', (req, res) => {
    fragment_code=req.body.story
    fragment_code="static char const triangle_frag_wgsl[] = R\"("+fragment_code+")\"; // fragment shader end"
    fs.readFile('/Users/jdg/Documents/GitHub/shadertoy-webgpu/main.cpp',function(err,data){
        if(err) throw err;
        else
        {
            code=data.toString().replace(/static char const triangle_frag_wgsl[\s\S]*?\/\/ fragment shader end/,fragment_code)
            fs.writeFile('/Users/jdg/Documents/GitHub/shadertoy-webgpu/main.cpp',code,function(err){
              if(err) throw err;
              cmd.run("cd /Users/jdg/Documents/GitHub/shadertoy-webgpu && make",function(err,data)
              {
                  if(err) throw err;
                  else
                  {
                      res.render(__dirname+"/new_template.html",{wgsl_code:req.body.story})
                  }
              })
          })
        }
    })
  })
app.use(function(request, response) {
  response.writeHead(404, { "Content-Type": "text/plain" });
  response.end("404 error!\n");
})
app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})