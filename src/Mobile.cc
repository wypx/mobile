/**************************************************************************
 *
 * Copyright (c) 2017-2020, luotang.me <wypx520@gmail.com>, China.
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

#include <base/Daemon.h>
#include <base/IniFile.h>
#include <base/Logger.h>
#include <base/Version.h>
#include <getopt.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Modem.h"

using namespace MSF::BASE;
using namespace MSF::EVENT;
using namespace mobile;
using namespace MSF::AGENT;

namespace mobile {

Mobile::Mobile() : os_(OsInfo()) { os_.enableCoreDump(); }

Mobile::~Mobile() {}
void Mobile::Init(int argc, char **argv) {
  ParseOption(argc, argv);
  assert(LoadConfig());

  if (config_.daemon()) {
    daemonize();
  }

  std::string logFile = config_.log_dir() + "Mobile.log";
  assert(Logger::getLogger().init(logFile.c_str()));

  stack_ = new EventStack();
  if (stack_ == nullptr) {
    MSF_FATAL << "Fail to alloc event stack for mobile.";
    return;
  }
  std::vector<ThreadArg> threadArgs;

  threadArgs.push_back(std::move(ThreadArg("ReadLoop")));
  threadArgs.push_back(std::move(ThreadArg("SendLoop")));
  threadArgs.push_back(std::move(ThreadArg("DialLoop")));
  threadArgs.push_back(std::move(ThreadArg("StatLoop")));
  assert(stack_->startThreads(threadArgs));

  agent_ = new AgentClient(stack_->getOneLoop(), "Mobile", Agent::APP_MOBILE,
                           config_.agent_ip(), config_.agent_port());
  assert(agent_);
  agent_->setRequestCb(std::bind(&Mobile::AgentReqCb, this,
                                 std::placeholders::_1, std::placeholders::_2,
                                 std::placeholders::_3));
  pool_ = new MemPool();
  assert(pool_->Init());

  modem_ = new Modem();
  assert(modem_);
}

void Mobile::DebugInfo() {
  std::cout << std::endl;
  std::cout << "Mobile Options:" << std::endl;
  std::cout << " -c, --conf=FILE        The configure file." << std::endl;
  std::cout << " -s, --host=HOST        The host that server will listen on"
            << std::endl;
  std::cout << " -p, --port=PORT        The port that server will listen on"
            << std::endl;
  std::cout << " -d, --daemon           Run as daemon." << std::endl;
  std::cout << " -g, --signal           restart|kill|reload" << std::endl;
  std::cout << "                        restart: restart process graceful"
            << std::endl;
  std::cout << "                        kill: fast shutdown graceful"
            << std::endl;
  std::cout << "                        reload: parse config file and reinit"
            << std::endl;
  std::cout << " -v, --version          Print the version." << std::endl;
  std::cout << " -h, --help             Print help info." << std::endl;
  std::cout << std::endl;

  std::cout << "Examples:" << std::endl;
  std::cout << " ./Mobile -c Mobile.conf -d -g kill" << std::endl;

  std::cout << std::endl;
  std::cout << "Reports bugs to <luotang.me>" << std::endl;
  std::cout << "Commit issues to <https://github.com/wypx/mobile>" << std::endl;
  std::cout << std::endl;
}

void Mobile::ParseOption(int argc, char **argv) {
  /* 浅谈linux的命令行解析参数之getopt_long函数
   * https://blog.csdn.net/qq_33850438/article/details/80172275
   **/
  int c;
  while (true) {
    int optIndex = 0;
    static struct option longOpts[] = {
        {"conf", required_argument, nullptr, 'c'},
        {"host", required_argument, nullptr, 's'},
        {"port", required_argument, nullptr, 'p'},
        {"signal", required_argument, nullptr, 'g'},
        {"daemon", no_argument, nullptr, 'd'},
        {"version", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, 0, 0}};
    c = getopt_long(argc, argv, "c:s:p:g:dvh?", longOpts, &optIndex);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'c':
        conf_file_ = std::string(optarg);
        break;
      case 'g':
        break;
      case 'd':
        config_.set_daemon(true);
        break;
      case 'v':
        MSF::BuildInfo();
        std::exit(0);
      case 'h':
        DebugInfo();
        std::exit(0);
      case '?':
      default:
        DebugInfo();
        std::exit(1);
    }
  }
  return;
}

bool Mobile::LoadConfig() {
  if (conf_file_.empty()) {
    conf_file_ = "/home/luotang.me/conf/Mobile.conf";
    MSF_INFO << "Use default config: " << conf_file_;
  }

  IniFile ini;
  if (ini.Load(conf_file_) != 0) {
    MSF_ERROR << "Confiure load failed";
    return false;
  }

  if (!ini.HasSection("Logger") || !ini.HasSection("System") ||
      !ini.HasSection("Network") || !ini.HasSection("Plugins")) {
    MSF_ERROR << "Confiure invalid, check sections";
    return false;
  }

  assert(ini.GetStringValue("", "Version", &config_.version_) == 0);
  // assert(ini.GetIntValue("Logger", "LogLevel",
  // &static_cast<int>(config_.log_level_)) == 0);
  assert(ini.GetStringValue("Logger", "LogDir", &config_.log_dir_) == 0);
  if (config_.log_dir_.empty()) {
    // logDir_ = "/var/log/luotang.me/";
  }

  assert(ini.GetStringValue("System", "PidFile", &config_.pid_path_) == 0);
  assert(ini.GetBoolValue("System", "Daemon", &config_.daemon_) == 0);
  // assert(ini.GetValues("Plugins", "Plugin", &config_.plugins_) == 0);

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

void Mobile::AgentReqCb(char **data, uint32_t *len, const Agent::Command cmd) {
  MSF_INFO << "Cmd: " << cmd << " len: " << *len;
  if (cmd == Agent::Command::CMD_REQ_MOBILE_READ) {
    MSF_INFO << "Read mobile param ====> ";

    struct ApnItem item = {0};
    item.cid_ = 1;
    item.active_ = 2;

    MSF_INFO << "ApnItem size: " << sizeof(struct ApnItem) << " len: " << len;

    *len = (uint32_t)sizeof(struct ApnItem);
    *data = agent_->allocBuffer(*len);
    assert(*data);
    memcpy(*data, &item, sizeof(struct ApnItem));
  }
}

void Mobile::Start() { stack_->start(); }
}  // namespace mobile

int main(int argc, char **argv) {
  Mobile mob = Mobile();
  mob.Init(argc, argv);
  // mob.setAgent("/var/tmp/mobile.sock");
  mob.SetAgent("luotang.me", 8888);
  mob.Start();
  return 0;
}
