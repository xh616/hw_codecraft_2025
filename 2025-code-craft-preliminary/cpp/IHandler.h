//
// Created by tzx on 25-3-11.
//

#ifndef HW_CODECRAFT_2025_IHANDLER_H
#define HW_CODECRAFT_2025_IHANDLER_H

#include "common.h"


class IHandler {
public:
  // global variable
  int T, M, N, V, G;

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
    static int timestamp;
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
