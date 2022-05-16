#include <jni.h>
#include <string>
#include "SocketCan.h"

extern "C" {
    /**
     * @brief 打开SocketCAN总线
     * @env JNI系统环境（系统自动传输）
     * @obj jclass java类型（系统自动传输）
     * @root jboolean 是否需要root权限打开
     * @return jboolean
     */
    JNIEXPORT jboolean Java_Can_SocketCAN_OpenCan(JNIEnv *env, jclass obj, jboolean root){
        jboolean res = false;
        SocketCan* instance = SocketCan::getSocketCanInstance(root);
        if(instance){
            res = true;
            LOG_INFO("打开can口成功，套接字创建成功123");
        } else {
            res = false;
            LOG_INFO("打开can口失败，套接字创建失败123");
        }
        return res;
    }

    /**
     * @brief 关闭SocketCan总线
     * @env JNI系统环境（系统自动传输）
     * @obj jclass java类型（系统自动传输）
     * @return void
     */
    JNIEXPORT void Java_Can_SocketCAN_CloseCan(JNIEnv *env, jclass obj){
        SocketCan::destroySocketCan();
        LOG_INFO("关闭can口");
    }

    /**
     * @brief CAN总线发送数据
     * @env JNI系统环境（系统自动传输）
     * @obj jclass java类型（系统自动传输）
     * @id jint 帧ID
     * @len jint 帧长度
     * @buf jbyteArray 帧数据
     * @return void
     */
    JNIEXPORT void Java_Can_SocketCAN_CanSendData(JNIEnv *env, jclass obj, jint id, jint len, jbyteArray buf){
        canData data = {0};
        data.id = id;
        data.len = len;
        if(data.len > 8){
            data.len = 8;
        }
        jbyte* pByte = env->GetByteArrayElements(buf, JNI_FALSE);
        for(int i = 0; i < data.len; i++){
            data.data[i] = *((char*)(pByte+i));
        }
//        LOG_INFO("发送数据，帧ID：0x%08X", data.id);
        SocketCan::sendData(&data);
        // 释放java引用
        env->ReleaseByteArrayElements(buf, pByte, JNI_FALSE);
    }

}




