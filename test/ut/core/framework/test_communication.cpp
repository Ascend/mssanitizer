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


#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include "communication.h"
#include "utility/domain_socket.h"

using namespace Sanitizer;

// 测试 CommunicationServer 的初始化与销毁
TEST(CommunicationServerTest, InitializationAndDestruction) {
    std::string socketPath = "/tmp/msop_connect.202511121043.12345.sock";
    CommunicationServer server(socketPath);
    EXPECT_NO_THROW(server.Close()); // 确保可以安全关闭
}

// 测试 CommunicationServer 的 StartListen
TEST(CommunicationServerTest, StartListen) {
    std::string socketPath = "/tmp/msop_connect.202511121043.12345.sock";
    CommunicationServer server(socketPath);

    // 假设 ListenAndBind 成功
    server.StartListen();
    EXPECT_NO_THROW(server.Close()); // 确保服务端可以安全关闭
}

// 测试 CommunicationClient 的 ConnectToServer
TEST(CommunicationClientTest, ConnectToServer) {
    std::string socketPath = "/tmp/msop_connect.202511121043.12345.sock";
    CommunicationClient client(socketPath);
    CommunicationServer server(socketPath);
    server.StartListen();

    Result connectResult = client.ConnectToServer(); // 调用真实的 Connect 方法
    EXPECT_TRUE(!connectResult.Fail());
}

TEST(CommunicationServerTest, RegisterMsgHandler_expect_callable) {
    std::string socketPath = "/tmp/msop_connect_register_msg.202511121043.12345.sock";
    CommunicationServer server(socketPath);
    bool handlerCalled = false;
    server.RegisterMsgHandler(
        [&handlerCalled](std::string msg, CommunicationServer::MsgResponseFunc &rsp) { handlerCalled = true; });
    server.StartListen();
    server.Close();
}

TEST(CommunicationServerTest, SetClientConnectHook_expect_callable) {
    std::string socketPath = "/tmp/msop_connect_set_hook.202511121043.12345.sock";
    CommunicationServer server(socketPath);
    bool hookCalled = false;
    CommunicationServer::ClientId connectedId = 0;
    server.SetClientConnectHook([&hookCalled, &connectedId](CommunicationServer::ClientId id) {
        hookCalled = true;
        connectedId = id;
    });
    server.StartListen();
    server.Close();
}

TEST(CommunicationClientTest, ClientReadWrite_expect_success) {
    std::string socketPath = "/tmp/msop_connect_rw.202511121043.12345.sock";
    CommunicationServer server(socketPath);
    server.StartListen();

    CommunicationClient client(socketPath);
    Result connectResult = client.ConnectToServer();
    ASSERT_TRUE(!connectResult.Fail());

    std::string testData = "hello_mssanitizer";
    Result writeResult = client.Write(testData);
    ASSERT_TRUE(!writeResult.Fail());
}
