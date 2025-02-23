<a id="readme-top"></a>



<!-- PROJECT SHIELDS -->
<!--
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]



<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/efchatz/Covert-C2/blob/main/images/covert-c2.png">
    <img src="images/logo.png" alt="Logo" width="600" height="300">
  </a>

<h3 align="center">Covert C2</h3>

   <p align="center">
     A PoC C2 implementation that uses Native Messaging API to execute direct commands in the Windows OS.
    <br />
    <a href="https://github.com/efchatz/Covert-C2/issues">Report Bug</a>
  </p>
</div>


<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

This is a new C2 implementation named Covert C2 or C3 (Covert+Command+Control=C3) that uses the Native Messaging API to establish a post-exploitation communication against a host. Native Messaging API is a browser API that is used from extension to communicate with desktop applications.

In this case, the Native Messaging API was weaponized to be used for post-exploitation attacks. This serves as a Proof-of-Concept (PoC) as it can take multiple directions and configurations. This means that it might need some additional implementations before being used in a real-case scenario.

Another reason that this serves as a PoC and not as a complete post-exploitation framework is because this is an open-source project and most EDRs will signature it. So, providing the methodology and simple way of communication could assist future and authorized red-team engagements to success.

All previous works focused on exploining the browser itself, e.g., stealing cookies, injecting different domains, causing redirects. Only EarthKitsune APT group used a similar techique in 2023 to load a shellcode in the memory of the host by using the Native Messaging API in their attack flow.

However, this work communicates directly with the OS, i.e., executes direct commands and uses minimal evasion tactics to achieve this post-exploitation attack. As a result, no tested EDR (4 were tested on free trial) was able to flag this communication as malicious.

This means that such an attack is mostly undocumented by EDR vendors and could cause serious issues when exploited. 

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- Capabilities & Advantages -->
## Capabilities & Advantages
The differencies and advantages with other C2 frameworks are the following:

* Decentralized approach: Each host can communicate separately with the C2 server, say by using subdomains or implementing cookie functionality.
* Expandability: The implementation right now supports only Windows, but it can be easily extended to be used against MacOS or Linux since it is mainly focused on using the Native Messaging API which is supported by both MacOS and Linux major browsers.
* Adaptability: It can adapt in different environments. For example, it can load the shellcode of another C2 framework to continue the post-exploitation attack or construct different type of commands for direct execution.
* Out-of-the-box Persistence: Persistence is quite crucial in a red team engagement. As a result, C3 offers out-of-the-box persistence to the post-exploitation attack.
* Direct Code Execution: It offers direct code execution. Although, in some cases this will limit the attack's execution. In the latter case, using any adapt feature could assist, like using the shellcode of another C2 framework.
* Enhanced Stealth: EDRs had zero detections against the post-exploitation attacks of this implementation. Considering this implementation had minimal usage of evasion techiques, the evasion capabilities can be further enhanced in the future, if needed. Also, it uses secure traffic with the C2 webserver with the assist of HTTP/3 and QUIC.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


### Built With

* [![C++][C++]][C-url]
* [![JavaScript][JavaScript]][JavaScript-url]
* [![Python][Python]][Python-url]
* [![PHP][PHP]][PHP-url]
* [![libcurl][libcurl]][libcurl-url]

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

The following section describes the installation procedure for the C2 webserver, the extension, and the native application. For testing purposes, only Chromium browsers were tested, i.e., Chrome and MSEdge.

Relevant testbed and versions:
* MS Windows 10 Enterprise (build 19045.5371)
* MSEdge v132.0.2957.140
* Chrome v132.0.6834.160
* Ubuntu Server 22.04 LTS
* Caddy v2.9.1

### Installation

#### C2 webserver. 
For testing purposes, an Azure VM was used, along with a free azurewebsites domain name. Consider using whatever is suitable.

1. Install an Ubuntu OS, e.g., 22.04.
2. ``` sudo apt update && sudo apt upgrade -y ```
3. ``` sudo apt install -y debian-keyring debian-archive-keyring apt-transport-https ```
4. ``` curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg ```
5. ``` curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | sudo tee /etc/apt/sources.list.d/caddy-stable.list ```
6. ``` sudo apt update ```
7. ``` sudo apt install php php-fpm ```
8. ``` sudo systemctl enable php8.4-fpm.service ```
9. ``` sudo apt install caddy=2.9.1 ```
10. ``` sudo mkdir /var/www/domain ```
11. ``` sudo mkdir /var/www/domain/uploads ```
12. ``` sudo chown -R www-data:www-data /var/www/domain ```
13. Allow in firewall the 443/tcp and 443/udp ports
14. ``` sudo nano /etc/caddy/Caddyfile ```
15. ``` domain {
        root * /var/www/domain

        # Enable HTTP/3
        @http3 {
            protocol h3
        }

        header {
            Strict-Transport-Security max-age=31536000;
            alt-svc "h3=\":443\"; ma=2592000"
        }

        @cors {
            method OPTIONS
            header Origin *
        }

        handle @cors {
            header Access-Control-Allow-Origin *
            header Access-Control-Allow-Methods "GET, POST, OPTIONS, HEAD"
            header Access-Control-Allow-Headers "Content-Type"
            respond 204
        }

        header {
            Access-Control-Allow-Origin *
            Access-Control-Allow-Methods "GET, POST, OPTIONS, HEAD"
            Access-Control-Allow-Headers "Content-Type"
        }

        # Enable the static file server.
        file_server
        bind 0.0.0.0
        tls {
                protocols tls1.3
        }

        # Another common task is to set up a reverse proxy:
        # reverse_proxy localhost:8080

        # Or serve a PHP site through php-fpm:
        # php_fastcgi localhost:9000
        php_fastcgi unix//run/php/php-fpm.sock  # Use php-fpm to process PHP files
} ```
16. ``` sudo systemctl restart caddy ```

