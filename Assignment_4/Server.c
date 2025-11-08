#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 4096

void send_file_list(SOCKET client_socket) {
    const char* folder = ".\\shared_files\\*";
    char file_list[4096] = "";
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = FindFirstFile(folder, &fdFile);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                strcat(file_list, fdFile.cFileName);
                strcat(file_list, "\n");
            }
        } while (FindNextFile(hFind, &fdFile));
        FindClose(hFind);
    } else {
        strcpy(file_list, "No files found or error opening directory.\n");
    }
    send(client_socket, file_list, strlen(file_list), 0);
}

void handle_get(SOCKET client_socket, const char* filename) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), ".\\shared_files\\%s", filename);
    FILE* fp = fopen(filepath, "rb");
    char buffer[BUFFER_SIZE];

    if (!fp) {
        char* err = "ERROR: File not found.\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }
    printf("Sending file: %s\n", filename);

    int n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_socket, buffer, n, 0);
    }
    fclose(fp);
    printf("File sent.\n");
}

void handle_put(SOCKET client_socket, const char* filename) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), ".\\shared_files\\%s", filename);
    FILE* fp = fopen(filepath, "wb");
    char buffer[BUFFER_SIZE];

    if (!fp) {
        printf("Failed to create file: %s\n", filename);
        return;
    }
    printf("Receiving file for upload: %s\n", filename);

    int n;
    while ((n = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, n, fp);
    }
    fclose(fp);
    printf("File upload complete.\n");
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, client_socket;
    struct sockaddr_in server, client;
    int c;

    WSAStartup(MAKEWORD(2, 2), &wsa);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    listen(server_fd, 5);

    printf("Server listening...\n");

    while (1) {
        c = sizeof(struct sockaddr_in);
        client_socket = accept(server_fd, (struct sockaddr *)&client, &c);
        if (client_socket == INVALID_SOCKET) continue;
        printf("\nClient connected.\n");

        send_file_list(client_socket);

        // Receive command
        char buffer[BUFFER_SIZE];
        int recv_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_size <= 0) {
            printf("Connection closed or error.\n");
            closesocket(client_socket);
            continue;
        }
        buffer[recv_size] = '\0';

        // Handle GET/PUT
        if (strncmp(buffer, "GET ", 4) == 0) {
            handle_get(client_socket, buffer + 4);
        } else if (strncmp(buffer, "PUT ", 4) == 0) {
            handle_put(client_socket, buffer + 4);
        } else {
            char* err = "ERROR: Unsupported command.\n";
            send(client_socket, err, strlen(err), 0);
        }

        closesocket(client_socket);
        printf("Session ended.\n---------------------\n");
    }
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
