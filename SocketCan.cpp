#include "SocketCan.h"
#include <errno.h>


// 互斥锁
std::mutex instanceMtx;

// 读写资源锁
std::mutex readWriteDataMtx;


// 初始化SocketCan静态数据
int SocketCan::socket_fd = -1;
pthread_t SocketCan::thread = 0;
selfThreadData SocketCan::threadData = {0};
SocketCan* SocketCan::instance = NULL;
struct sockaddr_can SocketCan::addr = {0};
int SocketCan::canNo = 0;
int SocketCan::baudRate = 125000;
bool SocketCan::isLoopback = false;
bool SocketCan::isReceiveSelf = false;
bool SocketCan::isExtendedFrame = false;


static canData globalCanData = {0};


/ java 虚拟机
static JavaVM* pJVM = NULL;
static jobject javaObj = NULL;
// JNI 初始化
jint JNI_OnLoad(JavaVM* vm, void* reserved){
    jint res = JNI_VERSION_1_6;
    do{
        pJVM = vm;
        JNIEnv* pENV = NULL;
        pJVM->AttachCurrentThread(&pENV, NULL);
//        pJVM->GetEnv((void**)&pENV, JNI_VERSION_1_6);
        if(pENV == NULL){
//            pJVM->DetachCurrentThread();
            res = -1;
            break;
        }
        jclass javaClass = pENV->FindClass("can/communication");
        if(javaClass == NULL){
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
        JNIEnv* pENV = NULL;
        vm->AttachCurrentThread(&pENV, NULL);
        if(pENV == NULL){
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
    struct timeval tval;
    tval.tv_sec=secs/1000;
    tval.tv_usec=(secs*1000)%1000000;
    select(0,NULL,NULL,NULL,&tval);
}

// 读取socket接收的数据
void* readCanDataThread(void *pData){
    pSelfThreadData pThreadData = (pSelfThreadData)pData;
    while (pThreadData->flag){
        char buf[1024] = {0};
        char cmd[256] = {0};
        sprintf(cmd, "su -c ip -detail link show can%d | grep BUS-OFF", SocketCan::canNo);
        FILE *fp = popen(cmd, "r");
        fread(buf, 1, 1024, fp);
        if(strstr(buf, "BUS-OFF")){
            memset(cmd, 0, 256);
            sprintf(cmd, "su -c netcfg can%d down", SocketCan::canNo);
            system(cmd);
            memset(cmd, 0, 256);
            sprintf(cmd, "su -c netcfg can%d up", SocketCan::canNo);
            system(cmd);
        }
        pclose(fp);
        if(pThreadData->socketCan){
            pThreadData->socketCan->saveData();
        }
        sleep_ms(5);
    }
    pthread_exit(0);
    return NULL;
}


SocketCan::SocketCan(){

    SocketCan::socket_fd =  socket(PF_CAN, SOCK_RAW, CAN_RAW);                      //创建套接字
    // 套接字创建成功
    if(SocketCan::socket_fd != -1){
        do{
            struct ifreq ifr;
            char canStr[32] = {0};
            sprintf(canStr, "can%d", SocketCan::canNo);
            strcpy(ifr.ifr_name, canStr);
            int ret = ioctl(SocketCan::socket_fd, SIOCGIFINDEX, &ifr);               //指定can设备
            if(ret != 0){
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }

            //设置回环功能
            if(!SocketCan::isLoopback){
                int loopback = 0; // 0 表示关闭, 1 表示开启( 默认)
                ret = setsockopt(SocketCan::socket_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
                if(ret != 0){
                    close(SocketCan::socket_fd);
                    SocketCan::socket_fd = -1;
                    break;
                }
            }


            SocketCan::addr.can_family = AF_CAN;
            SocketCan::addr.can_ifindex = ifr.ifr_ifindex;

            ret = bind(SocketCan::socket_fd, (struct sockaddr *)&addr, sizeof(addr));                                 //将套接字与can绑定
            if(ret != 0){
                close(SocketCan::socket_fd);
                SocketCan::socket_fd = -1;
                break;
            }

            struct timeval timeout = {0,10}; //10ms
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


            // 接收自己发送的数据
            if(!SocketCan::isReceiveSelf){
                int receiveSelf = 0;
                ret = setsockopt(SocketCan::socket_fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &receiveSelf, sizeof(receiveSelf));
                if(ret != 0) {
                    close(SocketCan::socket_fd);
                    SocketCan::socket_fd = -1;
                    break;
                }
            }



            // 开启接收数据线程
            SocketCan::threadData.flag = true;
            SocketCan::threadData.socketCan = this;
            pthread_create(&(SocketCan::thread), NULL, readCanDataThread, &(SocketCan::threadData));
            struct sigaction actions;
            memset(&actions, 0, sizeof(actions));
            sigemptyset(&actions.sa_mask);
            actions.sa_flags = 0;
            actions.sa_handler = NULL;
            sigaction(SIGUSR1,&actions,NULL);
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
// 初始化参数
void SocketCan::init(int _canNo, int _baudRate, bool _isLoopback, bool _isReceiveSelf, bool _isExtendedFrame){
    if(canNo == 0 || canNo == 1){
        SocketCan::canNo = _canNo;
    }
    if(_baudRate > 0){
        SocketCan::baudRate = _baudRate;
    }
    SocketCan::isLoopback = _isLoopback;
    SocketCan::isReceiveSelf = _isReceiveSelf;
    SocketCan::isExtendedFrame = _isExtendedFrame;
}
// 获取实例
SocketCan* SocketCan::getSocketCanInstance(){
    std::lock_guard<std::mutex> guard(instanceMtx);
    char cmd[256] = {0};
    sprintf(cmd, "su -c netcfg can%d down", SocketCan::canNo);
    system(cmd);
    memset(cmd, 0, 256);
    sprintf(cmd, "su -c ip link set can%d type can bitrate %d", SocketCan::canNo, SocketCan::baudRate);
    system(cmd);
    memset(cmd, 0, 256);
    sprintf(cmd, "su -c ip link set can%d type can restart-ms 10", SocketCan::canNo);
    system(cmd);
    memset(cmd, 0, 256);
    sprintf(cmd, "su -c ip link set can%d up", SocketCan::canNo);
    system(cmd);
    memset(cmd, 0, 256);
    sprintf(cmd, "su -c netcfg can%d up", SocketCan::canNo);
    system(cmd);
    if(SocketCan::instance == NULL){
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
        SocketCan::instance = NULL;
    }
    char cmd[256] = {0};
    sprintf(cmd, "su -c netcfg can%d down", SocketCan::canNo);
    system(cmd);
}
// 通过socket向can总线发送数据
void SocketCan::sendData(pCanData pData){
    if(SocketCan::instance){
        struct can_frame frame;
        if(SocketCan::isExtendedFrame){
            frame.can_id = CAN_EFF_FLAG | (canid_t)pData->id;
        } else{
            frame.can_id = (canid_t)pData->id;
        }

        frame.can_dlc = (__u8)pData->len;
        memcpy(&(frame.data), pData->data, (size_t)pData->len);
        char buf[1024] = {0};

        ssize_t len = sendto(SocketCan::socket_fd, &frame, sizeof(struct can_frame), 0, (struct sockaddr*)&SocketCan::addr, sizeof(SocketCan::addr));
        if(len <= 0){
            char cmd[256] = {0};
            sprintf(cmd, "su -c ip -detail link show can%d | grep BUS-OFF", SocketCan::canNo);
            FILE *fp = popen(cmd, "r");
            memset(buf, 0, 1024);
            fread(buf, 1, 1024, fp);
            if(strstr(buf, "BUS-OFF")){
                memset(cmd, 0, 256);
                sprintf(cmd, "su -c netcfg can%d down", SocketCan::canNo);
                system(cmd);
                memset(cmd, 0, 256);
                sprintf(cmd, "su -c netcfg can%d up", SocketCan::canNo);
                system(cmd);
                sleep_ms(5);
            }
            pclose(fp);
            sendto(SocketCan::socket_fd, &frame, sizeof(struct can_frame), 0, (struct sockaddr*)&SocketCan::addr, sizeof(SocketCan::addr));
        }
    }else {

    }
}
// 获取缓冲区数据
void SocketCan::getData(pCanData pData){
    if(SocketCan::instance){
        std::lock_guard<std::mutex> guard(readWriteDataMtx);

    }
}
// 将数据保存至缓冲区
void SocketCan::saveData(){
    if(SocketCan::instance){

        struct can_frame frame;
        memset(&frame, 0, sizeof(frame));

        socklen_t socket_len = sizeof(struct sockaddr_can);
        ssize_t nBytes = recvfrom(SocketCan::socket_fd, &frame, sizeof(struct can_frame), 0, (struct sockaddr *)&SocketCan::addr, &socket_len);
        if(nBytes > 0){
            std::lock_guard<std::mutex> guard(readWriteDataMtx);
            if(SocketCan::isExtendedFrame){
                frame.can_id &= CAN_EFF_MASK;
            }

            // 将数据反射到java，保证消息实时性
            if(frame.can_dlc){
                // 调用 java 方法，将数据传给 java
                if(javaObj){

                    do{
                        JNIEnv *pENV = NULL;
                        pJVM->AttachCurrentThread(&pENV, NULL);
                        if(NULL == pENV){
                            pJVM->DetachCurrentThread();
                            break;
                        }
                        jclass clazz = pENV->GetObjectClass(javaObj);
                        jmethodID receiveMID = pENV->GetStaticMethodID(clazz, "receive", "(II[B)V");
                        if(NULL == receiveMID){
                            pJVM->DetachCurrentThread();
                            break;
                        }
                        jbyteArray tmpArr = pENV->NewByteArray(8);
                        pENV->SetByteArrayRegion(tmpArr, 0, 8, (jbyte*)frame.data);
                        pENV->CallStaticIntMethod(clazz, receiveMID, frame.can_id, frame.can_dlc, tmpArr);
                        pJVM->DetachCurrentThread();
                    }while (0);
                }
            }
        }
    }
}
