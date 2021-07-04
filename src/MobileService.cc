
#include "MobileService.h"

using namespace MSF;

namespace MSF {

MobileServiceImpl::MobileServiceImpl(int echo_delay,
                                     const std::string& fail_reason)
    : echo_delay_(echo_delay), fail_reason_(fail_reason) {}

void MobileServiceImpl::GetMobileAPN(
    google::protobuf::RpcController* controller,
    const Mobile::GetMobileAPNRequest* request,
    Mobile::GetMobileAPNResponse* response, google::protobuf::Closure* done) {

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

void StartMobileServer(EventLoop* loop, const std::string& ip,
                       uint16_t port) {
  std::cout << "server port : " << port << std::endl;
  InetAddress addr(ip, port);
  FastRpcServer *server = new FastRpcServer(loop, addr);

  MobileServiceImpl echo_impl(1);
  server->HookService(&echo_impl);

  server->Running();

  //  pbrpcpp::RpcController controller;
  //
  //  string addr, port;
  //  while( ! rpcServer->getLocalEndpoint( addr, port ) );
  //
  //  shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel(
  // addr, port ) );
  //  EchoTestClient client( channel );
  //  echo::EchoRequest request;
  //  echo::EchoResponse response;
  //
  //  request.set_message("hello, world");
  //  client.echo( &controller, &request,&response, NULL, 5000 );
  //
  //  EXPECT_FALSE( controller.Failed() );
  //  EXPECT_EQ( response.response(), "hello, world");
  //   int temp;
  //   std::cin >> temp;

}
}