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
#include "service.h"

using namespace MSF;

namespace mobile {

MobileServiceImpl::MobileServiceImpl(int echo_delay,
                                     const std::string& fail_reason)
    : echo_delay_(echo_delay), fail_reason_(fail_reason) {}

void MobileServiceImpl::GetMobileAPN(
    google::protobuf::RpcController* controller,
    const mobile::MobileAPNRequest* request,
    mobile::MobileAPNResponse* response, google::protobuf::Closure* done) {
  if (fail_reason_.empty()) {
    // response->set_response(request->message());
    // Fill response.
    response->set_apn("public.vpdn.cn");
    response->set_cid(request->cid());
    response->set_type(1);
  } else {
    controller->SetFailed(fail_reason_);
  }

  if (echo_delay_ > 0) {
    // ::sleep(echo_delay_);
  }

  done->Run();
}
}  // namespace mobile