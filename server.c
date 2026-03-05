#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "common.h"

int recv_all(int sock, void *buf, int size)
{
    int received = 0;
    while(received < size)
    {
        int n = recv(sock, (char*)buf + received, size - received, 0);
        if(n <= 0)
            return -1;
        received += n;
    }
    return received;
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: server <port>\n");
        return 1;
    }
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
#endif
    int port = atoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
    listen(server_fd,5);
    printf("Server listening on port %d\n",port);
    int client = accept(server_fd,NULL,NULL);
    printf("Client connected\n");
    file_header fh;
    recv_all(client,&fh,sizeof(fh));
    printf("Receiving file: %s (%llu bytes)\n",
           fh.filename,(unsigned long long)fh.file_size);
    FILE *fp = fopen(fh.filename,"wb");
    if(!fp)
    {
        perror("fopen");
        return 1;
    }
    char buffer[CHUNK_SIZE];
    uint64_t total = 0;
    while(total < fh.file_size)
    {
        chunk_header ch;
        if(recv_all(client,&ch,sizeof(ch)) <= 0)
            break;
        if(recv_all(client,buffer,ch.size) <= 0)
            break;
        uint32_t check = crc32((unsigned char*)buffer,ch.size);
        if(check != ch.checksum)
        {
            printf("Checksum mismatch chunk %u\n",ch.seq);
            char nack = 0;
            send(client,&nack,1,0);
            continue;
        }
        fwrite(buffer,1,ch.size,fp);
        total += ch.size;
        char ack = 1;
        send(client,&ack,1,0);
        printf("Received chunk %u (%llu/%llu)\n",
                ch.seq,
                (unsigned long long)total,
                (unsigned long long)fh.file_size);
    }
    fclose(fp);
    printf("File transfer complete\n");
#ifdef _WIN32
    closesocket(client);
    closesocket(server_fd);
    WSACleanup();
#else
    close(client);
    close(server_fd);
#endif
    return 0;
}
