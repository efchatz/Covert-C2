rule Suspicious_Task_Executes_Browser_Shortcut
{
    meta:
        author = "Efstratios Chatzoglou"
        description = "Detects Task Scheduler executing a .lnk shortcut that runs chrome.exe or msedge.exe"
        version = "1.0"

    strings:
        $task_executes_lnk = /schtasks.*\/Create.*\/TN.*\.lnk/ nocase
        $chrome_exec = /C:\\.*\\chrome.exe/ nocase
        $edge_exec = /C:\\.*\\msedge.exe/ nocase
        $lnk_exec = /C:\\.*\.lnk/ nocase
        $suspicious_flags = /--headless|--disable-security|--disable-web-security|--remote-debugging-port|--load-extension/ nocase

    condition:
        ($task_executes_lnk and any of ($chrome_exec, $edge_exec)) or
        ($lnk_exec and any of ($chrome_exec, $edge_exec)) or
        ($suspicious_flags)
}
