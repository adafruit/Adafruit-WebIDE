(function( webide_utils, $, undefined ) {
    //xmlencode, ansi_colormap, fixConsole is modified from original, but is copyright iPython Development Team, and BSD Licensed: github.com/ipython/ipython
    //Fix raw text to parse correctly in crazy XML
    function xmlencode(string) {
        return string.replace(/\&/g,'&'+'amp;')
            .replace(/</g,'&'+'lt;')
            .replace(/>/g,'&'+'gt;')
            .replace(/\'/g,'&'+'apos;')
            .replace(/\"/g,'&'+'quot;')
            .replace(/`/g,'&'+'#96;');
    }


    //Map from terminal commands to CSS classes
    var ansi_colormap = {
        "30":"ansiblack", "31":"ansired",
        "32":"ansigreen", "33":"ansiyellow",
        "34":"ansiblue", "35":"ansipurple","36":"ansicyan", 
        "37":"ansigrey", "01":"ansibold"
    };

    // Transform ANI color escape codes into HTML <span> tags with css
    // classes listed in the above ansi_colormap object. The actual color used
    // are set in the css file.
    webide_utils.fix_console = function (txt) {
        //console.log(txt);
        txt = txt.replace('[?1034h', '');
        txt = xmlencode(txt);
        var re = /\033\[([\d;]*?)m/;
        var opened = false;
        var cmds = [];
        var opener = "";
        var closer = "";
        
        while (re.test(txt)) {
            cmds = txt.match(re)[1].split(";");
            closer = opened?"</span>":"";
            opened = cmds.length > 1 || cmds[0] !== 0;
            var rep = [];
            for (var i in cmds) {
                if (typeof(ansi_colormap[cmds[i]]) != "undefined") {
                    rep.push(ansi_colormap[cmds[i]]);
                }
            }
            opener = rep.length > 0?"<span class=\""+rep.join(" ")+"\">":"";
            txt = txt.replace(re, closer + opener);
        }
        if (opened) txt += "</span>";
        //console.log(txt.trim());
        return txt.trim();
    };
}( window.webide_utils = window.webide_utils || {}, jQuery ));

//http://paulirish.com/2009/throttled-smartresize-jquery-event-handler/
(function($,sr){
 
  // debouncing function from John Hann
  // http://unscriptable.com/index.php/2009/03/20/debouncing-javascript-methods/
  var debounce = function (func, threshold, execAsap) {
      var timeout;
 
      return function debounced () {
          var obj = this, args = arguments;
          function delayed () {
              if (!execAsap)
                  func.apply(obj, args);
              timeout = null;
          }
 
          if (timeout)
              clearTimeout(timeout);
          else if (execAsap)
              func.apply(obj, args);
 
          timeout = setTimeout(delayed, threshold || 100);
      };
  };
    // smartresize
    jQuery.fn[sr] = function(fn){  return fn ? this.bind('resize', debounce(fn)) : this.trigger(sr); };
 
})(jQuery,'smartresize');