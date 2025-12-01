#ifndef WEBSOCKET_HTML_H
#define WEBSOCKET_HTML_H

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Environmental Monitoring Gateway</title>
  <script src="/chart.js"></script>
  <style>
    /* General reset */
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: Arial, sans-serif;
      background: #f0f2f5;
      padding: 15px;
    }

    /* Container styling */
    .container {
      max-width: 800px;
      margin: 0 auto;
      background: white;
      border-radius: 12px;
      box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
      overflow: hidden;
    }

    /* Header styling */
    .header {
      background: #4CAF50;
      color: white;
      padding: 20px;
      text-align: center;
    }

    .header h1 {
      font-size: 22px;
      font-weight: normal;
    }

    /* Status box */
    .status {
      padding: 15px;
      text-align: center;
      font-size: 15px;
      font-weight: bold;
      background: #fff3cd;
      color: #856404;
      border-bottom: 1px solid #e0e0e0;
    }

    .status.connected {
      background: #d4edda;
      color: #155724;
    }

    .section {
      padding: 20px;
      border-bottom: 1px solid #e0e0e0;
    }

    .section:last-child {
      border-bottom: none;
    }

    .section-title {
      font-size: 14px;
      color: #666;
      margin-bottom: 10px;
    }

    /* Table styling */
    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 10px;
    }

    th, td {
      padding: 12px;
      text-align: center;
      font-size: 16px;
    }

    th {
      background-color: #4CAF50;
      color: white;
      font-weight: bold;
    }

    td {
      background-color: #f9f9f9;
    }

    tr:nth-child(even) td {
      background-color: #f1f1f1;
    }

    tr:hover td {
      background-color: #e0e0e0;
    }

    /* Button styling */
    button {
      padding: 12px 20px;
      border: none;
      border-radius: 6px;
      font-size: 15px;
      cursor: pointer;
      font-weight: bold;
    }

    .btn-primary {
      background: #4CAF50;
      color: white;
    }

    .btn-primary:active {
      background: #45a049;
    }

    .btn-danger {
      background: #f44336;
      color: white;
    }

    .btn-danger:active {
      background: #da190b;
    }

    .btn-secondary {
      background: #2196F3;
      color: white;
    }

    .btn-secondary:active {
      background: #0b7dda;
    }

  .status-line {
    text-align: center;
    padding: 15px;
    background: #f9f9f9;
    color: #666;
    font-size: 12px;
  }

  .status-line a {
    color: #4CAF50;
    text-decoration: none;
  }


    /* Responsive styling */
    @media (max-width: 600px) {
      .container {
        border-radius: 0;
      }

      .header h1 {
        font-size: 20px;
      }

      .section-title {
        font-size: 12px;
      }

      th, td {
        font-size: 14px;
      }

      .quick-btns {
        display: flex;
        justify-content: space-between;
        gap: 10px;
        margin-top: 10px;
      }
    }

    /* Chart styling */
    #temperatureChart {
      width: 100%;
      height: 300px;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>Environmental Monitoring Gateway</h1>
    </div>

    <div id="status" class="status">Disconnected</div>

    <div class="section">
      <div class="quick-btns">
        <button onclick="toggleConnection()" id="connBtn" class="btn btn-primary" style="width:49%">Connect</button>
        <button onclick="refreshTemperatures()" class="btn btn-secondary" style="width:49%">Refresh</button>
      </div>
    </div>

    <div class="section">
      <table id="temperatureTable">
        <tr>
          <th>T1</th>
          <td id="temp_1">--- °C</td>
          <th>T2</th>
          <td id="temp_2">--- °C</td>
        </tr>
      </table>
    </div>

    <div class="section">
      <canvas id="temperatureChart" width="400" height="200"></canvas>
    </div>

    <div class="status-line" id="statusLine">Idle</div>
  </div>

  <script>
    var ws = null;
    var connected = false;
    var wsUrl;
    if (window.location.protocol === 'https:') {
      /* When the page is served over HTTPS, switch to a secure WebSocket (wss)
        and use the default WebSocket endpoint `/ws`. */
      wsUrl = 'wss://' + window.location.hostname + '/ws';
    } else {
      /* Default to non-secure WebSocket on port 81 for HTTP. */
      wsUrl = 'ws://' + window.location.hostname + ':81';
    }
    var temp_1 = '--- °C';
    var temp_2 = '--- °C';
    var refresh_interval = 2000; // 2 seconds

    // Global chart variables
    var temperatureChart;
    var device1Temps = [];
    var device2Temps = [];

    function toggleConnection() {
      connected ? disconnect() : connect();
    }

    function connect() {
      updateStatusLine('Connecting...');
      ws = new WebSocket(wsUrl);
      ws.binaryType = 'arraybuffer';
      
      ws.onopen = function() {
        connected = true;
        updateStatus();
        updateStatusLine('Connected!');
      };
      
      ws.onmessage = function(e) {
        const binaryData = new Uint8Array(event.data);
        handleTemperatureData(binaryData); // Process temperature data
      };
      
      ws.onclose = function() {
        connected = false;
        updateStatus();
        updateStatusLine('Disconnected');
      };
      
      ws.onerror = function() {
        updateStatusLine('Connection error');
      };
    }

    function disconnect() {
      if (ws) ws.close();
    }

    function updateStatus() {
      var status = document.getElementById('status');
      var btn = document.getElementById('connBtn');
      
      if (connected) {
        status.textContent = 'Connected';
        status.className = 'status connected';
        btn.textContent = 'Disconnect';
        btn.className = 'btn btn-danger';
      } else {
        status.textContent = 'Disconnected';
        status.className = 'status';
        btn.textContent = 'Connect';
        btn.className = 'btn btn-primary';
      }
    }

    function updateStatusLine(message) {
      var statusLine = document.getElementById('statusLine');
      statusLine.textContent = message;
    }

    function plotTemperatureData(deviceId, temperatures) {
      // Ensure the chart exists or create it
      if (!temperatureChart) {
        const ctx = document.getElementById('temperatureChart').getContext('2d');
        temperatureChart = new Chart(ctx, {
          type: 'scatter',
          data: {
            labels: Array.from({ length: device1Temps.length }, (_, i) => i + 1), // Labels are just index values
            datasets: [
              {
                label: 'T1',
                data: device1Temps,
                borderColor: 'rgb(75, 192, 192)',
                tension: 0.1,
                fill: false,
              },
              {
                label: 'T2',
                data: device2Temps,
                borderColor: 'rgb(255, 99, 132)',
                tension: 0.1,
                fill: false,
              },
            ],
          },
          options: {
            responsive: true,
            plugins: {
              legend: {
                position: 'top',
              },
            },
            scales: {
              x: {
                title: {
                  display: true,
                  text: 'Sample Index',  // Adjust if you need specific time-based labels
                },
              },
              y: {
                min: -20,   // Set minimum value for y-axis
                max: 50,    // Set maximum value for y-axis
                ticks: {
                  stepSize: 5,   // Interval between ticks
                },
                title: {
                  display: true,
                  text: 'Temperature (°C)',
                },
              },
            },
          },
        });
      } else {
        // If chart already exists, just update the data
        temperatureChart.data.labels = Array.from({ length: device1Temps.length }, (_, i) => i + 1);
        temperatureChart.data.datasets[0].data = device1Temps;
        temperatureChart.data.datasets[1].data = device2Temps;
        temperatureChart.update();
      }
    }

    function handleTemperatureData(binaryData) {
      const buffer = binaryData.buffer;
      const dataView = new DataView(buffer);
      let index = 0;

      // Read the device ID (1 byte, uint8)
      const deviceId = dataView.getUint8(index);  // 1 byte
      index += 1;

      // Read the sequence number (2 bytes, uint16)
      const sequenceNumber = dataView.getUint16(index, true);  // 2 bytes, little-endian
      index += 2;

      // Read the sampling interval (4 bytes, uint32)
      const samplingInterval = dataView.getUint32(index, true);  // 4 bytes, little-endian
      index += 4;

      // Read the temperature values (each 4 bytes, float32)
      let temperatures = [];
      while (index < dataView.byteLength) {
        const temp = dataView.getFloat32(index, true); // 4 bytes for float32, little-endian
        temperatures.push(temp);
        index += 4;
      }

      // Update temp_1 or temp_2 based on the Device ID
      if (deviceId === 1) {
        temp_1 = temperatures[temperatures.length - 1].toFixed(2);  // Round to 2 decimal places
        document.getElementById('temp_1').textContent = temp_1 + " °C";

        // Add to device1Temps (for chart)
        device1Temps.push(...temperatures.map(temp => parseFloat(temp.toFixed(2))));
      } else if (deviceId === 2) {
        temp_2 = temperatures[temperatures.length - 1].toFixed(2);  // Round to 2 decimal places
        document.getElementById('temp_2').textContent = temp_2 + " °C";

        // Add to device2Temps (for chart)
        device2Temps.push(...temperatures.map(temp => parseFloat(temp.toFixed(2))));  // Add all temperatures for Device 2
      }

      // Update the chart with the latest data
      plotTemperatureData(deviceId, temperatures);
    }

    function refreshTemperatures() {
      if (connected && ws) {
        ws.send('refresh');
        updateStatusLine('Refreshing temperatures...')
      } else {
        updateStatusLine('Not connected!');
      }
    }

    window.onload = function() {
      updateStatusLine('Ready to connect');
      connect();                              // Auto connect on page load
      setInterval(refreshTemperatures, 2000); // Auto refresh temperatures every 2 seconds
    };
  </script>
</body>
</html>
)rawliteral";

#endif