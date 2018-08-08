
var LibraryEmberWebsocket = {
    $EmberWebsocket: {
        SyncSocket: function(ws) {
            this.ws = ws;
            this.msg_queue = [];
            
            var self = this;

            ws.onopen = function() {
                self.msg_queue.push({type:0});
            };

            ws.onmessage = function(msg) {
                self.msg_queue.push({type:1, message:msg.data});
            };
            
            ws.onclose = function(e) {
                self.msg_queue.push({type:2, message:{code:e.code, reason:e.reason}});
            };
        },
        websockets: {},
        websocket_next: 1,
        ws_open: function(addr, port) {
            var ws = new WebSocket("ws://" + addr + ":" + port + "/");
            this.websockets[this.websocket_next] = new this.SyncSocket(ws);
            return this.websocket_next++;
        },
        ws_poll: function(id) {
            var sock = this.websockets[id];
            var msg = sock.msg_queue.shift();
            if (msg === undefined) {
                return null;
            } else {
                return msg;
            }
        },
        ws_send: function(id, msg) {
            var sock = this.websockets[id];
            sock.ws.send(msg);
        },
        ws_close: function(id) {
            var sock = this.websockets[id];
            sock.ws.close();
            delete this.websockets[id];
        }
    },
    
    ember_ws_open: function(addr, port) {
        return EmberWebsocket.ws_open(Module.Pointer_stringify(addr), port);
    },
    ember_ws_poll: function(id) {
        var msg = EmberWebsocket.ws_poll(id);
        if (msg === null) {
            return 0;
        } else {
            var str = JSON.stringify(msg);
            var len = lengthBytesUTF8(str)+1;
            var mem = _malloc(len);
            stringToUTF8(str, mem, len);
            return mem;
        }
    },
    ember_ws_send: function(id, msg) {
        EmberWebsocket.ws_send(id, Module.Pointer_stringify(msg));
    },
    ember_ws_close: function(id) {
        EmberWebsocket.ws_close(id);
    },
};

autoAddDeps(LibraryEmberWebsocket, '$EmberWebsocket');
mergeInto(LibraryManager.library, LibraryEmberWebsocket);
