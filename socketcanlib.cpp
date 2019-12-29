#include "SocketCan.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 初始化 SocketCan
 * @param   canNo jint can 接口类型
 * @param   baudRate jint 波特率
 * @param   isLoopback jboolean 是否开启回环测试
 * @param   isReceiveSelf jboolean 是否接收自己消息
 * @param   isExtendedFrame boolean 是否设置扩展帧
 * @return void
*/

JNIEXPORT void Java_can_communication_init(
        JNIEnv *env /* in */,
        jobject obj /* in */,
        jint canNo /* in */,
        jint baudRate /* in */,
        jboolean isLoopback /* in */,
        jboolean isReceiveSelf /* in */,
        jboolean isExtendedFrame /* in */
){
    SocketCan::init(canNo, baudRate, isLoopback, isReceiveSelf, isExtendedFrame);
}

/*
 * 打开 SocketCan
 * @return jint    0 成功  -1 失败
*/
JNIEXPORT jint Java_can_communication_open(
        JNIEnv *env /* in */,
        jobject obj /* in */
){
    jint res = 0;
    do{
        SocketCan* socket = SocketCan::getSocketCanInstance();
        if(!socket){
            res = -1;
        }
    }while(0);
    return res;
}

/*
 * 关闭 SocketCan
 * @return void
 */
JNIEXPORT void Java_can_communication_close(
        JNIEnv *env /* in */,
        jobject obj /* in */
){
    SocketCan::destroySocketCan();
}

/*
 * 发送数据
 * @param id    jint 消息id
 * @param len   jint 消息长度
 * @param buf   jbyteArray 消息数据
 * @return void
 */
JNIEXPORT void Java_can_communication_send(JNIEnv *env, jobject obj, jint id, jint len, jbyteArray buf){
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
    SocketCan::sendData(&data);
    // 释放java引用
    env->ReleaseByteArrayElements(buf, pByte, JNI_FALSE);
}

#ifdef __cplusplus
}
#endif