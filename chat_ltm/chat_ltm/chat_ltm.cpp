#include <iostream>
#include <winsock2.h>
#include <map>
#include <string.h>
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
using namespace std;
SOCKET clients[64];
int numClients = 0;
SOCKET connected[64];
int numConnected = 0;
map<SOCKET, char*> accTable;
void RemoveClient(SOCKET client)
{
    int i = 0;
    for (; i < numConnected; i++)
        if (connected[i] == client) break;
    if (i < numConnected - 1)
        connected[i] = connected[numConnected - 1];
    numConnected--;
}
int sendAll(SOCKET client, char* data, int jump)
{
    int success = 0;
    for (int i = 0; i < numConnected; i++)
    {
        if (connected[i] != client)
        {
            success = send(connected[i], data + jump, strlen(data) - jump, 0);
        }
    }
    return success;
}
DWORD WINAPI ClientThread(LPVOID param) {
    SOCKET client = *(SOCKET*)param;
    int ret;
    char idInput[30];
    const char* signIn = "Moi ban nhap id cua minh :";
    const char* successAuth = "Dang nhap thanh cong\n";
    send(client, signIn, strlen(signIn), 0);
    while (true)
    {
        ret = recv(client, idInput, sizeof(idInput), 0);
        idInput[ret - 1] = 0;
        int found = 0;
        FILE* f = fopen("D:\\database\\input.txt", "r");
        while (!feof(f))
        {
            char line[256];
            fgets(line, sizeof(line), f);
            char user[32];
            sscanf(line, "%s", user);
            if (strcmp(idInput, user) == 0)
            {
                found = 1;
                break;
            }
        }
        fclose(f);
        if (found == 1)
        {
            send(client, successAuth, strlen(successAuth), 0);
            connected[numConnected] = client;
            accTable[connected[numConnected]] = idInput;
            numConnected++;
            char msg[30];
            sprintf(msg, "[USER_CONNECT] %s - Co user dang nhap!\n", accTable[client]);
            sendAll(client, msg, 0);
            break;
        }
        else
        {
            const char* msg = "Khong tim thay tai khoan. Hay nhap lai.\n";
            send(client, msg, strlen(msg), 0);
        }
    }
    char buff[256];
    char cmd[32], id[32], tmp[32];
    while (true)
    {
        ret = recv(client, buff, sizeof(buff), 0);
        buff[ret] = 0;
        printf("%d: %s", client, buff);
        ret = sscanf(buff, "%s %s %s", cmd, id, tmp);
        int lengthCode = strlen(cmd) + strlen(id) + 2;
        if (strcmp(cmd, "[SEND]") == 0)
        {
            if (strcmp(id, "ALL") == 0)
            {
                int success = sendAll(client, buff, lengthCode);
                if (success)
                {
                    const char* msg = "[SEND] OK-Gui tin nhan thanh cong!\n";
                    send(client, msg, strlen(msg), 0);
                }
                else
                {
                    const char* msg = "[SEND] ERROR error_message - Gui tin nhan that bai!\n";
                    send(client, msg, strlen(msg), 0);
                }
            }
            else
            {
                for (int i = 0; i < numConnected; i++)
                {
                    if (strncmp(id, accTable[connected[i]], strlen(id)) == 0)
                    {
                        send(connected[i], accTable[client], strlen(id), 0);
                        send(connected[i], ": ", 2, 0);
                        send(connected[i], buff + lengthCode, strlen(buff) - lengthCode, 0);
                    }
                    break;
                }
            }
        }
        else if (strcmp(cmd, "[LIST]") == 0)
        {
            for (size_t i = 0; i < numConnected; i++)
            {
                char temp[30];
                sprintf(temp, "%s\n", accTable[connected[i]]);
                send(client, temp, strlen(temp), 0);
            }
        }
        else if (strcmp(cmd, "[DISCONNECT]") == 0) {
            char msg[30];
            sprintf(msg, "[USER_DISCONNECT] %s - Co user da dang xuat!", accTable[client]);
            sendAll(client, msg, 0);
            RemoveClient(client);
        }
        else
        {
            const char* msg = "[ERROR] error_message - Sai lenh/giao thuc\n";
            send(client, msg, strlen(msg), 0);
        }
    }
}
int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);
    bind(listener, (SOCKADDR*)&addr, sizeof(addr));
    listen(listener, 5);
    char buffer[1024];
    printf("Sever chat - Wait client: \n");
    while (true)
    {
        clients[numClients] = accept(listener, NULL, NULL);
        printf("Client moi ket noi: %d\n", clients[numClients]);
        CreateThread(0, 0, ClientThread, &clients[numClients], 0, 0);
        numClients++;
    }
}