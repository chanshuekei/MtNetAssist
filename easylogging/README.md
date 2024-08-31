# pro工程文件中加入以下宏定义
```
DEFINES += ELPP_QT_LOGGING \
    ELPP_UNICODE \
    ELPP_NO_DEFAULT_LOG_FILE \
    ELPP_THREAD_SAFE \
    ELPP_WINSOCK2
```

# main函数初始化
```C++
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#include <QDir>

void setupEasyLogging(int argc, char *argv[])
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QDir dir(QCoreApplication::applicationDirPath() + "/logs");
    if (!dir.exists()) {
        dir.mkdir(dir.absolutePath());
    }

    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
    el::Configurations conf("./config/log.conf");
    el::Loggers::reconfigureAllLoggers(conf);
    START_EASYLOGGINGPP(argc, argv);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    setupEasyLogging(argc, argv);

    return a.exec();
}

```

# 其他
* 定义了宏`#define X(text) QString::fromLocal8Bit(text)`
* 配置文件:log.conf

# TODO
* 测试是否按日期切换