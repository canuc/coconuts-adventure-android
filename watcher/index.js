var AWS = require('aws-sdk'); 
var s3 = new AWS.S3({apiVersion: '2006-03-01'}); 
var fs = require('fs');

var params = {Bucket: 'meow-android', Key: 'game.lua'};
const fileToWatch = '../app/src/main/assets/scripts/game.lua';
fs.watchFile(fileToWatch,function(event, filename) {
    console.log("New file!");
    var params = {Bucket: 'meow-android', Key: 'game.lua'};
    params.Body = fs.readFile(fileToWatch,'utf8', function readFileCallback(err, data) {
        params.Body = data;
        s3.putObject(params, function(err, data) {
            console.log(err,data);
        });
    });
})

