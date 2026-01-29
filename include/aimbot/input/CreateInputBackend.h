// input/CreateInputBackend.h
#pragma once

#include <memory>

#include "aimbot/input/IInputBackend.h"

namespace aimbot::input {
	std::unique_ptr<IInputBackend> CreateInputBackend();
}
