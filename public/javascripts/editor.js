$(function () {
  var editor = ace.edit("editor");
  editor.setTheme("ace/theme/twilight");
  editor.getSession().setMode("ace/mode/javascript");

  $('#navigator').jstree({
"json_data" : {
      // This tree is ajax enabled - as this is most common, and maybe a bit more complex
      // All the options are almost the same as jQuery's AJAX (read the docs)
      "ajax" : {
        // the URL to fetch the data
        "url" : "/editor/filesystem",
        // the `data` function is executed in the instance's scope
        // the parameter is the node being loaded
        // (may be -1, 0, or undefined when loading the root nodes)
        "data" : function (n) {
          // the result is fed to the AJAX request `data` option
          return {
            "repository": $('input[name="repository"]').val(),
            "operation" : "get_children",
            "id" : n.attr ? n.attr("id").replace("node_","") : 1
          };
        }
      }
    },
    "themes" : {
      "theme" : "default",
      "dots" : false,
      "icons" : false
    },
    "plugins" : [ "themes", "json_data" ]
  }).bind("select_node.jstree", function (e, data) {
    alert(data.rslt.obj.data("id"));
  });


  //$.get('/editor/', function(data) {
  //  $('.result').html(data);
    //alert('Load was performed.');
  //});
});