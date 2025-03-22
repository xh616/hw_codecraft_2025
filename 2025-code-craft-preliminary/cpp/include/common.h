#pragma once

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace std;

// macro
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define MAX_TAG_NUM (16 + 1)
#define MAX_TIME_NUM (86400 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define MAX_OBJECT_BLOCK_NUM (5 + 1)

// global variables
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

// global functions
static inline void print_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "\033[1;31m"); // 设置红色高亮
  vfprintf(stderr, format, args);// 格式化输出
  fprintf(stderr, "\033[0m");    // 恢复默认颜色
  va_end(args);
}

template<typename T>
class ConcurrencyQueue {
private:
  mutex mtx;
  condition_variable cv;
  queue<T> queue_;

public:
  void push(const T &req) {
    unique_lock<mutex> lock(mtx);
    queue_.push(req);
    cv.notify_one();
  }

  T pop() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [this] { return !queue_.empty(); });
    T req = queue_.front();
    queue_.pop();
    return req;
  }

  auto size() {
    unique_lock<mutex> lock(mtx);
    return queue_.size();
  }


  bool empty() {
    unique_lock<mutex> lock(mtx);
    return queue_.empty();
  }
};