//Ace mode setup code derived from: https://github.com/ajaxorg/ace/tree/master/demo (Thanks!)

(function( occEditor, $, undefined ) {
  var editor, modes = [],
      //socket = io.connect('http://76.17.224.82');
      //socket = io.connect('http://localhost');
      socket = io.connect('http://raspberrypi.local');

  var templates = {
    "editor_bar_init":              '<p><i class="icon-edit"></i> Open a file to the left, to edit and run.</p>',
    "editor_bar_interpreted_file":  '<p class="editor-bar-actions">' +
                                      '<a href="" class="run-file"><i class="icon-play"></i> Save and Run</a>' +
                                      '<a href="" class="save-file"><i class="icon-save"></i> Save</a>' +
                                    '</p>',
    "editor_bar_file":              '<p class="editor-bar-actions">' +
                                      '<a href="" class="save-file"><i class="icon-save"></i> Save</a>' +
                                    '</p>',
    "create_clone_repository":      'Clone a repository by pasting in the full git ssh url found at Bitbucket or Github.<br/><br/>' +
                                    '<span class="small">Example Read-Only: git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git</span><br/>' +
                                    '<span class="small">Example Read-Write: git@bitbucket.org:adafruit/adafruit-raspberry-pi-python-code.git</span><br/><br/>' +
                                    'This will also push the latest version of this repository to your Bitbucket account, if it doesn\'t already exist.<br/><br/>' +
                                    '<form id="clone-repository-form" method="post" action="/create/repository">' +
                                      '<label for="repository_url">Remote Repository URL:</label>' +
                                      '<input name="repository_url" type="text">' +
                                    '</form>'
  };

  occEditor.init = function(id) {
    editor = ace.edit("editor");
    editor.setTheme("ace/theme/merbivore_soft");
    editor.getSession().setMode("ace/mode/python");

    occEditor.populate_navigator();
    occEditor.populate_editor_bar();

    handle_navigator_actions();
    handle_editor_bar_actions();
    handle_program_output();
  };

  occEditor.populate_editor = function(file) {
    var EditSession = require("ace/edit_session").EditSession;
    var UndoManager = require("ace/undomanager").UndoManager;

    function handler(err, data) {
      var session = new EditSession(data);
      session.setUndoManager(new UndoManager());

      var file_mode = getModeFromPath(file.path);
      session.setMode(file_mode.mode);

      editor.setSession(session);
    }
    davFS.read(file.path, handler);
  };

  occEditor.populate_editor_bar = function() {
    var $editor_bar = $('#editor-bar');

    function editor_bar_actions(event, data) {
      //console.log(data);
      if (data.extension === 'py' || data.extension === 'rb') {
        $editor_bar.html(templates.editor_bar_interpreted_file);
      } else {
        $editor_bar.html(templates.editor_bar_file);
      }
    }
    $editor_bar.html(templates.editor_bar_init);
    
    $(document).on('file_open', editor_bar_actions);
  };

  occEditor.populate_navigator = function(path) {
    path = path || '/filesystem';
    function populateFileSystem(err, list) {
      build_navigator_top(list[0]);
      build_navigator_list(list);
      build_navigator_bottom(list[0]);
    }

    davFS.listDir(path, populateFileSystem);
  };

  function build_navigator_top(item) {
    var ul = $(".filesystem").html('');
    //console.log("item.name", item.name);
    if (item.name === 'filesystem') {
      $('#navigator-top p').html('');
      $('#navigator-folder p').text('All Repositories');
    } else {
      var title = "";
      if (item.parent_name === 'filesystem') {
        title = "All Repositories";
      } else {
        title = item.parent_name;
      }
      $('#navigator-top p').addClass('navigator-item-back').data("file", item).html("<a href=''><i class='icon-chevron-left'></i> " + title + "</a>");
      $('#navigator-folder p').text(item.name);
    }
  }

  function build_navigator_list(list) {
    var ul = $(".filesystem").html('');
    $.each(list, function(i, item) {
      if (i > 0) {
        item.id = i + "-item";
        $("<li id='" + i + "-item' class='navigator-item'></li>")
        .data( "file", item )
        .append("<a href=''>" + item.name + "</a><i class='icon-chevron-right'></i>")
        .appendTo(ul);
      }
    });
  }

  function build_navigator_bottom(item) {
    //console.log(item);
    var $link = $('.navigator-item-create a');
    var $create_modal = $('#create-modal');
    if (item.name === 'filesystem') {
      $link.text('+ Clone Repository');
      $('h3', $create_modal).text("Clone Repository");
      $('.modal-body p', $create_modal).html(templates.create_clone_repository);
      $('.modal-submit', $create_modal).text('Clone Repository');
    } else if (item.parent_name === 'filesystem') {
      $link.text('+ Create Project Folder');
    } else {
      $link.text('+ Create New File');
    }
  }

  function handle_editor_bar_actions() {
    function save_file(event) {
      event.preventDefault();
      var file = $('.file-open').data('file');
      var editor_content = editor.getSession().getDocument().getValue();

      function save_callback(err, status) {
        //TODO Handle save Notification
        //console.log(err);
        //console.log(status);
        socket.emit('commit-file', { file: file});
      }

      davFS.write(file.path, editor_content, save_callback);
    }

    function run_file(event) {
      event.preventDefault();
      var file = $('.file-open').data('file');
      var editor_content = editor.getSession().getDocument().getValue();

      function save_run_callback(err, status) {
        //TODO Handle save Notification
        //console.log(err);
        //console.log(status);
        socket.emit('commit-run-file', { file: file});
      }

      davFS.write(file.path, editor_content, save_run_callback);
    }
    $(document).on('click touchstart', '.save-file', save_file);
    $(document).on('click touchstart', '.run-file', run_file);
  }

  function handle_program_output() {
    var i = 0;
    var dragging = false;
    var editor_output_visible = false;

    function show_editor_output() {
      if (!editor_output_visible) {
        editor_output_visible = true;
        $('#editor-output').height('150px');
        $('#dragbar').show();
        $('#editor-output div').css('padding', '10px');
        $('#editor').css('bottom', '153px');
      }
    }

    socket.on('program-stdout', function(data) {
      show_editor_output();
      $('#editor-output div pre').append(data.output);
      $("#editor-output").animate({ scrollTop: $(document).height() }, "slow");
      //console.log(data);
    });
    socket.on('program-stderr', function(data) {
      show_editor_output();
      $('#editor-output div pre').append(data.output);
      $("#editor-output").animate({ scrollTop: $(document).height() }, "slow");
      //console.log(data);
    });
    socket.on('program-exit', function(data) {
      show_editor_output();
      $('#editor-output div pre').append("code: " + data.code + '\n');
      $("#editor-output").animate({ scrollTop: $(document).height() }, "slow");
      //console.log(data);
    });

    /*
     * pane resize inspired by...
     * http://stackoverflow.com/questions/6219031/how-can-i-resize-a-div-by-dragging-just-one-side-of-it
    */
    function handle_dragbar_mousedown(event) {
      event.preventDefault();
      dragging = true;
      var $editor = $('#editor-output-wrapper');
      var ghostbar = $('<div>',
                      {id:'ghostbar',
                       css: {
                              top: $editor.offset().top,
                              left: $editor.offset().left,
                              width: $editor.width()
                             }
                      }).appendTo('body');
      $(document).mousemove(function(event){
        ghostbar.css("top",event.pageY+2);
      });
    }

    function handle_dragbar_mouseup(event) {
      var bottom = $(document).height() - event.pageY;

      if (dragging) {
        $('#editor').css("bottom", bottom + 3);
        $('#editor-output').css("height", bottom);
        $('#ghostbar').remove();
        $(document).unbind('mousemove');
        editor.resize();
        dragging = false;
      }
    }

    $('#dragbar').mousedown(handle_dragbar_mousedown);
    $(document).mouseup(handle_dragbar_mouseup);
  }

  function handle_navigator_actions() {
    function navigator_item_selected(event) {
      event.preventDefault();
      var file = $(this).data('file');
      if (file.type === 'directory') {
        occEditor.populate_navigator(file.path);
      } else {
        $(document).trigger('file_open', file);
        $('.filesystem li').removeClass('file-open');
        $(this).addClass('file-open');
        occEditor.populate_editor(file);
      }
    }

    function navigator_back_selected(event) {
      event.preventDefault();
      var file = $(this).data('file');
      //console.log(file);
      occEditor.populate_navigator(file.parent_path);
    }

    function navigator_create_selected(event) {
      event.preventDefault();
      $('#create-modal').modal('show');
    }

    function create_modal_submit(event) {
      event.preventDefault();
      var $form = $('#create-modal form');

      if ($form.attr('id') === "clone-repository-form") {
        clone_repository($form);
      }

    }
    //clicking a file or folder in the list.
    $(document).on('click touchstart', '.navigator-item', navigator_item_selected);
    $(document).on('click touchstart', '.navigator-item-back', navigator_back_selected);
    $(document).on('click touchstart', '.navigator-item-create', navigator_create_selected);
    $(document).on('click touchstart', '#create-modal .modal-submit', create_modal_submit);
  }

  function clone_repository($form) {
    function handler(err, data, jqXHR) {
      $('.modal-submit').removeClass('disabled');
      if (jqXHR.status === 200) {
        console.log('here');
        $('#create-modal').modal('hide');
        occEditor.populate_navigator();
        occEditor.populate_editor_bar();
      } else {
        $('#clone-repository-form').prepend('<span class="error">' + jqXHR.responseText + '</span>');
      }
      console.log(err);
      console.log(data);
      console.log(jqXHR);
    }

    var request = $.ajax({
      url: $form.attr('action'),
      type: $form.attr('method'),
      dataType: 'html',
      data: $form.serialize(),
      beforeSend: function(xhr) {
        $('.modal-submit').addClass('disabled');
      }
    }).success(function(data, textStatus, jqXHR) {
      handler(null, textStatus, jqXHR);
    }).fail(function(jqXHR, textStatus) {
      handler(textStatus, null, jqXHR);
    });
  }

  function getModeFromPath(path) {
      var mode = modesByName.text;
      for (var i = 0; i < modes.length; i++) {
          if (modes[i].supportsFile(path)) {
              mode = modes[i];
              break;
          }
      }
      return mode;
  }

  var Mode = function(name, desc, extensions) {
      this.name = name;
      this.desc = desc;
      this.mode = "ace/mode/" + name;
      this.extRe = new RegExp("^.*\\.(" + extensions + ")$", "g");
  };

  Mode.prototype.supportsFile = function(filename) {
      return filename.match(this.extRe);
  };

  var modesByName = {
      c9search:   ["C9Search"     , "c9search_results"],
      coffee:     ["CoffeeScript" , "coffee|^Cakefile"],
      coldfusion: ["ColdFusion"   , "cfm"],
      csharp:     ["C#"           , "cs"],
      css:        ["CSS"          , "css"],
      diff:       ["Diff"         , "diff|patch"],
      golang:     ["Go"           , "go"],
      groovy:     ["Groovy"       , "groovy"],
      haxe:       ["haXe"         , "hx"],
      html:       ["HTML"         , "htm|html|xhtml"],
      c_cpp:      ["C/C++"        , "c|cc|cpp|cxx|h|hh|hpp"],
      clojure:    ["Clojure"      , "clj"],
      java:       ["Java"         , "java"],
      javascript: ["JavaScript"   , "js"],
      json:       ["JSON"         , "json"],
      jsx:        ["JSX"          , "jsx"],
      latex:      ["LaTeX"        , "latex|tex|ltx|bib"],
      less:       ["LESS"         , "less"],
      liquid:     ["Liquid"       , "liquid"],
      lua:        ["Lua"          , "lua"],
      luapage:    ["LuaPage"      , "lp"], // http://keplerproject.github.com/cgilua/manual.html#templates
      markdown:   ["Markdown"     , "md|markdown"],
      ocaml:      ["OCaml"        , "ml|mli"],
      perl:       ["Perl"         , "pl|pm"],
      pgsql:      ["pgSQL"        , "pgsql"],
      php:        ["PHP"          , "php|phtml"],
      powershell: ["Powershell"   , "ps1"],
      python:     ["Python"       , "py"],
      ruby:       ["Ruby"         , "ru|gemspec|rake|rb"],
      scad:       ["OpenSCAD"     , "scad"],
      scala:      ["Scala"        , "scala"],
      scss:       ["SCSS"         , "scss|sass"],
      sh:         ["SH"           , "sh|bash|bat"],
      sql:        ["SQL"          , "sql"],
      svg:        ["SVG"          , "svg"],
      tcl:        ["Tcl"          , "tcl"],
      text:       ["Text"         , "txt"],
      textile:    ["Textile"      , "textile"],
      xml:        ["XML"          , "xml|rdf|rss|wsdl|xslt|atom|mathml|mml|xul|xbl"],
      xquery:     ["XQuery"       , "xq"],
      yaml:       ["YAML"         , "yaml"]
  };

  for (var name in modesByName) {
      var mode = modesByName[name];
      mode = new Mode(name, mode[0], mode[1]);
      modesByName[name] = mode;
      modes.push(mode);
  }

}( window.occEditor = window.occEditor || {}, jQuery ));

$(function () {
  occEditor.init();
});