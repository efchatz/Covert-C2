#include "pch.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>

// Structure to hold file data
struct FileData {
    std::vector<char> data;
    std::string filename;
};

// Static buffer for response
static char output_buffer[32768] = {0};

FileData ReadFileToMemory(const std::string& filepath) {
    FileData result;
    result.filename = filepath.substr(filepath.find_last_of("/\\") + 1);

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    // Read file into vector
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    result.data.resize(fileSize);
    file.read(result.data.data(), fileSize);

    return result;
}

// Base64 encoding function
std::string Base64Encode(const std::vector<char>& input) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encoded;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = input.size();
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.data());

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                encoded += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            encoded += base64_chars[char_array_4[j]];

        while(i++ < 3)
            encoded += '=';
    }

    return encoded;
}

extern "C" __declspec(dllexport) const char* Run() {
    try {
        // Replace "C:\\path\\to\\file.txt" with the file you want to retrieve
        FileData fileData = ReadFileToMemory("C:\\path\\to\\file.txt");
        
        // Create JSON response with base64 encoded file and filename
        std::string response = "{\"type\":\"file\",\"filename\":\"" + 
                             fileData.filename + 
                             "\",\"data\":\"" + 
                             Base64Encode(fileData.data) + 
                             "\"}";

        strncpy_s(output_buffer, response.c_str(), sizeof(output_buffer) - 1);
        return output_buffer;
    }
    catch (const std::exception& e) {
        std::string error = "{\"type\":\"error\",\"message\":\"" + std::string(e.what()) + "\"}";
        strncpy_s(output_buffer, error.c_str(), sizeof(output_buffer) - 1);
        return output_buffer;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
