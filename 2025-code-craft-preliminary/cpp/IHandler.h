//
// Created by tzx on 25-3-11.
//

#ifndef HW_CODECRAFT_2025_IHANDLER_H
#define HW_CODECRAFT_2025_IHANDLER_H

#include "common.h"


class IHandler {
public:
  // timestamp：当前时间戳
  int timestamp;
  // T：时间片数量，范围 1 ≤ T ≤ 86400
  int T;
  // M：对象标签数量，对象标签编号为1 ~ 𝑀，范围 1 ≤ M ≤ 16
  int M;
  // N：硬盘个数，硬盘编号为1 ~ 𝑁，范围 3 ≤ N ≤ 10
  int N;
  // V：每个硬盘的存储单元数，存储单元编号为1 ~ 𝑉，范围 1 ≤ V ≤ 16384，且至少保留10%的空闲空间
  int V;
  // G：每个磁头在每个时间片可消耗的令牌数，范围 64 ≤ G ≤ 1000
  int G;

  // 读取全局变量
  inline void InPut_Global_Var() {
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
  }
  // 初始化
  virtual void Init_Global_Var() = 0;
  // 读取输入
  virtual int Input_Global_Tag_Info() = 0;
  // 全局预处理阶段
  virtual int Init_Global_Tag_Info() = 0;
  // 初始化磁盘指针
  virtual void Init_Disk_Point() = 0;

  inline void Timestamp_action() {
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);
  };

  virtual void Delete_action() = 0;
  virtual void Write_action() = 0;
  virtual void Read_action() = 0;

  // 返回策略名称，DEBUG用。
  virtual std::string name() = 0;

  // 析构
  virtual ~IHandler() = default;
};


#endif//HW_CODECRAFT_2025_IHANDLER_H
