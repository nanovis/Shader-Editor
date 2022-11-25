var cmd = require('node-cmd')
var fs = require('fs');
const archiver = require('archiver');
const sizeOf = require('image-size')
const { dirname } = require('path');
const req = require('express/lib/request');
var MongoClient = require('mongodb').MongoClient;
var url = 'mongodb://localhost:27017/';
const tf = require('@tensorflow/tfjs-node')
const nsfw = require('nsfwjs');
const session = require('express-session');


exports.header = function (req, res, next) {
  res.setHeader('Access-Control-Allow-Origin', '*')
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST');
  res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type, Authorization')
  next()
};

function chunkArray(arr, n) {
  let chunkLength = Math.max(arr.length / n, 1);
  let chunks = [];
  for (let i = 0; i < n; i++) {
    if (chunkLength * (i + 1) <= arr.length) chunks.push(arr.slice(chunkLength * i, chunkLength * (i + 1)));
  }
  return chunks;
}

exports.index = function (req, res) {
  MongoClient.connect(url, async (err, db) => {
    if (err) {
      console.error(err);
      throw err
    };
    const dbo = db.db('shadereditor');
    var _data = await dbo.collection('shader').find({$or:[{ "status": "public" }, { "status": "Public" }]}).toArray();
    var _newdata=[];
    for( var i=0;i<_data.length;i++)
    {
        const path=__dirname+"/../view/"+_data[i]["name"]+"_"+_data[i]["user"]+".png";
        if(fs.existsSync(path))
        {
          _newdata[_newdata.length]=_data[i];
        }
    }
    const _chunked = chunkArray(_newdata, Math.ceil(_newdata.length / 4));
    res.render(__dirname + "/../browse.hbs", { username: req.session.username, data: JSON.stringify(_chunked) })
  })
};
exports.new = function (req, res) {
  gettexturecode(req.session.username, function (err, texturecode) {
    console.log(__dirname)
    res.render(__dirname + "/../new.hbs", { texture_code: texturecode, username: req.session.username })
  });
};

