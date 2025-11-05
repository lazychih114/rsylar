//
// Created by swx on 23-12-28.
//
#include <iostream>
#include "raft.h"
#include "kvServer.h"
#include <unistd.h>
#include <iostream>
#include <random>
#include "sylar/sylar.h"
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("raft");

void runkv(int nodeidx,int port){
  auto kvServer = new KvServer(nodeidx, 500, "test.conf", port);
}
int main(int argc, char **argv) {
  auto nodeidx = atoi(argv[1]);
  auto port = atoi(argv[2]);
  std::cout << port << std::endl;
  g_logger->setLevel(sylar::LogLevel::INFO);
  sylar::Config::LoadFromConfDir(argv[3]);
  sylar::IOManager iom(2,true,"iom");


  iom.schedule(std::bind(runkv, nodeidx, port));
  return 0;
}
