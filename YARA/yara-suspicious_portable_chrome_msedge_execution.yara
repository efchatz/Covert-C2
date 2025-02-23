rule Suspicious_Portable_Chrome_Edge_Execution
{
    meta:
        author = "Efstratios Chatzoglou"
        description = "Detects execution of portable Chrome or Edge versions outside official install paths"
        version = "1.0"

    strings:
        $chrome_portable = /C:\\Users\\.*\\Downloads\\.*\\chrome.exe/ nocase
        $chrome_portable2 = /C:\\Users\\.*\\Desktop\\.*\\chrome.exe/ nocase
        $chrome_portable3 = /C:\\Users\\.*\\AppData\\Local\\Temp\\.*\\chrome.exe/ nocase
        $chrome_portable4 = /C:\\Users\\.*\\PortableApps\\ChromePortable\\chrome.exe/ nocase
        $edge_portable = /C:\\Users\\.*\\Downloads\\.*\\msedge.exe/ nocase
        $edge_portable2 = /C:\\Users\\.*\\Desktop\\.*\\msedge.exe/ nocase
        $edge_portable3 = /C:\\Users\\.*\\AppData\\Local\\Temp\\.*\\msedge.exe/ nocase
        $edge_portable4 = /C:\\Users\\.*\\PortableApps\\EdgePortable\\msedge.exe/ nocase

        $suspicious_flags = /--no-sandbox|--disable-web-security|--headless|--remote-debugging-port/ nocase

    condition:
        any of ($chrome_portable*) or any of ($edge_portable*) or $suspicious_flags
}
