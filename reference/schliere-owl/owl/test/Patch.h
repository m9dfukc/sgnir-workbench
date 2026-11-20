// Minimaler Ersatz für die OwlProgram-API, nur für den Host-Test
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
enum PatchParameterId { PARAMETER_A, PARAMETER_B, PARAMETER_C, PARAMETER_D, PARAMETER_AA = 8, PARAMETER_AB, PARAMETER_AC, PARAMETER_AD };
struct FloatArray { float* d; size_t n; size_t getSize() const { return n; } float& operator[](size_t i){ return d[i]; } };
struct AudioBuffer { std::vector<std::vector<float>> ch; size_t getSize(){ return ch[0].size(); } size_t getChannels(){ return ch.size(); }
  FloatArray getSamples(int c){ return { ch[c].data(), ch[c].size() }; } };
struct Patch { float params[16] = {}; virtual ~Patch(){} virtual void processAudio(AudioBuffer&) = 0;
  float getParameterValue(PatchParameterId p){ return params[p]; } void setParameterValue(PatchParameterId p, float v){ params[p] = v; }
  void registerParameter(PatchParameterId, const char*){} int getNumberOfChannels(){ return 4; } float getSampleRate(){ return 48000; } };
