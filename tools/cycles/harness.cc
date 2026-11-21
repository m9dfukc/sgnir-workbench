// Bare-metal harness for the Schliere cycle model (tools/cycles/cycle_model.py).
//
// Linked with rings/dsp/schliere.cc into a small Cortex-M4 ELF. The model
// calls h_init() once and h_block() per audio block, and reads/writes
// `params`, `in_`, `out_l` and `out_r` directly in the emulated memory.

#include "rings/dsp/schliere.h"

using namespace rings;

Schliere schliere;
SchliereParameters params;
float in_[24];
float out_l[24];
float out_r[24];

extern "C" {

__attribute__((noinline, used)) void h_init(int oversampled_feedback) {
  schliere.Init(SCHLIERE_VARIANT_S);
  schliere.set_oversampled_feedback(oversampled_feedback);
}

__attribute__((noinline, used)) void h_block() {
  schliere.Process(params, in_, out_l, out_r, 24);
}

// Minimal runtime stubs: no startup files, no syscalls.
void _exit(int) { for (;;) { } }
int* __errno() { static int e; return &e; }
void* __dso_handle;
void _fini() { }
void _init() { }

}  // extern "C"
