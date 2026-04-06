#pragma once

#include <map>
#include <string>
#include "Buffer.h"
#include <functional>

namespace mymuduo {
namespace net {

class HttpResponse {
public:
    // 定义响应回调函数类型
    using ResponseCallback = std::function<void(const HttpResponse&)>;

    enum HttpStatusCode {
        kUnknown = 0,
        k200Ok = 200,
        k206PartialContent = 206,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k416RangeNotSatisfiable = 416, //客户端请求的资源范围无效或无法满足
        k500InternalServerError = 500,
    };

    explicit HttpResponse(bool close)
        : statusCode_(kUnknown),
          closeConnection_(close),
          async_(false)
    {
    }
    ~HttpResponse() {
        LOG_INFO << "HttpResponse::~HttpResponse()";
    }

    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }
    void setStatusMessage(const std::string& message) { statusMessage_ = message; }
    void setCloseConnection(bool on) { closeConnection_ = on; }
    bool closeConnection() const { return closeConnection_; }

    void setContentType(const std::string& contentType) { addHeader("Content-Type", contentType); }
    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }
    void setBody(const std::string& body) { body_ = body; }

    // 设置为异步响应
    void setAsync(bool async) { async_ = async; }
    bool isAsync() const { return async_; }

    // 设置响应回调
    void setResponseCallback(const ResponseCallback& cb) { responseCallback_ = cb; }
    const ResponseCallback& getResponseCallback() const { return responseCallback_; }

    // 将HttpResponse对象转换为HTTP响应格式并写入Buffer
    void appendToBuffer(Buffer* output) const {
        char buf[32];

        // 构建状态行
        snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", statusCode_);
        output->append(buf);
        output->append(statusMessage_);
        output->append("\r\n"); 

        if (headers_.find("Connection") == headers_.end() &&
            headers_.find("connection") == headers_.end()) { // 如果没有显式设置Connection头，根据closeConnection_的值添加相应的Connection头
            if (closeConnection_) {
                output->append("Connection: close\r\n");
            } else {
                output->append("Connection: Keep-Alive\r\n");
            }
        }

        if (headers_.find("Content-Length") == headers_.end() &&
            headers_.find("content-length") == headers_.end() &&
            headers_.find("Transfer-Encoding") == headers_.end()) { // 如果没有显式设置Content-Length或Transfer-Encoding头，根据body_的长度添加Content-Length头
            output->append("Content-Length: ");
            output->append(std::to_string(body_.size()));
            output->append("\r\n");
        }

        for (const auto& header : headers_) { // 添加用户自定义的头部
            output->append(header.first);
            output->append(": ");
            output->append(header.second);
            output->append("\r\n");
        }

        output->append("\r\n");
        output->append(body_);
    }

private:
    std::map<std::string, std::string> headers_;
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    bool closeConnection_;
    std::string body_;
    bool async_;                    // 是否为异步响应
    ResponseCallback responseCallback_;  // 响应回调函数
}; // class HttpResponse

} // namespace net
} // namespace mymuduo 