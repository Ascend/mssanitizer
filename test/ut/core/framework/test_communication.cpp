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


#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <thread>
#define private public
#define protected public
#include "communication.h"
#include "utility/domain_socket.h"
#undef protected
#undef private

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
    std::atomic<bool> handlerCalled{false};
    server.RegisterMsgHandler(
        [&handlerCalled](std::string msg, CommunicationServer::MsgResponseFunc &rsp) { handlerCalled.store(true); });
    server.StartListen();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(handlerCalled.load());
    server.Close();
}

TEST(CommunicationServerTest, SetClientConnectHook_expect_callable) {
    std::string socketPath = "/tmp/msop_connect_set_hook.202511121043.12345.sock";
    CommunicationServer server(socketPath);
    std::atomic<bool> hookCalled{false};
    server.SetClientConnectHook([&hookCalled](CommunicationServer::ClientId id) {
        hookCalled.store(true);
    });
    server.StartListen();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(hookCalled.load());
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

// 覆盖 CommunicationServer::Read/Write：socket_ 为 nullptr 时返回错误
TEST(CommunicationServerTest, NullSocket_expect_fail)
{
    std::string socketPath = "/tmp/msop_null_socket.12345.sock";
    CommunicationServer server(socketPath);
    server.socket_ = nullptr;
    std::string msg;
    EXPECT_TRUE(server.Read(0, msg).Fail());
    EXPECT_TRUE(server.Write(0, "test").Fail());
}

// 覆盖 CommunicationServer::Listen：接收客户端消息 + 通过 MsgResponseFunc 回复
TEST(CommunicationServerTest, Listen_ReceiveAndResponse_expect_success)
{
    std::string socketPath = "/tmp/msop_listen_full.12345.sock";
    CommunicationServer server(socketPath);
    std::promise<std::string> msgPromise;
    auto msgFuture = msgPromise.get_future();
    server.RegisterMsgHandler([&](std::string msg, CommunicationServer::MsgResponseFunc &rsp) {
        msgPromise.set_value(msg);
        rsp("server_response");
    });
    server.SetClientConnectHook([](CommunicationServer::ClientId) {});
    server.StartListen();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        CommunicationClient client(socketPath);
        ASSERT_TRUE(!client.ConnectToServer().Fail());
        ASSERT_TRUE(!client.Write("ping").Fail());
        ASSERT_EQ(msgFuture.wait_for(std::chrono::seconds(3)), std::future_status::ready);
        EXPECT_EQ(msgFuture.get(), "ping");

        std::string response;
        Result result = client.Read(response);
        EXPECT_TRUE(!result.Fail());
        EXPECT_EQ(response, "server_response");
    }
    server.Close();
}

// 覆盖 CommunicationClient::Read/Write：未连接时返回错误
TEST(CommunicationClientTest, NotConnected_expect_fail)
{
    std::string socketPath = "/tmp/msop_client_fail.12345.sock";
    CommunicationClient client(socketPath);
    std::string msg;
    EXPECT_TRUE(client.Read(msg).Fail());
    EXPECT_TRUE(client.Write("test").Fail());
}
