# SocketCan
Android JNI 应用，用于 嵌入式 Android can总线通信

Socket Can子系统是在Linux下CAN协议(Controller Area Network)实现的一种实现方法。  
本库主要应用于 Android 下实现 can 总线通信  

**主要功能：**  
1.**OpenCan** 打开 SocketCan  
2.**closeCan** 关闭 SocketCan  
3.**CanSendData**  发送数据
4.**CanReceiveDataFromJNI** 通过反射函数，将数据传递到java端
