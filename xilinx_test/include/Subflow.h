#pragma once

using Data_t = int;
constexpr int kSize = 128;

extern "C" void Subflow(const Data_t* memIn, Data_t* memOut);
