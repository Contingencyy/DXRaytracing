#pragma once

/*

	Standard library includes

*/
#include <iostream>
#include <cstdio>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>
#include <assert.h>

/*

	Windows includes

*/
#include "WinIncludes.h"
#define DX_CALL(hr) if (hr != S_OK) throw std::exception()

/*
	
	External includes

*/
#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/compatibility.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/quaternion.hpp>

#define ASSERT(x, y) if (!(x)) { printf(y); assert(false); }

constexpr boolean GPU_VALIDATION_ENABLED = false;

/*

	Utility includes

*/
#include "Util/Logger.h"
#include "Util/Profiler.h"
#include "Util/StringHelper.h"
#include "Util/MathHelper.h"
