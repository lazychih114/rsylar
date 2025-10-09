//
// Created by swx on 23-12-28.
//

#include "raftRpcUtil.h"

#include <mprpcchannel.h>
#include <mprpccontroller.h>
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
bool RaftRpcUtil::AppendEntries(raftRpcProctoc::AppendEntriesArgs *args, raftRpcProctoc::AppendEntriesReply *response) {
  MprpcController controller;
  stub_->AppendEntries(&controller, args, response, nullptr);
  return !controller.Failed();
}

bool RaftRpcUtil::InstallSnapshot(raftRpcProctoc::InstallSnapshotRequest *args,
                                  raftRpcProctoc::InstallSnapshotResponse *response) {
  MprpcController controller;
  stub_->InstallSnapshot(&controller, args, response, nullptr);
  return !controller.Failed();
}

bool RaftRpcUtil::RequestVote(raftRpcProctoc::RequestVoteArgs *args, raftRpcProctoc::RequestVoteReply *response) {
  MprpcController controller;
  while(true){
    if(m_mutex.try_lock()) {
      stub_->RequestVote(&controller, args, response, nullptr); 
      break;
    }else{
      auto cur = sylar::Fiber::GetThis();
      cur->yield();
      SYLAR_LOG_INFO(g_logger) << "------- 释放锁，等待重试";
    }
  }
  m_mutex.unlock();

  return !controller.Failed();
}

//先开启服务器，再尝试连接其他的节点，中间给一个间隔时间，等待其他的rpc服务器节点启动

RaftRpcUtil::RaftRpcUtil(std::string ip, short port) {
  //*********************************************  */
  //发送rpc设置
  stub_ = new raftRpcProctoc::raftRpc_Stub(new sylar::rpc::MprpcChannel(ip, port, true));
}

RaftRpcUtil::~RaftRpcUtil() { delete stub_; }
