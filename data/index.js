/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initButton();
}

// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    let data = JSON.parse(event.data);
    document.getElementById('led').className = 'off';
    document.getElementById('ledg').className = 'off';
    document.getElementById('ledb').className = 'off';
    document.getElementById('ledy').className = 'off';
    if (data.status=='red') document.getElementById('led').className = 'on';
    if (data.status=='green') document.getElementById('ledg').className = 'on';
    if (data.status=='blue') document.getElementById('ledb').className = 'on';
    if (data.status=='yellow') document.getElementById('ledy').className = 'on';
    
    
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('led').addEventListener('click', onRed);
    document.getElementById('ledg').addEventListener('click', onGreen);
    document.getElementById('ledb').addEventListener('click', onBlue);
    document.getElementById('ledy').addEventListener('click', onYellow);
}

function onRed(event) {
    websocket.send(JSON.stringify({'action':'red'}));
}

function onGreen(event) {
    websocket.send(JSON.stringify({'action':'green'}));
}

function onBlue(event) {
    websocket.send(JSON.stringify({'action':'blue'}));
}

function onYellow(event) {
    websocket.send(JSON.stringify({'action':'yellow'}));
}