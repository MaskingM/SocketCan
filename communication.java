package can;

public class communication {
    static {
        System.loadLibrary("socketcanlib");
    }

    /*
     * 初始化 SocketCan
     * @param   canNo int can 接口类型
     * @param   baudRate int 波特率
     * @param   isLoopback boolean 是否开启回环测试
     * @param   isReceiveSelf boolean 是否接收自己消息
     * @param   isExtendedFrame boolean 是否设置扩展帧
     * @return void
     */
    public static native void init(int canNo, int baudRate, boolean isLoopback, boolean isReceiveSelf, boolean isExtendedFrame);

    /*
     * 初始化 SocketCan
     * @return int    0 成功  -1 失败
     */
    public static native int open();

    /*
     * 关闭 SocketCan
     * @return void
     */
    public static native void close();

    /*
     * 发送数据
     * @param id    int 消息id
     * @param len   int 消息长度
     * @param data   byte[] 消息数据
     * @return void
     */
    public static native void send(int id, int len, byte[] data);

    /*
     * 接收数据
     * @param id    int 消息id
     * @param len   int 消息长度
     * @param data   byte[] 消息数据
     * @return void
     */
    public static void receive(int id, int len, byte[] data){
        // 此处实现自己的业务逻辑
    }
}
