#include "HttpServer.h"
#include "base/Logging.h"

using namespace mymuduo;
using namespace mymuduo::net;

namespace mymuduo {
namespace net {
namespace detail {

bool defaultHttpCallback(const TcpConnectionPtr&, const HttpRequest&, HttpResponse* resp) {
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
    return true;
}

} // namespace detail
} // namespace net
} // namespace mymuduo

HttpServer::HttpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const std::string& name)
    : server_(loop, listenAddr, name),
      httpCallback_(detail::defaultHttpCallback)
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1)); // 实际在 main 中被 HttpUploadHandler::onConnection 覆盖掉了
    server_.setMessageCallback( // 当连接调用 readv 读取数据后，触发这个回调函数 【TcpConnection::handleRead】
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1,
                 std::placeholders::_2, std::placeholders::_3));
}

void HttpServer::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        auto context = std::make_shared<HttpContext>();
        conn->setContext(context);
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                         Buffer* buf,
                         Timestamp receiveTime) {
    auto context = std::static_pointer_cast<HttpContext>(conn->getContext());

    if(!context){
        LOG_INFO << "context is null";
        return;
    }

    HttpContext::ParseResult result = context->parseRequest(buf, receiveTime);
    LOG_INFO << "Http req parse state = " << result;
    if (result == HttpContext::kError) {  // 解析出错
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
        return;
    }

    if (result == HttpContext::kHeadersComplete) {  // 仅头部解析完成，请求体还没解析完成
        // 如果是大文件上传，在这里就可以开始处理了
        if (context->expectBody()) { // 需要解析请求体
            HttpRequest& req = context->request();
            // 检查是否是文件上传请求
            if (req.path() == "/upload" && req.method() == HttpRequest::kPost) {
                // 检查body数据大小
                size_t bufSize = req.body().size();
                // LOG_INFO << "bufSize = " << bufSize;
                if (bufSize >= 1024 * 1024) {  // 如果数据超过1MB
                    // LOG_INFO << "Buffer size exceeds 1MB, processing chunk";
                    HttpResponse response(false);  // 不关闭连接
                    // 为什么仅解析头部后就调用回调函数处理请求？因为大文件上传时，数据可能分多次到达，头部解析完成后就可以开始处理了，后续的数据会继续到达并被解析到请求体中，回调函数可以根据已经解析的头部信息来处理后续的数据，比如写入文件等，而不需要等到整个请求体都解析完成后才处理，这样可以提高效率和响应速度
                    bool syncProcessed = httpCallback_(conn, req, &response); // 调用 HttpUploadHandler::onRequest 的回调函数处理请求
                    if (!syncProcessed) {
                        // 异步处理，不重置 context
                        LOG_INFO << "Async upload chunk processing";
                        return;
                    } else {
                        LOG_INFO << "Sync upload chunk processed";
                    }
                }
            }
        }
    } else if (result == HttpContext::kGotRequest) {  // 整个请求解析完成
        bool syncProcessed = onRequest(conn, context->request());
        if (syncProcessed) {
            LOG_INFO << "context->reset()";
            context->reset();
        }
    } else {
        LOG_INFO << "need more data";
    }
    // 如果不是文件上传请求，或者文件上传请求但数据还没超过1MB，就等到整个请求体都解析完成后再处理，这样可以避免频繁调用回调函数处理小块数据，提高效率
    // q: 如果不是文件上传请求，那么要不要给发送方响应？
    // a: 如果不是文件上传请求，那么在整个请求体都解析完成后才调用回调函数处理请求，这样可以确保回调函数处理的是完整的请求数据，避免处理不完整的数据导致错误；
    // 如果在头部解析完成后就调用回调函数处理请求，那么回调函数可能会处理不完整的数据，导致错误或者需要额外的逻辑来处理这种情况，这样会增加复杂性和出错的风险，所以通常情况下，非文件上传请求应该等到整个请求体都解析完成后再处理。
    LOG_DEBUG << "onMessage end";
}

bool HttpServer::onRequest(const TcpConnectionPtr& conn, HttpRequest& req) {
    // LOG_DEBUG << "onRequest start";
    const std::string& connection = req.getHeader("Connection"); // 获取 Connection 头部字段的值，判断是否需要关闭连接
    bool close = connection == "close" ||
        (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive"); // HTTP/1.0 默认关闭连接，除非 Connection 头部字段是 Keep-Alive；HTTP/1.1 默认保持连接，除非 Connection 头部字段是 close
    HttpResponse response(close);

    // 调用用户的回调函数处理请求
    bool syncProcessed = httpCallback_(conn, req, &response); // 调用 HttpUploadHandler::onRequest 的回调函数处理请求

    // 如果是同步处理完成，或者不是异步响应，直接发送响应
    // q: 为什么要区分同步处理和异步处理请求？
    // a: 因为如果是同步处理完成，说明回调函数已经处理完请求并生成了响应，可以直接发送响应；
    //    如果是异步处理，说明回调函数还在处理请求或者等待某些事件完成，不能直接发送响应，需要等到异步处理完成后再发送响应，这样可以避免在回调函数还没处理完请求时就发送了不完整的响应，
    //    导致错误或者不一致的行为。例如，在文件上传的场景中，回调函数可能需要等待文件写入完成或者其他异步事件完成后才能生成完整的响应，如果在回调函数还没处理完请求时就发送了响应，可能会导致响应不完整或者错误，所以需要区分同步处理和异步处理。
    if (syncProcessed) { // 同步处理完成，直接发送响应
        Buffer buf;
        response.appendToBuffer(&buf);
        conn->send(&buf);
        if (response.closeConnection()) {
            conn->shutdown();
        }
        LOG_INFO << "Sync request completed";
    } else {
        LOG_INFO << "Async request, waiting for response";
    }
    
    return syncProcessed;
} 