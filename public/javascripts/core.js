$(function () {
  occEditor.init();

  function populateFileSystem(err, list) {
    var ul = $("<ul></ul>");
    ul.appendTo('#navigator');
    $.each(list, function(i, item) {
      if (i === 0) {
        if (item.name === '') {
          $('#navigator-folder p').text('All Repositories');
        } else {
          $('#navigator-top p').text('All Repositories');
          $('#navigator-top p').data("file", item).replaceWith("<p><i class='icon-chevron-left'></i><a href='' class='navigator-item'>" + item.name + "</a></p>");
          $('#navigator-folder p').text(item.name);
        }
      }
      if (i > 0) {
        $("<li></li>")
        .data( "file", item )
        .append("<a href='' class='navigator-item'>" + item.name + "</a><i class='icon-chevron-right'></i>")
        .appendTo(ul);
      }
    });
  }

  davFS.listDir('/filesystem', populateFileSystem);
});