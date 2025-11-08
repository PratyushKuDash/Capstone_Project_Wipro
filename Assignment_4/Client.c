#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

void receive_file_list(SOCKET s) {
    char buffer[4096];
    int recv_size = recv(s, buffer, sizeof(buffer) - 1, 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        printf("Files on server:\n%s\n", buffer);
    } else {
        printf("Failed to get file list from server.\n");
    }
}

void download_file(SOCKET s) {
    char filename[256], buffer[4096];
    printf("Enter filename to download: ");
    scanf("%255s", filename);

    // Send GET request
    char request[300];
    sprintf(request, "GET %s", filename);
    send(s, request, strlen(request), 0);

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error: Could not create file %s for writing.\n", filename);
        return;
    }

    printf("Receiving file: %s\n", filename);
    int recv_size;
    int total_bytes = 0;
    while ((recv_size = recv(s, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, recv_size, fp);
        total_bytes += recv_size;
    }
    fclose(fp);

    if (total_bytes > 0)
        printf("Download complete. File saved as: %s (%d bytes)\n", filename, total_bytes);
    else
        printf("Download failed or file not found on server.\n");
}

void upload_file(SOCKET s) {
    char filename[256], buffer[4096];
    printf("Enter local filename to upload: ");
    scanf("%255s", filename);

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Error: Could not open %s for reading.\n", filename);
        return;
    }

    char request[300];
    sprintf(request, "PUT %s", filename);
    send(s, request, strlen(request), 0);

    printf("Uploading file: %s\n", filename);
    int n, total_bytes = 0;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(s, buffer, n, 0);
        total_bytes += n;
    }
    fclose(fp);

    printf("File upload complete. Sent %d bytes.\n", total_bytes);
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server;
    char buffer[4096];

    WSAStartup(MAKEWORD(2, 2), &wsa);

    while (1) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_port = htons(PORT);

        // Connect to server
        if (connect(s, (struct sockaddr *)&server, sizeof(server)) != 0) {
            printf("Unable to connect to server\n");
            closesocket(s);
            break;
        }

        receive_file_list(s);

        printf("\nChoose operation: 1.Download 2.Upload 3.Exit: ");
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            download_file(s);
        } else if (choice == 2) {
            upload_file(s);
        } else if (choice == 3) {
            // Send exit command (optional, according to your protocol)
            closesocket(s);
            break;
        } else {
            printf("Invalid choice. Try again.\n");
        }

        closesocket(s);
        printf("\n------------------------\n");
    }

    WSACleanup();
    return 0;
}
