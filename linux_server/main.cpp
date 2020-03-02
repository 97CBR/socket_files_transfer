#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

using namespace std;

#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

int main(int argc, const char *argv[]) {

    std::cout << "Hello World!\n";
    const char *exce_path = argv[0];
    const char *address = argv[1];
    int port = atoi(argv[2]);


//     声明并初始化一个服务器端的socket地址结构
    struct sockaddr_in server_addr{};
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (address == nullptr) {
        server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    } else {
        server_addr.sin_addr.s_addr = inet_addr(address);
    }
    if (port == 0) {
        server_addr.sin_port = htons(3366);
    } else {
        server_addr.sin_port = htons(port);
    }


    // 创建socket，若成功，返回socket描述符
    int server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("创建socket失败");
        exit(1);
    }
    int opt = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定socket和socket地址结构
    if (-1 == (bind(server_socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)))) {
        perror("服务 bind 失败");
        exit(1);
    }
    // socket监听
    if (-1 == (listen(server_socket_fd, LENGTH_OF_LISTEN_QUEUE))) {
        perror("服务 listen 失败");
        exit(1);
    }

    while (true) {
        // 定义客户端的socket地址结构
        struct sockaddr_in client_addr{};
        socklen_t client_addr_length = sizeof(client_addr);
        // 接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信
        // accept函数会把连接到的客户端信息写到client_addr中
        int new_server_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_addr, &client_addr_length);
        if (new_server_socket_fd < 0) {
            perror("服务 接受链接 失败");
            break;
        }
        // recv函数接收数据到缓冲区buffer中
        char buffer[BUFFER_SIZE];
        bzero(buffer, BUFFER_SIZE);
        if (recv(new_server_socket_fd, buffer, BUFFER_SIZE, 0) < 0) {
            perror("服务 接受数据 失败");
            break;
        }
        // 然后从buffer(缓冲区)拷贝到file_name中
        char file_name[FILE_NAME_MAX_SIZE + 1];
        bzero(file_name, FILE_NAME_MAX_SIZE + 1);
        char tmp[FILE_NAME_MAX_SIZE + 1];
        bzero(tmp, FILE_NAME_MAX_SIZE + 1);
        strncpy(tmp, buffer, strlen(buffer) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));
        char *current_path;
        //也可以将buffer作为输出参数
        if ((current_path = getcwd(nullptr, 0)) == nullptr) {
            perror("get current_path error");
        }
//        current_path = "./";
        sprintf(file_name, "%s/%s", current_path, tmp);
        cout << file_name << endl;

        // 打开文件并读取文件数据
        FILE *fp = fopen(file_name, "r");
        if (nullptr == fp) {
            char tmp[256];
            printf("File:%s Not Found\n", file_name);
            sprintf(tmp, "File:%s Not Found\n", file_name);
            send(new_server_socket_fd, tmp, 256, 0);
        } else {
            bzero(buffer, BUFFER_SIZE);
            sprintf(buffer, "File:%s has Found\n", file_name);
            send(new_server_socket_fd, buffer, 256, 0);
            bzero(buffer, BUFFER_SIZE);

            int length = 0;
            int allCount = 0;
            // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止
            while ((length = (int) fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) {
                if (send(new_server_socket_fd, buffer, length, 0) < 0) {
                    printf("Send File:%s Failed./n", file_name);
                    break;
                }
                allCount++;
                bzero(buffer, BUFFER_SIZE);
            }
            // 关闭文件
            fclose(fp);
            printf("File:%s Transfer Successful! 共%dK\n", file_name, allCount);
        }
        // 关闭与客户端的连接
        close(new_server_socket_fd);
    }
    // 关闭监听用的socket
    close(server_socket_fd);
    return 0;


    return 0;
}

