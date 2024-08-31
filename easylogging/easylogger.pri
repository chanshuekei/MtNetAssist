INCLUDEPATH += $$PWD

HEADERS += \
        $$PWD/easylogging++.h

SOURCES += \
        $$PWD/easylogging++.cc

DEFINES += ELPP_QT_LOGGING \
    ELPP_UNICODE \
    ELPP_NO_DEFAULT_LOG_FILE \
    ELPP_THREAD_SAFE \
    ELPP_WINSOCK2
