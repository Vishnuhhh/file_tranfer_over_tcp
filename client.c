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

int send_all(int sock, void *buf, int size)
{
    int sent = 0;
    while(sent < size)
    {
        int n = send(sock,(char*)buf + sent,size - sent,0);
        if(n <= 0)
            return -1;
        sent += n;
    }
    return sent;
}

int main(int argc,char *argv[])
{
    if(argc != 4)
    {
        printf("Usage: client <ip> <port> <file>\n");
        return 1;
    }
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
#endif
    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *filename = argv[3];
    FILE *fp = fopen(filename,"rb");
    if(!fp)
    {
        perror("fopen");
        return 1;
    }
    fseek(fp,0,SEEK_END);
    uint64_t file_size = ftell(fp);
    rewind(fp);
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    connect(sock,(struct sockaddr*)&addr,sizeof(addr));
    file_header fh;
    fh.file_size = file_size;
    fh.chunk_size = CHUNK_SIZE;
    memset(fh.filename,0,FILENAME_SIZE);
    strncpy(fh.filename,filename,FILENAME_SIZE-1);
    send_all(sock,&fh,sizeof(fh));
    char buffer[CHUNK_SIZE];
    uint32_t seq = 0;
    while(1)
    {
        int n = fread(buffer,1,CHUNK_SIZE,fp);
        if(n <= 0)
            break;
        chunk_header ch;
        ch.seq = seq;
        ch.size = n;
        ch.checksum = crc32((unsigned char*)buffer,n);
        send_all(sock,&ch,sizeof(ch));
        send_all(sock,buffer,n);
        char ack;
        recv(sock,&ack,1,0);
        if(!ack)
        {
            printf("Retrying chunk %u\n",seq);
            fseek(fp,-n,SEEK_CUR);
            continue;
        }
        printf("Sent chunk %u\n",seq);
        seq++;
    }
    printf("File transfer complete\n");
    fclose(fp);
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}
