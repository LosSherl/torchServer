#include <unistd.h>
#include <cassert>
#include <vector>

#include "server/server.h"


int main(int argc, char* argv[]) {
    /* 守护进程 后台运行 */
    // daemon(1, 0); 
    server web_server(1316, 3, 5000, false,        /* 端口 ET模式 timeoutMs  优雅退出  */
                6, false, 0, 1024, "Node");              /* 线程池数量 日志开关 日志等级 日志异步队列容量 */
    web_server.start();

} 
  