/*
 * @Author       : mark
 * @Date         : 2020-06-18
 * @copyleft Apache 2.0
 */ 
#include <unistd.h>
#include "server/server.h"

int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    server server(
        1316, 3, 5000, false, 6,       /* 端口 ET模式 timeoutMs  优雅退出  */
        3306, "root", "root", "webserver", 12,
        false, 0, 1024, "Node", "effNet_DCL.pt", "idx2label.json");              /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.start();
} 
  
