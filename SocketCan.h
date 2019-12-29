//
// Created by ASUS on 2019/10/18.
//

#ifndef CANTEST_SOCKETCAN_H
#define CANTEST_SOCKETCAN_H

#include <android/log.h>
#include <jni.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <queue>
#include <pthread.h>
#include <mutex>
#include <sys/select.h>
#include <unistd.h>
#include <string>



#define LOG_TAG "SOCKETCAN-LIB消息: "
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)



class SocketCan;

typedef struct _can_data {
    int id;
    int len;
    char data[8];
} canData, *pCanData;

typedef struct _self_thread_data{
    bool flag;                  // 线程运行标志
    SocketCan* socketCan;
} selfThreadData, *pSelfThreadData;


class SocketCan{
private:
    static int socket_fd;                   // socket 套接字
    static pthread_t thread;
    static selfThreadData threadData;
    static SocketCan* instance;
    static struct sockaddr_can addr;

    static int baudRate;
    static bool isLoopback;
    static bool isReceiveSelf;
    static bool isExtendedFrame;
public:
    static int canNo;
private:
    SocketCan();
    ~SocketCan();
public:
    // 获取实例对象
    static SocketCan* getSocketCanInstance();

    static void init(int canNo = 0, int baudRate = 125000, bool isLoopback = false, bool isReceiveSelf = false, bool isExtendedFrame = false);
    // 销毁SocketCan实例
    static void destroySocketCan();
    // 通过socket向can总线发送数据
    static void sendData(pCanData pData);
    // 获取缓冲区数据
    static void getData(pCanData pData);
    // 将数据保存至缓冲区
    static void saveData();
};



#endif //CANTEST_SOCKETCAN_H
