#pragma once

#include <cstdint>
#include <memory>
#include <AppCommon.hpp>
#include <GenericWindow.hpp>

// 这里不能前向申明 SDL_Window, 不然在析构中 ::SDL_DestroyWindow(NativeWindow) 会 
// 解析到 NativeWindow 的类型是 app::SDL_Window, SDL_DestroyWindow 不认识这个类型
#include <SDL3/SDL.h> 

namespace app::details
{

class SDLWindow final : public IGenericWindow
{
public:
	explicit SDLWindow(SDL_Window* InNativeWindow) 
		: NativeWindow(InNativeWindow) {}
	~SDLWindow() override;
	
	void show() override;
	void close() override;
	void minimize() override;
	void maximize() override;
	void setTitle(const std::string& InTitle) override;
	void setPosition(int32_t X, int32_t Y) override;
	void setSize(int32_t Width, int32_t Height) override;
	void setWindowType(EWindowType Type) override;
	[[nodiscard]] WindowId_t getWindowId() override;
	[[nodiscard]] core::math::Vec2i getPosition() override;
	[[nodiscard]] core::math::Vec2i getSize() override;
	[[nodiscard]] void* getNativeHandle() const noexcept override;
private:
	SDL_Window* NativeWindow;
};

} // namespace app::details