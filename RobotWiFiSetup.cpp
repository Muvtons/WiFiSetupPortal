#include "RobotWiFiSetup.h"
#include "RingBuffer.h"

// Embedded HTML content (optimized for size)
const char RobotWiFiSetup::INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1" />
<title>Robot WiFi Setup</title>
<style>
:root{
  --bg1:#0f1724; --bg2:#19233a; --accent:#00d4ff; --accent2:#4d4dff;
  --glass: rgba(255,255,255,0.035);
  --muted: rgba(255,255,255,0.65);
  --radius:12px;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial;
}
*{box-sizing:border-box}
html,body{height:100%;margin:0;background:linear-gradient(160deg,var(--bg1),var(--bg2));color:#fff}
.container{max-width:520px;margin:20px auto;padding:18px;background:var(--glass);border-radius:16px;backdrop-filter:blur(8px);box-shadow:0 10px 30px rgba(0,0,0,0.5)}
header{text-align:center;margin-bottom:16px}
h1{font-size:20px;margin-bottom:6px;background:linear-gradient(90deg,var(--accent2),var(--accent));-webkit-background-clip:text;color:transparent;font-weight:700}
.status-pill{display:inline-block;padding:6px 14px;border-radius:30px;font-weight:600;font-size:13px;background:rgba(255,255,255,0.04)}
.card{background:rgba(255,255,255,0.03);padding:14px;border-radius:12px;margin-bottom:12px;border:1px solid rgba(255,255,255,0.03)}
.card-title{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;font-weight:600;color:var(--accent)}
.scan-btn{background:linear-gradient(90deg,var(--accent2),var(--accent));border:none;color:#001;padding:8px 12px;border-radius:8px;font-weight:700;cursor:pointer}
.scan-btn:disabled{opacity:0.6;cursor:not-allowed}
#network-list{max-height:240px;overflow:auto;display:flex;flex-direction:column;gap:8px}
.net-item{display:flex;justify-content:space-between;align-items:center;padding:10px;border-radius:10px;background:rgba(0,0,0,0.12);cursor:pointer;border:1px solid transparent}
.net-item:hover{background:rgba(255,255,255,0.03)}
.net-item.selected{border-color:var(--accent);background:linear-gradient(90deg,rgba(0,212,255,0.06),rgba(77,77,255,0.04))}
.net-left{display:flex;align-items:center;gap:10px;flex:1;min-width:0}
.net-name{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.net-meta{font-size:12px;color:var(--muted);min-width:68px;text-align:right}
.form-row{margin-bottom:10px}
label{display:block;color:var(--muted);font-size:13px;margin-bottom:6px}
input[type="text"],input[type="password"]{width:100%;padding:12px;border-radius:10px;border:1px solid rgba(255,255,255,0.04);background:rgba(0,0,0,0.15);color:#fff;font-size:15px}
.row{display:flex;gap:8px}
.pass-toggle{background:transparent;border:1px solid rgba(255,255,255,0.04);color:var(--muted);padding:10px;border-radius:10px;cursor:pointer}
.btn-connect{width:100%;padding:12px;border-radius:12px;border:none;background:linear-gradient(90deg,var(--accent2),var(--accent));font-weight:800;color:#001;cursor:pointer;margin-top:6px}
.btn-dashboard{width:100%;padding:12px;border-radius:12px;border:none;background:linear-gradient(90deg,#00b09b,#96c93d);font-weight:800;color:#011;margin-top:10px}
.hidden{display:none}
.spinner{width:14px;height:14px;border:2px solid rgba(255,255,255,0.7);border-bottom-color:transparent;border-radius:50%;display:inline-block;animation:rot 0.9s linear infinite;vertical-align:middle;margin-right:6px}
@keyframes rot{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}
.footer{font-size:12px;color:var(--muted);text-align:center;margin-top:6px}
@media (max-width:420px){.container{margin:10px;padding:12px}}
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>🤖 Robot Link</h1>
    <div id="status-pill" class="status-pill">BOOTING</div>
  </header>

  <div id="config-section">
    <div class="card">
      <div class="card-title">
        <div>Available Networks</div>
        <button id="scan-btn" class="scan-btn" onclick="scanNetworks()">↻ Scan</button>
      </div>
      <div id="network-list">
        <div style="text-align:center;color:var(--muted);padding:8px">Press Scan to discover networks</div>
      </div>
      <div class="footer" id="last-scan">Last scan: never</div>
    </div>

    <form id="wifi-form" class="card" onsubmit="submitForm(event)">
      <div class="form-row">
        <label for="ssid">Selected Network (SSID)</label>
        <input id="ssid" type="text" placeholder="Select from list or type..." required />
      </div>

      <div class="form-row">
        <label for="pass">Password</label>
        <div class="row">
          <input id="pass" type="password" placeholder="Enter WiFi password" />
          <button type="button" class="pass-toggle" onclick="togglePass()" title="Show/Hide password">👁</button>
        </div>
      </div>

      <button type="submit" class="btn-connect">Connect Robot</button>
    </form>
  </div>

  <div id="connected-section" class="card hidden" style="text-align:center">
    <h3>✅ Connected!</h3>
    <p style="color:var(--muted)">Robot is online and ready.</p>
    <button id="open-dash" class="btn-dashboard" onclick="openDashboard()">Open Dashboard</button>
  </div>

  <div class="footer">ESP32 Captive Portal — Connect to: <strong>%AP_SSID%</strong></div>
</div>

<script>
let dashboardURL = "%DASHBOARD_URL%";
const statusPill = document.getElementById('status-pill');

function rssiBars(rssi) {
  if (rssi > -55) return '📶📶📶📶';
  if (rssi > -65) return '📶📶📶';
  if (rssi > -75) return '📶📶';
  return '📶';
}

function updateUIStatus(status, connected) {
  let displayStatus = status.replace(/_/g,' ');
  if(displayStatus.length > 20) displayStatus = displayStatus.substring(0,17) + '...';
  
  statusPill.innerText = displayStatus;
  statusPill.className = 'status-pill';
  
  if(status.includes('CONNECTED_OK') || status.includes('CONNECTED_ASSOCIATED')) {
    statusPill.style.background = 'rgba(0,212,255,0.2)';
    statusPill.style.color = '#00d4ff';
  } 
  else if(status.includes('NO_INTERNET')) {
    statusPill.style.background = 'rgba(255,209,102,0.2)';
    statusPill.style.color = '#ffd166';
  }
  else if(status.includes('ERROR') || status.includes('FAIL') || status.includes('INCORRECT')) {
    statusPill.style.background = 'rgba(239,71,111,0.2)';
    statusPill.style.color = '#ef476f';
  }
  else {
    statusPill.style.background = 'rgba(255,255,255,0.04)';
    statusPill.style.color = 'rgba(255,255,255,0.85)';
  }

  if (connected) {
    document.getElementById('config-section').classList.add('hidden');
    document.getElementById('connected-section').classList.remove('hidden');
  } else {
    document.getElementById('config-section').classList.remove('hidden');
    document.getElementById('connected-section').classList.add('hidden');
  }
}

function scanNetworks(){
  const btn = document.getElementById('scan-btn');
  const list = document.getElementById('network-list');
  const last = document.getElementById('last-scan');

  btn.disabled = true;
  btn.innerHTML = '<span class="spinner"></span> Scanning...';
  fetch('/scan').then(r => r.json()).then(data => {
    list.innerHTML = '';
    if (!data.networks || data.networks.length === 0) {
      list.innerHTML = '<div style="text-align:center;color:var(--muted);padding:8px">No networks found</div>';
    } else {
      data.networks.forEach(net => {
        const div = document.createElement('div');
        div.className = 'net-item';
        div.innerHTML = `
          <div class="net-left">
            <div class="net-name">${rssiBars(net.rssi)} ${net.secure ? '🔒' : '🔓'} <span style="margin-left:6px">${escapeHtml(net.ssid)}</span></div>
          </div>
          <div class="net-meta">${net.rssi} dBm</div>
        `;
        div.onclick = () => selectNetwork(net.ssid, div);
        list.appendChild(div);
      });
    }
    last.innerText = 'Last scan: ' + new Date().toLocaleTimeString();
  }).catch(err => {
    console.error('Scan error', err);
    list.innerHTML = '<div style="text-align:center;color:var(--muted);padding:8px">Scan failed</div>';
  }).finally(()=>{
    btn.disabled = false;
    btn.innerText = '↻ Scan';
  });
}

function selectNetwork(ssid, el){
  document.getElementById('ssid').value = ssid;
  document.getElementById('pass').value = '';
  document.getElementById('pass').focus();
  document.querySelectorAll('.net-item').forEach(n => n.classList.remove('selected'));
  el.classList.add('selected');
}

function togglePass(){
  const p = document.getElementById('pass');
  p.type = (p.type === 'password') ? 'text' : 'password';
}

function submitForm(e){
  e.preventDefault();
  const ssid = document.getElementById('ssid').value;
  const pass = document.getElementById('pass').value;
  statusPill.innerText = 'SENDING CREDENTIALS';
  statusPill.style.background = 'rgba(255,255,255,0.04)';
  fetch('/connect', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
  }).then(()=> {
    document.getElementById('pass').value = '';
  }).catch(err=>console.error(err));
}

function openDashboard(){
  window.open(dashboardURL, '_blank');
}

function escapeHtml(unsafe) {
  return unsafe.replace(/[&<"'>]/g, function(m){ return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#039;'}[m]; });
}

const evt = new EventSource('/status');
evt.onmessage = function(e){
  try {
    const data = JSON.parse(e.data);
    if (data.url) dashboardURL = data.url;
    updateUIStatus(data.status || 'UNKNOWN', data.connected || false);
  } catch(err) {
    console.error('SSE parse error', err);
  }
};
evt.onerror = function(){ /* will reconnect automatically */ };
</script>
</body>
</html>
)rawliteral";

// Constructor
RobotWiFiSetup::RobotWiFiSetup() 
    : server(nullptr), 
      numNetworks(0),
      currentStatus("BOOTING"),
      statusType(StatusType::BOOTING),
      isConnected(false),
      robotDashboardURL(DEFAULT_DASHBOARD_URL) {
    // Initialize config with defaults
    config.ap_ssid = DEFAULT_AP_SSID;
    config.ap_password = DEFAULT_AP_PASSWORD;
    config.ap_channel = DEFAULT_AP_CHANNEL;
    config.default_dashboard_url = DEFAULT_DASHBOARD_URL;
    config.captive_portal_url = DEFAULT_CAPTIVE_PORTAL_URL;
    config.enable_captive_portal = true;
    config.server_port = 80;
}

// Destructor
RobotWiFiSetup::~RobotWiFiSetup() {
    if (server) {
        delete server;
        server = nullptr;
    }
}

// Begin the WiFi setup
bool RobotWiFiSetup::begin(const Config& cfg) {
    portENTER_CRITICAL(&coreMutex);
    config = cfg;
    portEXIT_CRITICAL(&coreMutex);
    
    Serial.println("[RobotWiFi] Starting AP and web server...");
    
    // Use AP + Station mode to allow scanning while AP is active
    WiFi.mode(WIFI_AP_STA);
    
    // Start Access Point
    bool apStarted = WiFi.softAP(config.ap_ssid, config.ap_password, config.ap_channel);
    if (!apStarted) {
        Serial.println("[RobotWiFi] FAILED to start AP");
        return false;
    }
    
    apIP = WiFi.softAPIP();
    Serial.printf("[RobotWiFi] AP started: %s | IP: %s\n", config.ap_ssid, apIP.toString().c_str());
    
    // Start DNS server for captive portal (if enabled)
    if (config.enable_captive_portal) {
        dnsServer.start(53, "*", apIP);
        Serial.println("[RobotWiFi] DNS server started for captive portal");
    }
    
    // Initialize web server
    server = new WebServer(config.server_port);
    
    // Register routes
    server->on("/", HTTP_GET, std::bind(&RobotWiFiSetup::handleRoot, this));
    server->on("/connect", HTTP_POST, std::bind(&RobotWiFiSetup::handleConnect, this));
    server->on("/scan", HTTP_GET, std::bind(&RobotWiFiSetup::handleScan, this));
    server->on("/status", HTTP_GET, std::bind(&RobotWiFiSetup::handleStatus, this));
    server->onNotFound(std::bind(&RobotWiFiSetup::handleNotFound, this));
    
    server->begin();
    Serial.println("[RobotWiFi] Web server started");
    
    // Initial status
    portENTER_CRITICAL(&coreMutex);
    currentStatus = "READY_FOR_SETUP";
    statusType = StatusType::BOOTING;
    isConnected = false;
    robotDashboardURL = config.default_dashboard_url;
    portEXIT_CRITICAL(&coreMutex);
    
    return true;
}

// Must be called regularly in loop()
void RobotWiFiSetup::handle() {
    // Process DNS requests for captive portal
    if (config.enable_captive_portal) {
        dnsServer.processNextRequest();
    }
    
    // Handle web server requests
    if (server) {
        server->handleClient();
    }
    
    // Process serial input (non-blocking)
    processSerialInput();
}

// Process serial input from RPi or monitor
void RobotWiFiSetup::processSerialInput() {
    // Read available serial data
    while (Serial.available()) {
        char c = Serial.read();
        
        // Add to buffer (thread-safe)
        if (c == '\n' || c == '\r') {
            // End of command - process buffer
            String command = "";
            char ch;
            while (serialBuffer.read(&ch)) {
                command += ch;
            }
            
            if (command.length() > 0) {
                // Add to command queue
                if (commandBuffer.write(command)) {
                    // Call callback if registered
                    if (commandCallback) {
                        commandCallback(command);
                    }
                    
                    // Process RPi status messages
                    if (command.startsWith("CONNECTED_") || 
                        command.startsWith("PASSWORD_INCORRECT") || 
                        command.startsWith("SSID_NOT_FOUND") || 
                        command.startsWith("CONNECTION_FAILED")) {
                        processRPiStatus(command);
                    }
                }
            }
        } else if (c >= 32 && c <= 126) { // Only printable ASCII
            serialBuffer.write(c);
        }
    }
}

// Process status messages from RPi
void RobotWiFiSetup::processRPiStatus(const String& msg) {
    portENTER_CRITICAL(&coreMutex);
    
    bool wasConnected = isConnected;
    String newStatus = msg;
    StatusType newStatusType = statusType;
    
    if (msg.startsWith("CONNECTED_OK") || msg.startsWith("CONNECTED_ASSOCIATED")) {
        isConnected = true;
        newStatusType = StatusType::CONNECTED_OK;
        
        // Extract URL if present
        int urlPos = msg.indexOf("http");
        if (urlPos != -1) {
            robotDashboardURL = msg.substring(urlPos);
        }
    } 
    else if (msg.startsWith("CONNECTED_NO_INTERNET")) {
        isConnected = true;
        newStatusType = StatusType::CONNECTED_NO_INTERNET;
    } 
    else if (msg.startsWith("PASSWORD_INCORRECT")) {
        isConnected = false;
        newStatusType = StatusType::PASSWORD_INCORRECT;
    }
    else if (msg.startsWith("SSID_NOT_FOUND")) {
        isConnected = false;
        newStatusType = StatusType::SSID_NOT_FOUND;
    }
    else if (msg.startsWith("CONNECTION_FAILED")) {
        isConnected = false;
        newStatusType = StatusType::CONNECTION_FAILED;
    }
    else {
        isConnected = false;
        newStatusType = StatusType::UNKNOWN;
    }
    
    currentStatus = newStatus;
    statusType = newStatusType;
    
    portEXIT_CRITICAL(&coreMutex);
    
    // Notify status changed
    notifyStatusChanged();
    
    // Call status callback if registered
    if (statusCallback) {
        statusCallback(newStatus, isConnected);
    }
}

// Notify that status has changed (for SSE)
void RobotWiFiSetup::notifyStatusChanged() {
    // This is just a placeholder - in a real implementation,
    // this would trigger SSE updates to connected clients
    // For now, the web server handles status requests on demand
}

// Handle web requests
void RobotWiFiSetup::handleRoot() {
    String html = String(INDEX_HTML);
    html.replace("%DASHBOARD_URL%", robotDashboardURL);
    html.replace("%AP_SSID%", config.ap_ssid);
    server->send(200, "text/html", html);
}

void RobotWiFiSetup::handleConnect() {
    if (server->hasArg("ssid") && server->hasArg("pass")) {
        targetSSID = server->arg("ssid");
        targetPassword = server->arg("pass");

        // Send credentials to RPi via serial
        Serial.printf("SSID=%s;PASS=%s\n", targetSSID.c_str(), targetPassword.c_str());
        
        portENTER_CRITICAL(&coreMutex);
        currentStatus = "CREDENTIALS_SENT";
        statusType = StatusType::CREDENTIALS_SENT;
        isConnected = false;
        portEXIT_CRITICAL(&coreMutex);
        
        notifyStatusChanged();
        
        server->send(200, "application/json", "{\"status\":\"Credentials received\"}");
    } else {
        server->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
    }
}

void RobotWiFiSetup::scanNetworks() {
    Serial.println("[RobotWiFi] Scanning for WiFi networks...");
    
    // Do a synchronous scan
    int n = WiFi.scanNetworks();
    numNetworks = (n > MAX_NETWORKS) ? MAX_NETWORKS : n;
    Serial.printf("[RobotWiFi] Found %d networks\n", numNetworks);

    // Clear networks
    for (int i = 0; i < MAX_NETWORKS; i++) {
        networks[i].ssid = "";
        networks[i].rssi = 0;
        networks[i].secure = false;
    }

    // Store networks
    for (int i = 0; i < numNetworks; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
}

void RobotWiFiSetup::handleScan() {
    // Update status
    portENTER_CRITICAL(&coreMutex);
    String oldStatus = currentStatus;
    currentStatus = "SCANNING_NETWORKS";
    statusType = StatusType::SCANNING;
    portEXIT_CRITICAL(&coreMutex);
    notifyStatusChanged();
    
    // Perform scan
    scanNetworks();
    
    // Restore previous status
    portENTER_CRITICAL(&coreMutex);
    currentStatus = oldStatus;
    portEXIT_CRITICAL(&coreMutex);
    notifyStatusChanged();
    
    // Prepare JSON response
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("networks");

    for (int i = 0; i < numNetworks; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["ssid"] = networks[i].ssid;
        obj["rssi"] = networks[i].rssi;
        obj["secure"] = networks[i].secure;
    }

    String json;
    serializeJson(doc, json);
    server->send(200, "application/json", json);
}

void RobotWiFiSetup::handleStatus() {
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, "text/event-stream");
    server->sendContent("retry: 1000\n\n");

    StaticJsonDocument<256> doc;
    
    portENTER_CRITICAL(&coreMutex);
    doc["status"] = currentStatus;
    doc["connected"] = isConnected;
    doc["url"] = robotDashboardURL;
    portEXIT_CRITICAL(&coreMutex);

    String json;
    serializeJson(doc, json);

    server->sendContent("data: " + json + "\n\n");
    server->client().stop();
}

void RobotWiFiSetup::handleNotFound() {
    if (config.enable_captive_portal) {
        // Redirect to captive portal
        String header = "HTTP/1.1 302 Found\r\nLocation: ";
        header += config.captive_portal_url;
        header += "\r\nCache-Control: no-cache\r\n\r\n";
        server->sendContent(header);
        server->client().stop();
    } else {
        server->send(404, "text/plain", "Not found");
    }
}

// Public API methods
void RobotWiFiSetup::setConfig(const Config& cfg) {
    portENTER_CRITICAL(&coreMutex);
    config = cfg;
    portEXIT_CRITICAL(&coreMutex);
}

String RobotWiFiSetup::getStatus() const {
    portENTER_CRITICAL(&coreMutex);
    String status = currentStatus;
    portEXIT_CRITICAL(&coreMutex);
    return status;
}

RobotWiFiSetup::StatusType RobotWiFiSetup::getStatusType() const {
    portENTER_CRITICAL(&coreMutex);
    StatusType type = statusType;
    portEXIT_CRITICAL(&coreMutex);
    return type;
}

bool RobotWiFiSetup::isConnectedToNetwork() const {
    portENTER_CRITICAL(&coreMutex);
    bool connected = isConnected;
    portEXIT_CRITICAL(&coreMutex);
    return connected;
}

String RobotWiFiSetup::getDashboardURL() const {
    portENTER_CRITICAL(&coreMutex);
    String url = robotDashboardURL;
    portEXIT_CRITICAL(&coreMutex);
    return url;
}

String RobotWiFiSetup::getSerialCommand() {
    String command = "";
    commandBuffer.read(&command);
    return command;
}

void RobotWiFiSetup::sendSerialStatus(const String& status) {
    Serial.println(status);
}

void RobotWiFiSetup::registerCommandCallback(CommandCallback callback) {
    commandCallback = callback;
}

void RobotWiFiSetup::registerStatusCallback(StatusCallback callback) {
    statusCallback = callback;
}

int RobotWiFiSetup::getAvailableNetworks(NetworkInfo* outNetworks, int maxNetworks) {
    portENTER_CRITICAL(&coreMutex);
    int count = (numNetworks < maxNetworks) ? numNetworks : maxNetworks;
    
    for (int i = 0; i < count; i++) {
        outNetworks[i] = networks[i];
    }
    
    portEXIT_CRITICAL(&coreMutex);
    return count;
}

const char* RobotWiFiSetup::getStatusTypeString(StatusType type) {
    switch(type) {
        case StatusType::BOOTING: return "BOOTING";
        case StatusType::SCANNING: return "SCANNING";
        case StatusType::CREDENTIALS_SENT: return "CREDENTIALS_SENT";
        case StatusType::CONNECTED_OK: return "CONNECTED_OK";
        case StatusType::CONNECTED_NO_INTERNET: return "CONNECTED_NO_INTERNET";
        case StatusType::PASSWORD_INCORRECT: return "PASSWORD_INCORRECT";
        case StatusType::SSID_NOT_FOUND: return "SSID_NOT_FOUND";
        case StatusType::CONNECTION_FAILED: return "CONNECTION_FAILED";
        default: return "UNKNOWN";
    }
}
