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
    "core": {
      "animation": 100
    },
    "themes" : {
      "theme" : "apple",
      "dots" : false,
      "icons" : false
    },
    "plugins" : [ "themes", "json_data", "ui" ]
  }).bind("select_node.jstree", function (event, data) {
      // `data.rslt.obj` is the jquery extended node that was clicked
      //console.log(data);

      //directory
      if (!data.rslt.obj.attr("id")) {
        data.inst.toggle_node();
      } else {
        openFile(data.rslt.obj.attr("id"), editor);
      }
    });

});

function openFile(path, editor) {
  var EditSession = require("ace/edit_session").EditSession;
  var UndoManager = require("ace/undomanager").UndoManager;
  var url = '/editor/file?path=' + path;
  $.get(url, function(data) {
    var session = new EditSession(data);
    session.setUndoManager(new UndoManager());
    session.setMode("ace/mode/python");
    editor.setSession(session);
    
    //alert('Load was performed.');
  });
}