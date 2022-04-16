var cmd=require('node-cmd')
var fs = require('fs');
const { dirname } = require('path');


texture_code="<a href='#' onclick='click_texture(texturenum,imgnum,\"London.jpg\")'><img src='texture/London.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"happytree.jpg\")'><img src='texture/happytree.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"stock.jpg\")'><img src='texture/stock.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"wall.jpg\")'><img src='texture/wall.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"black.jpg\")'><img src='texture/black.jpg' class='img-thumbnail' width='84' height='84'> </a>"
view_texture_code="<a href='#' onclick='click_texture(texturenum,imgnum,\"London.jpg\")'><img src='../texture/London.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"happytree.jpg\")'><img src='../texture/happytree.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"stock.jpg\")'><img src='../texture/stock.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"wall.jpg\")'><img src='../texture/wall.jpg' class='img-thumbnail' width='84' height='84'> </a><a href='#' onclick='click_texture(texturenum,imgnum,\"black.jpg\")'><img src='../texture/black.jpg' class='img-thumbnail' width='84' height='84'> </a>"
exports.header=function(req,res,next)
{
    res.setHeader('Access-Control-Allow-Origin','*')
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST');
    res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type, Authorization')
    next()
};
exports.index=function(req,res)
{
    res.render(__dirname+"/../browse.html",{username:req.session.username})
};
exports.new=function(req,res)
{
    res.render(__dirname+"/../new.html",{texture_code:texture_code,username:req.session.username})
};
exports.browse=function(req,res)
{
    res.render(__dirname+"/../browse.html",{username:req.session.username})
};
exports.about=function(req,res)
{
    res.render(__dirname+"/../about.html",{username:req.session.username})
};
exports.compile=function(req,res)
{
    texture1_code=""
    texture2_code=""
    texture3_code=""
    texture4_code=""
    if(req.body.texture1!="")
    {
      texture1_code="image=IMG_Load(\"out/texture/"+ req.body.texture1+"\");//texture1"
    }
    if(req.body.texture2!="")
    {
      texture2_code="image=IMG_Load(\"out/texture/"+ req.body.texture2+"\");//texture2"
    }
    if(req.body.texture3!="")
    {
      texture3_code="image=IMG_Load(\"out/texture/"+ req.body.texture3+"\");//texture3"
    }
    if(req.body.texture4!="")
    {
      texture4_code="image=IMG_Load(\"out/texture/"+ req.body.texture4+"\");//texture4"
    }
    description="@group(0) @binding(0) var<uniform> Time : f32;\n@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;\n@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;\n@group(0) @binding(3) var<uniform> Date1 : vec3<i32>;\n@group(0) @binding(4) var<uniform> Date2 : vec3<i32>;\n@group(1) @binding(0) var texture1: texture_2d<f32>;\n@group(1) @binding(1) var texture2: texture_2d<f32>;\n@group(1) @binding(2) var texture3: texture_2d<f32>;\n@group(1) @binding(3) var texture4: texture_2d<f32>;\n@group(1) @binding(4) var sampler_: sampler;\n"
    fragment_code=description+req.body.code
    fragment_code="static char const triangle_frag_wgsl[] = R\"("+fragment_code+")\"; // fragment shader end"
    //console.log(__dirname)
    fs.readFile(__dirname+'/../../main.cpp',function(err,data){
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
            fs.writeFile(__dirname+'/../../main.cpp',code,function(err){
              if(err) throw err;
              cmd.run("cd "+__dirname+"/../../"+" && make",function(err,data)
              {
                  if(err) throw err;
                  else
                  {
                      res.render(__dirname+"/../new_template.html",{wgsl_code:req.body.code,texture1:req.body.texture1,texture2:req.body.texture2,texture3:req.body.texture3,texture4:req.body.texture4,texture_code:texture_code,username:req.session.username})
                  }
              })
          })
        }
    })
};
exports.view=function(req,res)
{
    fs.readFile(__dirname+'/../view.json','utf8',function (err, _data) {
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
        returndata.data.texture_code=view_texture_code
        returndata.data.username=req.session.username
        res.render(__dirname+"/../view/template.html",returndata)
    })
};
exports.file_upload=function(req,res)
{
    console.log(req.files[0]);  // upload file information
  
    var des_file = __dirname + "/../texture/" + req.files[0].originalname; //file name
    console.log(des_file)
    fs.readFile( req.files[0].path, function (err, data) {  
         fs.writeFile(des_file, data, function (err) { 
          if( err ){
               console.log( err );
               console.log( data );
          }else{
                // upload success
                response = {
                    message:'File uploaded successfully', 
                    filename:req.files[0].originalname
               };
            console.log(response)
            texture_code+="<a href='#' onclick='click_texture(texturenum,imgnum,\""+req.files[0].originalname+"\")'><img src='texture/"+req.files[0].originalname+ "' class='img-thumbnail' width='84' height='84'> </a>"
            res.send(texture_code)
           }
        });
    });
};
exports.userprofile=function(req,res)
{
  res.render(__dirname+"/../userprofile.html",{username:req.session.username})
};