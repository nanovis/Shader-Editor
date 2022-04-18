var MongoClient =require('mongodb').MongoClient;
var url = 'mongodb://localhost:27017/';

const { check, validationResult } = require('express-validator');


//initialize the database


exports.signinsubmit=function(req,res)
{
    MongoClient.connect(url, function(err, db) {
        if (err) throw err;
        var dbo = db.db("shadereditor");
        var whereStr = {"email":req.body.email,"pwd":req.body.pwd}; 
        dbo.collection("user").find(whereStr).toArray(function(err, result) {
            if (err) throw err;
            db.close();
            if(result.length==0)
            {
                res.send("Username or password is wrong!")
            }
            else
            {
                req.session.username=result[0].username
                req.session.email=result[0].email
                res.send("Login")
            }
        });
    });

};
exports.signupsubmit=function(req,res)
{
    if(req.body.pwd!=req.body.repeatpwd)
    {
        res.send("Please repeat the password correctly.")
    }
    else
    {
    MongoClient.connect(url, function(err, db) {
        if (err) throw err;
        var dbo = db.db("shadereditor");
        var inform = {"username":req.body.username,"email":req.body.email,"pwd":req.body.pwd,"created":Date.now()}; 
        dbo.collection("user").find({$or:[{"email":req.body.email},{"username":req.body.username}]}).toArray(function(err, result) {
            if (err) throw err;
            if(result.length!=0)
            {
                res.send("This email or username is already taken!")
            }
            else
            {
                dbo.collection("user").insertOne(inform, function(err, result) {
                    if (err) throw err;
                    db.close();
                    res.send("success")
                });
            }
        });
    });}

};
exports.logout=function(req,res)
{
    req.session.destroy(function (err) {})
    res.redirect("/")
};
exports.signin=function(req,res)
{
    res.render(__dirname+"/../signin.html")
};
exports.signup=function(req,res)
{
    res.render(__dirname+"/../signup.html")
};
exports.changepassword=function(req,res)
{
    if(req.body.newpwd!=req.body.repeatpwd)
    {
        res.send("Please repeat the new password correctly.")
    }
    else
    {
    MongoClient.connect(url, function(err, db) {
        if (err) throw err;
        var dbo = db.db("shadereditor");
        var inform = {"email":req.body.email,"username":req.session.username,"pwd":req.body.oldpwd}; 
        var updateStr = {$set: { "pwd" : req.body.newpwd }};
        dbo.collection("user").find(inform).toArray(function(err, result) {
            if (err) throw err;
            if(result.length==0)
            {
                res.send("Email address or the ole password is incorrect.")
            }
            else
            {
                dbo.collection("user").updateOne(inform, updateStr, function(err, result) {
                    if (err) throw err;
                    db.close();
                    res.send("success")
                });
            }
        });
    });}
};

exports.deleteuser=function(req,res)
{
    if (req.body.email!=req.session.email)
    {
        res.send("The emal address does not match!")
    }
    else
    {
    MongoClient.connect(url, function(err, db) {
        if (err) throw err;
        var dbo = db.db("shadereditor");
        var inform = {"email":req.body.email,"username":req.session.username}; 
        dbo.collection("user").deleteOne(inform, function(err, result) {
            if (err) throw err;
            db.close();
            req.session.destroy(function (err) {})
            res.redirect("/")
        });
    });}
};
exports.userprofile=function(req,res)
{
  if(req.session.username==undefined)
  {
    res.send("Please first log in")
  }
  else
  {
    MongoClient.connect(url, function(err, db) {
        if (err) throw err;
        var dbo = db.db("shadereditor");
        dbo.collection("shader").find({"user":req.session.username}).sort({"created":1}).toArray(function(err, result) {
            if (err) throw err;
            for(var i=0; i < result.length; i++){
                result[i].id=i+1
             }
            res.render(__dirname+"/../userprofile.html",{username:req.session.username,result:result})
        });
    })
  
  }
};