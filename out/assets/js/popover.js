$(function (){
  $("[data-toggle='popover1']").popover(
    { html: true,
      content: function() {
        return content("\"texture1\"","\"img1\"");
      }
    }
  );
  $("[data-toggle='popover2']").popover(
    { html: true,
      content: function() {
        return content("\"texture2\"","\"img2\"");
      }
    }
  );
  $("[data-toggle='popover3']").popover(
    { html: true,
      content: function() {
        return content("\"texture3\"","\"img3\"");
      }
    }
  );
  $("[data-toggle='popover4']").popover(
    { html: true,
      content: function() {
        return content("\"texture4\"","\"img4\"");
      }
    }
  );
});
