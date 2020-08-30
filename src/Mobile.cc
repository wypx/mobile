/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "Mobile.h"

#include <base/Version.h>
#include <base/ConfigParser.h>
#include <base/Daemon.h>
#include <getopt.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ATChannel.h"
#include "Sms.h"
#include "Modem.h"

using namespace MSF;
using namespace mobile;
namespace mobile {

static const std::string kMobileVersion = "beta v1.0";
static const std::string kMobileProject = "https://github.com/wypx/mobile";

DEFINE_string(conf, "/home/luotang.me/conf/Mobile.conf", "Configure file.");
DEFINE_bool(daemon, false, "Run as daemon mode");
DEFINE_bool(echo_attachment, true, "Attachment as well");
DEFINE_bool(send_attachment, true, "Carry attachment along with response");
DEFINE_int32(port, 8001, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000,
             "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");

Mobile::Mobile() { }

Mobile::~Mobile() { google::ShutDownCommandLineFlags(); }

brpc::Server Mobile::server_;

void Mobile::Init(int argc, char** argv) {
  OsInfo::EnableCoredump();
  google::SetVersionString(kMobileVersion);
  google::SetUsageMessage(
      "Command line mode\n"
      "usage: Moible <command> <args>\n\n"
      "commands:\n"
      "  conf            config file path\n"
      "  daemon          run as daemon mode\n"
      "  port            service tcp port\n"
      "  time            benchmark model execution time");
  google::ParseCommandLineFlags(&argc, &argv, true);

  logging::LoggingSettings log_setting;
  log_setting.logging_dest = logging::LOG_TO_ALL;
  log_setting.log_file = "/home/luotang.me/log/Mobile.log";
  log_setting.delete_old = logging::APPEND_TO_OLD_LOG_FILE;
  if (!logging::InitLogging(log_setting)) {
    return;
  }

  if (!LoadConfig()) {
    LOG(FATAL) << "Fail to load config file.";
    return;
  }

  BuildProject(kMobileVersion, kMobileProject);
  BuildLogo();

  if (FLAGS_daemon) {
    Daemonize();
  }

  stack_ = new EventStack();
  if (stack_ == nullptr) {
    LOG(FATAL) << "Fail to alloc event stack for mobile.";
    return;
  }
  std::vector<ThreadArg> threadArgs;
  threadArgs.push_back(std::move(ThreadArg("ATCmdLoop")));
  threadArgs.push_back(std::move(ThreadArg("DialLoop")));
  threadArgs.push_back(std::move(ThreadArg("StatLoop")));
  assert(stack_->startThreads(threadArgs));

  pool_ = new MemPool();
  assert(pool_->Init());

  modem_ = new Modem(stack_->GetFixedLoop(THREAD_ATCMDLOOP));
  modem_->Init();
  // agent_ = new AgentClient(stack_->getOneLoop(), "Mobile", Agent::APP_MOBILE,
  //                          config_.agent_ip(), config_.agent_port());
  // assert(agent_);
  // agent_->setRequestCb(std::bind(&Mobile::AgentReqCb, this,
  //                                std::placeholders::_1, std::placeholders::_2,
  //                                std::placeholders::_3));
  // stack_->start();
}

bool Mobile::LoadConfig() {
  ConfigParser ini;
  if (ini.Load(FLAGS_conf) != 0) {
    return false;
  }

  if (!ini.HasSection("Logger") || !ini.HasSection("System") ||
      !ini.HasSection("Network") || !ini.HasSection("Plugins")) {
    LOG(ERROR) << "Confiure invalid, check sections";
    return false;
  }

  if (ini.GetStringValue("", "Version", &config_.version_) != 0) {
    config_.version_ = kMobileVersion;
  }
  if (ini.GetStringValue("Logger", "LogLevel", &config_.log_level_) != 0) {
    config_.version_ = "INFO";
  }
  if (ini.GetStringValue("Logger", "LogDir", &config_.log_dir_) != 0) {
    config_.log_dir_ = "/home/luotang.me/log/";
  }

  if (ini.GetStringValue("System", "PidFile", &config_.pid_path_) != 0) {

  }
  if (ini.GetBoolValue("System", "Daemon", &config_.daemon_) != 0) {

  }
  if (ini.GetValues("Plugins", "Plugin", &config_.plugins_) != 0) {

  }

  assert(ini.GetBoolValue("Network", "AgentEnable", &config_.agent_enable_) ==
         0);
  std::string agentNet;
  assert(ini.GetStringValue("Network", "AgentNet", &agentNet) == 0);
  // config_.agent_net_type_ ;
  assert(ini.GetStringValue("Network", "AgentIP", &config_.agent_ip_) == 0);

  int agentPort = 0;
  assert(ini.GetIntValue("Network", "AgentPort", &agentPort) == 0);
  config_.agent_port_ = static_cast<uint16_t>(agentPort);

  assert(ini.GetStringValue("Network", "AgentUnixServer",
                            &config_.agent_unix_server_) == 0);
  assert(ini.GetStringValue("Network", "AgentUnixClient",
                            &config_.agent_unix_client_) == 0);
  // assert(ini.GetStringValue("Network", "AgentUnixClientMask",
  //                           &static_cast<int>(config_.agent_unix_mask_)) ==
  //                           0);

  assert(ini.GetBoolValue("Network", "AgentAuthChap",
                          &config_.agent_auth_chap_) == 0);
  assert(ini.GetStringValue("Network", "AgentPackType",
                            &config_.agent_pack_type_) == 0);

  return true;
}

void Mobile::GetMobileAPN(google::protobuf::RpcController* cntl_base,
                          const GetMobileAPNRequest* request,
                          GetMobileAPNResponse* response,
                          google::protobuf::Closure* done) {
  // This object helps you to call done->Run() in RAII style. If you need
  // to process the request asynchronously, pass done_guard.release().
  brpc::ClosureGuard done_guard(done);

  brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);

  // The purpose of following logs is to help you to understand
  // how clients interact with servers more intuitively. You should
  // remove these logs in performance-sensitive servers.
  LOG(INFO) << "Received request[log_id=" << cntl->log_id() << "] from "
            << cntl->remote_side() << " to " << cntl->local_side() << ": "
            << request->cid() << " (attached=" << cntl->request_attachment()
            << ")";

  // Fill response.
  response->set_apn("public.vpdn.cn");
  response->set_cid(request->cid());
  response->set_type(1);

  // You can compress the response by setting Controller, but be aware
  // that compression may be costly, evaluate before turning on.
  // cntl->set_response_compress_type(brpc::COMPRESS_TYPE_GZIP);
  if (FLAGS_echo_attachment) {
    // Set attachment which is wired to network directly instead of
    // being serialized into protobuf messages.
    cntl->response_attachment().append(cntl->request_attachment());
  }
}

// Instance of your service.
// Add the service into server. Notice the second parameter, because the
// service is put on stack, we don't want server to delete it, otherwise
// use brpc::SERVER_OWNS_SERVICE.
bool Mobile::RegisterService() {
  if (server_.AddService(this, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add service: get apn";
    return false;
  }
  return true;
}

int32_t Mobile::Start() {
  if (!RegisterService()) {
    return -1;
  }
  // Generally you only need one Server.
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  if (server_.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Fail to start moible protocol server.";
    return -1;
  }

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  server_.RunUntilAskedToQuit();
  return 0;
}
}  // namespace mobile

int main(int argc, char** argv) {
  Mobile* app = new Mobile();
  app->Init(argc, argv);
  return app->Start();
}
