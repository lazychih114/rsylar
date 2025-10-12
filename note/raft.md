下面是针对你当前工作区（以 /home/reversedog/projects/rsylar/raft/raftCore 为根）的“阅读笔记”任务清单。每项都尽量给出要打开的文件/函数、断点位置、观察点和小测试建议，便于在 VSCode 中逐步消化 kvServer 与 Raft 交互的实现细节（重点参考你当前打开的 kvServer.h）。

总体优先级：1（高）→ 6（低）。

1) 关键流程：从 Raft apply 到 KV 应用（高优先）
- kvServer.h
- 关注函数：
  - KvServer::ReadRaftApplyCommandLoop() —— 主循环，接收 Raft 的 ApplyMsg
  - KvServer::GetCommandFromRaft(ApplyMsg) —— 处理单条 apply 消息
  - KvServer::SendMessageToWaitChan(const Op&, int raftIndex) —— 将执行结果通知等待的 RPC 调用
  - KvServer::ExecutePutOpOnKVDB / ExecuteGetOpOnKVDB / ExecuteAppendOpOnKVDB
- 在 VSCode 断点：
  - ReadRaftApplyCommandLoop() 循环入口
  - GetCommandFromRaft() 头部与分支（normal log vs snapshot）
  - SendMessageToWaitChan() 返回路径
- 观察点：
  - applyChan 的类型/容量是否会阻塞（LockQueue 的行为）
  - commitIndex -> lastApplied 推进逻辑
  - waitApplyCh 在 map 中的添加/删除时机，是否有并发竞态
- 小测试：
  - 启动节点，发送一个 PutAppend 请求，单步跟踪消息从 Raft 到 kvServer 的完整路径，验证客户端收到响应。

2) 快照序列化与恢复（高优先）
- 打开文件：raft/raftCore/include/kvServer.h（serialize、getSnapshotData、parseFromString）、kvServer.cpp（MakeSnapShot、GetSnapShotFromRaft、ReadSnapShotToInstall）
- 关注点：
  - getSnapshotData()：m_serializedKVData = m_skipList.dump_file(); 然后序列化 this；注意 m_serializedKVData 在序列化后被 clear 的时序是否合理
  - parseFromString()：反序列化后用 m_serializedKVData 调用 m_skipList.load_file(...)
- 在 VSCode 断点：
  - MakeSnapShot() 返回值处，确认 snapshot 内容大小/格式
  - parseFromString() 调用后的 skiplist 状态
- 观察点：
  - 当 snapshot 被截断/传输/保存时，skipList 数据是否完整
- 小测试：
  - 强制生成快照（可临时把 m_maxRaftState 调小），重启节点，验证数据恢复一致性。

3) skipList 与持久化（中高）
- 打开文件：raft/raftCore/include/skipList.h 和实现（若存在），kvServer.h 的 m_skipList 使用点
- 关注点：
  - m_skipList.dump_file() / load_file() 的返回数据格式与大小
  - 与 m_kvDB（unordered_map） 的关系：当前设计同时存在 skipList 与 m_kvDB，确认哪个是主存储（看 Execute* 函数）
- 观察点：
  - 是否存在重复维护两份数据的风险（一致性问题）
- 小测试：
  - 在 kvServer 上执行大量 Put，触发 snapshot，检查 dump/load 前后数据是否一致。

4) RPC 接口与并发（中）
- 打开文件：raft/raftCore/include/kvServer.h（RPC override）、kvServer.cpp（PutAppend/Get 的实现）、raftRpcUtil.cpp
- 关注点：
  - PutAppend/Get 的 sync RPC 路径：如何把客户端请求转为 Raft::Start，并等待 apply 通知
  - waitApplyCh 的生命周期（谁 new/delete、何时清理）
- 在 VSCode 断点：
  - Rpc 接收处（PutAppend override）、发送 Raft::Start 的地方、等待通道被通知处
- 小测试：
  - 模拟重复请求（相同 clientId/requestID）验证 ifRequestDuplicate 的效果与幂等性

5) 重复请求判定与客户端状态（中低）
- 打开文件：kvServer.h（m_lastRequestId）、kvServer.cpp（ifRequestDuplicate 与更新逻辑）
- 关注点：
  - m_lastRequestId 的存取是否加锁、持久化到 snapshot 中（kvServer::serialize 中序列化了 m_lastRequestId）
  - 更新时机（apply 成功后才更新？）
- 小测试：
  - 发相同 clientId/requestId 的请求两次，确认服务器不会重复执行写操作

6) 潜在 bug 与改进点（低但重要）
- waitApplyCh 是裸指针 LockQueue<Op>*：检查内存/并发安全（谁负责释放）
- m_skipList 和 m_kvDB 并存：确认只使用其中一个作为持久化源，或保证二者严格同步
- getSnapshotData 与 parseFromString：注意 m_serializedKVData 在序列化/反序列化过程中被清空的时序，必要时记录日志检查
- Lock 的粒度：m_mtx 是否包住所有共享结构修改（m_lastRequestId、waitApplyCh、m_kvDB 等）
- RPC 线程与 apply 线程之间的唤醒/阻塞策略，避免死锁或内存泄漏

开发/调试命令（在项目根目录，Linux）：
```bash
# build
mkdir -p build && cd build
cmake ..
make -j

# 启动二进制（视 CMake 输出而定，替换下面的可执行名）
./bin/raftCoreRun &

# 在运行时观察日志（根据项目实际日志路径）
tail -f bin/log/root0.txt bin/log/root1.txt
```

建议的短期学习计划（2 天）：
- Day 1: 跟踪一次 PutAppend 的完整路径（客户端 → RPC → Raft::Start → apply → KvServer apply），并记录时间线与关键变量（index/term/commitIndex）
- Day 2: 强制触发 snapshot，做一次重启恢复测试，观察序列化/反序列化流程与 skipList 恢复

需要我进一步做哪件事？
- 生成逐行注释版的“关键函数阅读笔记”（例如 GetCommandFromRaft、MakeSnapShot）
- 为 PutAppend/快照流程写一个小的自动化集成测试脚本（启动 3 节点，发送请求，断言恢复）