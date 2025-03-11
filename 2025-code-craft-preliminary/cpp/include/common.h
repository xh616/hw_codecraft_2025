#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

// macro
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)

// global functions
#ifdef DEBUG
static inline void print_log(const string &msg) {
  cerr << "\033[1;31m" << msg << "\033[0m" << endl;
}
#else
static inline void print_log([[maybe_unused]] const string &msg) {
}
#endif
