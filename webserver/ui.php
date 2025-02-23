<?php
// Display the query UI for the user
if ($requestUri === '/query') {
    ?>
<!DOCTYPE html>
<html>
<head>
    <title>Plugin Query Interface</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            line-height: 1.6;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        .output-box {
            margin-top: 20px;
            padding: 10px;
            background-color: #f5f5f5;
            border: 1px solid #ddd;
            white-space: pre-wrap;
            font-family: monospace;
        }
        .button-group {
            margin-top: 10px;
            margin-bottom: 20px;
        }
        button {
            padding: 8px 16px;
            margin-right: 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        button:disabled {
            background-color: #cccccc;
            cursor: not-allowed;
        }
        button:hover:not(:disabled) {
            background-color: #45a049;
        }
        input[type="text"] {
            padding: 8px;
            margin-right: 10px;
            width: 300px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .status {
            padding: 10px;
            margin: 10px 0;
            background-color: #e7f3fe;
            border-left: 6px solid #2196F3;
        }
        .execution-info {
            margin-top: 20px;
            padding: 10px;
            background-color: #fff3cd;
            border-left: 6px solid #ffc107;
        }
        .files-section {
            margin-top: 20px;
            border-top: 1px solid #ddd;
            padding-top: 20px;
        }
        .file-list {
            list-style: none;
            padding: 0;
        }
        .file-list li {
            padding: 10px;
            margin: 5px 0;
            background-color: #f9f9f9;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .command-output {
            margin-top: 20px;
            padding: 10px;
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 4px;
        }
    </style>
    <script>
        let intervalId;

        function checkForRequest() {
            fetch('/get-plugin-request')
                .then(response => response.json())
                .then(data => {
                    if (data.requested) {
                        document.getElementById('responseField').disabled = false;
                        document.getElementById('submitResponse').disabled = false;
                        document.getElementById('autoDllButton').disabled = false;
                        document.getElementById('status').innerText = "Request received from plugin! Please respond.";
                        clearInterval(intervalId);
                    } else {
                        document.getElementById('status').innerText = "Waiting for a request from the plugin...";
                    }
                })
                .catch(error => console.error("Error checking for plugin request:", error));
        }

        function updateExecutionInfo() {
            fetch('/show-response')
                .then(response => response.json())
                .then(data => {
                    const executionInfo = document.getElementById('executionInfo');
                    let html = '<h3>Last Execution</h3>';

                    if (data.execution_summary) {
                        html += `<p>Type: ${data.execution_summary.type}</p>`;
                        html += `<p>Status: ${data.execution_summary.status}</p>`;
                        html += `<p>Time: ${data.execution_summary.timestamp}</p>`;
                    }

                    if (data.last_command_output) {
                        html += '<div class="command-output">';
                        html += '<h4>Command Output:</h4>';
                        html += `<pre>${data.last_command_output.output}</pre>`;
                        html += '</div>';
                    }

                    executionInfo.innerHTML = html;

                    // Update file list
                    const fileList = document.getElementById('fileList');
                    fileList.innerHTML = '';

                    if (data.files && Object.keys(data.files).length > 0) {
                        Object.entries(data.files).forEach(([filename, details]) => {
                            const li = document.createElement('li');
                            li.innerHTML = `
                                <strong>${filename}</strong><br>
                                <div class="file-details">
                                    Size: ${details.size} bytes<br>
                                    Modified: ${details.modified}<br>
                                    <a href="${details.download_url}" class="download-link">Download</a>
                                </div>
                            `;
                            fileList.appendChild(li);
                        });
                    } else {
                        fileList.innerHTML = '<li>No files available</li>';
                    }
                })
                .catch(error => console.error("Error updating execution info:", error));
        }

        function sendResponse(response = null) {
            const finalResponse = response || document.getElementById('responseField').value.trim();
            if (!finalResponse) {
                alert("Please provide a response.");
                return;
            }

            fetch('/send-plugin-response', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    response: finalResponse,
                    status: "success"
                })
            })
                .then(res => res.text())
                .then(data => {
                    document.getElementById('status').innerText = data;
                    document.getElementById('responseField').disabled = true;
                    document.getElementById('submitResponse').disabled = true;
                    document.getElementById('autoDllButton').disabled = true;
                    intervalId = setInterval(checkForRequest, 2000);
                    setTimeout(updateExecutionInfo, 1000);
                })
                .catch(error => console.error("Error sending response:", error));
        }

        function sendAutoDllResponse() {
            sendResponse('auto');
        }

        window.onload = function () {
            intervalId = setInterval(checkForRequest, 2000);
            setInterval(updateExecutionInfo, 5000);
            updateExecutionInfo();
        };
    </script>
