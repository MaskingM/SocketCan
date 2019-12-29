# SocketCan
Android JNI 应用，用于 嵌入式 Android can总线通信

Socket Can子系统是在Linux下CAN协议(Controller Area Network)实现的一种实现方法。  
本库主要应用于 Android 下实现 can 总线通信  

**主要功能：**  
1.**init** 初始化 SocketCan 用于配置接收发送相关的配置  
2.**open** 打开 SocketCan  
3.**close** 关闭 SocketCan  
4.**send**  发送数据
5.**receive** 接收数据，接收数据采用反射的方式，保证数据实时性
