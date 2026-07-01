/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */


#include "domain_socket.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <system_error>
#include "umask_guard.h"
#include "securec.h"
#include "file_system.h"
#include "log.h"

namespace {
constexpr mode_t SOCK_UMASK = 0177;
}

namespace Sanitizer {
DomainSocket::DomainSocket(std::string socketPath) : socketPath_(socketPath) { }

DomainSocket::~DomainSocket()
{
    if (sfd_ != -1) {
        close(sfd_);
    }
}

Result DomainSocket::CreateSocket()
{
    Result result;
    sfd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd_ == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("socket failed. " + ec.message());
        return result;
    }

    if (memset_s(&addr_, sizeof(addr_), 0, sizeof(addr_)) != 0) {
        result.SetError("socket struct memset_s zero failed.");
        return result;
    }
    addr_ = sockaddr_un{};
    addr_.sun_family = AF_UNIX;
    size_t minSunPathLeft = 2;
    socketPath_.copy(addr_.sun_path + 1, std::min(sizeof(addr_.sun_path) - minSunPathLeft, socketPath_.size()));

    auto timeout = timeval {};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout,
        sizeof(timeout)) == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("setsockopt failed. " + ec.message());
        return result;
    }
    int opt = 1;
    if (setsockopt(sfd_, SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt)) == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("Socket set SO_PEERCRED failed: " + ec.message());
        return result;
    }

    return result;
}

Result DomainSocket::Clean(void)
{
    Result result;
    if (remove(addr_.sun_path) == -1 && errno != ENOENT) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("remove failed. " + ec.message());
    }
    return result;
}

DomainSocketServer::DomainSocketServer(std::string socketPath, std::size_t maxClientNum)
    : DomainSocket(socketPath), maxClientNum_(maxClientNum) { }

DomainSocketServer::~DomainSocketServer(void)
{
    for (int32_t fd : cfds_) {
        close(fd);
    }
    unlink(addr_.sun_path);
}

Result DomainSocketServer::ListenAndBind()
{
    //  for server: socket() -> bind() -> listen -> accept() -> read/write()
    Result result = CreateSocket();
    if (result.Fail()) {
        return result;
    }
    {
        UmaskGuard umaskGuard(SOCK_UMASK);
        if (bind(sfd_, reinterpret_cast<sockaddr *>(&addr_), sizeof(addr_)) == -1) {
            std::error_condition ec(errno, std::generic_category());
            result.SetError("bind failed. " + ec.message());
            return result;
        }
    }

    if (listen(sfd_, 1) == -1) { // 仅支持最多1个client
        std::error_condition ec(errno, std::generic_category());
        result.SetError("listen failed. " + ec.message());
        return result;
    }
    return result;
}

