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
#ifndef ECHOTESTSERVER_HPP
#define ECHOTESTSERVER_HPP

#include <frpc/frpc_server.h>

#include <string>

#include "mobile.pb.h"

using namespace MSF;

namespace mobile {

class MobileServiceImpl : public mobile::GetMobileAPNService {
 public:
  MobileServiceImpl(int echo_delay,
                    const std::string& fail_reason = std::string());
  virtual void GetMobileAPN(google::protobuf::RpcController* controller,
                            const mobile::MobileAPNRequest* request,
                            mobile::MobileAPNResponse* response,
                            google::protobuf::Closure* done);

 private:
  // echo delay in seconds
  int echo_delay_;

  // if not empty, send an error response
  std::string fail_reason_;
};
}  // namespace mobile

#endif /* ECHOTESTSERVER_HPP */
