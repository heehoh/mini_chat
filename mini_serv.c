#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int maxSocket;
char buffer[300000];
fd_set activeSockets, readSockets;

int createSocket(int port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        write(2, "Fatal Error\n", strlen("Fatal Error\n"));
        exit(1);
    }
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Your program must only listen to 127.0.0.1
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        write(2, "Fatal Error\n", strlen("Fatal Error\n"));
        exit(1);
    }

    if (listen(serverSocket, 128) < 0)
    {
        write(2, "Fatal Error\n", strlen("Fatal Error\n"));
        exit(1);
    }
    return serverSocket;
}

void broadCast(int id)
{
    for (int i = 3; i <= maxSocket; ++i)
    {
        if (i != id && FD_ISSET(i, &activeSockets))
            write(i, buffer, strlen(buffer));
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

    int clientSockets[1000];
    int nextId = 0;
    int serverSocket = createSocket(atoi(argv[1]));

    FD_ZERO(&activeSockets);
    FD_SET(serverSocket, &activeSockets);
    maxSocket = serverSocket;

    while (1)
    {
        readSockets = activeSockets;
        if (select(maxSocket + 1, &readSockets, NULL, NULL, NULL) < 0)
        {
            write(2, "Fatal Error\n", strlen("Fatal Error\n"));
            exit(1);
        }

        for (int id = 3; id <= maxSocket; ++id)
        {
            if (FD_ISSET(id, &readSockets))
            {
                bzero(buffer, sizeof(buffer));
                if (id == serverSocket)
                {
                    int clientSocket = accept(serverSocket, NULL, NULL);
                    if (clientSocket < 0)
                    {
                        write(2, "Fatal Error\n", strlen("Fatal Error\n"));
                        exit(1);
                    }
                    FD_SET(clientSocket, &activeSockets);
                    maxSocket = (clientSocket > maxSocket) ? clientSocket : maxSocket;
                    sprintf(buffer, "server: client %d just arrived\n", nextId);
                    clientSockets[clientSocket] = nextId++;
                }
                else
                {
                    int n = read(id, buffer, sizeof(buffer));
                    if (n <= 0)
                    {
                        close(id);
                        FD_CLR(id, &activeSockets);
                        sprintf(buffer, "server: client %d just left\n", clientSockets[id]);
                    }
                    else
                    {
                        buffer[n] = '\0';
                        char tmp[300000];
                        strcpy(tmp, buffer);
                        sprintf(buffer, "client %d: %s", clientSockets[id], tmp);
                    }
                }
                broadCast(id);
                break;
            }
        }
    }
    return 0;
}
