package Can;

public class SocketCAN  {
    /*
    * @brief 本地库
    */
    static {
        System.loadLibrary("socketcan_lib");
    }

    /**
     * @brief 打开Socket Can
     * @return void
     */
    public static native boolean OpenCan(boolean root);

    /**
     * @brief 关闭Socket Can
     */
    public static native void CloseCan();

    /**
     * @brief 打开Socket Can
     * @param id int 帧ID
     * @param len int 数据长度
     * @param data byte[]
     */
    public static native void CanSendData(int id, int len, byte[] data);

    /**
    * @brief 反射CAN数据*
    * @param id int 数据帧ID
    * @param len int 数据长度
    * @param data byte[]
    */
    public static void CanReceiveDataFromJNI(int id, int len, byte[] data){
        // 数据会反射到此处，在此处添加程序数据处理功能

    }
}