$.fn.filterNode = function(name) {
      return this.find('*').filter(function() {
        return this.nodeName === name;
      });
};
(function( davFS, $, undefined ) {
  //Public Methods
  davFS.listDir = function(path, cb) {
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
      //console.log(data);
      var list = [];
      $(data).filterNode("d:href").each(function(i, result) {
        var result_path = $(result).text();
        var filename = result_path.replace(path, '').replace(/\//g, '');
        list.push({name: filename, path: result_path});
      });
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