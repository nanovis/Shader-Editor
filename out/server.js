var express = require('express')
var users=require('./router/user')
var router=require('./router/routes')
var multer = require('multer')
var bodyParser=require("body-parser")
var hbs = require('hbs')
const { check, validationResult } = require('express-validator');
var session = require('express-session')
const { route } = require('express/lib/application')
var app = express()
var port = 8080
app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())
app.use(session({
  secret: 'keyboard cat',
  resave: false,
  saveUninitialized: true
}));

const tf = require('@tensorflow/tfjs-node')
const nsfw = require('nsfwjs')
var fs = require('fs');
async function fn() {
  const model = await nsfw.load() // To load a local model, nsfw.load('file://./path/to/model/')
  // Image must be in tf.tensor3d format
  // you can convert image to tf.tensor3d with tf.node.decodeImage(Uint8Array,channels)
  const imageBuffer = await fs.readFileSync(__dirname+'/texture/admin_London.jpg');
  const image = await tf.node.decodeImage(imageBuffer)
  const predictions = await model.classify(image)
  image.dispose() // Tensor memory must be managed explicitly (it is not sufficient to let a tf.Tensor go out of scope for its memory to be released).
  console.log(predictions)
}
fn()


app.use(express.static(__dirname))
app.use(express.static(".."))
app.use(express.static(__dirname+"/view"))
app.use(express.static(__dirname+"/texture"))
app.use(express.static(__dirname+"/temp"))
app.set('view engine', 'html')
app.engine('html', hbs.__express)
app.use(multer({ dest: __dirname + "/temp/"}).array('image'))
app.all('*', router.header)
app.get('/', router.index)
app.get('/new', router.new)
app.get('/signin', users.signin)
app.get('/browse', router.browse)
app.get('/about', router.about)
app.get('/signup',users.signup)
app.post('/compile',router.compile)
app.get('/view/*', router.view)
app.get('/userprofile', users.userprofile)
app.post('/file_upload', router.file_upload)
app.post('/signinsubmit',users.signinsubmit)
app.post('/signupsubmit',users.signupsubmit)
app.get('/logout', users.logout)
app.get('/deleteshader', users.deleteshader)
app.get('/deletetexture', users.deletetexture)
app.get('/view_user',router.view_user)
app.post('/changepassword',users.changepassword)
app.post('/deleteuser',users.deleteuser)
app.post('/save_shader',router.saveshader)
app.use(function(request, response) {
  response.writeHead(404, { "Content-Type": "text/plain" });
  response.end("404 error!\n");
})

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})