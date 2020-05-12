﻿// windows_server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include <fstream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <direct.h>
#include "CbrThreadPool.h"



#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)


#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

void listen_thread(const int server_socket_fd) {

    while (true) {
	    std::cout <<"当前线程:\t" << GetCurrentThreadId() <<std::endl;
        // 定义客户端的socket地址结构
        struct sockaddr_in client_addr {};
        socklen_t client_addr_length = sizeof(client_addr);
        // 接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信
        // accept函数会把连接到的客户端信息写到client_addr中
        int new_server_socket_fd = accept(
                                       server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_length);
        if (new_server_socket_fd < 0) {
            perror("服务 接受链接 失败");
            break;
        }
        // recv函数接收数据到缓冲区buffer中
        char buffer[BUFFER_SIZE];
        // bzero(buffer, BUFFER_SIZE);
        memset(&buffer, 0, sizeof(buffer));
        if (recv(new_server_socket_fd, buffer, BUFFER_SIZE, 0) < 0) {
            perror("服务 接受数据 失败");
            break;
        }
        // 然后从buffer(缓冲区)拷贝到file_name中
        char file_name[FILE_NAME_MAX_SIZE + 1];
        // bzero(file_name, FILE_NAME_MAX_SIZE + 1);
        memset(&file_name, 0, sizeof(file_name));

        char tmp[FILE_NAME_MAX_SIZE + 1];
        // bzero(tmp, FILE_NAME_MAX_SIZE + 1);
        memset(&tmp, 0, sizeof(tmp));
        strncpy(tmp, buffer,
                strlen(buffer) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE
                : strlen(buffer));
        char* current_path;
        //也可以将buffer作为输出参数
        if ((current_path = getcwd(nullptr, 0)) == nullptr)
            perror("get current_path error");
        //        current_path = "./";
        sprintf(file_name, "%s/%s", current_path, tmp);
        std::cout << file_name << std::endl;

        // 打开文件并读取文件数据
        std::ifstream fp;
        fp.open(file_name, std::ios::binary);
        if (!fp) {
            char tmp[256];
            printf("File:%s Not Found\n", file_name);
            sprintf(tmp, "File:%s Not Found\n", file_name);
            send(new_server_socket_fd, tmp, 256, 0);
        } else {
            // bzero(buffer, BUFFER_SIZE);
            memset(&buffer, 0, sizeof(buffer));
            sprintf(buffer, "File:%s has Found\n", file_name);
            send(new_server_socket_fd, buffer, 256, 0);
            // bzero(buffer, BUFFER_SIZE);
            memset(&buffer, 0, sizeof(buffer));
            int length = 0;
            int allCount = 0;
            int readLen = 0;
            // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止
            while (!fp.eof()) {
                fp.read(buffer, BUFFER_SIZE);
                readLen = fp.gcount();
                send(new_server_socket_fd, buffer, readLen, 0);
                allCount += readLen;
            }

            fp.close();
            // 关闭文件
            /*fclose(fp);*/
            printf("File:%s Transfer Successful! 共%.2fK\n", file_name,
                   allCount / 1024.0);
        }
        // 关闭与客户端的连接
        closesocket(new_server_socket_fd);
    }

}


int main(int argc, const char* argv[]) {
    std::cout << "Hello World!\n";
    const char* exce_path = argv[0];
    const char* address = argv[1];
    int port = atoi(argv[2]);

    //初始化WSA
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
        return 0;
    //     声明并初始化一个服务器端的socket地址结构
    struct sockaddr_in server_addr {};
    //bzero(&server_addr, sizeof(server_addr));

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    if (address == nullptr)
        server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    else
        server_addr.sin_addr.s_addr = inet_addr(address);
    if (port == 0)
        server_addr.sin_port = htons(3366);
    else
        server_addr.sin_port = htons(port);


    // 创建socket，若成功，返回socket描述符
    int server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("创建socket失败");
        exit(1);
    }
    int opt = 1000;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // 绑定socket和socket地址结构
    if (-1 == (bind(server_socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)))) {
        perror("服务 bind 失败");
        exit(1);
    }
    // socket监听
    if (-1 == (listen(server_socket_fd, LENGTH_OF_LISTEN_QUEUE))) {
        perror("服务 listen 失败");
        exit(1);
    }

    CbrThreadPool ctp(1, 8);

    for (int i = 0; i < 8; ++i)
        ctp.PushTask(listen_thread, server_socket_fd);

    while (true) {
	    
    }
	
    // 关闭监听用的socket
    closesocket(server_socket_fd);
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧:
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
