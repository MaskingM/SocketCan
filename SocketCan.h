//
// Created by Mask on 2022/5/16.
//

#ifndef SOCKETCAN_SOCKETCAN_H
#define SOCKETCAN_SOCKETCAN_H

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

#define LOG_TAG "SOCKETCAN日志: "
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define CAN_FRAME_BUF_SIZE          10

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



class SocketCan {
private:
    static int socket_fd;                   // socket 套接字
    static pthread_t thread;
    static selfThreadData threadData;
    static SocketCan* instance;
    static struct sockaddr_can address;
    static bool isRoot;                     //是否需要加系统权限

private:
    SocketCan();
    ~SocketCan();

public:
    // 获取实例对象
    static SocketCan* getSocketCanInstance(bool root);

    // 销毁SocketCan实例
    static void destroySocketCan();

    // 通过socket向can总线发送数据
    static void sendData(pCanData pData);

    // 读取数据
    static void readData();

};


#endif //SOCKETCAN_SOCKETCAN_H
