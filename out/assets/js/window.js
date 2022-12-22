function reportWindowSize()
{
      var obj = document.getElementById("canvas"); 
      var width= window.innerWidth;
      var height=window.innerHeight;
      var new_height=45*screen.height/height;
      var new_width=45*screen.width/width;
      if(new_width>70)
      {
        var factor=new_width/new_height;
        new_width=45;
        new_height=new_width/factor;
      }
      if(new_height>70)
      {
        var factor=new_width/new_height;
        new_height=45;
        new_width=new_height*factor;
      }
      obj.style.height=String(Math.floor(new_height))+"%";
      obj.style.width=String(Math.floor(new_width))+"%";
}
window.onresize = reportWindowSize;