#### Extension
Regarding the extension installation, we will need the ``` background.js ``` and ``` manifest.json ``` files. Before proceeding with the installation, it should be mentioned there is a different behavior in Chrome and MSEdge. MSEdge can load extensions from anywhere without any issues. However, when the malicious extension is installed and the user opens the browser, the browser popups a message named "Turn off extensions in developer mode." with a highlighted blue button that when it clicked it disables the malicious extension. This can be bypassed with the usage of "headless" mode which will be explained later on.

On the other hand, Chrome is restricted when trying to install extensions outside of the Chrome store. For instance, when using the "--load-extension" flag to load the malicious extension, when the user or the process terminates, Chrome deletes the complete folder of the loaded extension. 
To achieve installation and bypass this issue, a legitimate pre-installed extension must be replaced with the malicious one, e.g., nmmhkkegccagdldgiimedpiccmgmieda, with the files of the malicious extension (background.js and manifest.json). The latter step and the "--load-extension" flag are needed to be used to assure that Chrome will not delete the malicious extension.

#### Native App
Native app can be installed by simply storing the ``` native_app.exe ```, the ``` native_app.json ```, and the ``` libcurl.dll ``` files in a directory with user access, say AppData. The Dll ``` libcurl.dll ``` is needed from the native app to communicate with HTTP/3. Of course, this communication can be done with say, HTTP/1. So, by altering the code, the ``` libcurl.dll ``` usage is not nessesary.

Then, a new registry entry is required for the extension to be able to communicate with the native app. So, a new entry in ``` HKEY_CURRENT_USER\SOFTWARE\Google\Chrome\NativeMessagingHosts\NEW ENTRY NAME ``` and add as a path the directory that ``` native_app.json ``` resides.

The ``` native_app.json ``` needs the extension ID of the malicious extension to allow the communication with the native app. So, obtain the extension ID from the Secure Preferences file of either MSEdge or Chrome and add it to ``` native_app.json ``` either manually or automatically.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- USAGE EXAMPLES -->
## Usage
This section is dedicated to the usage of C3 separated in the following parts: Persistence, and Direct Command Execution. 

### Persistence
Persistence is by default enabled in the C3 since in most cases a user will open their infected browser and as a result, the malicious extension will communicate directly with the webserver.

To further enhance peristence, an attacker can use either shortcuts or the headless mode. First, by simply replacing the legitimate shortcuts and adding the "--load-extension" flag could allow the attacker to achieve peristence and communication with the webserver.

However, to avoid having user interaction, an attacker can use the "--headless" mode. MSEdge can use headless mode by adding the "--user-data-dir" flag during execution. This could create a directory with all user's preferences in the desired location, allowing the execution of the malicious extension in headless mode. This means that an attacker can use MSEdge with a malicious extension without having issues with the relevant popup message of disabling the development (malicious) extension.

The same case can be applied to Chrome. However, Chrome for some reason requires to be executed twice with the same flags, i.e., headless and user-data-dir to establish a stable communication with the webserver. As a result, both browsers can be used with headless mode and say a Task Scheduler, without requiring user's interaction for persistence.

### Direct Command Execution
C3 operates with direct command execution, having the following flow between the attacker and the victim:

1. The attacker's C2 server waits for the browser extension on the victim's machine to initiate a connection.
2. Upon connection, the C2 server sends a URL to the extension.
3. The extension forwards the URL to the native application.
4. The native application downloads the DLL from the URL via HTTP/3 (QUIC), loads it into memory, and executes it.
5. The native application sends the execution result to the extension.
6. The extension relays the result back to the C2 server.
7. The attacker views the execution result.

The following figure demostrates this communication.

<div align="center">
  <a href="https://github.com/efchatz/Covert-C2/blob/main/images/communication.png">
    <img src="images/logo.png" alt="Logo" width="300" height="150">
  </a>
</div>

For testing purposes, the following six scenarios were implemented and tested to verify the EDR's detection capabilities and the C3 functionality. The relevant commands are comment-out in the Dll code.