Result DomainSocketServer::Accept(ClientId &id)
{
    Result result;
    if (cfds_.size() >= maxClientNum_) {
        result.SetError("over max client num. " + std::string(strerror(errno)));
        return result;
    }
    int32_t cfd = accept(sfd_, nullptr, nullptr);
    if (cfd == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("accept failed. " + ec.message());
        return result;
    }
    // 获取客户端凭证（uid/gid）
    struct ucred cred{};
    socklen_t cred_len = sizeof(cred);
    if (getsockopt(cfd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("get client SO_PEERCRED failed: " + ec.message());
        close(cfd);
        return result;
    }
    // 检查客户端凭证（uid/gid）与当前进程是否一致
    if (getuid() != cred.uid || getgid() != cred.gid) {
        result.SetError("client SO_PEERCRED check permission failed, recv id: uid=" + std::to_string(cred.uid) +
                        ", gid=" + std::to_string(cred.gid));
        return result;
    }
    {
        std::lock_guard<std::mutex> guard(cfdsMutex_);
        id = cfds_.size();
        cfds_.push_back(cfd);
    }
    return result;
}

std::size_t DomainSocketServer::GetClientNum() const
{
    return cfds_.size();
}

Result DomainSocketServer::Read(ClientId id, std::string &message, size_t maxBytes, size_t &receivedBytes)
{
    Result result;
    int32_t cfd;
    {
        std::lock_guard<std::mutex> guard(cfdsMutex_);
        if (id >= cfds_.size() || cfds_[id] == -1) {
            result.SetError("invalid client id ");
            return result;
        }
        cfd = cfds_[id];
    }

    std::vector<char> buffer(maxBytes);
    ssize_t ret = read(cfd, buffer.data(), maxBytes);
    if (ret == -1) {
        result.SetError("recv failed. " + std::string(strerror(errno)));
        return result;
    }
    receivedBytes = static_cast<size_t>(ret);
    message.assign(buffer.data(), receivedBytes);
    return result;
}

Result DomainSocketServer::Write(ClientId id, const std::string &message, size_t &sentBytes)
{
    Result result;
    int32_t cfd;
    {
        std::lock_guard<std::mutex> guard(cfdsMutex_);
        if (id >= cfds_.size() || cfds_[id] == -1) {
            result.SetError("invalid client id ");
            return result;
        }
        cfd = cfds_[id];
    }

    auto buffer = message.data();
    auto size = message.size();
    ssize_t ret;
    sentBytes = 0;

    while (size > 0) {
        ret = write(cfd, buffer, size);
        if (ret == -1) {
            // 如果write返回-1说明对端已关闭，返回已写入字符数
            result.SetError("write failed. " + std::string(strerror(errno)));
            break;
        }
        // 写入成功，ret为已写入字节数
        size_t writeBytes = static_cast<size_t>(ret);
        sentBytes += writeBytes;
        size -= writeBytes;
        buffer += writeBytes;
    }
    return result;
}

DomainSocketClient::DomainSocketClient(std::string socketPath) : DomainSocket(socketPath) {}

DomainSocketClient::~DomainSocketClient(void) { }

Result DomainSocketClient::Connect()
{
    // for client: socket() -> connect() -> read/write()
    Result result = CreateSocket();
    if (result.Fail()) {
        std::cerr<<"Error in create socket:"<<strerror(errno)<<"(errno:"<<errno<<")"<<std::endl;
        return result;
    }
    // client uses socket sfd_ for read/write operation
    if (connect(sfd_, reinterpret_cast<sockaddr *>(&addr_), sizeof(addr_)) == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("connect failed. " + ec.message());
        return result;
    }
    return result;
}

Result DomainSocketClient::Read(std::string &message, uint64_t maxBytes, size_t &receivedBytes)
{
    Result result;
    if (sfd_ == -1) {
        result.SetError("connect failed. ");
        return result;
    }

    std::vector<char> buffer(maxBytes);
    ssize_t ret = read(sfd_, buffer.data(), maxBytes);
    if (ret == -1) {
        std::error_condition ec(errno, std::generic_category());
        result.SetError("read failed. " + ec.message());
        return result;
    }
    receivedBytes = static_cast<size_t>(ret);
    message.assign(buffer.data(), receivedBytes);
    return result;
}

Result DomainSocketClient::Write(const std::string &message, size_t &sentBytes)
{
    Result result;
    if (sfd_ == -1) {
        result.SetError("connect failed. ");
        return result;
    }

    auto buffer = message.data();
    auto size = message.size();
    ssize_t ret;
    sentBytes = 0;

    while (size > 0) {
        ret = write(sfd_, buffer, size);
        if (ret == -1) {
            // 如果write返回-1，则直接返回，sentBytes返回已发送字节数
            std::error_condition ec(errno, std::generic_category());
            result.SetError("write failed. " + ec.message());
            break;
        }
        // 发送成功，ret为已发送字节数
        size_t writeBytes = static_cast<size_t>(ret);
        sentBytes += writeBytes;
        size -= writeBytes;
        buffer += writeBytes;
    }

    return result;
}

}
