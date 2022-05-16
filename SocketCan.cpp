//
// Created by Mask on 2022/5/16.
//

#include "SocketCan.h"


// 初始化SocketCan静态数据
int SocketCan::socket_fd = -1;
pthread_t SocketCan::thread = 0;
selfThreadData SocketCan::threadData = { 0 };
SocketCan* SocketCan::instance = nullptr;
struct sockaddr_can SocketCan::address = { 0 };
bool SocketCan::isRoot = false;

// java 虚拟机
static JavaVM* pJVM = nullptr;
static jobject javaObj = nullptr;

// JNI 初始化
jint JNI_OnLoad(JavaVM* vm, void* reserved){
    jint res = JNI_VERSION_1_6;
    do{
        pJVM = vm;
        JNIEnv* pENV = nullptr;
        pJVM->AttachCurrentThread(&pENV, nullptr);
//        pJVM->GetEnv((void**)&pENV, JNI_VERSION_1_6);
        if(pENV == nullptr){
//            pJVM->DetachCurrentThread();
            res = -1;
            break;
        }
        jclass javaClass = pENV->FindClass("Can/SocketCAN");
        if(javaClass == nullptr){
//            pJVM->DetachCurrentThread();
            res = -1;
            break;
        }
        jmethodID initFunc = pENV->GetMethodID(javaClass, "<init>", "()V");
        jobject obj = pENV->NewObject(javaClass, initFunc);
        javaObj = pENV->NewGlobalRef(obj);
    }while (0);
    return res;
}

// JNI 释放
void JNI_OnUnload(JavaVM* vm, void* reserved) {
    if(javaObj){
        JNIEnv* pENV = nullptr;
        vm->AttachCurrentThread(&pENV, nullptr);
        if(pENV == nullptr){
            vm->DetachCurrentThread();
            return;
        }
        pENV->DeleteGlobalRef(javaObj);
        vm->DetachCurrentThread();
    }
}

/*
 * 休眠毫秒值
 * @param   secs        毫秒值
 * @return  void
*/
void sleep_ms(unsigned int secs /*in*/) {
    struct timeval tval{};
    tval.tv_sec=secs/1000;
    tval.tv_usec=(secs*1000)%1000000;
    select(0,nullptr,nullptr,nullptr,&tval);
}

// 读取socket接收的数据
void* readCanDataThread(void *pData){
    auto pThreadData = (pSelfThreadData)pData;
    while (pThreadData->flag){
        char buf[1024] = {0};
        if(pThreadData->socketCan){
            SocketCan::readData();
        }
        sleep_ms(50);
    }
    pthread_exit((void*)2);
    return (void *) 0;
}

