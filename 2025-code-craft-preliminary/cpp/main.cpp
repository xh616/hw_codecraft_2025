//#define DEBUG

#include "common.h"

#include "HandlerImpl_sample.h"
#include "HandlerImpl_tzx.h"
#include "HandlerImpl_tzx2.h"

int main() {
  unique_ptr<IHandler> handler;
  handler = make_unique<HandlerImpl_tzx2>();

#ifdef DEBUG
  print_log("DEBUG MODE, main() start, handler: %s\n", handler->name().c_str());
#endif

  handler->InPut_Global_Var();// scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);

  handler->Init_Global_Var();

  handler->Input_Global_Tag_Info();

  handler->Init_Global_Tag_Info();

  handler->Init_Disk_Point();

  for (int t = 1; t <= T + EXTRA_TIME; t++) {
    handler->Timestamp_action();
    handler->Delete_action();
    handler->Write_action();
    handler->Read_action();
  }

//  handler->End();
  return 0;
}