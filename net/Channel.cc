#include "Channel.h"
#include "EventLoop.h"
#include <sstream>
#include <poll.h>
#include "base/Logging.h"

using namespace mymuduo;
using namespace mymuduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI; // poll 的参数与 epoll 的参数一样
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      eventHandling_(false),
      addedToLoop_(false),
      logHup_(true),
      tied_(false) {
}

Channel::~Channel() {
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if (loop_->isInLoopThread()) {
        assert(!loop_->hasChannel(this));
    }
}

void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::tie(const std::shared_ptr<void>& obj) { // 监听 fd 的 Channel 不需要绑定主人，其他 fd 的 Channel 都需要绑定主人（TcpConnection）
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) { // 1. 检查：我有没有被绑定给某个主人(TcpConnection)？
        // 2. 尝试用 weak_ptr 的 lock() 提升为一个 shared_ptr。
        // lock() 的作用是：如果主人还活着，就返回一个指向它的智能指针，
        // 并且！在 guard 存活的这几行代码期间，主人的引用计数会+1，绝对死不了！
        // 如果主人已经被销毁了，它就会返回空指针。
        std::shared_ptr<void> guard = tie_.lock();
        
        if (guard) { 
            // 3. 运气好，主人还活着。在这期间，由于 guard 的存在，
            // 主人肯定不会死，所以放心地进去调用主人的回调函数！
            handleEventWithGuard(receiveTime);
            // 离开大括号后，guard 销毁，主人该死的话正常去死。
        }
        // 如果 guard 为空，说明主人已经先走一步了。
        // 那我就什么也不做，一声不吭，防止调用主人的废弃回调导致崩溃程序。
    } else {
        // 如果没人给我绑定主人（对于 Listen fd 的 AcceptChannel 一般不用绑，
        // 因为它的生命周期跟整个 HttpServer 一样长，永远不会提前死）
        // 那就直接调用。
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    eventHandling_ = true;
    LOG_TRACE << reventsToString();

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) { // POLLHUP 连接断开， POLLIN 没有数据可读了
        if (logHup_) {
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL) { // POLLNVAL 文件描述符无效，可能是 Channel 没有正确添加到 EventLoop 中，或者已经被删除了
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)) { // POLLERR 发生错误， POLLNVAL 文件描述符无效
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) { // POLLIN 有数据可读， POLLPRI 有紧急数据可读， POLLRDHUP 对端关闭连接或者半关闭连接
        if (readCallback_) readCallback_(receiveTime);
    }

    if (revents_ & POLLOUT) { // POLLOUT 文件描述符可写
        if (writeCallback_) writeCallback_();
    }

    eventHandling_ = false;
}

string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev) {
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN)
        oss << "IN ";
    if (ev & POLLPRI)
        oss << "PRI ";
    if (ev & POLLOUT)
        oss << "OUT ";
    if (ev & POLLHUP)
        oss << "HUP ";
    if (ev & POLLRDHUP)
        oss << "RDHUP ";
    if (ev & POLLERR)
        oss << "ERR ";
    if (ev & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}


