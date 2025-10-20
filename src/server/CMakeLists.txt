# 定义SRC_LIST变量，包含次目录下所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
# 指定生成可执行的稳健
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 链接动态库
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)