</head>
<body>
    <div class="container">
        <h1>Plugin Query Interface</h1>
        <div class="status" id="status">Waiting for a request...</div>

        <div class="input-section">
            <input type="text" id="responseField" placeholder="Enter DLL URL or 'no'" disabled>
            <div class="button-group">
                <button id="submitResponse" onclick="sendResponse()" disabled>Submit Custom URL</button>
                <button id="autoDllButton" onclick="sendAutoDllResponse()" disabled>Use Local DLL</button>
            </div>
        </div>

        <div id="executionInfo" class="execution-info">
            <h3>Last Execution</h3>
            <p>Loading execution information...</p>
        </div>

        <div class="files-section">
            <h2>Retrieved Files</h2>
            <ul id="fileList" class="file-list">
                <li>Loading files...</li>
            </ul>
        </div>
    </div>
</body>
</html>
    <?php
    exit;
}

// Default landing page
?>
<!DOCTYPE html>
<html>
<head>
    <title>DLL Command Execution Server</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            line-height: 1.6;
            max-width: 800px;
            margin: 0 auto;
        }
        .endpoints {
            margin: 20px 0;
            padding: 20px;
            background: #f5f5f5;
            border-radius: 5px;
        }
        .endpoint {
            margin: 10px 0;
            padding: 10px;
            background: white;
            border-radius: 4px;
            border: 1px solid #ddd;
        }
        .endpoint h3 {
            margin: 0;
            color: #333;
        }
        .endpoint p {
            margin: 5px 0;
            color: #666;
        }
        .summary {
            margin-top: 20px;
            padding: 15px;
            background-color: #e7f3fe;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <h1>DLL Command Execution Server</h1>

    <div class="summary">
        <h2>System Status</h2>
        <p>Upload Directory: <?php echo UPLOAD_DIR; ?></p>
        <p>Files Available: <?php echo count(array_diff(scandir(UPLOAD_DIR), array('.', '..'))); ?></p>
        <p>Last Update: <?php echo date('Y-m-d H:i:s', filemtime(STATE_FILE)); ?></p>
    </div>

    <div class="endpoints">
        <h2>Available Endpoints:</h2>

        <div class="endpoint">
            <h3><a href="/show-response">/show-response</a></h3>
            <p>View the latest response, command output, and file information</p>
        </div>

        <div class="endpoint">
            <h3><a href="/query">/query</a></h3>
            <p>Interactive interface to respond to plugin requests and view files</p>
        </div>

        <div class="endpoint">
            <h3><a href="/get-dll">/get-dll</a></h3>
            <p>Download the command execution DLL</p>
        </div>

        <div class="endpoint">
            <h3>/plugin-request</h3>
            <p>POST endpoint for plugin requests</p>
        </div>

        <div class="endpoint">
            <h3>/get-plugin-request</h3>
            <p>GET endpoint to check for active plugin requests</p>
        </div>

        <div class="endpoint">
            <h3>/send-plugin-response</h3>
            <p>POST endpoint to submit responses and handle file uploads</p>
        </div>

        <div class="endpoint">
            <h3>/download/{filename}</h3>
            <p>Download retrieved files from the server</p>
        </div>
    </div>
</body>
</html><?php
?>
