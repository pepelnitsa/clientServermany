#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096

#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT "8888"

SOCKET client_socket;
string client_nick;     // Нік клієнта
int client_color = 15;  // Номер кольору клієнта, за замовчуванням білий (15)

DWORD WINAPI Sender(void* param)
{
    cout << "You can start typing your messages:\n";

    while (true) {
        string message;
        getline(cin, message);

        // Відправлення повідомлення на сервер у форматі "нік: повідомлення: колір"
        string formatted_message = client_nick + ": " + message + ": " + to_string(client_color);
        send(client_socket, formatted_message.c_str(), formatted_message.length(), 0);
    }
}

DWORD WINAPI Receiver(void* param)
{
    while (true) {
        char response[DEFAULT_BUFLEN];
        int result = recv(client_socket, response, DEFAULT_BUFLEN, 0);
        response[result] = '\0';

        // Розділення повідомлення на нік, текст та колір
        string received_message = response;
        size_t colon_pos1 = received_message.find(':');
        size_t colon_pos2 = received_message.find(':', colon_pos1 + 1);

        if (colon_pos1 != string::npos && colon_pos2 != string::npos) {
            string received_nick = received_message.substr(0, colon_pos1);
            string message_and_color = received_message.substr(colon_pos1 + 1, colon_pos2 - colon_pos1 - 1);
            string received_color_str = received_message.substr(colon_pos2 + 1);
            int received_color = stoi(received_color_str);

            // Перевірка, чи повідомлення не відноситься до поточного клієнта
            if (received_nick != client_nick) {
                // Встановлення кольору консолі для виведення повідомлення
                HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(hConsole, received_color);

                cout << received_message << endl;

                // Повернення коліру консолі до білого (за замовчуванням)
                SetConsoleTextAttribute(hConsole, 15);
            }
        }
    }
}

BOOL ExitHandler(DWORD whatHappening)
{
    switch (whatHappening)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        cout << "Shutting down...\n";
        Sleep(1000);
        send(client_socket, "off", 3, 0);
        return(TRUE);
        break;
    default:
        return FALSE;
    }
}

int main()
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ExitHandler, true);

    system("title Client");

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 2;
    }

    addrinfo* ptr = nullptr;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        client_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (client_socket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 3;
        }

        iResult = connect(client_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (client_socket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 5;
    }

    cout << "Please enter your nickname: ";
    getline(cin, client_nick);

    cout << "Please enter your color number (1-15): ";
    cin >> client_color;

    CreateThread(0, 0, Sender, 0, 0, 0);
    CreateThread(0, 0, Receiver, 0, 0, 0);

    Sleep(INFINITE);
}
