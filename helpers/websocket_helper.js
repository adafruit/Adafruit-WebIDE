var config = require('../config/config');

exports.send_message = function send_message(ws, type, data) {
  if (!ws) {
    ws = config.editor_ws;
  }
  ws.send(JSON.stringify({type: type, data: data}));
}
