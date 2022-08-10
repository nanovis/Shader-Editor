
 var Module;
 (async () => {
   Module = {
     preRun: [],
     postRun: [],
     print: (function() {
         return function(text) {
             text = Array.prototype.slice.call(arguments).join(' ');
             console.log(text);
         };
     })(),
     printErr: function(text) {
         text = Array.prototype.slice.call(arguments).join(' ');
         console.error(text);
     },
     canvas: (function() {
         var canvas = document.getElementById('canvas');
 
         return canvas;
     })(),
     setStatus: function(text) {
         console.log("status: " + text);
     },
     monitorRunDependencies: function(left) {
         // no run dependencies to log
     }
   };
   window.onerror = function() {
     console.log("onerror: " + event);
   };
 
 // Initialize the graphics adapter
 {
     const adapter = await navigator.gpu.requestAdapter();
     if (!adapter) {
      console.error('[Shader Editor] GPU Not Found')
      return;
    }
     const device = await adapter.requestDevice();
     Module.preinitializedWebGPUDevice = device;
 }
 
 {
     const js = document.createElement('script');
     js.async = true;
     if (typeof(jsname)!="undefined")
     {js.src = jsname;}
     document.body.appendChild(js);
 }
 })();
function htmlDecode (text){
    var temp = document.createElement("div");
    temp.innerHTML = text;
    var output = temp.innerText || temp.textContent;
      temp = null;
      return output;
}


function  content(texturenum,imgnum)  { 
  var  code=global_popover_code
  if(whether_decode)
  {code=htmlDecode(code)}
  code=code.replace(/texturenum/g,texturenum).replace(/imgnum/g,imgnum);
  var  data  =  $(code); 
  return  data;  
} 

function toggle(targetid){
    if (document.getElementById){
        target=document.getElementById(targetid);
            if (target.style.display=="block"){
                target.style.display="none";
            } else {
                target.style.display="block";
            }
    }
}
function isAssetTypeAnImage(ext) {
    return (['png', 'jpg', 'jpeg', 'bmp'].indexOf(ext.toLowerCase()) !== -1)
}
function upload(){
    el=document.getElementById("image")
    var files = el.files[0]
    if (files!= undefined)
    {
    var index= files.name.lastIndexOf(".");
    var ext = files.name.substr(index+1);
    }
    var formDate = new FormData()
    formDate.set("image",files)
    if (files!= undefined)
    {
      if(isAssetTypeAnImage(ext)==true)
      {$.ajax({
        url: '/file_upload',
        data: formDate,
        type: 'post',
        contentType: false,
        processData: false,
        success: function(res){
            if(res[0]=='fail')
            {
              alert(res[1])
            }
            else
            {
            global_popover_code=res[1];
            whether_decode=false
            alert("upload successfully.")
            }
        }
      })}
      else
      {
        alert("Now we just support .png .jpg .jpeg .bmp")
      }
    }
    else
    {
      alert("please select an image.")
    }
} 

function save(){
    var code=document.getElementById("code").value
    var name=document.getElementById('save_name').value
    var status=document.getElementById('save_select').value
    var formDate = new FormData()
    formDate.set("code",code)
    formDate.set("name",name)
    formDate.set("texture1",document.getElementById("texture1").value)
    formDate.set("texture2",document.getElementById("texture2").value)
    formDate.set("texture3",document.getElementById("texture3").value)
    formDate.set("texture4",document.getElementById("texture4").value)
    formDate.set("status",status)
    
    if (name!= undefined)
    {
      $.ajax({
        url: '/save_shader',
        data: formDate,
        type: 'post',
        contentType: false,
        processData: false,
        success: function(res){

            alert(res)
        }
      })

    }
} 



if(typeof(username)!="undefined")
{
  if(username.length!=0)
  {
  document.getElementById("signin").href="/logout";
  document.getElementById("signin").innerText="Log Out";
  document.getElementById("welcome").href="/userprofile";
  document.getElementById("welcome").innerHTML="Welcome,<B><b>"+username+"</b></B>";
  }
}




