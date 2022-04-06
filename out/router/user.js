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
                res.send("login")
            }
        });
    });

};
