#pragma once

#include "RE/Skyrim.h"
#include <xbyak/xbyak.h>  // must be between these two
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <SimpleIni.h>

#ifndef NDEBUG
#include <spdlog/sinks/msvc_sink.h>
#endif

namespace logger = SKSE::log;
using namespace SKSE::util;

namespace stl
{
	using nonstd::span;
	using SKSE::stl::report_and_fail;
}

#define DLLEXPORT __declspec(dllexport)
