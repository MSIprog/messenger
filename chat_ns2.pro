TARGET = chat_ns2
TEMPLATE = app
QT += widgets network
SOURCES += main.cpp \
    user_list_widget.cpp \
    type_field.cpp \
    detection_server.cpp \
    seeker_client.cpp \
    signaling.cpp \
    message_form.cpp \
    block_queue.cpp \
    messenger_signaling.cpp \
    resource_holder.cpp \
    signaling_facade.cpp \
    file_form.cpp \
    settings.cpp \
    file_signaling.cpp
HEADERS += user_list_widget.h \
    type_field.h \
    detection_server.h \
    message_form.h \
    seeker_client.h \
    signaling.h \
    block_queue.h \
    messenger_signaling.h \
    resource_holder.h \
    signaling_facade.h \
    file_form.h \
    settings.h \
    file_signaling.h
FORMS += user_list_widget.ui \
    message_form.ui \
    file_form.ui
RESOURCES += resources.qrc
