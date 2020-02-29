// hainan_client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <fstream>
#include <string>
#include <sysinfoapi.h>


#pragma warning(disable : 4996)
#pragma comment(lib, "ws2_32.lib") //连接winsock2.h的静态库文件
using namespace std;

#define MAXSPEED 299
#define MAXBYTE     512

void limit_speed(float speed, int max_speed) {
    if (speed > max_speed) {
        int ns = (speed - max_speed) / speed * 1000;
        Sleep(ns);
        //cout << "time pass " << GetTickCount() - startTimeStamp << endl;
    }
}

BOOL judge_file_exist(SOCKET clientSock) {
    char Buffer[MAXBYTE] = { 0 };
    int size = 0;
    if (size = recv(clientSock, Buffer, MAXBYTE, 0) < 0) {
        cout << "rec data error";
        return false;
    }

    string tmp = Buffer;
    int pos = tmp.find("Not Found");
    if (pos != tmp.npos) {
        cout << Buffer << endl;
        return false;
    }
    cout << Buffer << endl;
    return true;

}

SOCKET client_socket(const char* address, int port) {
    //加载winsock库
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 3), &wsadata);

    //客户端socket
    SOCKET clientSock = socket(PF_INET, SOCK_STREAM, 0);
    //初始化socket信息
    sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(SOCKADDR));
    //clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_addr.s_addr = inet_addr(address);
    clientAddr.sin_family = PF_INET;
    clientAddr.sin_port = htons(port);
    //建立连接
    int connect_status = connect(clientSock, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR));
    if (connect_status == SOCKET_ERROR) {
        cout << "连接失败" << endl;

        return 0;
    } else
        cout << "已建立连接。" << endl;
    return clientSock;
}

// 192.168.0.113 1789 200 cbr.jar cbr.jar 

int main(int argc, const char** argv) {


    //std::cout << "Hello World!\n";
    const char* exce_path = argv[0];
    const char* address = argv[1];
    int port = atoi(argv[2]);

    int max_speed = (atoi(argv[3]) - 1);
    const char* target = argv[4];
    const char* local = argv[5];

    // 连接服务器
    SOCKET clientSock = client_socket(address, port);


    while (true) {

        //string aimfile = target;
        //aimfile = "/home/cbr/cbr30.AppImage";

        //aimfile = "/home/cbr/cbr.jar";
        send(clientSock, target, strlen(target)*sizeof(char*), NULL);
        cout << "已向服务器请求文件:" << target << endl;

        BOOL status = judge_file_exist(clientSock);
        if (!status)
            break;
        char Buffer[MAXBYTE] = { 0 }; // 文件缓冲区
        char wb_file[100] = { 0 }; //写入的文件

        //string wb_f = local ;

        //cout << "请输入想要写入的文件: ";
        //cin >> wb_file;

        //wb_f = "cbr.jar";

        FILE* fp = fopen(local, "wb");
        //FILE* fp = fopen(wb_file, "wb");
        if (fp == NULL) {
            cout << "操作文件时出错" << endl;
            system("pause");
        } else {
            memset(&Buffer, 0, MAXBYTE);
            int size = 0;
            //当成功接收文件（size > 0）时，判断写入的时候文件长度是否等于接收的长度
            int rec_size = 0;
            int file_size = 0;
            DWORD startTimeStamp;
            startTimeStamp = GetTickCount();
            while (true) {

                size = recv(clientSock, Buffer, MAXBYTE, 0);
                if (size <= 0)
                    break;
                if (fwrite(Buffer, sizeof(char), size, fp) < size)
                    cout << "写入出错，部分文件缺失。" << endl;
                else {
                    rec_size += size;
                    file_size += size;
                }
                DWORD endTimeStamp = GetTickCount();
                //判断是否接收够速度限制字节，且时间限位1s
                if ((rec_size / 1024 > max_speed) && ((endTimeStamp - startTimeStamp) < 1000))
                    Sleep(1000 + startTimeStamp - endTimeStamp);

                float rec_t = (endTimeStamp - startTimeStamp) / 1000.0;
                if (rec_t >= 1) {
                    //DWORD endTimeStamp = GetTickCount();

                    float speed = (rec_size / 1024) / rec_t ;
                    //更新运行时钟
                    startTimeStamp = GetTickCount();
                    cout << flush << "\33[2K\r下载速度" << speed << "KB/s";

                    limit_speed(speed, max_speed);

                    rec_size = 0;
                    //rec_size = 0;
                    //startTimeStamp = GetTickCount();
                }
                //清空缓存区以便下一次接收
                memset(&Buffer, 0, MAXBYTE);
            }
            float speed = (rec_size / 1024) / 1.0;
            cout << flush << "\33[2K\r下载速度" << speed << "KB/s" << endl;
            cout << "接收完成	" << target << "共" << file_size / 1023.999 << "KB" << endl;
            cout << "另存为	" << local  << endl; 
            fclose(fp);
        }
        break;
    }


    closesocket(clientSock);
    WSACleanup();

    cout << "客户端连接已关闭。" << endl;


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
