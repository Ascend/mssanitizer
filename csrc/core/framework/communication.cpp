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


#include "communication.h"

#include <memory>
#include <sstream>
#include <functional>
#include "utility/log.h"
#include "utility/cpp_future.h"
#include "checker.h"
#include "protocol.h"


namespace Sanitizer {

/* Communication */
CommunicationServer::CommunicationServer(const std::string& socketPath)
{
    acceptWorkerRun_ = true;
    socket_ = MakeUnique<DomainSocketServer>(socketPath, maxClientNum_);
}

CommunicationServer::~CommunicationServer()
{
    acceptWorkerRun_ = false;
    if (acceptWorker_.joinable()) {
        acceptWorker_.join();
    }
}

void CommunicationServer::StartListen()
{
    Result result = socket_->ListenAndBind();
    runFlag_ = true;
    if (result.Fail()) {
        socket_ = nullptr;
        return;
    }

    acceptWorker_ = std::thread([this]() {
        while (acceptWorkerRun_ && socket_->GetClientNum() < maxClientNum_) {
            ClientId clientId;
            Result result = socket_->Accept(clientId);
            if (result.Fail()) {
                continue;
            }
            if (clientConnectHook_) {
                std::lock_guard<std::mutex> lock(threadMutex_);
                clientConnectHook_(clientId);
                std::thread th = std::thread(&CommunicationServer::Listen, this, clientId);
                clientThreads_.emplace_back(std::move(th));
            }
        }
    });
}

void CommunicationServer::Listen(ClientId clientId)
{
    std::string msg;
    while (msg.size() != 0 || runFlag_) {
        msg.clear();
        Result result = Read(clientId, msg);
        if (result.Fail()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 等待1000ms重试
            continue;
        }
        // 用于写回执行结果
        MsgResponseFunc msgRspFunc = [this, clientId](const std::string &response) {
            if (!response.empty() && this->Write(clientId, response).Fail()) {
                SAN_ERROR_LOG("Failed to write back message response. response:(len: %lu).", response.size());
            }
        };
        if (msgHandler_ != nullptr && msg.size()) {
            msgHandler_(msg, msgRspFunc);
        }
    }
}

Result CommunicationServer::Read(ClientId clientId, std::string &msg)
{
    Result result;
    if (socket_ == nullptr) {
        result.SetError("socket is null");
        return result;
    }

    constexpr std::size_t maxSize = 1024ULL;
    size_t readSize = 0;
    result = socket_->Read(clientId, msg, maxSize, readSize);
    if (result.Fail()) {
        result.SetError("read error");
        return result;
    }
    return result;
}

Result CommunicationServer::Write(ClientId clientId, std::string const &msg)
{
    Result result;
    if (socket_ == nullptr) {
        result.SetError("socket is null");
        return result;
    }

    size_t sendBytes = 0;
    result = socket_->Write(clientId, msg, sendBytes);
    if (result.Fail()) {
        result.SetError("write error");
        return result;
    }
    return result;
}

void CommunicationServer::Close()
{
    runFlag_ = false;
    {
        std::lock_guard<std::mutex> lock(threadMutex_);
        for (std::thread &t : clientThreads_) {
        if (t.joinable()) {
            t.join();
            }
        }
    }
    socket_->Clean();
}

void CommunicationServer::SetClientConnectHook(ClientConnectHook &&hook)
{
    clientConnectHook_ = hook;
}

void CommunicationServer::RegisterMsgHandler(const MsgHandleFunc &func)
{
    msgHandler_ = func;
}
/* CommunicationClient */

CommunicationClient::CommunicationClient(std::string socketPath)
{
    socket_ = MakeUnique<DomainSocketClient>(socketPath);
}

Result CommunicationClient::ConnectToServer(void) const
{
    Result result = socket_->Connect();
    return result;
}

Result CommunicationClient::Read(std::string &msg) const
{
    constexpr std::size_t maxSize = 1024ULL;
    size_t readSize = 0;
    Result result = socket_->Read(msg, maxSize, readSize);
    return result;
}

Result CommunicationClient::Write(const std::string &msg) const
{
    size_t sentBytes = 0;
    Result result = socket_->Write(msg, sentBytes);
    return result;
}
}
