rule Detect_Browser_CMD_Execution {
    meta:
        description = "Detects Chrome or Edge executing cmd.exe, including potential chained executions"
        author = "Efstratios Chatzoglou"
        version = "1.0"
    strings:
        $browser_chrome = "chrome.exe" nocase ascii wide
        $browser_edge = "msedge.exe" nocase ascii wide
        $cmd = "cmd.exe" nocase ascii wide
        $exe = ".exe" nocase ascii wide
		$string = "--load-extension" nocase ascii wide
    condition:
        ( 
            1 of ($browser_chrome, $browser_edge) and 
            $cmd and
            $exe and
			$string
        )
}
