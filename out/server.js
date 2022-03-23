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
app.use(express.static(__dirname+"/view"))
app.set('view engine', 'html');
app.engine('html', hbs.__express);

app.all('*', function(req, res, next) {
  res.setHeader('Access-Control-Allow-Origin','*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST'); 
  res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type, Authorization'); 
  // res.setHeader("Content-Type", "application/json;charset=utf-8");
  next();
})

app.get('/', (req, res) => {

  res.render('index.html')
})
app.get('/new', (req, res) => {

  res.render(__dirname+"/new.html")
})
app.get('/signin', (req, res) => {

  res.render(__dirname+"/signin.html")
})
app.get('/browse', (req, res) => {

  res.render(__dirname+"/browse.html")
})
app.get('/about', (req, res) => {

  res.render(__dirname+"/about.html")
})
app.get('/signup', (req, res) => {
  res.render(__dirname+"/signup.html")
})
app.post('/compile', (req, res) => {
    texture1_code=""
    texture2_code=""
    texture3_code=""
    texture4_code=""
    if(req.body.texture1!="")
    {
      texture1_code="image=IMG_Load(\"texture/"+ req.body.texture1+"\");//texture1"
    }
    if(req.body.texture2!="")
    {
      texture2_code="image=IMG_Load(\"texture/"+ req.body.texture2+"\");//texture2"
    }
    if(req.body.texture3!="")
    {
      texture3_code="image=IMG_Load(\"texture/"+ req.body.texture3+"\");//texture3"
    }
    if(req.body.texture4!="")
    {
      texture4_code="image=IMG_Load(\"texture/"+ req.body.texture4+"\");//texture4"
    }
    description="@group(0) @binding(0) var<uniform> Time : f32;\n@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;\n@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;\n@group(0) @binding(3) var<uniform> Date1 : vec3<i32>;\n@group(0) @binding(4) var<uniform> Date2 : vec3<i32>;\n@group(1) @binding(0) var texture1: texture_2d<f32>;\n@group(1) @binding(1) var texture2: texture_2d<f32>;\n@group(1) @binding(2) var texture3: texture_2d<f32>;\n@group(1) @binding(3) var texture4: texture_2d<f32>;\n@group(1) @binding(4) var sampler_: sampler;\n"
    fragment_code=description+req.body.story
    fragment_code="static char const triangle_frag_wgsl[] = R\"("+fragment_code+")\"; // fragment shader end"
    fs.readFile(__dirname+'/../main.cpp',function(err,data){
        if(err) throw err;
        else
        {
            code=data.toString().replace(/static char const triangle_frag_wgsl[\s\S]*?\/\/ fragment shader end/,fragment_code)
            if (texture1_code!="")
            {
              code=code.replace(/image=IMG_Load.*?\/\/texture1/,texture1_code)
            }
            if (texture2_code!="")
            {
              code=code.replace(/image=IMG_Load.*?\/\/texture2/,texture2_code)
            }
            if (texture3_code!="")
            {
              code=code.replace(/image=IMG_Load.*?\/\/texture3/,texture3_code)
            }
            if (texture4_code!="")
            {
              code=code.replace(/image=IMG_Load.*?\/\/texture4/,texture4_code)
            }
            fs.writeFile(__dirname+'/../main.cpp',code,function(err){
              if(err) throw err;
              cmd.run("cd "+__dirname+"/../"+" && make",function(err,data)
              {
                  if(err) throw err;
                  else
                  {
                      res.render(__dirname+"/new_template.html",{wgsl_code:req.body.story,texture1:req.body.texture1,texture2:req.body.texture2,texture3:req.body.texture3,texture4:req.body.texture4})
                  }
              })
          })
        }
    })
  })
app.get('/view/*', (req, res) => {
    fs.readFile(__dirname+'/view.json','utf8',function (err, _data) {
      if(err) console.log(err);

      var view=JSON.parse(_data);

      var returndata
      for (var index in view)
      {
        if (view[index].name==req.path)
        {
          returndata=view[index]
        }
      }
      res.render(__dirname+"/view/template.html",returndata)
  })

  })
app.use(function(request, response) {
  response.writeHead(404, { "Content-Type": "text/plain" });
  response.end("404 error!\n");
})

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})