(function( context_menu, $, undefined ) {
  var is_visible = false;

  var templates = {
    "context_menu":               '<ul class="context-menu">' +
                                    '<li class="context-menu-rename">' +
                                      '<a href=""><i class="icon-play"></i> Rename</a>' +
                                    '</li>' +
                                    '<li class="context-menu-delete">' +
                                      '<a href=""><i class="icon-cloud"></i> Delete</a>' +
                                    '</li>' +
                                  '</ul>'
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
    console.log($(this).data('file'));
  }

  function delete_option(event) {
    event.preventDefault();
    console.log($(this));
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