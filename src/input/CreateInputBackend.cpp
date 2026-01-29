// CreateInputBackend.cpp

#ifdef _WIN32
#include "aimbot/input/Win32InputBackend.h"
#endif

#include "aimbot/input/CreateInputBackend.h"

namespace aimbot::input {

    namespace {
        class NullInputBackend final : public IInputBackend {
        public:
            void pump() override {}
            bool keyDown(Key) const override { return false; }
            bool mouseDown(MouseButton) const override { return false; }
            bool gamepadDown(int, GamepadButton) const override { return false; }
        };
    }

    std::unique_ptr<IInputBackend> CreateInputBackend() {
#ifdef _WIN32
        return std::make_unique<Win32InputBackend>();
#else
        return std::make_unique<NullInputBackend>();
#endif
    }

} // namespace
