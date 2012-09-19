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

    $(".context-menu").remove();

    var $menu = $(templates.context_menu);
    $menu.appendTo('body');
    console.log(event.pageY);
    $menu.css({'top': event.pageY, 'left': event.pageX - 10});

    // create and show menu      
    $(document).on('click', '.context-menu-rename', $.proxy(rename_option, this));
    $(document).on('click', '.context-menu-delete', $.proxy(delete_option, this));
  }  

  function rename_option(event) {
    event.preventDefault();
    console.log($(this).data('file'));
  }

  function delete_option(event) {
    event.preventDefault();
        console.log($(this).data('file'));
  }  

}( window.context_menu = window.context_menu || {}, jQuery ));