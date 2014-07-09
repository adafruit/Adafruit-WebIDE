//Ace mode setup code derived from: https://github.com/ajaxorg/ace/tree/master/demo (Thanks!)

(function( occEditor, $, undefined ) {
  var editor, modes = [], max_reconnects = 50,
      socket = io.connect(null, {'reconnection limit': 2000, 'max reconnection attempts': max_reconnects}),
      dirname, updating = false,
      editor_output_visible = false,
      is_terminal_open = false,
      terminal_win,
      job_list, settings = {};

  var templates = {
    "editor_bar_init":              '<p class="editor-bar-actions">' +
                                      '<a href="" class="open-terminal"><i class="icon-list-alt"></i> Terminal</a>' +
                                      '<i class="icon-circle-arrow-left"></i> Open a file to the left to edit and run.' +
                                    '</p>',
    "editor_bar_blank":             '<p class="editor-bar-actions">' +
                                      '<a href="" class="open-terminal"><i class="icon-list-alt"></i> Terminal</a>' +
                                    '</p>',
    "editor_bar_schedule_manager":  '<p class="editor-bar-actions">' +
                                      'Manage your scheduled scripts' +
                                      '<a href="" class="close-schedule-manager"><i class="icon-remove"></i> Close</a>' +
                                    '</p>',
    "editor_bar_settings_manager":  '<p class="editor-bar-actions">' +
                                      'Edit your settings' +
                                      '<span class="saved-setting" style="display: none;"></span>' +
                                      '<a href="" class="close-settings-manager"><i class="icon-remove"></i> Close</a>' +
                                    '</p>',
    "editor_bar_interpreted_file":  '<p class="editor-bar-actions">' +
                                      '<a href="" class="open-terminal"><i class="icon-list-alt"></i> Terminal</a>' +
                                      '<a href="" class="run-file"><i class="icon-play"></i> Run</a>' +
                                      '<a href="" class="debug-file"><i class="icon-cogs"></i> Debug</a>' +
                                      '<a href="" class="trace-file"><i class="icon-sitemap"></i> Visualize</a>' +
                                      '<a href="" class="save-file"><i class="icon-cloud"></i> Save</a>' +
                                      '<a href="" class="schedule-file"><i class="icon-time"></i> Schedule</a>' +
                                    '</p>',
    "editor_bar_debug_file":        '<p class="editor-bar-actions">' +
                                      '<a href="" class="debug-restart"><i class="icon-refresh"></i> Save/Restart</a>' +
                                      '<a href="" class="debug-stop"><i class="icon-list-alt"></i> Exit</a>' +
                                      '<a href="" class="debug-run"><i class="icon-play"></i> Run</a>' +
                                      '<a href="" class="debug-step-over"><i class="icon-cogs"></i> Step Over</a>' +
                                      '<a href="" class="debug-step-in"><i class="icon-sitemap"></i> Step In</a>' +
                                      '<span class="debug-status">Initializing...</a>' +
                                    '</p>',
    "editor_bar_run_link":          '<a href="" class="run-file"><i class="icon-play"></i> Run</a>',
    "editor_bar_make_link":          '<a href="" class="make-file"><i class="icon-wrench"></i> Make</a>',
    "editor_bar_git_link":          '<a href="" class="git-file"><i class="icon-cloud"></i> Commit and Push</a>',
    "editor_bar_debug_link":        '<a href="" class="debug-file"><i class="icon-debug"></i> Debug</a>',
    "editor_bar_trace_link":        '<a href="" class="trace-file"><i class="icon-sitemap"></i> Visualize</a>',
    "editor_bar_schedule_link":     '<a href="" class="schedule-file"><i class="icon-time"></i> Schedule</a>',
    "editor_bar_copy_link":         '<a href="" class="copy-project"><i class="icon-copy"></i> Copy this project to My Pi Projects</a>',
    "editor_bar_tutorial_link":     '<a href="" class="open-tutorial" target="_blank"><i class="icon-book"></i> Project Guide Available</a>',
    "editor_bar_file":              '<p class="editor-bar-actions">' +
                                      '<a href="" class="open-terminal"><i class="icon-list-alt"></i> Terminal</a>' +
                                      '<a href="" class="save-file"><i class="icon-cloud"></i> Save</a>' +
                                    '</p>',
    "update_link":                  '<a href="" class="editor-update-link" target="_blank"><i class="icon-download"></i> Editor Update Available</a>',
    "create_clone_repository":      'Clone a repository by pasting in the full git ssh url found at Bitbucket or Github.<br/><br/>' +
                                    '<span class="small">Example Read-Only: git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git</span><br/>' +
                                    '<span class="small">Example Read-Write: git@bitbucket.org:adafruit/adafruit-raspberry-pi-python-code.git</span><br/><br/>' +
                                    'This will also push the latest version of this repository to your Bitbucket account, if it doesn\'t already exist.<br/><br/>' +
                                    '<form id="clone-repository-form" method="post" action="/create/repository">' +
                                      '<fieldset>' +
                                      '<label for="repository_url">Remote Repository URL:</label>' +
                                      '<input name="repository_url" type="text">' +
                                      '<label class="checkbox">' +
                                        '<input name="retain_remote" type="checkbox"> Clone without changing remote repository to bitbucket [not recommended]' +
                                      '</label>' +
                                      '</fieldset>' +
                                    '</form>',
    "create_project_folder":        '<form class="create-form" id="create-project-form">' +
                                      '<a href="" class="create-cancel"><i class="icon-remove-sign"></i></a>' +
                                      '<label for="folder_name">+ Create Project Folder</label>' +
                                      '<div class="create-input-wrapper">' +
                                        '<a class="create-save" href="">Submit</a>' +
                                        '<input name="folder_name" placeholder="Project Name" type="text">' +
                                      '</div>' +
                                    '</form>',
    "create_nested_folder":        '<form class="create-form" id="create-project-form">' +
                                      '<a href="" class="create-cancel"><i class="icon-remove-sign"></i></a>' +
                                      '<label for="folder_name">+ Create Folder</label>' +
                                      '<div class="create-input-wrapper">' +
                                        '<a class="create-save" href="">Submit</a>' +
                                        '<input name="folder_name" placeholder="Folder Name" type="text">' +
                                      '</div>' +
                                    '</form>',
    "create_file_folder":         '<form class="create-form" id="create-file-form">' +
                                      '<a href="" class="create-cancel"><i class="icon-remove-sign"></i></a>' +
                                      '<label for="file_name">+ Create File</label>' +
                                      '<div class="create-input-wrapper">' +
                                        '<a class="create-save" href="">Submit</a>' +
                                        '<input name="file_name" placeholder="File Name" type="text">' +
                                      '</div>' +
                                    '</form>',
    "upload_file_form":           '<form class="upload-form" id="upload-file-form" action="/editor/upload" enctype="multipart/form-data">' +
                                    '<span class="fileinput-button">' +
                                      '<span>+ Upload File</span>' +
                                      '<input id="fileupload" type="file" name="files[]" data-url="/editor/upload" multiple>' +
                                    '</span>' +
                                  '</form>'
  };

  occEditor.path = null;
  occEditor.is_debug_active = false;

  occEditor.cwd = function() {
    var cwd;
    if (!occEditor.path) {
      occEditor.path = '';
    }
    //console.log(occEditor.path);
    //console.log(dirname);
    return dirname + occEditor.path.replace('/filesystem', '');
  };

  occEditor.init = function(id) {
    occEditor.set_page_title("All Repositories");

    editor = ace.edit("editor");
    editor.setTheme("ace/theme/merbivore_soft");
    editor.getSession().setMode("ace/mode/python");

    occEditor.init_commands(editor);
    occEditor.init_events(editor);

    context_menu.init();

    occEditor.populate_editor_bar();
    occEditor.self_check(function() {
      occEditor.populate_navigator();
      occEditor.open_readme();
    });

    handle_navigator_actions();
    handle_editor_bar_actions();
    handle_footer_actions();
    handle_program_output();
    handle_scheduler_events();
    handle_update_action();
  };

  occEditor.get_socket = function() {
    return socket;
  };

  occEditor.self_check = function(cb) {
    editor_startup("Checking Editor Health");
    socket.emit("self-check-request");
    socket.on("self-check-message", function(message) {
      editor_startup(message);
    });
    socket.on("self-check-settings", function(data) {
      if (data) {
        settings = data;
      }

      editor_startup("Editor settings received");
    });
    socket.on("self-check-complete", function() {
      editor_startup("Editor Health Check Complete");
      cb();
    });
  };

  occEditor.init_commands = function(editor) {
    editor_startup("Initializing Editor Commands");
    var commands = editor.commands;
    commands.addCommand({
        name: "save",
        bindKey: {win: "Ctrl-S", mac: "Command-S"},
        exec: function() {
          if (occEditor.is_debug_active) {
            occEditor.debug_save_restart();
            return;
          }
          occEditor.save_file();
        }
    });
    commands.addCommand({
        name: "run",
        bindKey: {win: "Ctrl-Return", mac: "Command-Return"},
        exec: function() {
          occEditor.run_file();
        }
    });
  };

  occEditor.init_events = function(editor) {
    var reconnect_attempts = 0;

    $(window).bind("beforeunload",function(event) {
      return "Please confirm that you would like to leave the editor.";
    });

    $(document).on('click', '#editor, #editor-bar, #navigator', function() {
      occEditor.focus_terminal(false);
      if (terminal_win) {
        terminal_win.blur();
      }
    });

    editor.on('focus', function() {
      occEditor.focus_terminal(false);
      if (terminal_win) {
        terminal_win.blur();
      }
    });

    editor_startup("Initializing Editor Events");
    editor.on('change', function() {
      var $file_element = $('.filesystem li.file-open');
      var file = $file_element.data('file');
      if (!is_adafruit_project(file.path)) {
        var editor_content = editor.getSession().getDocument().getValue();
        $file_element.data('content', editor_content).addClass('edited');
        $('a', $file_element).css('font-style', 'italic').text(file.name + '*');
      }
    });

    editor.on("guttermousedown", function(e) {
      if (!occEditor.is_debug_active) {
        return;
      }

      var target = e.domEvent.target;
      if (target.className.indexOf("ace_gutter-cell") == -1) {
          return;
      }
      if (!editor.isFocused()) {
          return;
      }
      if (e.clientX > 25 + target.getBoundingClientRect().left) {
          return;
      }

      var row = e.getDocumentPosition().row;
      var breakpoints = e.editor.session.getBreakpoints();

      var file = $('.file-open').data('file');

      if (breakpoints.length > row && breakpoints[row]) {
        e.editor.session.clearBreakpoint(row);
        if (occEditor.is_debug_active) {
          socket.emit('debug-command', {command: "REMOVE_BP", file: file, line_no: (row+1)});
        }
      } else {
        e.editor.session.setBreakpoint(row);
        if (occEditor.is_debug_active) {
          socket.emit('debug-command', {command: "ADD_BP", file: file, line_no: (row+1)});
        }
      }
      e.stop();
    });

    socket.on('connect', function () {
      $('.connection-state').removeClass('disconnected').addClass('connected').text('Connected');
      occEditor.check_for_updates();
      occEditor.load_scheduled_jobs();
    });
    socket.on('disconnect', function () {
      if (updating) {
        $('.connection-state').text('Restarting');
      } else {
        $('.connection-state').removeClass('connected').addClass('disconnected').text('Disconnected');
      }
    });
    socket.on('reconnecting', function () {
      if (!updating) {
        ++reconnect_attempts;

        if(reconnect_attempts >= max_reconnects) {
          $('.connection-state').removeClass('connected').addClass('disconnected').text('Error: check server and refresh browser');
        } else {
          $('.connection-state').removeClass('connected').addClass('disconnected').text('Attempting to reconnect');
        }
      }
    });
    socket.on('cwd-init', function(data) {
      dirname = data.dirname;
    });

    socket.on('commit-file-complete', function(data) {
      if (data.err) {
        occEditor.display_error(data.err);
      }
    });

    socket.on('git-delete-complete', function(data) {
      if (data.err) {
        occEditor.display_error(data.err);
      }
    });

    socket.on('git-push-error', function(data) {
      if (data.err) {
        occEditor.display_error(data.err);
      }
    });

    socket.on('git-pull-complete', function(data) {
      if (data.err) {
        occEditor.display_error(data.err);
      } else {
        occEditor.display_notification("Update Repository Complete");
      }
    });

    socket.on('trace-program-exit', function(data) {
      output = $.parseJSON(data.output);

      if (!output || !output.trace || (output.trace.length === 0) ||
         (output.trace[output.trace.length - 1].event === 'uncaught_exception')) {
        //alert(output.trace[output.trace.length - 1].exception_msg);
        $('#trace-wrapper').hide();
        $('#editor-wrapper').show();
        var error_line = output.trace[0].line - 1;
        var error_msg = output.trace[output.trace.length - 1].exception_msg;
        if (error_line !== undefined && error_msg !== undefined) {
          editor.getSession().setAnnotations([{
            row: error_line,
            text: error_msg,
            type: "error" // also warning and information
          }]);
        }
      } else {
        $('#trace-loader').hide();
        $('#trace-container').show();
        var v = new ExecutionVisualizer("trace-container", output, {});
      }
    });
  };

  occEditor.display_notification = function(text) {
    if (text) {
      $('.connection-state').text(text);

      setTimeout(function() {
        $('.connection-state').text('Connected');
      },5000);
    }
  };

  occEditor.display_error = function(error_text) {
    if (error_text) {
      $('.connection-state').removeClass('connected').addClass('disconnected').text(error_text);

      setTimeout(function() {
        $('.connection-state').removeClass('disconnected').addClass('connected').text('Connected');
      },15000);
    }
  };

  occEditor.check_for_updates = function() {
    socket.emit('editor-check-updates');

    socket.on('editor-update-status', function(data) {
      if (data.has_update) {
        var update_link = $(templates.update_link).append(" (v" + data.version + ")");
        $('.update-wrapper').data('update', data).html(update_link);
      } else {
        $('.update-wrapper').html('');
      }
    });
  };

  occEditor.load_scheduled_jobs = function() {
    socket.on('scheduled-job-list', function(data) {
      job_list = data;
      //console.log(job_list);
    });
  };


  occEditor.set_page_title = function(name) {
    //update page title
    if (name === 'filesystem') {
      name = "All Repositories";
    }
    document.title = name + " - Adafruit Learning System Raspberry Pi WebIDE";
  };

  occEditor.populate_editor = function(file, content) {
    $('#trace-wrapper').hide();
    $('#editor').show();
    $('#schedule-manager').hide();
    $('#editor-wrapper').show();

    if (occEditor.is_debug_active) {
      //hide, and exit the debugger
      occEditor.debug_close();
    }

    var EditSession = require("ace/edit_session").EditSession;
    var UndoManager = require("ace/undomanager").UndoManager;

    function handler(err, data) {
      file.data = data;

      $(document).trigger('file_open', file);
      var session = new EditSession(data);
      session.setUndoManager(new UndoManager());


      if (file.path) {
        var file_mode = getModeFromPath(file.path);
        session.setMode(file_mode.mode);
        occEditor.handle_scheduled_file(file);
      }
      editor.setSession(session);
      editor.setReadOnly(false);

      if (file.read_only) {
        editor.setReadOnly(true);
      }

      //default settings (may get overridden below)
      editor.getSession().setUseSoftTabs(true);
      editor.getSession().setTabSize(4);
      editor.setShowInvisibles(false);

      if (typeof settings !== 'undefined') {
        if (settings.font_size) {
          editor.setFontSize(settings.font_size + "px");
        }

        if (settings.use_soft_tabs) {
          var soft_tabs = settings.use_soft_tabs == "on" ? true : false;
          editor.getSession().setUseSoftTabs(soft_tabs);
        }

        if (settings.tab_size) {
          editor.getSession().setTabSize(parseInt(settings.tab_size, 10));
        }

        if (settings.show_invisibles) {
          var show_invisibles = settings.show_invisibles == "on" ? true : false;
          editor.setShowInvisibles(show_invisibles);
        }
      }

      editor.resize();
      editor.focus();

      editor_startup("Populating Editor");
    }

    if (content) {
      //file has already been opened in this session, and edited
      handler(null, content);
    } else {
      if (is_image(file)) {
        open_image(file);
      } else {
        davFS.read(file.path, handler);
      }
    }

  };

  occEditor.focus_terminal = function(should_focus) {
    if (should_focus) {
      $('.bar').css('background-color', '#2c58bd');
      //$('#editor-output-wrapper').css({ 'opacity' : 1 });
      //$('#editor, #editor-bar').css({ 'opacity' : 0.95 });
    } else {
      $('.bar').css('background-color', '#323233');
      //$('#editor-output-wrapper').css({ 'opacity' : 0.95 });
      //$('#editor, #editor-bar').css({ 'opacity' : 1 });
    }
  };

  /*
   * Populates the editor bar if this is a scheduled file.  Also populates the scheduled input text
   */

  occEditor.handle_scheduled_file = function(file) {
    var is_scheduled_file = false;
    if (!file) {
      return;
    }

    var file_path = file.path.replace('\/filesystem\/', '\/repositories\/');

    //loop through the job list, and check if this file is scheduled, if it is populate the valid DOM elements
    if (job_list && job_list.length) {
      for (var i=0; i<job_list.length; i++) {
        if (job_list[i].path === file_path) {
          $('.schedule-file').html('<i class="icon-time"></i> Scheduled');
          $('input[name="schedule"]').val(job_list[i].text);
          is_scheduled_file = true;

          break;
        }
      }
    }

    //clear out the input text if this isn't a scheduled file
    if (!is_scheduled_file) {
      $('input[name="schedule"]').val("");
    }
  };

  occEditor.clear_editor = function() {
    var EditSession = require("ace/edit_session").EditSession;
    var UndoManager = require("ace/undomanager").UndoManager;
    var session = new EditSession('');
    session.setUndoManager(new UndoManager());
    editor.setSession(session);
    editor.setReadOnly(true);
    //reset editor bar as well
    $('#editor-bar').html(templates.editor_bar_init);
  };

  occEditor.populate_editor_bar = function() {
    editor_startup("Populating Editor Bar");
    var $editor_bar = $('#editor-bar');

    function is_script(extension) {
      return (extension === 'py' || extension === 'rb' || extension === 'js');
    }

    function editor_bar_actions(event, file) {
      //console.log('editor_bar_actions', file);
      if (occEditor.is_debug_active) {
        return;
      }

      if (file.extension) {
        if (is_script(file.extension)) {
          $editor_bar.html(templates.editor_bar_interpreted_file);

          //manually committing and pushing of git files is enabled
          if (settings.offline || settings.manual_git === 'on') {
            $('.save-file i').removeClass().addClass('icon-save');
            socket.emit("git-is-modified", { file: file});

            socket.on("git-is-modified-complete", function(data) {
              socket.removeAllListeners("git-is-modified-complete");
              if (data.is_modified && ($('.git-file').length <= 0)) {
                $(templates.editor_bar_git_link).appendTo('.editor-bar-actions');
              }
            });
          }
        } else {
          $editor_bar.html(templates.editor_bar_file);
        }
      }

      if (file.path) {
        if (is_adafruit_project(file.path)) {
          $editor_bar.html(templates.editor_bar_blank);
          if (is_script(file.extension)) {
            $(templates.editor_bar_run_link).appendTo('.editor-bar-actions');
            $(templates.editor_bar_debug_link).appendTo('.editor-bar-actions');
            //$(templates.editor_bar_trace_link).appendTo('.editor-bar-actions');
          }

          var copy_path;
          if (file.type === 'file') {
            copy_path = file.parent_path;
          } else {
            copy_path = file.path;
          }
          //console.log(file);
          var $copy_link = $(templates.editor_bar_copy_link).attr('href', copy_path);
          $copy_link.appendTo($('.editor-bar-actions'));
        }
      }

      if (file.data) {
        var als_link = file.data.match(/ALS Guide:[ ]?(.*)$/mi);
        if (als_link && als_link[1] && als_link[1].indexOf("learn.adafruit.com") !== -1) {
          var $tutorial_link = $(templates.editor_bar_tutorial_link).attr('href', als_link[1]);
          $tutorial_link.appendTo($('.editor-bar-actions'));
        }
      }

      if (settings.enable_make === 'on') {
        var path = file.parent_path ? file.parent_path : file.path;
        davFS.listDir(path, function(err, list, parent) {
          $.each(list, function(index, f) {
            var make_names = ["makefile", "GNUmakefile", "Makefile"];
            if ($.inArray(f.name, make_names) !== -1) {
              $(templates.editor_bar_make_link).appendTo('.editor-bar-actions');
            }
          });
        });
      }
    }
    $editor_bar.html(templates.editor_bar_init);

    $(document).off('file_open');
    $(document).on('file_open', editor_bar_actions);
  };

  occEditor.populate_navigator = function(path, cb) {
    //console.log(path);
    editor_startup("Populating Navigator");
    occEditor.path = path;
    path = path || '/filesystem';
    function populateFileSystem(err, list, parent) {
      //console.log(list);
      build_navigator_top(parent);
      build_navigator_list(list);
      build_navigator_bottom(parent);
      editor_startup("Navigator Populated", true);

      occEditor.handle_navigator_scroll();

      if (cb) {
        cb();
      }
    }

    occEditor.clear_editor();
    $('#editor').show();
    $('#schedule-manager').hide();

    $(document).trigger('file_open', {path: path});
    davFS.listDir(path, populateFileSystem);
  };

  occEditor.navigator_remove_item = function($element) {
    $element.remove();
    occEditor.handle_navigator_scroll();
  };

  occEditor.open_readme = function() {
    editor_startup("Opening Readme");
    var file = {
      path: '/filesystem/my-pi-projects/README.md',
      read_only: true
    };
    occEditor.populate_editor(file);
  };

  occEditor.save_file = function(event) {
    if (event) {
      event.preventDefault();
    }

    var file = $('.filesystem li.file-open').data('file');

    if (is_adafruit_project(file.path)) {
      //don't allow saving of adafruit project files
      return;
    }

    $('.filesystem li.file-open').removeClass('edited');
    //reset from italic file
    $('.filesystem li.file-open a').css('font-style', 'normal').text(file.name);
    var editor_content = editor.getSession().getDocument().getValue();

    occEditor.save_edited_files(file, editor_content);
    $('.save-file').html('<i class="icon-ok"></i> Saved').delay(100).fadeOut().fadeIn('slow');
    setTimeout(function() {
      $(document).trigger('file_open', file);
    }, 1500);
  };

  occEditor.save_edited_files = function(file, content) {
    function save_callback(err, status) {
      //TODO Handle save Notification
      //console.log(err);
      //console.log(status);

      //$('.save-file i').removeClass('icon-cloud').addClass('icon-ok');
      if (settings.offline || settings.manual_git === 'on') {
        //don't commit and push files
      } else {
        socket.emit('commit-file', { file: file});
      }
    }

    davFS.write(file.path, content, save_callback);
  };

  occEditor.rename = function(item, new_name) {
    var destination_path = item.parent_path + '/' + new_name;
    item.destination = destination_path;

    socket.emit('move-file', { file: item });

    function move_file_callback(data) {
      var item_html = $('.rename-form').parent().data('old');

      if (data.err) {
        $('.rename-form').replaceWith(item_html);

        $('.connection-state').removeClass('connected').addClass('disconnected').text(data.err);

        setTimeout(function() {
          $('.connection-state').removeClass('disconnected').addClass('connected').text('Connected');
        },15000);
      } else {
        item.path = item.destination;
        item.name = new_name;

        $('.rename-form').parent().data('file', item);


        if (item.type === 'file') {
          item_html = $(item_html).html(new_name + '<i class="icon-chevron-right"></i>').attr('title', new_name);
        } else {
          item_html = $(item_html).html(new_name + '<i class="icon-folder-open"></i>').attr('title', new_name);
        }

        $('.rename-form').replaceWith(item_html);
      }

      socket.removeListener('move-file-complete', move_file_callback);
    }

    socket.on('move-file-complete', move_file_callback);
  };

  occEditor.send_terminal_command = function(command) {
    if (is_terminal_open) {
      terminal_win.tabs[0].sendString(command);
      editor.focus();
    }
  };

  occEditor.close_terminal = function() {
    if (is_terminal_open) {
      terminal_win.destroy();
    }
  };

  occEditor.open_terminal = function(path, command) {
    if (is_terminal_open) {
      if (command) {
        terminal_win.tabs[0].sendString(command);
        terminal_win.focus();
      }
      return;
    }
    is_terminal_open = true;

    occEditor.show_editor_output();
    terminal_win = new tty.Window(null, path);

    tty.on('open tab', function(){
      tty.on('tab-ready', function() {
        tty.off('open tab');
        tty.off('tab-ready');

        terminal_win.maximize();
        terminal_win.tabs[0].sizeToFit();

        $(window).smartresize(function() {
          if (is_terminal_open) {
            terminal_win.maximize();
            terminal_win.tabs[0].sizeToFit();
          }
        });

        if (typeof settings !== 'undefined' && settings.font_size) {
          $('.terminal').css('font-size', settings.font_size + "px");
        }

        if (command) {
          terminal_win.tabs[0].sendString(command);
        }
        terminal_win.focus();

      });
    });

    terminal_win.on('focus', function() {
      occEditor.focus_terminal(true);
    });

    tty.on('close window', function() {
      tty.off('close window');
      is_terminal_open = false;
      terminal_win = undefined;
      occEditor.hide_editor_output();
      editor.focus();
    });

    //var maskHeight = $(window).height();
    //var maskWidth = $(window).width();
    //var windowTop =  (maskHeight  - $('.window').height())/2;
    //var windowLeft = (maskWidth - $('.window').width())/2;
    //$('.window').css({ top: windowTop, left: windowLeft, position:"absolute"}).show();
  };


  occEditor.handle_navigator_scroll = function() {
    //pretty ugly, but seems to work in firefox and chrome so far
    var nav_height = $('#navigator').outerHeight();
    var nav_footer_height = $('#navigator-bottom').outerHeight();
    var nav_top_height = $('#navigator-top').outerHeight();
    var nav_folder_height = $('#navigator-folder').outerHeight();
    //minor hack to force an accurate scrollheight;
    $('#navigator ul').height(0);
    var nav_list_height = $('#navigator ul').prop('scrollHeight');
    var possible_height = nav_height - (nav_footer_height + nav_top_height + nav_folder_height);

    if (nav_list_height < possible_height) {
      $('#navigator ul').height(nav_list_height);
    } else {
      $('#navigator ul').height(possible_height - 4);
    }
  };

  function is_image(file) {
    //very, very basic image detection.
    var ext = file.extension;

    if (ext === 'png' || ext === 'jpg' || ext === 'jpeg' || ext === 'gif') {
      return true;
    } else {
      return false;
    }
  }

  function open_image(file) {
    var src = '/editor/image?path=' + file.path;
    $.colorbox({href: src, photo: true, maxWidth: "75%", maxHeight: "75%"});
  }

  function is_adafruit_project(path) {
    var adafruit_root = "/filesystem/Adafruit-Raspberry-Pi-Python-Code/";
    return (path.indexOf(adafruit_root) !== -1 && path !== adafruit_root);
  }

  function is_adafruit_repository(path) {
    var adafruit_root = "/filesystem/Adafruit-Raspberry-Pi-Python-Code/";
    return (path.indexOf(adafruit_root) !== -1);
  }

  function build_navigator_top(item) {
    var $nav_top, ul = $(".filesystem").html('');
    //console.log("item", item);
    if (item.name === 'filesystem') {
      var username = $('input[name="username"]').val();
      $nav_top = $('#navigator-top p').addClass('navigator-item-back').data("file", null).html("<a class='editor-settings' href=''><i class='icon-user'></i> " + username + " (settings)</a>");

      $('#navigator-folder p').html('').text('All Repositories').append('<a href="#" class="refresh-directory" title="Refresh Directory and Repositories"><i class="icon-refresh"></i></a>');
    } else {
      var title = "";
      if (item.parent_name === 'filesystem') {
        title = "All Repositories";
      } else {
        title = item.parent_name;
      }
      $nav_top = $('#navigator-top p').addClass('navigator-item-back').data("file", item).html("<a href=''><i class='icon-chevron-left'></i> " + title + "</a>");

      $('#navigator-folder p').html('').text(item.name).append('<a href="#" class="refresh-directory" title="Refresh Directory"><i class="icon-refresh"></i></a>');
    }
  }

  function build_navigator_list(list) {
    var item_icon, item_name, ul = $(".filesystem").html('');
    $.each(list, function(i, item) {

      if (item.name.length <= 33) {
        item_name = item.name;
        if (item.type === 'file') {
          item_icon = "<i class='icon-chevron-right'></i>";
        } else {
          item_icon = "<i class='icon-folder-open'></i>";
        }
      } else {
        item_icon = "";
        item_name = item.name.slice(0, 30) + "...";
      }
      //if (i > 0) {
        item.id = i + "-item";
        $("<li id='" + i + "-item' class='navigator-item'></li>")
        .data( "file", item )
        .append("<a href='' title='" + item.name + "'>" + item_name + item_icon)
        .appendTo(ul);
      //}
    });
  }

  function attach_file_upload_listener() {
    var file = $('.navigator-item-back').data('file');

    var $upload_span = $('.fileinput-button span');

    $('#fileupload').fileupload({
      dataType: 'json',
      formData: {path: file.path},
      add: function(e, data) {
        $upload_span.text('Uploading File...');
        data.submit();
      },
      done: function (e, data) {
        $upload_span.text('+ Upload File');
        occEditor.populate_navigator(file.path);
      }
    });
  }

  function build_navigator_bottom(item) {
    $('#navigator-bottom').show();

    if (is_adafruit_repository(item.path)) {
      $('#navigator-bottom').hide();
      return;
    }
    //console.log(item);
    $('.create-form').remove();
    $('.navigator-item-create:first-child').html('<a class="create-link"></a>');
    var $create_link = $('.navigator-item-create:first-child .create-link');
    var $create_modal = $('#create-modal');
    if (item.name === 'filesystem') {
      $('.navigator-item-create:nth-child(2)').remove();
      $('.navigator-item-upload').remove();
      $create_link.text('+ Clone Repository');
      $('h3', $create_modal).text("Clone Repository");
      $('.modal-body p', $create_modal).html(templates.create_clone_repository);
      $('.modal-submit', $create_modal).text('Clone Repository');
    } else if (item.parent_name === 'filesystem') {
      $('.navigator-item-create:nth-child(2)').remove();
      $('.navigator-item-upload').remove();
      $create_link.text('+ Create Project Folder');
    } else {
      $create_link.text('+ Create New File');

      if ($('.navigator-item-create:nth-child(2)').length === 0) {
        var $create_folder_link = $('<p class="navigator-item-create"><a class="create-link">+ Create New Folder</a></p>');
        $create_folder_link.appendTo($('#navigator-bottom'));
      }

      if ($('.navigator-item-upload').length === 0) {
        var $upload_form = $('<p class="navigator-item-upload"></p>');
        $upload_form.html(templates.upload_file_form);
        $upload_form.appendTo($('#navigator-bottom'));
      }
      attach_file_upload_listener();
    }
  }

  occEditor.stop_file = function(event) {
    if (event) {
      event.preventDefault();
    }

    var file = $('.file-open').data('file');
    socket.emit('stop-script-execution', { file: file});
    $('.stop-file').html('<i class="icon-play"></i> Run').removeClass('stop-file').addClass('run-file');
  };

  occEditor.trace_file = function(event) {
    event.preventDefault();

    var file = $('.file-open').data('file');
    file.content = editor.getSession().getDocument().getValue();

    function close_trace(event) {
      event.preventDefault();

      $('#trace-wrapper').hide();
      $('#editor-wrapper').show();
      //occEditor.hide_editor_output();
      editor.focus();
      occEditor.populate_editor(file);
      $(document).off('click touchstart', '.close-trace', close_trace);
    }

    $('#editor-wrapper').hide();
    $('#trace-wrapper').show();
    $('#trace-loader').show();
    $('#trace-container').hide();

    socket.emit('trace-file', {file: file});
    
    $(document).on('click touchstart', '.close-trace', close_trace);
  };

  occEditor.debug_toggle_buttons = function(active, completed) {
    if (active) {
      $('.debug-run, .debug-step-over, .debug-step-in, .debug-restart').removeClass('debug-link-disabled');
    } else {
      $('.debug-run, .debug-step-over, .debug-step-in, .debug-restart').addClass('debug-link-disabled');
    }

    if (completed) {
      $('.debug-restart').removeClass('debug-link-disabled');
    }
  };

  //called after hitting ctrl-s or clicking save/restart
  occEditor.debug_save_restart = function(event) {
    if (event) {
      event.preventDefault();
    }
    occEditor.debug_toggle_buttons(false);
    occEditor.debug_message("Initializing...");
    occEditor.save_file();

    $('#variables-wrapper pre').text('');
    $('#pre-wrapper pre').text('');

    var file = $('.file-open').data('file');
    socket.emit('debug-file', {file: file});
  };

  occEditor.debug_message = function(message) {
    $('.debug-status').html(message);
  };

  occEditor.debug_file = function(event) {
    event.preventDefault();

    var file = $('.file-open').data('file');
    var markerId;

    function is_link_active($link) {
      return !$link.hasClass('debug-link-disabled');
    }

    function populate_variables(data) {
      $('#variables-wrapper pre').text("");
      for (var i=0; i<data.length; i++) {
        if (data[i].name && data[i].content && data[i].type) {
          var content = data[i].name + ": " + data[i].content + '\n';
          $('#variables-wrapper pre').append(document.createTextNode(content));
        }

      }
    }

    function debug_file_response(data) {
      occEditor.debug_toggle_buttons(true);
      occEditor.debug_message('Ready');
      //console.log(data);
      if (data.cmd === "NEXT" || data.cmd === "STEP" || data.cmd === "DEBUG" || data.cmd === "RUN") {
        var Range = require("ace/range").Range;
        var rg = new Range(data.line_no - 1, 0, data.line_no, 0);
        editor.session.removeMarker(markerId);
        markerId = editor.session.addMarker(rg, "debug-line", "line", false);
        if (data.locals) {
          populate_variables(data.locals);
        }
        editor.renderer.scrollToLine(data.line_no, true, true, function() {
        });
      } else if (data.cmd === "STDOUT") {
        //console.log(data.content);
        $('#editor-output #pre-wrapper pre').append(document.createTextNode(data.content));
      } else if (data.cmd === "EXCEPTION") {
        if (data.content) {
          for (var d in data.content) {
            $('#editor-output #pre-wrapper pre').append(document.createTextNode(data.content[d]));
          }
        }
        

      } else if (data.cmd === "COMPLETE" || data.cmd === "ERROR_COMPLETE") {
        occEditor.debug_toggle_buttons(false, true);
        var markers = editor.session.getMarkers();
        for (var id in markers) {
          if (markers[id].clazz === "debug-line") {
            editor.session.removeMarker(id);
          }
        }
        
        var message = "";
        if (data.cmd === "COMPLETE") {
          message = "--- Program Completed Successfully ---\n";
        } else {
          message = "--- Program Exited with an Exception ---\n";
        }
        
        $('#editor-output #pre-wrapper pre').append(document.createTextNode(message));
      }

      $("#pre-wrapper").animate({ scrollTop: $(document).height() }, "fast");
      $("#pre-wrapper").scrollTop($(document).height());
      editor.focus();
    }

    function get_breakpoints() {
      var editor_breakpoints = editor.getSession().getBreakpoints();
      var breakpoints = "";

      for (var i = 0; i < editor_breakpoints.length; i++) {
          if (editor_breakpoints[i]) {
            breakpoints = breakpoints + (i+1) + "~";
          }
      }

      return breakpoints;
    }

    function debug_run(event) {
      event.preventDefault();
      if (is_link_active($(this))) {
        occEditor.debug_toggle_buttons(false);
        occEditor.debug_message('Running');

        socket.emit('debug-command', {command: "RUN"});
      }
    }

    function debug_step_over(event) {
      event.preventDefault();
      if (is_link_active($(this))) {
        occEditor.debug_toggle_buttons(false);
        occEditor.debug_message('Stepping');
        //console.log('step over');
        socket.emit('debug-command', {command: "NEXT"});
      }
    }

    function debug_step_in(event) {
      event.preventDefault();
      if (is_link_active($(this))) {
        occEditor.debug_toggle_buttons(false);
        occEditor.debug_message('Stepping');
        //console.log('step in');
        socket.emit('debug-command', {command: "STEP"});
      }
    }


    occEditor.debug_message('Initializing...');
    occEditor.close_terminal();
    occEditor.is_debug_active = true;
    $('#editor-output .outputTitleBar .left-title').html('Debug Output');
    $('#editor-output .outputTitleBar .right-title').html('Debug Variables');
    occEditor.show_editor_output();
    editor.resize();
    $('#editor-bar').html(templates.editor_bar_debug_file);
    socket.emit('debug-file', {file: file});

    //clear out any listeners hanging around before creating a new one for this debug session.
    socket.removeAllListeners('debug-file-response');
    socket.on('debug-file-response', debug_file_response);

    $(document).off('click touchstart', '.debug-run');
    $(document).off('click touchstart', '.debug-step-over');
    $(document).off('click touchstart', '.debug-step-in');
    $(document).off('click touchstart', '.debug-stop');
    $(document).off('click touchstart', '.debug-restart');
    $(document).on('click touchstart', '.debug-step-in', debug_step_in);
    $(document).on('click touchstart', '.debug-step-over', debug_step_over);
    $(document).on('click touchstart', '.debug-run', debug_run);
    $(document).on('click touchstart', '.debug-restart', occEditor.debug_save_restart);
    $(document).on('click touchstart', '.debug-stop', occEditor.debug_close);
    //console.log('debugging');
  };

  occEditor.debug_close = function(event) {
    if (event) {
      event.preventDefault();
    }
    occEditor.is_debug_active = false;
    occEditor.debug_message('Stopping...');
    $('#variables-wrapper pre').text('');
    $('#pre-wrapper pre').text('');
    $('#editor-output-wrapper').hide();

    //$('#editor-wrapper').show();
    socket.emit('debug-command', {command: "QUIT"});
    var markers = editor.session.getMarkers();
    editor.session.removeMarker(markers);
    occEditor.hide_editor_output();
    editor.resize();
    editor.focus();

    var file = $('.file-open').data('file');
    occEditor.populate_editor(file);

    //clean up listeners
    socket.removeAllListeners('debug-file-response');
    $(document).off('click touchstart', '.debug-step-over');
    $(document).off('click touchstart', '.debug-step-in');
    $(document).off('click touchstart', '.debug-run');
    $(document).off('click touchstart', '.debug-stop');
  };

  occEditor.manual_git_file = function(event) {
    event.preventDefault();
    $('#manual-git-modal').modal('show');

    function handle_commit_push(event) {
      $('#manual-git-modal .modal-submit').off('click');
      event.preventDefault();
      var file = $('.file-open').data('file');
      var comment = $('input[name="comment"]').val();
      socket.emit('commit-file', { file: file, message: comment});
      $('.git-file').remove();
      $('#manual-git-modal').modal('hide');
      $('#manual-git-modal .modal-submit').text('Commit and Push');
    }

    $('#manual-git-modal .modal-submit').click(handle_commit_push);
  };

  occEditor.make_file = function(event) {
    event.preventDefault();

    var command = "sudo make run";
    occEditor.open_terminal(occEditor.cwd(), command);
  };

  occEditor.run_file = function(event) {
    if (event) {
      event.preventDefault();
    }

    var file = $('.file-open').data('file');
    var editor_content = editor.getSession().getDocument().getValue();

    function run_callback(err, status) {
      //console.log(err);
      //console.log(status);
      //var command;
      //Running as sudo is temporary.  It's a necessary evil to access GPIO at this point.
      if (file.extension === 'py') {
        command = "sudo python ";
      } else if (file.extension === 'rb') {
        command = "sudo ruby ";
      } else if (file.extension === 'js') {
        command = "sudo node ";
      }
      command += file.name;
      //$('#editor-output div pre').append('------------------------------------------------------------\n');
      //socket.emit('commit-run-file', { file: file});
      occEditor.open_terminal(occEditor.cwd(), command);
    }

    //$('.run-file').html('<i class="icon-remove"></i> Stop').removeClass('run-file').addClass('stop-file');
    //occEditor.show_editor_output();
    davFS.write(file.path, editor_content, run_callback);
  };

  function handle_editor_bar_actions() {

    function open_terminal(event) {
      event.preventDefault();

      occEditor.open_terminal(occEditor.cwd(), null);
    }

    function copy_project(event) {
      $('.copy-project').text("Copying into your project folder...");
      event.preventDefault();

      var source = $(this).attr('href');
      if(source.substr(-1) == '/') {
        //strip trailing slash
        source = source.substr(0, source.length - 1);
      }

      var path_array = source.split('/');
      var directory = path_array[path_array.length - 1];
      var destination = '/filesystem/my-pi-projects/' + directory;
      
      davFS.copy(source, destination, false, function(err, status) {
        socket.emit('commit-file', { file: {path: destination, name: directory}, message: "Copied to my-pi-projects " + directory});
        $('.copy-project').replaceWith($("<span>Project copy completed...</span>"));
      });
    }

    function open_scheduler(event) {
      event.preventDefault();

      function populate_scheduler_input(event) {
        event.preventDefault();

        var schedule = $(this).text();
        $('input[name="schedule"]').val(schedule);
      }

      function submit_schedule(event) {
        event.preventDefault();

        var file = $('.file-open').data('file');
        var schedule_text = $('input[name="schedule"]').val().trim();
        var parsed_schedule = enParser().parse(schedule_text);

        if (!schedule_text.length || parsed_schedule.error !== -1) {

          //found an error parsing, split the schedule string based on where the error occurs
          var schedule_good = schedule_text.slice(0,parsed_schedule.error);
          var schedule_bad = schedule_text.slice(parsed_schedule.error);
          if (!schedule_text.length) {
            schedule_good = "Please add a schedule for your job.";
          }

          $('.scheduler-error').html('Invalid Schedule: ' + schedule_good + '<strong>' + schedule_bad + '</strong>');
        } else {
          //all is good, submit schedule to backend
          socket.emit('submit-schedule', {text: schedule_text, schedule: parsed_schedule, file: file});
          $('#schedule-modal').modal('hide');

          $('.schedule-file').html('<i class="icon-time"></i> Scheduled').delay(100).fadeOut().fadeIn('slow');
        }

      }

      occEditor.handle_scheduled_file($('.file-open').data('file'));

      $('#schedule-modal').modal('show');

      $('#schedule-modal').on('hidden', function () {
        $('#schedule-modal').off('hidden');
        $(document).off('click touchstart', '.scheduler-links');
        $(document).off('click touchstart', '#schedule-modal .modal-submit');
      });

      $(document).on('click touchstart', '.scheduler-links a', populate_scheduler_input);
      $(document).on('click touchstart', '#schedule-modal .modal-submit', submit_schedule);
    }

    $(document).on('click touchstart', '.open-terminal', open_terminal);
    $(document).on('click touchstart', '.copy-project', copy_project);
    $(document).on('click touchstart', '.save-file', occEditor.save_file);
    $(document).on('click touchstart', '.trace-file', occEditor.trace_file);
    $(document).on('click touchstart', '.run-file', occEditor.run_file);
    $(document).on('click touchstart', '.make-file', occEditor.make_file);
    $(document).on('click touchstart', '.git-file', occEditor.manual_git_file);

    $(document).off('click touchstart', '.debug-file', occEditor.debug_file);
    $(document).on('click touchstart', '.debug-file', occEditor.debug_file);

    $(document).on('click touchstart', '.stop-file', occEditor.stop_file);
    $(document).on('click touchstart', '.schedule-file', open_scheduler);
  }

  function handle_footer_actions() {
    function close_schedule_manager(event) {
      event.preventDefault();
      $(document).off('click touchstart', '.close-schedule-manager');
      $(document).off('click touchstart', '.schedule-delete-link');
      $(document).off('click touchstart', '.schedule-toggle-link');

      $('#schedule-manager').hide();
      $('#editor').show();
      
      var file = $('.filesystem li.file-open').data('file');
      if (file) {
        $(document).trigger('file_open', file);
      } else {
        occEditor.populate_editor_bar();
      }

    }

    function delete_scheduled_job(event) {
      event.preventDefault();

      var key = $(this).attr('id');
      socket.emit('schedule-delete-job', key);
      $(this).parents('tr').remove();
    }

    function toggle_scheduled_job(event) {
      var key = $(this).val();
      socket.emit('schedule-toggle-job', key);
    }

    function format_schedule_last_run(date) {
      if (!date.length) return "";

      var d = new Date(date);
      var str = "";
      str += d.getFullYear() + "-";
      str += (d.getMonth() + 1) + "-";
      str += d.getDate() + " ";
      str += d.toLocaleTimeString();
      //console.log(d.getDate());
      return str;
    }

    function show_schedule_manager(event) {
      event.preventDefault();

      var $table = $('<table><tr><th>Activate</th><th>Name</th><th>Frequency</th><th>Path</th><th>Last Run</th><th>Actions</th></tr></table>');

      if (job_list && job_list.length) {
        for (var i=0; i<job_list.length; i++) {
          $('<tr class="spacer"><td></td></tr>').appendTo($table);
          //console.log(job_list[i]);
          var $tr = $('<tr></tr>');
          var checked = "checked";
          if (job_list[i].active == 0) { //intentional double quotes...active is a string
            checked = "";
          }
          $('<td class="schedule-toggle"><input type="checkbox" name="schedule-toggle" value="' + job_list[i].key + '"'+ checked +'></td>').appendTo($tr);
          $('<td>' + job_list[i].name + '</td>').appendTo($tr);
          $('<td>' + job_list[i].text + '</td>').appendTo($tr);
          $('<td>' + job_list[i].path.replace('\/repositories\/', '') + '</td>').appendTo($tr);
          $('<td>' + format_schedule_last_run(job_list[i].last_run) + '</td>').appendTo($tr);
          $('<td><a href="" class="schedule-delete-link" id="' + job_list[i].key + '">delete</a></td>').appendTo($tr);
          $tr.appendTo($table);
        }
      }

      $('#editor').hide();
      $('#schedule-manager').show();
      $('#editor-bar').html(templates.editor_bar_schedule_manager);
      $('#schedule-manager').html($table);

      $(document).on('click touchstart', '.close-schedule-manager', close_schedule_manager);
      $(document).on('click touchstart', '.schedule-delete-link', delete_scheduled_job);
      $(document).on('click touchstart', '.schedule-toggle input', toggle_scheduled_job);
    }

    $(document).on('click touchstart', '.schedule-manager-link', show_schedule_manager);
  }

  function handle_scheduler_events() {
    socket.on('scheduler-start', function(data) {
      //console.log(data);
      $('.schedule-status').text('Initializing Job: ' + data.file.name);
    });
    socket.on('scheduler-executing', function(data) {
      //console.log('scheduler-executing');
      $('.schedule-status').text('Ran Job: ' + data.file.name);
    });
    socket.on('scheduler-error', function(data) {
      //console.log('scheduler-error');
      $('.schedule-status').text('Job Error: ' + data.file.name);
    });
    socket.on('scheduler-exit', function(data) {
      //console.log('scheduler-exit');
      $('.schedule-status').text('Last Run Job: ' + data.file.name);
    });
  }

  function handle_update_action() {
    function load_update_notes() {
      var update_data = $('.update-wrapper').data('update');
      occEditor.populate_editor({name: "notes.md", path: "notes.md", read_only: true}, update_data.notes);
    }

    function update_editor(event) {
      event.preventDefault();
      $(this).hide();
      socket.emit('editor-update');
      $('.connection-state').text('Updating');
      updating = true;
      load_update_notes();
    }

    socket.on('editor-update-download-start', function() {
      $('.connection-state').text('Downloading (~30 seconds)');
    });

    socket.on('editor-update-download-end', function() {
      //console.log(data);
    });

    socket.on('editor-update-unpack-start', function() {
      $('.connection-state').text('Unpacking (~60 seconds)');
    });

    socket.on('editor-update-unpack-end', function() {
      $('.connection-state').text('Restarting (~30 seconds)');
    });

    socket.on('editor-update-complete', function(data) {
      $('.connection-state').text('Update Complete, Refreshing Browser');
      updating = false;

      setTimeout(function() {
        location.reload(true);
      }, 1500);
    });
    
    $(document).on('click touchstart', '.editor-update-link', update_editor);
  }

  occEditor.show_editor_output = function() {
    if (!editor_output_visible) {
      editor_output_visible = true;
      $('#editor-output-wrapper').show();
      $('#editor-output pre').html('');
      $('#editor-output').height('325px');
      $('.dragbar').show();
      $('#editor-output #pre-wrapper, #editor-output #variables-wrapper').css('padding', '10px');
      $('#editor').css('bottom', '328px');
      editor.resize();
    }
  };

  occEditor.hide_editor_output = function() {
    if (editor_output_visible) {
      editor_output_visible = false;
      $('#editor-output').height('0px');
      $('.dragbar').hide();
      $('#editor-output div').css('padding', '0px');
      $('#editor').css('bottom', '0px');
      editor.resize();
    }
  };

  function handle_program_output() {
    var i = 0;
    var dragging = false;
    var buffer = "", buffer_start = false;
    var termOffsetWidth, termOffsetHeight;
    /*
    socket.on('program-stdout', function(data) {
      console.log(data);
      occEditor.show_editor_output();
      $('#editor-output div pre').append(webide_utils.fix_console(data.output));
      $("#editor-output").animate({ scrollTop: $(document).height() }, "fast");
      $("#editor-output").scrollTop($(document).height());
      editor.focus();
      //console.log(data);
    });
    socket.on('program-stderr', function(data) {
      occEditor.show_editor_output();
      $('#editor-output div pre').append(webide_utils.fix_console(data.output));
      //$("#editor-output").animate({ scrollTop: $(document).height() }, "fast");
      $("#editor-output").scrollTop($(document).height());
      editor.focus();
      //console.log(data);
    });
    socket.on('program-exit', function(data) {
      occEditor.show_editor_output();
      $('#editor-output div pre').append('\n\n');
      $('.stop-file').html('<i class="icon-play"></i> Run').removeClass('stop-file').addClass('run-file');
      editor.focus();
      //$('#editor-output div pre').append("code: " + data.code + '\n');
      //$("#editor-output").animate({ scrollTop: $(document).height() }, "slow");
      //console.log(data);
    });*/

    /*
     * pane resize inspired by...
     * http://stackoverflow.com/questions/6219031/how-can-i-resize-a-div-by-dragging-just-one-side-of-it
    */
    function handle_dragbar_mousedown(event) {
      event.preventDefault();
      dragging = true;
      termOffsetWidth = $('.terminal').width();
      termOffsetHeight = $('.terminal').height();
      var $editor = $('#editor-output-wrapper, #progOutputs');
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
        $('#editor, .visualizer').css("bottom", bottom + 3);
        $('#editor-output, #progOutputs').css("height", bottom);
        $('#ghostbar').remove();
        $(document).unbind('mousemove');
        editor.resize();
        dragging = false;
        terminal_win.maximize();
      }
    }

    $('.dragbar').mousedown(handle_dragbar_mousedown);
    $(document).mouseup(handle_dragbar_mouseup);
  }

  function handle_navigator_actions() {
    function alert_changed_file(cb) {

      var $edited_elements = $('.filesystem  li.edited');
      if ($('.filesystem li.edited').length > 0) {
        $('#confirm-modal').removeData("modal").modal({'show': true, 'backdrop': 'static', 'keyboard': false});
        $('#confirm-modal .modal-yes').click(function() {
          $('#confirm-modal .modal-yes').off('click');
          $('#confirm-modal').modal('hide');

          $edited_elements.each(function() {
            var file = $(this).data('file');
            var content = $(this).data('content');
            $(this).removeClass('edited');
            occEditor.save_edited_files(file, content);
          });
          cb(true);
        });

        $('#confirm-modal .modal-no').click(function() {
          $('#confirm-modal .modal-no').off('click');
          $('#confirm-modal').modal('hide');

          cb(true);
        });

        $('#confirm-modal .modal-cancel').click(function() {
          $('#confirm-modal .modal-cancel').off('click');
          $('#confirm-modal').modal('hide');

          cb(false);
        });
      } else {
        cb(true);
      }
    }

    function navigator_delete_item($item) {
      var file = $item.data('file');
      var parent_path = file.parent_path;

      if (file.type === 'directory') {
        davFS.remove(file.path, function(err, status) {
          if (settings.offline || settings.manual_git === 'on') {
            //don't push folders
          } else {
            socket.emit('git-delete', { file: file});
          }
       });
      } else {
        davFS.remove(file.path, function(err, status) {
          if (settings.offline || settings.manual_git === 'on') {
            //don't push folders
          } else {
            socket.emit('git-delete', { file: file});
          }
        });
      }

      $item.remove();
      occEditor.handle_navigator_scroll();
    }

    function navigator_item_selected(event) {
      event.preventDefault();

      if ($('#rename-file-folder-form').length > 0) {
        //we're renaming a file, quit out of here.
        return;
      }

      var file = $(this).data('file'), content;

      occEditor.set_page_title(file.name);

      //user clicked on delete file or folder
      if (event.target.className === 'icon-minus-sign') {
        navigator_delete_item($(this));
        return;
      }

      if (file.type === 'directory') {
        alert_changed_file(function(should_navigate) {
          if (should_navigate) {
            var path = occEditor.cwd() + '/' + file.name;
            path = path.replace('//', '/');
            occEditor.send_terminal_command('cd ' + path);
            occEditor.populate_navigator(file.path);
          }
        });


      } else {
        $('.filesystem li').removeClass('file-open');
        $(this).addClass('file-open');
        if ($(this).hasClass('edited')) {
          content = $(this).data('content');
        }
        occEditor.populate_editor(file, content);
      }
    }

    function view_settings() {
      function set_settings(value) {
        if (typeof settings === 'undefined') {
          settings = {};
        }
        settings = $.extend({}, settings, value);
        //console.log(settings);
        socket.emit("set-settings", value);
        $('.saved-setting').html('<i class="icon-ok"></i> Saved').delay(100).fadeIn('slow').fadeOut();
      }

      function close_settings_manager(event) {
        event.preventDefault();
        $(document).off('click touchstart', '.close-settings-manager');
        $(document).off('click touchstart', '.font-size-value');

        $('#settings-manager').hide();
        $('#editor').show();

        var file = $('.filesystem li.file-open').data('file');
        if (file) {
          $(document).trigger('file_open', file);
        } else {
          occEditor.populate_editor_bar();
        }
      }

      function set_font_size(event) {
        $('.font-size-value').removeClass('selected');
        $(this).addClass('selected');
        set_settings({"font_size": $(this).text()});
      }

      function set_soft_tabs(event) {
        $('.soft-tab-value').removeClass('selected');
        $(this).addClass('selected');
        set_settings({"use_soft_tabs": $(this).text().toLowerCase()});
      }

      function set_tab_size(event) {
        $('.tab-size-value').removeClass('selected');
        $(this).addClass('selected');
        set_settings({"tab_size": $(this).text()});
      }

      function set_show_invisibles(event) {
        $('.invisibles-value').removeClass('selected');
        $(this).addClass('selected');
        set_settings({"show_invisibles": $(this).text().toLowerCase()});
      }

      function set_manual_git(event) {
        $('.manual-git-value').removeClass('selected');
        $(this).addClass('selected');
        set_settings({"manual_git": $(this).text().toLowerCase()});
      }

      if (typeof settings !== 'undefined') {
        if (settings.font_size) {
          $('.font-size-value.' + settings.font_size + 'px').addClass('selected');
        } else {
          $('.font-size-value.12px').addClass('selected');
        }
        if (settings.use_soft_tabs) {
          $('.soft-tab-value.' + settings.use_soft_tabs).addClass('selected');
        } else {
          $('.soft-tab-value.on').addClass('selected');
        }
        if (settings.tab_size) {
          $('.tab-size-value.' + settings.tab_size + "-value").addClass('selected');
        } else {
          $('.tab-size-value.4-value').addClass('selected');
        }
        if (settings.show_invisibles) {
          $('.invisibles-value.' + settings.show_invisibles).addClass('selected');
        } else {
          $('.invisibles-value.off').addClass('selected');
        }
        if (settings.manual_git) {
          $('.manual-git-value.' + settings.manual_git).addClass('selected');
        } else {
          $('.manual-git-value.off').addClass('selected');
        }
      } else {
        $('.font-size-value.12px').addClass('selected');
        $('.soft-tab-value.on').addClass('selected');
        $('.tab-size.4').addClass('selected');
        $('.invisibles-value.off').addClass('selected');
        $('.manual-git-value.off').addClass('selected');
      }

      $('#editor').hide();
      $('#settings-manager').show();
      $('#editor-bar').html(templates.editor_bar_settings_manager);

      $(document).on('click touchstart', '.font-size-value', set_font_size);
      $(document).on('click touchstart', '.soft-tab-value', set_soft_tabs);
      $(document).on('click touchstart', '.tab-size-value', set_tab_size);
      $(document).on('click touchstart', '.invisibles-value', set_show_invisibles);
      $(document).on('click touchstart', '.manual-git-value', set_manual_git);
      $(document).on('click touchstart', '.close-settings-manager', close_settings_manager);
    }

    function navigator_back_selected(event) {
      event.preventDefault();

      var that = this;

      if ($('a', this).hasClass("editor-settings")) {
        view_settings();
        return;
      }

        alert_changed_file(function(should_navigate) {
          if (should_navigate) {
            var file = $('a', that).parent().data('file');

            occEditor.set_page_title(file.parent_name);

            //console.log(file);
            var path = dirname + file.parent_path.replace('/filesystem', '');
            occEditor.send_terminal_command('cd ' + path);
            occEditor.populate_navigator(file.parent_path);
          }
        });
    }

    function navigator_refresh(event) {
      event.preventDefault();
      var parent = $('.navigator-item-back').data("file");

      var path = parent ? parent.path : null;

      occEditor.populate_navigator(path);
    }

    function navigator_create_selected(event) {
      event.preventDefault();
      var link_text = $('a', this).text();

      if (/repository/i.test(link_text)) {
        $('#create-modal').modal('show');
      } else if (/project/i.test(link_text)) {
        $(this).data('link', $(this).html()).html(templates.create_project_folder);
        $('input[name="folder_name"]').focus();
      } else if (/folder/i.test(link_text)) {
        $(this).data('link', $(this).html()).html(templates.create_nested_folder);
        $('input[name="folder_name"]').focus();
      } else if (/file/i.test(link_text)) {
        $(this).data('link', $(this).html()).html(templates.create_file_folder);
        $('input[name="file_name"]').focus();
      }
      occEditor.handle_navigator_scroll();
    }

    function create_modal_submit(event) {
      event.preventDefault();
      var $form = $('#create-modal form');

      if ($form.attr('id') === "clone-repository-form") {
        clone_repository($form);
      }
    }

    function create_cancel(event) {
      var $parent = $(this).closest('p');
      create_replace($parent);
    }

    function create_replace($element) {
      var link = $element.data('link');
      $element.replaceWith('<p class="navigator-item-create">' + link + '</p>');
      occEditor.handle_navigator_scroll();
    }

    function create_fs_response(err, status, $create_wrapper, item) {
      var folder_name = $('input[name="folder_name"]').val();
      var parent_folder = $('.navigator-item-back').data("file");

      $('.create-save').text('Submit');
      $('.create-input-wrapper input').prop('disabled', false);

      if (err) {
        if (!$create_wrapper.find('.error').length) {
          $create_wrapper.prepend($('<span class="error">' + err + '</span>'));
        } else {
          $('.error', $create_wrapper).replaceWith($('<span class="error">' + err + '</span>'));
        }
        occEditor.handle_navigator_scroll();
      } else {
        create_replace($create_wrapper);

        occEditor.set_page_title(item.name);

        if (item.type === 'file') {
          occEditor.populate_navigator(parent_folder.path, function() {
            var file = $('.filesystem li a[title="' + item.name + '"]').closest('li').addClass('file-open').data('file');
            occEditor.populate_editor(file);
            occEditor.populate_editor_bar();
          });
        } else {
          var path = occEditor.cwd() + '/' + item.name;
          path = path.replace('//', '/');
          occEditor.send_terminal_command('cd ' + path);

          occEditor.populate_navigator(item.path);
        }

        occEditor.populate_editor_bar();
      }
    }

    function create_folder(event) {
      event.preventDefault();
      $('.create-save').text('Working');
      $('.create-input-wrapper input').prop('disabled', true);

      var $create_wrapper = $(this).closest('.navigator-item-create');
      var folder_name = $('input[name="folder_name"]').val();
      folder_name = folder_name.replace(" ", "_");
      var parent_folder = $('.navigator-item-back').data("file");
      var path = parent_folder.path + folder_name;
      var item = {path: path, name: folder_name, type: "folder"};

      davFS.mkDir(path, function(err, status) {
        if (settings.offline || settings.manual_git === 'on') {
          //don't push folders
        } else {
          socket.emit('commit-file', { file: item });
        }
        create_fs_response(err, status, $create_wrapper, item);
      });
    }

    function create_file(event) {
      event.preventDefault();
      $('.create-save').text('Working');
      $('.create-input-wrapper input').prop('disabled', true);

      var $create_wrapper = $(this).closest('.navigator-item-create');
      var file_name = $('input[name="file_name"]').val();
      file_name = file_name.replace(" ", "_");
      var parent_folder = $('.navigator-item-back').data("file");
      var path = parent_folder.path + file_name;
      var file = {path: path, name: file_name, type: "file"};

      davFS.write(parent_folder.path + file_name, '', function(err, status) {
        if (settings.offline || settings.manual_git === 'on') {
          //don't push files
        } else {
          socket.emit('commit-file', { file: file});
        }

        create_fs_response(err, status, $create_wrapper, file);
      });
    }

    //clicking a file or folder in the list.
    $(document).on('click touchstart', '.navigator-item', navigator_item_selected);
    $(document).on('click touchstart', '.navigator-item-back', navigator_back_selected);
    $(document).on('click touchstart', '.navigator-item-create', navigator_create_selected);
    $(document).on('click touchstart', '#navigator-folder .refresh-directory', navigator_refresh);
    $(document).on('click touchstart', '#create-modal .modal-submit', create_modal_submit);
    $(document).on('click touchstart', '#create-project-form .create-save', create_folder);
    $(document).on('click touchstart', '#create-project-form .create-cancel', create_cancel);
    $(document).on('click touchstart', '#create-file-form .create-save', create_file);
    $(document).on('click touchstart', '#create-file-form .create-cancel', create_cancel);
    $(document).on('submit', '#create-project-form', create_folder);
    $(document).on('submit', '#create-file-form', create_file);
  }


  function editor_startup(string, is_complete) {
    //$('.connection-state').html(string);
    $('#editor-startup').append($('<p>' + string + '</p>'));
    if (is_complete) {
      $('#editor-startup').hide();
      $('#editor-container').show();
    }
  }

  function clone_repository($form) {
    function handler(err, data, jqXHR) {
      $('.modal-submit').removeClass('disabled');
      if (jqXHR.status === 200) {
        $('#create-modal').modal('hide');
        occEditor.populate_navigator();
        occEditor.populate_editor_bar();
      } else {
        $('#clone-repository-form').prepend('<span class="error">' + jqXHR.responseText + '</span>');
      }
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
      abap:       ["ABAP",        , "abap"],
      actionscript:["ActionScript",  "as"],
      ada:        ["ADA"          , "ada|adb"],
      apacheconf: ["Apache_Conf"  , "^htaccess|^htgroups|^htpasswd|^conf|htaccess|htgroups|htpasswd"],
      asciidoc:   ["AsciiDoc"     , "asciidoc"],
      assemblyx86:["Assembly_x86" , "asm"],
      autohot:    ["AutoHotKey"   , "ahk"],
      batchfile:  ["BatchFile"    , "bat|cmd"],
      c9search:   ["C9Search"     , "c9search_results"],
      coffee:     ["coffee"       , "coffee|cf|cson|^Cakefile"],
      coldfusion: ["ColdFusion"   , "cfm"],
      csharp:     ["CSharp"       , "cs"],
      css:        ["CSS"          , "css"],
      curly:      ["Curly"        , "curly"],
      d:          ["D"            , "d|di"],
      dart:       ["Dart"         , "dart"],
      diff:       ["Diff"         , "diff|patch"],
      docker:     ["Dockerfile"   , "^Dockerfile"],
      dot:        ["Dot"          , "dot"],
      ejs:        ["EJS"          , "ejs"],
      forth:      ["Forth"        , "frt|fs|ldr"],
      ftl:        ["FTL"          , "ftl"],
      gherkin:    ["Gherkin"      , "feature"],
      glsl:       ["Glsl"         , "glsl|frag|vert"],
      golang:     ["golang"       , "go"],
      groovy:     ["Groovy"       , "groovy"],
      haml:       ["HAML"         , "haml"],
      handlebars: ["Handlebars"   , "hbs|handlebars|tpl|mustache"],
      haskell:    ["Haskell"      , "hs"],
      haxe:       ["haXe"         , "hx"],
      html:       ["HTML"         , "htm|html|xhtml"],
      html_ruby:  ["HTML_Ruby"    , "erb|rhtml|html.erb"],
      c_cpp:      ["C_Cpp"        , "c|cc|cpp|cxx|h|hh|hpp"],
      cirru:      ["Cirru"        , "cirru|cr"],
      clojure:    ["Clojure"      , "clj|cljs"],
      ini:        ["INI"          , "ini|conf|cfg|prefs"],
      jack:       ["JACK"         , "jack"],
      jade:       ["Jade"         , "jade"],
      java:       ["Java"         , "java"],
      erlang:     ["Erlang"       , "erl|hrl"],
      javascript: ["JavaScript"   , "js"],
      json:       ["JSON"         , "json"],
      jsoniq:     ["JSONiq"       , "jq"],
      jsp:        ["JSP"          , "jsp"],
      jsx:        ["JSX"          , "jsx"],
      julia:      ["Julia"        , "julia"],
      latex:      ["LaTeX"        , "latex|tex|ltx|bib"],
      less:       ["LESS"         , "less"],
      liquid:     ["Liquid"       , "liquid"],
      lisp:       ["Lisp"         , "lisp"],
      livescript: ["LiveScript"   , "ls"],
      logiql:     ["LogiQL"       , "logic|lql"],
      lsl:        ["LSL"          , "lsl"],
      lua:        ["Lua"          , "lua"],
      luapage:    ["LuaPage"      , "lp"], // http://keplerproject.github.com/cgilua/manual.html#templates
      lucene:     ["Lucene"       , "lucene"],
      makefile:   ["Makefile"     , "^Makefile|^GNUmakefile|^makefile|^OCamlMakefile|make"],
      matlab:     ["MATLAB"       , "matlab"],
      markdown:   ["Markdown"     , "md|markdown"],
      mel:        ["MEL"          , "mel"],
      mysql:      ["MySQL"        , "mysql"],
      mushcode:   ["MUSHCode"     , "mc|mush"],
      nix:        ["Nix"          , "nix"],
      objectivec: ["ObjectiveC"   , "m|mm"],
      ocaml:      ["OCaml"        , "ml|mli"],
      pascal:     ["Pascal"       , "pas|p"],
      perl:       ["Perl"         , "pl|pm"],
      pgsql:      ["pgSQL"        , "pgsql"],
      php:        ["PHP"          , "php|phtml"],
      powershell: ["Powershell"   , "ps1"],
      prolog:     ["Prolog"       , "plg|prolog"],
      properties: ["Properties"   , "properties"],
      protobuf:   ["Protobuf"     , "proto"],
      python:     ["Python"       , "py"],
      r:          ["R"            , "r"],
      rdoc:       ["RDoc"         , "Rd"],
      rhtml:      ["RHTML"        , "Rhtml"],
      ruby:       ["Ruby"         , "ru|gemspec|rake|rb"],
      rust:       ["Rust"         , "rs"],
      sass:       ["SASS"         , "sass"],
      scad:       ["SCAD"         , "scad"],
      scala:      ["Scala"        , "scala"],
      smarty:     ["Smarty"       , "smarty|tpl"],
      scheme:     ["Scheme"       , "scm|rkt"],
      scss:       ["SCSS"         , "scss|sass"],
      sh:         ["SH"           , "sh|bash|bat"],
      sjs:        ["SJS"          , "sjs"],
      space:      ["Space"        , "space"],
      snippets:   ["snippets"     , "snippets"],
      soy_tmplt:  ["Soy_Template" , "soy"],
      sql:        ["SQL"          , "sql"],
      stylus:     ["Stylus"       , "styl|stylus"],
      svg:        ["SVG"          , "svg"],
      tcl:        ["Tcl"          , "tcl"],
      tex:        ["Tex"          , "tex"],
      text:       ["Text"         , "txt"],
      textile:    ["Textile"      , "textile"],
      toml:       ["Toml"         , "toml"],
      twig:       ["Twig"         , "twig"],
      typescript: ["Typescript"   , "ts|typescript|str"],
      vala:       ["Vala"         , "vala"],
      vbscript:   ["VBScript"     , "vbs"],
      velocity:   ["Velocity"     , "vm"],
      verilog:    ["Verilog"      , "v|vh|sv|svh"],
      xml:        ["XML"          , "xml|rdf|rss|wsdl|xslt|atom|mathml|mml|xul|xbl"],
      xquery:     ["XQuery"       , "xq"],
      yaml:       ["YAML"         , "yaml|yml"]
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
  tty.open();
});
