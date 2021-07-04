/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef ECHOTESTSERVER_HPP
#define ECHOTESTSERVER_HPP

#include <string>

#include "Mobile.pb.h"
#include "frpc/frpc_server.h"

using namespace MSF;

namespace MSF {

class MobileServiceImpl : public Mobile::GetMobileAPNService {
 public:
  MobileServiceImpl(int echo_delay,
                    const std::string& fail_reason = std::string());
  virtual void GetMobileAPN(google::protobuf::RpcController* controller,
                            const Mobile::GetMobileAPNRequest* request,
                            Mobile::GetMobileAPNResponse* response,
                            google::protobuf::Closure* done);

 private:
  // echo delay in seconds
  int echo_delay_;

  // if not empty, send an error response
  std::string fail_reason_;
};

void StartMobileServer(EventLoop* loop, const std::string& addr = "",
                       uint16_t port = 8888);
}

#endif /* ECHOTESTSERVER_HPP */
