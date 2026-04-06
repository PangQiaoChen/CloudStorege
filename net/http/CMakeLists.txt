set(http_SRCS
    HttpServer.cc
)

add_library(mymuduo_net_http ${http_SRCS})
target_link_libraries(mymuduo_net_http mymuduo_net)

install(TARGETS mymuduo_net_http DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/mymuduo/net/http) 