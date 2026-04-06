# C++ 云存储项目

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.10+-brightgreen.svg)](https://cmake.org/)

## 项目概述
本项目是一个基于 muduo 网络库风格实现的云存储项目。项目采用 C++17 标准，实现了完整的 HTTP 服务器、文件上传下载（断点续传）、用户认证、会话管理（Session-ID）和文件分享等功能。


### 主要特性
- 🚀 **高性能网络库**：基于 Reactor 模式的高并发网络库，支持多线程
- 📁 **文件管理**：完整的文件上传、下载（断点续传）、列表展示功能
- 🔐 **用户认证**：支持用户注册、登录、会话管理（Session-ID）
- 🔗 **文件分享**：支持多种分享方式（私有、公开、受保护（分享码机制）、指定用户）
- 🎯 **现代化架构**：清晰的模块分离，遵循 C++ 实践
- 📊 **数据库支持**：完整的 MySQL 数据库设计和数据持久化

## 目录结构
```
cloudStorege/
├── base/           # 基础设施
├── net/            # 网络核心
├── application/    # 项目代码
``` 

## 快速开始

### 环境要求

- **编译器**: GCC 7+ 或 Clang 5+ 支持C++17
- **构建工具**: CMake 3.10+
- **数据库**: MySQL 5.7+
- **依赖库**: 
  - Boost 库
  - MySQL 客户端库 (libmysqlclient-dev)

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev libmysqlclient-dev
```

### 编译与运行

#### 编译
```bash
mkdir build
cd build
cmake ..
make
```

#### 运行
```bash
# 进入构建目录
cd build/bin

# 启动文件上传服务
./http_upload

# 服务默认监听 8080 端口
# 在浏览器中访问 http://localhost:8080
```

## 数据库设计

项目使用 MySQL 数据库存储用户信息、文件数据和分享记录。数据库设计包含以下表：

1. 用户表(users)
2. 会话表(sessions)
3. 文件表(files)
4. 文件分享表(file_shares)

### 数据库常用命令
```
// 使用用户名+密码导入数据库
mysql -u username -ppassword < file_manager.sql
// 启动 MySQL 服务
sudo systemctl start mysql
// 停止 MySQL 服务
sudo system stop mysql
// 设置开机自启
sudo systemctl enable mysql
// 查看数据库状态
sudo systemctl status mysql
// 导入 file_manager.sql （仅需一次）
mysql -u <你的数据库用户名> -p<你的密码> < file_manager.sql
``` 
例如 username 是 root，password 是 123456，即是
```
mysql -u root -p123456 < file_manager.sql
```

## API接口

### 文件管理接口

```
GET  /                    # 首页，文件列表
GET  /files               # 获取文件列表
POST /upload              # 上传文件
GET  /download/{filename} # 下载文件
POST /share/{file_id}     # 分享文件
GET  /shared/{share_code} # 访问分享文件
```

### 用户接口

```
POST /register           # 用户注册
POST /login              # 用户登录
POST /logout             # 用户登出
GET  /profile            # 获取用户信息
```

## 系统时序图

项目包含了详细的系统时序图，帮助理解各个模块的工作流程：

### 📊 核心时序图

1. **回调函数时序图**
   - 路径: `时序图/回调函数时序图.md` / `时序图/回调函数时序图.png`
   - 描述: 展示了事件循环中回调函数的执行流程

2. **事件监听与通信时序图**
   - 路径: `时序图/事件监听与通信时序图.md` / `时序图/事件监听与通信时序图.png`
   - 描述: 展示了事件监听器与各组件间的通信机制

### 📁 文件管理时序图

3. **文件上传底层流转时序图**
   - 路径: `时序图/文件上传底层流转时序图.md` / `时序图/文件上传底层流转时序图.png`
   - 描述: 详细展示了文件从客户端到服务器的完整流转过程

4. **文件下载与断点续传**
   - 路径: `时序图/文件下载与断点续传.md` / `时序图/文件下载与断点续传.png`
   - 描述: 展示了文件下载的流程和断点续传的实现机制

5. **文件分享时序图**
   - 路径: `时序图/文件分享时序图.md` / `时序图/文件分享时序图.jpg`
   - 描述: 展示了文件分享的完整流程，包括分享码生成和权限验证

### 🔐 安全与认证时序图

6. **用户认证与Session管理**
   - 路径: `时序图/用户认证与Session管理.md` / `时序图/用户认证与Session管理.png`
   - 描述: 展示了用户登录、认证和Session管理的完整流程

### 🎥 多媒体时序图

7. **视频流式传输时序图**
   - 路径: `时序图/视频流式传输时序图.md` / `时序图/视频流式传输时序图.png`
   - 描述: 展示了视频文件的流式传输和播放机制



## 性能优化

### 已实现的优化

1. **多线程支持**：使用线程池处理并发请求
2. **零拷贝技术**：优化文件传输性能
3. **连接复用**：保持数据库连接池
4. **内存池**：减少内存分配开销

### 进一步的优化方向

1. **缓存策略**：添加 Redis 缓存支持
2. **CDN集成**：集成 CDN 加速文件分发
3. **负载均衡**：支持多实例部署
4. **监控指标**：添加性能监控和日志系统

