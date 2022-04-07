var MongoClient =require('mongodb').MongoClient;
var url = 'mongodb://localhost:27017/';

const { check, validationResult } = require('express-validator');

exports.signinsubmit=function(req,res)
{
    console.log(req.body.email)
    console.log(req.body.pwd)
    const errors = validationResult(req)
    if (!errors.isEmpty()) {
        return res.status(422).json({ errors: errors.array() })
    }
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
                res.redirect("/")
                //res.render(__dirname+"/../browse.html",{username:req.session.username})
            }
        });
    });

};
exports.signupsubmit=function(req,res)
{
    console.log(req.body.email)
    console.log(req.body.pwd)
    console.log(req.body.username)
    console.log(req.body.repeatpwd)
    const errors = validationResult(req)
    if (!errors.isEmpty()) {
        return res.status(422).json({ errors: errors.array() })
    }
    MongoClient.connect(url, function(err, db) {
        if (err) throw err;
        var dbo = db.db("shadereditor");
        var inform = {"username":req.body.username,"email":req.body.email,"pwd":req.body.pwd,"repeatpwd":req.body.repeatpwd,"created":Date.now()}; 
        dbo.collection("user").find({"email":req.body.email}).toArray(function(err, result) {
            if (err) throw err;
            if(result.length!=0)
            {
                res.send("This email is already taken!")
            }
            else
            {
                dbo.collection("user").insertOne(inform, function(err, res) {
                    if (err) throw err;
                    res.send("register successfully ")
                    db.close();
                });
            }
        });
    });

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