#pragma once

#include <atomic>

#include "engine/Engine.hpp"

#define GSeedEngine runtime::Engine::self()

inline std::atomic<uint64_t> GCurrentFrameIndex { 0 };