exports.browse = function (req, res) {
  MongoClient.connect(url, async (err, db) => {
    if (err) {
      console.error(err);
      throw err
    };
    const dbo = db.db('shadereditor');
    const _data = await dbo.collection('shader').find({$or:[{ "status": "public" }, { "status": "Public" }]}).toArray();
    const _chunked = chunkArray(_data, Math.ceil(_data.length / 4));
    res.render(__dirname + "/../browse.hbs", { username: req.session.username, data: JSON.stringify(_chunked) })
  })
};
exports.about = function (req, res) {
  res.render(__dirname + "/../about.hbs", { username: req.session.username })
};
exports.compile = function (req, res) {
  texture1_code = ""
  texture2_code = ""
  texture3_code = ""
  texture4_code = ""
  if (req.body.texture1 != "") {
    texture1_code = "image=IMG_Load(\"out/texture/" + req.body.texture1 + "\");//texture1"
  }
  if (req.body.texture2 != "") {
    texture2_code = "image=IMG_Load(\"out/texture/" + req.body.texture2 + "\");//texture2"
  }
  if (req.body.texture3 != "") {
    texture3_code = "image=IMG_Load(\"out/texture/" + req.body.texture3 + "\");//texture3"
  }
  if (req.body.texture4 != "") {
    texture4_code = "image=IMG_Load(\"out/texture/" + req.body.texture4 + "\");//texture4"
  }
  description = "@group(0) @binding(0) var<uniform> Time : f32;\n"+
        "@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;\n"+
        "@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;\n"+
        "@group(0) @binding(3) var<uniform> Date1 : vec3<i32>;\n"+
        "@group(0) @binding(4) var<uniform> Date2 : vec3<i32>;\n"+
        "@group(0) @binding(5) var<uniform> Key : i32;\n"+
        "@group(0) @binding(6) var<uniform> Position : vec2<f32>;\n"+
        "@group(0) @binding(7) var<uniform> Random : f32;\n"+
        "@group(0) @binding(8) var<uniform> randomarray: array<vec4<f32>,25>;\n"+
        "@group(0) @binding(9) var<uniform> Position_dino : vec2<f32>;\@"+
        "group(1) @binding(0) var texture1: texture_2d<f32>;\n"+
        "@group(1) @binding(1) var texture2: texture_2d<f32>;\n"+
        "@group(1) @binding(2) var texture3: texture_2d<f32>;\n"+
        "@group(1) @binding(3) var texture4: texture_2d<f32>;\n"+
        "@group(1) @binding(4) var sampler_: sampler;\n"+
        "@group(2) @binding(0) var<storage,read_write> vec4Buffer: array<vec4<f32>,50>;\n"+
        "@group(2) @binding(1) var<storage,read_write> floatBuffer: array<f32,50>;\n"+
        "@group(2) @binding(2) var<storage,read_write> intBuffer: array<i32,50>;\n"+
        "@group(2) @binding(3) var<storage,read_write> frameBuffer: array<vec4<f32>,480000>;\n"+
        "@group(3) @binding(0) var<storage,read_write> matrixBuffer: array<mat4x4<f32>,50>;\n"
  fragment_code = description + req.body.code
  fragment_code = "static char const triangle_frag_wgsl[] = R\"(" + fragment_code + ")\"; // fragment shader end"
  fs.readFile(__dirname + '/../../main.cpp', function (err, data) {
    if (err) throw err;
    else {
      code = data.toString().replace(/static char const triangle_frag_wgsl[\s\S]*?\/\/ fragment shader end/, fragment_code)
      if (texture1_code != "") {
        code = code.replace(/image=IMG_Load.*?\/\/texture1/, texture1_code)
      }
      if (texture2_code != "") {
        code = code.replace(/image=IMG_Load.*?\/\/texture2/, texture2_code)
      }
      if (texture3_code != "") {
        code = code.replace(/image=IMG_Load.*?\/\/texture3/, texture3_code)
      }
      if (texture4_code != "") {
        code = code.replace(/image=IMG_Load.*?\/\/texture4/, texture4_code)
      }
      fs.writeFile(__dirname + '/../../main.cpp', code, function (err) {
        if (err) throw err;
        cmd.run("cd " + __dirname + "/../../" + " && make", function (err, data) {
          if (err) throw err;
          else {
            gettexturecode(req.session.username, function (err, texturecode) {
              res.render(__dirname + "/../new_template.hbs", { wgsl_code: req.body.code, texture1: req.body.texture1, texture2: req.body.texture2, texture3: req.body.texture3, texture4: req.body.texture4, texture_code: texturecode, username: req.session.username })
            });

          }
        })
      })
    }
  })
};
exports.view = function (req, res) {

  MongoClient.connect(url, function (err, db) {
    if (err) throw err;
    var dbo = db.db("shadereditor");
    dbo.collection("shader").find({ "name": req.query.name, "user": req.query.user }).toArray(function (err, result) {
      if (err) throw err;
      db.close();
      if (result.length != 0) {
        if (result[0].status == "private") {
          res.send("404 error")
        }
        getviewtexturecode(req.session.username, function (err, texturecode) {
          var returndata = {}
          returndata.texture_code = texturecode
          returndata.username = req.session.username
          returndata.code = result[0].code
          returndata.texture1 = result[0].texture1
          returndata.texture2 = result[0].texture2
          returndata.texture3 = result[0].texture3
          returndata.texture4 = result[0].texture4
          returndata.jsname = result[0].jsname
          if (req.query.canvas == undefined) { res.render(__dirname + "/../view/template.hbs", returndata) }
          else {
            returndata.redirect = "/view?name=" + req.query.name + "&user=" + req.query.user
            res.render(__dirname + "/../view/canvas.hbs", returndata)
          }
        });
      }
    });
  });
};
exports.file_upload = function (req, res) {
  var savename = req.session.username + "_" + req.files[0].originalname
  var des_file = __dirname + "/../texture/" + savename; //file name
  fs.readFile(req.files[0].path, function (err, data) {
    fs.writeFile(des_file, data, function (err) {
      if (err) {
        console.log(err);
        console.log(data);
      } else {
        // upload success
        response = {
          message: 'File uploaded successfully',
          filename: req.files[0].originalname
        };
        figurechecking(des_file, function (err, predictions) {
          if (predictions[0].className == 'Neutral' || predictions[0].className == 'Drawing') {
            const dimensions = sizeOf(des_file)
            MongoClient.connect(url, function (err, db) {
              if (err) throw err;
              var dbo = db.db("shadereditor");
              var inform = { "user": req.session.username, "name": req.files[0].originalname, "width": dimensions.width, "height": dimensions.height, "type": dimensions.type, "created": Date.now() };
              dbo.collection("texture").find({ "user": req.session.username, "name": req.files[0].originalname }).toArray(function (err, result) {
                if (err) throw err;
                if (result.length != 0) {
                  res.send(["fail", "This texture name is already taken!"])
                }
                else {
                  dbo.collection("texture").insertOne(inform, function (err, result) {
                    if (err) throw err;
                    db.close();
                    gettexturecode(req.session.username, function (err, texturecode) {
                      res.send(["success", texturecode])
                    });

                  });
                }
              });
            });
          }
          else { res.send(['fail', 'This figure doesn\'t pass the content checking']) }

        });

      }
    });
  });
};
exports.saveshader = function (req, res) {
  MongoClient.connect(url, function (err, db) {
    if (err) throw err;
    var dbo = db.db("shadereditor");
    if (req.session.username == undefined) {
      res.send("You should first log in.")
    }
    var query = { "user": req.session.username, "name": req.body.name };
    var information = { "name": req.body.name, "user": req.session.username, "status": req.body.status, "code": req.body.code, "jsname": req.body.name + ".js", "texture1": req.body.texture1, "texture2": req.body.texture2, "texture3": req.body.texture3, "texture4": req.body.texture4, "created": Date.now() }
    dbo.collection("shader").find(query).toArray(function (err, result) {
      if (err) throw err;
      if (result.length != 0) {
        var updateStr = { $set: { "status": req.body.status, "code": req.body.code, "texture1": req.body.texture1, "texture2": req.body.texture2, "texture3": req.body.texture3, "texture4": req.body.texture4 } }
        dbo.collection("shader").updateOne(query, updateStr, function (err, result) {
          if (err) throw err;
          db.close();
          res.send("The shader name is already used. Update Successful.")
        });

      }
      else {
        dbo.collection("shader").insertOne(information, function (err, result) {
          if (err) throw err;
          db.close();
          res.send("Save Success")
        });
      }
    });
  })
};
exports.downloadshader = function (req, res) {
  MongoClient.connect(url, function (err, db) {
    if (err) throw err;
    var dbo = db.db("shadereditor");
    if (req.session.username == undefined) {
      res.send("You should first log in.")
    }
    var query = { "user": req.session.username};
   
    dbo.collection("shader").find(query).toArray(function (err, result) {
      if (err) throw err;
      var dirpath=__dirname+"/../shaders/"+req.session.username;
      if (!fs.existsSync(dirpath)) {
        fs.mkdirSync(dirpath);
      }
      const files = fs.readdirSync(dirpath);
      files.forEach(file => {
        const filePath = `${dirpath}/${file}`;
        const stats = fs.statSync(filePath);
        if (!stats.isDirectory()) {
            fs.unlinkSync(filePath);
        }
      });

      for(var i=0;i<result.length;i++)
      {
        var path=__dirname+"/../shaders/"+req.session.username+"/"+result[i]["name"]+".wgsl";
        fs.writeFileSync(path,result[i]["code"]);
      }
      const output = fs.createWriteStream(__dirname + "/../shaders/"+req.session.username+".zip");
      const archive = archiver('zip', {zlib: {level: 9}});
      archive.pipe(output);
      archive.directory(dirpath, false);
      archive.finalize();
      res.send("Please wait for a while before downloading starts");
    });
   
  })
};
exports.view_user = function (req, res) {
  MongoClient.connect(url, function (err, db) {
    if (err) throw err;
    var dbo = db.db("shadereditor");
    if (req.session.username==undefined) 
    {
      var query={$or:[{ "status": "public", "user": req.query.user, "name": req.query.name}, { "status": "Public","user": req.query.user, "name": req.query.name }]};
    }
    else
    {
      if(req.session.username==req.query.user)
      {
        var query = { "user": req.query.user, "name": req.query.name };
      }
      else
      {
        var query={$or:[{ "status": "public", "user": req.query.user, "name": req.query.name}, { "status": "Public","user": req.query.user, "name": req.query.name }]};
      }
    }
    dbo.collection("shader").find(query).toArray(function (err, result) {
      if (err) throw err;
      db.close();
      if (result.length == 0 ) {
        res.send("Nothing find")
      }
      else {
        
        code = result[0].code
        texture1 = result[0].texture1
        texture2 = result[0].texture2
        texture3 = result[0].texture3
        texture4 = result[0].texture4
        texture1_code = ""
        texture2_code = ""
        texture3_code = ""
        texture4_code = ""
        if (texture1 != "") {
          texture1_code = "image=IMG_Load(\"out/texture/" + texture1 + "\");//texture1"
        }
        if (texture2 != "") {
          texture2_code = "image=IMG_Load(\"out/texture/" + texture2 + "\");//texture2"
        }
        if (texture3 != "") {
          texture3_code = "image=IMG_Load(\"out/texture/" + texture3 + "\");//texture3"
        }
        if (texture4 != "") {
          texture4_code = "image=IMG_Load(\"out/texture/" + texture4 + "\");//texture4"
        }
        description = "@group(0) @binding(0) var<uniform> Time : f32;\n"+
        "@group(0) @binding(1) var<uniform> Resolution : vec2<f32>;\n"+
        "@group(0) @binding(2) var<uniform> Mouse : vec4<f32>;\n"+
        "@group(0) @binding(3) var<uniform> Date1 : vec3<i32>;\n"+
        "@group(0) @binding(4) var<uniform> Date2 : vec3<i32>;\n"+
        "@group(0) @binding(5) var<uniform> Key : i32;\n"+
        "@group(0) @binding(6) var<uniform> Position : vec2<f32>;\n"+
        "@group(0) @binding(7) var<uniform> Random : f32;\n"+
        "@group(0) @binding(8) var<uniform> randomarray: array<vec4<f32>,25>;\n"+
        "@group(0) @binding(9) var<uniform> Position_dino : vec2<f32>;\@"+
        "group(1) @binding(0) var texture1: texture_2d<f32>;\n"+
        "@group(1) @binding(1) var texture2: texture_2d<f32>;\n"+
        "@group(1) @binding(2) var texture3: texture_2d<f32>;\n"+
        "@group(1) @binding(3) var texture4: texture_2d<f32>;\n"+
        "@group(1) @binding(4) var sampler_: sampler;\n"+
        "@group(2) @binding(0) var<storage,read_write> vec4Buffer: array<vec4<f32>,50>;\n"+
        "@group(2) @binding(1) var<storage,read_write> floatBuffer: array<f32,50>;\n"+
        "@group(2) @binding(2) var<storage,read_write> intBuffer: array<i32,50>;\n"+
        "@group(2) @binding(3) var<storage,read_write> frameBuffer: array<vec4<f32>,480000>;\n"+
        "@group(3) @binding(0) var<storage,read_write> matrixBuffer: array<mat4x4<f32>,50>;\n"
        
        fragment_code = description + code
        fragment_code = "static char const triangle_frag_wgsl[] = R\"(" + fragment_code + ")\"; // fragment shader end"
        fs.readFile(__dirname + '/../../main.cpp', function (err, data) {
          if (err) throw err;
          else {
            code = data.toString().replace(/static char const triangle_frag_wgsl[\s\S]*?\/\/ fragment shader end/, fragment_code)
            if (texture1_code != "") {
              code = code.replace(/image=IMG_Load.*?\/\/texture1/, texture1_code)
            }
            if (texture2_code != "") {
              code = code.replace(/image=IMG_Load.*?\/\/texture2/, texture2_code)
            }
            if (texture3_code != "") {
              code = code.replace(/image=IMG_Load.*?\/\/texture3/, texture3_code)
            }
            if (texture4_code != "") {
              code = code.replace(/image=IMG_Load.*?\/\/texture4/, texture4_code)
            }
            fs.writeFile(__dirname + '/../../main.cpp', code, function (err) {
              if (err) throw err;
              cmd.run("cd " + __dirname + "/../../" + " && make", function (err, data) {
                if (err) throw err;
                else {
                  gettexturecode(req.session.username, function (err, texturecode) {
                    res.render(__dirname + "/../new_template.hbs", { wgsl_code: result[0].code, texture1: texture1, texture2: texture2, texture3: texture3, texture4: texture4, texture_code: texturecode, username: req.session.username, shadername: result[0].name })
                  });

                }
              })
            })
          }
        })
      }

    });
  });
};
function gettexturecode(username, callback) {
  texture_code = "";
  MongoClient.connect(url, function (err, db) {
    if (err) throw err;
    var dbo = db.db("shadereditor");
    if (username != undefined) {
      query = { $or: [{ "user": username }, { "user": "admin" }] }
    }
    else {
      query = { "user": "admin" }
    }
    dbo.collection("texture").find(query).sort({ "created": 1 }).toArray(function (err, result) {
      if (err) throw err;
      for (var i = 0; i < result.length; i++) {
        savename = result[i].user + "_" + result[i].name
        texture_code += "<a href='#' onclick='click_texture(texturenum,imgnum,\"" + savename + "\")'><img src='texture/" + savename + "' class='img-thumbnail' width='84' height='84'> </a>"
      }
      callback(null, texture_code)
    });
  });

};
function getviewtexturecode(username, callback) {
  MongoClient.connect(url, function (err, db) {
    if (err) throw err;
    var dbo = db.db("shadereditor");
    texture_code = "";
    if (username != undefined) {
      query = { $or: [{ "user": username }, { "user": "admin" }] }
    }
    else {
      query = { "user": "admin" }
    }
    dbo.collection("texture").find(query).sort({ "created": 1 }).toArray(function (err, result) {
      if (err) throw err;
      for (var i = 0; i < result.length; i++) {
        savename = result[i].user + "_" + result[i].name
        texture_code += "<a href='#' onclick='click_texture(texturenum,imgnum,\"" + savename + "\")'><img src='../texture/" + savename + "' class='img-thumbnail' width='84' height='84'> </a>"
      }
      callback(null, texture_code)
    });
  });
};
async function figurechecking(path, callback) {
  const model = await nsfw.load() // To load a local model, nsfw.load('file://./path/to/model/')
  // Image must be in tf.tensor3d format
  // you can convert image to tf.tensor3d with tf.node.decodeImage(Uint8Array,channels)
  const imageBuffer = await fs.readFileSync(path);
  const image = await tf.node.decodeImage(imageBuffer, 3)
  const predictions = await model.classify(image)
  image.dispose() // Tensor memory must be managed explicitly (it is not sufficient to let a tf.Tensor go out of scope for its memory to be released).
  callback(null, predictions)
};

