var connection = null;

function log(message) {
    var elem = document.getElementById('log');
    elem.value = elem.value + message + "\n";
}

function connect() {
    if(connection !== null) {
        log("Connection is non-null");
        return;
    }
    log("Connecting");
    connection = new WebSocket("ws://localhost:8080/ws");
    connection.onclose = function(err) { 
        log('Got close: ' + err.reason);
        connection = null;
    }
    connection.onerror = function(err) { 
        log('Got error: ' + err.data);
        connection = null;
    }
    connection.onmessage = function (ev) {
      log('Got data: ' + ev.data);
    }
    connection.onopen = function(ev) {
        log('Got open: ' + ev.data)
    }
}

function disconnect() {
    if(connection == null) {
        log("Not connected");
        return;
    }
    log("Disconnecting");
    connection.close();
    connection = null;
}

function sendmessage() {
    if(connection === null) { log("Connection is not open"); return; }
    var elem = document.getElementById('message');
    log('Sending ' + elem.value);
    connection.send(elem.value);
    elem.value = "";
}
