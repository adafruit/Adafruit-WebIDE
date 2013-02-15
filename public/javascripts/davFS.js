//This rudimentary WebDav client was inspired by http://debris.demon.nl/projects/davclient.js/doc/README.html (Thanks!)

$.fn.filterNode = function(name) {
      return this.find('*').filter(function() {
        return this.nodeName === name;
      });
};
(function( davFS, $, undefined ) {
  //Public Methods
  davFS.listDir = function(path, cb) {
    function filter(item) {
      console.log(item.path);
      var filter_list = ['.git', 'DS_Store', 'pyc', '/filesystem/my-pi-projects/.gitignore'];
      if ($.inArray(item.name, filter_list) !== -1) return true;
      if ($.inArray(item.extension, filter_list) !== -1) return true;
      if ($.inArray(item.path, filter_list) !== -1) return true;
      return false;
    }

    path = path || '/filesystem/';
    var options = {
      url: path,
      type: "PROPFIND",
      dataType: "xml"
    };
    options.header = {
      'Depth': 2,
      'Content-type': 'text/xml; charset=UTF-8'
    };
    _request(options, function(err, data, jqXHR) {
      var list = [], item = {};
      //console.log(data);
      $(data).filterNode("d:response").each(function(i, result) {
        var item ={};

        var result_path = $(result).filterNode('d:href').text();
        var temp_array = result_path.split('/');
        var collection = $(result).filterNode('d:collection');

        item.path = result_path;
        

        if (collection.length) {
          item.type = 'directory';
          item.name = temp_array[temp_array.length - 2];
          item.parent_path = temp_array.slice(0, temp_array.length - 2).join('/');
          item.parent_name = temp_array[temp_array.length - 3];
        } else {
          item.type = 'file';
          item.name = temp_array[temp_array.length - 1];
          item.parent_path = temp_array.slice(0, temp_array.length - 1).join('/');
          item.parent_name = temp_array[temp_array.length - 2];
          item.extension = item.name.split('.').pop();
        }
        if (!filter(item)) {
          list.push(item);
        }
      });
      //console.log(list);
      cb(null, list);
    });
  };

  davFS.mkDir = function(path, cb) {
    var options = {
      url: path,
      type: "MKCOL",
      dataType: "xml"
    };
    _request(options, function(err, data, jqXHR) {
      if (err) cb($(jqXHR.responseXML).filterNode("a:message").text(), false);
      else cb(null, true);
    });
  };

  davFS.read = function(path, cb) {
    var options = {
      url: path,
      type: "GET",
      dataType: "html"
    };
    options.header = {
      'Content-type': 'text/xml; charset=UTF-8'
    };
    _request(options, function(err, data, jqXHR) {
      if (err) cb(err, null);
      else cb(null, data);
    });
  };

  davFS.write = function(path, content, cb) {
    var options = {
      url: path,
      type: "PUT",
      dataType: "xml",
      data: content
    };
    options.header = {
      'Content-type': 'text/xml; charset=UTF-8'
    };
    _request(options, function(err, data, jqXHR) {
      if (err) cb($(jqXHR.responseXML).filterNode("a:message").text(), false);
      else cb(null, true);
    });
  };

  davFS.copy = function(path, to_path, overwrite, cb) {
    var options = {
      url: path,
      type: "COPY",
      dataType: "xml"
    };
    options.header = {
      'Destination': to_path,
      'Overwrite': overwrite ? "T" : "F"
    };
    _request(options, function(err, data, jqXHR) {
      if (err) cb($(jqXHR.responseXML).filterNode("a:message").text(), false);
      else cb(null, true);
    });
  };

  davFS.move = function(path, to_path, overwrite, cb) {
    var options = {
      url: path,
      type: "MOVE",
      dataType: "xml"
    };
    options.header = {
      'Destination': to_path,
      'Overwrite': overwrite ? "T" : "F"
    };
    _request(options, function(err, data, jqXHR) {
      if (err) cb($(jqXHR.responseXML).filterNode("a:message").text(), false);
      else cb(null, true);
    });
  };

  davFS.remove = function(path, cb) {
    var options = {
      url: path,
      type: "DELETE",
      dataType: "xml"
    };
    options.header = {
      'Content-type': 'text/xml; charset=UTF-8'
    };
    _request(options, function(err, data, jqXHR) {
      if (err) cb($(jqXHR.responseXML).filterNode("a:message").text(), false);
      else cb(null, true);
    });
  };
    
  //Private Methods
  function _request(options, handler ) {
    var request = $.ajax({
      url: options.url || '/',
      type: options.type,
      dataType: options.dataType,
      data: options.data || '',
      beforeSend: function(xhr) {
        if (options.header) {
          $.each(options.header, function(i, val) {
            xhr.setRequestHeader(i, val);
          });
        }
      }
    }).success(function(data, textStatus, jqXHR) {
      handler(null, data, jqXHR);
    }).fail(function(jqXHR, textStatus) {
      handler(textStatus, null, jqXHR);
    });
  }
}( window.davFS = window.davFS || {}, jQuery ));