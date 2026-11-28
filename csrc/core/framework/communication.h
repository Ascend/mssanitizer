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


#ifndef CORE_FRAMEWORK_COMMUNICATION_H
#define CORE_FRAMEWORK_COMMUNICATION_H

#include <memory>
#include <functional>
#include <thread>
#include <string>
#include <queue>
#include "checker.h"
#include "utility/domain_socket.h"
#include "utility/result.h"
#include "protocol.h"

namespace Sanitizer {

class CommunicationServer {
public:
    using ClientId = std::size_t;
    using MsgResponseFunc = std::function<void(const std::string&)>;
    using MsgHandleFunc = std::function<void(pid_t, std::string, MsgResponseFunc &)>;
    using ClientConnectHook = std::function<void(ClientId)>;

    explicit CommunicationServer(const std::string& socketPath);
    ~CommunicationServer();

    /** 回调函数
     * 通知服务器已经读完所有数据
     */
    void RegisterMsgHandler(const MsgHandleFunc &func);

    /** 启动服务端
     * @description 手动启动服务端，在此之前可以设置 SetClientConnectHook
     * 回调，防止回调设置前一些客户端已经连接
     */
    void StartListen();

    void Listen(ClientId clientId);

    /** 从客户端读取数据
     * @description 当客户端未写入数据时阻塞，目前超时时间固定为 1s
     * @param clientId 要读取的客户端 id
     * @param msg 读取到的数据，当接口返回 -1 时为无效值
     * @return -1 表示读取失败或超时
     *         >0 表示读取成功，并返回读取到的数据长度
     */
    Result Read(ClientId clientId, std::string &msg);

    /** 向客户端写入数据
     * @description 当缓冲区满时阻塞
     * @param clientId 要写入的客户端 id
     * @param msg 要写入的数据
     * @return -1 表示写入失败
     *         >0 表示写入成功，并返回已写入的数据长度
     */
    Result Write(ClientId clientId, std::string const& msg);

    /** 设置客户端连接通知回调函数
     * @description 当有新客户端连接时，func 回调会被调用，并传入新客户端的
     * id。需要注意回调函数是在一个另一个线程中被调用，如果回调函数中捕获了
     * 其他变量，需要调用者自己在回调函数中对变量加锁处理线程竞争问题
     * @param func 通知回调函数
     */
    void SetClientConnectHook(ClientConnectHook &&hook);

    void WriteHook(ClientId clientId);

    void Close();

protected:
    bool runFlag_ { false };
    std::unique_ptr<Sanitizer::DomainSocketServer> socket_;
    // maxClientNum_不作为参数传入，默认为9
    std::size_t maxClientNum_ = 30;
    std::thread acceptWorker_;
    std::atomic<bool> acceptWorkerRun_;
    ClientConnectHook clientConnectHook_;
    MsgHandleFunc msgHandler_;
    std::vector<std::thread> clientThreads_;
    std::mutex threadMutex_;
    std::string configMsg;
};

class CommunicationClient {
public:
    explicit CommunicationClient(std::string socketPath);

    /** 连接服务端
     * @description 服务端未启动时连接会失败，需要调用者自行处理重试
     */
    Result ConnectToServer() const;

    /** 从服务端读取数据
     * @description 当服务端未写入数据时阻塞，目前超时时间固定为 1s
     * @param msg 读取到的数据，当接口返回 -1 时为无效值
     * @return -1 表示读取失败或超时
     *         >0 表示读取成功，并返回读取到的数据长度
     */
    Result Read(std::string &msg) const;

    // /** 向服务端写入数据
    //  * @description 当缓冲区满时阻塞
    //  * @param msg 要写入的数据
    //  * @return -1 表示写入失败
    //  *         >0 表示写入成功，并返回已写入的数据长度
    //  */
    Result Write(std::string const &msg) const;
protected:
    std::unique_ptr<Sanitizer::DomainSocketClient> socket_;
};
}

#endif
