var express = require('express')
var users=require('./router/user')
var router=require('./router/routes')
var multer = require('multer')
var bodyParser=require("body-parser")
var hbs = require('hbs')
const { check, validationResult } = require('express-validator');
var session = require('express-session')

var app = express()
var port = 8080
app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())
app.use(session({
  secret: 'keyboard cat',
  resave: false,
  saveUninitialized: true,
  cookie: { secure: true }
}));
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
app.get('/signin', (req, res) => {
  res.render(__dirname+"/signin.html")
})
app.get('/browse', router.browse)
app.get('/about', router.about)
app.get('/signup', (req, res) => {
  res.render(__dirname+"/signup.html")
})
app.post('/compile',router.compile)
app.get('/view/*', router.view)
app.post('/file_upload', router.file_upload)
app.post('/signinsubmit',[check('email').isEmail(),check('pwd').isLength({ min: 6,max:16 })],users.signinsubmit)

app.use(function(request, response) {
  response.writeHead(404, { "Content-Type": "text/plain" });
  response.end("404 error!\n");
})

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})