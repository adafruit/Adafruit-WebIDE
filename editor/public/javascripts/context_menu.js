(function( context_menu, $, undefined ) {
  var is_visible = false;

  var templates = {
    "context_menu":               '<ul class="context-menu">' +
                                    '<li class="context-menu-rename">' +
                                      '<a href=""><i class="icon-edit"></i> Rename</a>' +
                                    '</li>' +
                                    '<li class="context-menu-delete">' +
                                      '<a href=""><i class="icon-remove-sign"></i> Delete</a>' +
                                    '</li>' +
                                  '</ul>',
    "rename_file_folder":         '<form class="rename-form" id="rename-file-folder-form">' +
                                      '<div class="rename-input-wrapper">' +
                                        '<a class="rename-submit" href="">Save</a>' +
                                        '<input class="file-name" name="file_name" type="text">' +
                                      '</div>' +
                                    '</form>'
  };


  context_menu.init = function() {
    init_events();
  };

  function init_events() {
    $(document).on('contextmenu', '.navigator-item', show_menu);
    $(document).on('mousedown', close_menu);
    $(document).on('mouseenter', '.context-menu', function() {
      $('.context-menu').addClass('hover');
    }).on('mouseleave', '.context-menu', function() {
      $('.context-menu').removeClass('hover');
    });
  }

  function close_menu() {
    if (!$('.context-menu').hasClass('hover')) {
      $('.context-menu').remove();
    }
  }

  function show_menu() {
    event.preventDefault();
    $(document).off('click', '.context-menu-rename');
    $(document).off('click', '.context-menu-delete');

    $(".context-menu").remove();

    var $menu = $(templates.context_menu);
    $menu.appendTo('body');
    
    $menu.css({'top': event.pageY, 'left': event.pageX - 10});

    // create and show menu      
    $(document).on('click', '.context-menu-rename', $.proxy(rename_option, this));
    $(document).on('click', '.context-menu-delete', $.proxy(delete_option, this));
  }  

  function rename_option(event) {
    event.preventDefault();
    $(".context-menu").remove();
    var $item = $(this);
    $item.data('old', $item.html());
    var file = $item.data('file');
    //console.log($item.data('file'));

    $item.html(templates.rename_file_folder);
    occEditor.handle_navigator_scroll();
    $('.file-name').val(file.name);
    $('.file-name').focus();

    function disable_rename() {
      $(document).off('blur', '.file-name');
      $item.html($item.data('old'));
      occEditor.handle_navigator_scroll();
    }

    function rename_action(event) {
      event.preventDefault();
      $(document).off('submit', '#rename-file-folder-form');
      var file = $item.data('file');
      var new_name = $('.file-name').val();

      occEditor.rename(file, new_name);
    }

    $(document).on('blur', '.file-name', disable_rename);
    $(document).on('submit', '#rename-file-folder-form', rename_action);
  }

  function delete_option(event) {
    event.preventDefault();
    $(".context-menu").remove();

    var socket = occEditor.get_socket();

    var file = $(this).data('file');

    if (file.type === 'directory') {
      davFS.remove(file.path, function(err, status) {
        socket.emit('git-delete', { file: file});
     });
    } else {
      davFS.remove(file.path, function(err, status) {
        socket.emit('git-delete', { file: file});
      });
    }

    occEditor.navigator_remove_item($(this));
  }  

}( window.context_menu = window.context_menu || {}, jQuery ));