exports.send_message = function send_message(ws, type, data) {
  ws.send(JSON.stringify({type: type, data: data}));
}
