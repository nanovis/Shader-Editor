const express = require('express')
const users = require('./router/user')
const router = require('./router/routes')
const multer = require('multer')
const bodyParser = require("body-parser")
const hbs = require('hbs')
//const { check, validationResult } = require('express-validator');
const session = require('express-session')
//const { route } = require('express/lib/application')
const app = express()
const port = 8080


app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())
app.use(session({
  secret: 'keyboard cat',
  resave: false,
  saveUninitialized: true
}));
app.use(express.static(__dirname))
app.use(express.static(".."))
app.use(express.static(__dirname + "/view"))
app.use(express.static(__dirname + "/texture"))
app.use(express.static(__dirname + "/temp"))
app.set('view engine', 'hbs')
app.engine('hbs', hbs.__express)
app.use(multer({ dest: __dirname + "/temp/" }).array('image'))
app.all('*', router.header)
app.get('/', router.index)
app.get('/new', router.new)
app.get('/signin', users.signin)
app.get('/browse', router.browse)
app.get('/about', router.about)
app.get('/signup', users.signup)
app.post('/compile', router.compile)
app.get('/view', router.view)
app.get('/userprofile', users.userprofile)
app.post('/file_upload', router.file_upload)
app.post('/signinsubmit', users.signinsubmit)
app.post('/signupsubmit', users.signupsubmit)
app.get('/logout', users.logout)
app.get('/deleteshader', users.deleteshader)
app.get('/deletetexture', users.deletetexture)
app.get('/view_user', router.view_user)
app.post('/changepassword', users.changepassword)
app.post('/deleteuser', users.deleteuser)
app.post('/save_shader', router.saveshader)
app.use(function (request, response) {
  response.writeHead(404, { "Content-Type": "text/plain" });
  response.end("404 error!\n");
})

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})