SocketCan::SocketCan(){

    SocketCan::socket_fd =  socket(PF_CAN, SOCK_RAW, CAN_RAW);                      //创建套接字
    // 套接字创建成功
    if(SocketCan::socket_fd != -1){
        do{
            struct ifreq ifr = { 0 };
            strcpy(ifr.ifr_name, "can0" );
            int ret = ioctl(SocketCan::socket_fd, SIOCGIFINDEX, &ifr);               //指定can0设备
            if(ret != 0){
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }

            //设置过滤规则
            //  setsockopt(SocketCan::socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
            //设置回环功能
            int loopback = 0; // 0 表示关闭, 1 表示开启( 默认)
            ret = setsockopt(SocketCan::socket_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
            if(ret != 0){
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }

            SocketCan::address.can_family = AF_CAN;
            SocketCan::address.can_ifindex = ifr.ifr_ifindex;

            ret = bind(SocketCan::socket_fd, (struct sockaddr *)&address, sizeof(address));                                 //将套接字与can0绑定
            if(ret != 0){
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }

            struct timeval timeout = {0,20}; //20ms
            //设置发送超时
            ret = setsockopt(SocketCan::socket_fd, SOL_SOCKET,SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
            if(ret != 0){
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }
            //设置接收超时
            ret = setsockopt(SocketCan::socket_fd, SOL_SOCKET,SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
            if(ret != 0) {
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }
            // 初始化缓冲区
            //  SocketCan::initBuffs();
            // 开启接收数据线程
            SocketCan::threadData.flag = true;
            SocketCan::threadData.socketCan = this;
            pthread_create(&(SocketCan::thread), nullptr, readCanDataThread, &(SocketCan::threadData));
            struct sigaction actions{};
            memset(&actions, 0, sizeof(actions));
            sigemptyset(&actions.sa_mask);
            actions.sa_flags = 0;
            actions.sa_handler = nullptr;
            sigaction(SIGUSR1,&actions,nullptr);
        }while(0);
    }
}

SocketCan::~SocketCan(){
    SocketCan::threadData.flag = false;
    sleep_ms(10);
    if(SocketCan::thread){
        pthread_kill(SocketCan::thread, SIGUSR1);
    }

    if(SocketCan::socket_fd != -1){
        close(SocketCan::socket_fd);
        SocketCan::socket_fd = -1;
    }
}

// 获取实例
SocketCan* SocketCan::getSocketCanInstance(bool root){
    SocketCan::isRoot = root;
    if(isRoot){
        system("su -c netcfg can0 down");
        system("su -c ip link set can0 type can bitrate 125000");
        system("su -c ip link set can0 type can restart-ms 10");
        system("su -c ip link set can0 up");
        system("su -c netcfg can0 up");
    }else{
        system("netcfg can0 down");
        system("ip link set can0 type can bitrate 125000");
        system("ip link set can0 type can restart-ms 10");
        system("ip link set can0 up");
        system("netcfg can0 up");
    }

    if(SocketCan::instance == nullptr){
        SocketCan::instance = new SocketCan();
        if(SocketCan::socket_fd == -1){
            SocketCan::destroySocketCan();
        }
    }
    return  SocketCan::instance;
}

// 销毁实例
void SocketCan::destroySocketCan(){
    if(SocketCan::instance){
        delete SocketCan::instance;
        SocketCan::instance = nullptr;
    }

    if( SocketCan::isRoot){
        system("su -c netcfg can0 down");
    }else{
        system("can0 down");
    }

}

// 通过socket向can总线发送数据
void SocketCan::sendData(pCanData pData){
    if(SocketCan::instance){
        struct can_frame frame = {0};
        frame.can_id = CAN_EFF_FLAG | (canid_t)pData->id;
        frame.can_dlc = (__u8)pData->len;
        memcpy(&(frame.data), pData->data, (size_t)pData->len);


        ssize_t len = sendto(SocketCan::socket_fd, &frame, sizeof(struct can_frame), 0, (struct sockaddr*)&SocketCan::address, sizeof(SocketCan::address));
        if(len <= 0){
            LOG_INFO("数据发送失败 errno:%d",errno);
            FILE *fp;
            if( SocketCan::isRoot){
                fp = popen("su -c ip -detail link show can0 | grep BUS-OFF", "r");
            }else{
                fp = popen("ip -detail link show can0 | grep BUS-OFF", "r");
            }
            char buf[1024] = {0};
            memset(buf, 0, 1024);
            fread(buf, 1, 1024, fp);
            if(strstr(buf, "BUS-OFF")){
                LOG_INFO("发送数据,总线离线,正在重启总线");
                if( SocketCan::isRoot){
                    system("su -c ifconfig can0 down");
                    system("su -c ifconfig can0 up");
                }else{
                    system("ifconfig can0 down");
                    system("ifconfig can0 up");
                }

                sleep_ms(5);
            }
            pclose(fp);
            sendto(SocketCan::socket_fd, &frame, sizeof(struct can_frame), 0, (struct sockaddr*)&SocketCan::address, sizeof(SocketCan::address));
        }
    }else {
        LOG_INFO("实例已销毁，无法发送数据");
    }
}


// 将数据保存至缓冲区
void SocketCan::readData(){
    if(SocketCan::instance){
        struct can_frame frame_buf[CAN_FRAME_BUF_SIZE] = {0};
        memset(frame_buf, 0, sizeof(frame_buf));
        int read_size = 0;
        for (auto & i : frame_buf){
            ssize_t res = recv(SocketCan::socket_fd, &i, sizeof(can_frame), 0);
            if(res > 0 && i.can_dlc){
                i.can_id &= CAN_EFF_MASK;
                read_size++;
                //   LOG_INFO("zhouyp 接收数据::： ID: 0x%08X，帧长度: %d\n",frame_buf[i].can_id, frame_buf[i].can_dlc & 0xFF);
            }else {
                break;
            }
        }

        if(read_size){
            for(int i = 0; i < read_size; i++){

                if(javaObj){
                    do{
                        JNIEnv *pENV = nullptr;
                        pJVM->AttachCurrentThread(&pENV, nullptr);
                        if(nullptr == pENV){
                            pJVM->DetachCurrentThread();
                            break;
                        }
                        jclass clazz = pENV->GetObjectClass(javaObj);
                        jmethodID receiveMID = pENV->GetStaticMethodID(clazz, "CanReceiveDataFromJNI", "(II[B)V");
                        if(nullptr == receiveMID){
                            pJVM->DetachCurrentThread();
                            break;
                        }
                        jbyteArray tmpArr = pENV->NewByteArray(8);
                        pENV->SetByteArrayRegion(tmpArr, 0, 8, (jbyte*)frame_buf[i].data);
                        pENV->CallStaticVoidMethod(clazz, receiveMID, (jint)frame_buf[i].can_id, frame_buf[i].can_dlc, tmpArr);
                        pJVM->DetachCurrentThread();
                    }while (0);
                }
            }
        }
    }
}