1. Basic command execution: The ``` whoami ``` command was executed via CMD.
2. Network connectivity test: The ``` ping 8.8.8.8 ``` command was executed via CMD.
3. Data Exfiltration: A file in the Downloads folder was converted to base64 and uploaded to the C2 server.
4. Scheduled task execution as another user: The ``` schtasks ``` command was used to schedule and execute a task as a different user. Due to the limitations of direct command execution without an interactive shell, the output of the task was redirected to a file, which was then retrieved. While this file-based approach impacts OpSec, it was necessary given the constraints of Covert C2's direct command execution and lack of shellcode support. Alternative solutions for avoiding file system interaction include blind command execution or using a C2 framework with shellcode capabilities.
5. Lateral movement: The ``` net use ``` command was employed to establish a connection to another host via SMB. Similar to the ``` schtasks ``` scenario, file redirection was used to capture the output. It is noted that enabling WinRM on target hosts would simplify lateral movement and remote command execution with output capture.
6. Random: The last scenario was employed to illustrate the effectiveness of the evasion with random command execution. This means that this scenario used all the previous scenarios in random order.

To evaluate the detectability of Covert C2 by each EDR, we first installed the necessary components: the Covert C2 framework (including the web server and workstation components) and then the EDR software on the target workstation. The evaluation involved initiating a connection from the Chrome browser to the C2 server every 10 sec. If a command was available, the communication proceeded as described in relevant figure. Otherwise, the C2 server responded with "No". 

Each of the six test scenarios was executed for five minutes with randomized command selection, i.e., either a specific scenario URL or a "No" response from the C2 server, for a total of 15 min per EDR. A final 10-min mixed scenario was then conducted, where commands were randomly selected from any of the five test cases (command execution or file upload).

Remarkably, the proposed scheme evaded detection by all tested EDR solutions across all six scenarios. This finding suggests a significant vulnerability in a range of EDR products to this post-exploitation technique. The fact that zero detections were recorded, despite the absence of code obfuscation (excluding the necessary Base64 encoding for file uploads), further underscores the potential impact of this approach. 

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- Analysis of Post-Exploitation Behavior and Defense Mechanisms -->
## Analysis of Post-Exploitation Behavior and Defense Mechanisms

Several potential detection points emerge from the usage of C3. In detail, the following list illustrates different protection mechanisms that can be used to mitigate the usage of C3.

1. Using Group Policy Objects (GPO) to disable the installation of browser extensions, apart from the ones that are whitelisted.
2. Using AppLocker and/or Windows Defender Application Control (WDAC).
3. Registry entries, Task Scheduler, portable execution of browsers, shortcuts, and browsers executing CMD can be potentially motitored with different YARA rules. While not foulproof, examples of three (3) YARA rules are provided in the YARA directory.

From the above mitigations, only the GPO is the most effective one if configured correctly since it blocks the installation of any extension ID that it is not in the whitelist of the relevant policy. Two (2) and three (3) options in the above list can be potentially bypassed with different implementation and/or misconfigurations.

For example, AppLocker can be bypassed if a portable browser is used or a filetype that is not in the restricted list, say ``` .vbs ``` or with a DLl sideloading attack. Additionally, monitoring different functionalities functions as a signatured way of flagging these operations. As a result, an attacker that uses a different functionality could potentially bypass this detection method.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- LICENSE -->
## License

Distributed under the project_license. See `LICENSE.txt` for more information. C3 repository is provided for educational and/or legitimate purposes with no guaranty.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Efstratios Chatzoglou - efchatzoglou@gmail.com

<p align="right">(<a href="#readme-top">back to top</a>)</p>




<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* [Choose an Open Source License](https://choosealicense.com)
* [Img Shields](https://shields.io)
* [Best-README-Template](https://github.com/othneildrew/Best-README-Template)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]:https://img.shields.io/github/contributors/efchatz/Covert-C2?style=for-the-badge
[contributors-url]: https://github.com/efchatz/Covert-C2/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/efchatz/Covert-C2?style=for-the-badge
[forks-url]: https://github.com/efchatz/Covert-C2/network/members
[stars-shield]: https://img.shields.io/github/stars/efchatz/Covert-C2?style=for-the-badge
[stars-url]: https://github.com/efchatz/Covert-C2/stargazers
[issues-shield]: https://img.shields.io/github/issues/efchatz/Covert-C2?style=for-the-badge
[issues-url]: https://github.com/efchatz/Covert-C2/issues
[license-shield]: https://img.shields.io/github/license/efchatz/Covert-C2?style=for-the-badge
[license-url]: https://github.com/efchatz/Covert-C2/blob/main/LICENSE
[product-screenshot]: images/screenshot.png
[C++]: https://img.shields.io/badge/C++-blue?style=for-the-badge&logo=c++&logoColor=white
[C-url]: https://cplusplus.com/
[JavaScript]: https://img.shields.io/badge/JavaScript-blue?style=for-the-badge&logo=JavaScript&logoColor=white
[JavaScript-url]: https://www.w3schools.com/js/default.asp
[Python]: https://img.shields.io/badge/Python-blue?style=for-the-badge&logo=Python&logoColor=white
[Python-url]: https://www.python.org/
[PHP]: https://img.shields.io/badge/PHP-blue?style=for-the-badge&logo=PHP&logoColor=white
[PHP-url]: https://www.php.net/
[libcurl]: https://img.shields.io/badge/libcurl-blue?style=for-the-badge&logo=libcurl&logoColor=white
[libcurl-url]: https://curl.se/libcurl/
