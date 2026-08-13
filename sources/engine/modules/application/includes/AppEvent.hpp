#pragma once
#include <cstdint>
#include <math/Vector.hpp>
#include <wrappers/Flag.hpp>
namespace app
{
using WindowId_t = std::uint64_t;
using InputDeviceId_t = std::uint64_t;
enum class EAppEvent
{
	None = 0x00,
// App 事件
	Quit,               // App 退出
	Termination,        // App 终止
    AppFirstEvent = Quit,
	AppLastEvent = Termination,

	LocaleChange,       // 语言环境改变
	ThemeChange,        // 主题改变
// Display 事件
	DisplayOrientationChange, // 显示器方向改变
	DisplayAdd,                     // 显示器加入
	DisplayRemove,                  // 显示器移除

// 窗口事件
	WindowResize,       // 窗口大小改变
    WindowMotion,     // 窗口位置改变
    WindowGainMouse,    // 窗口获得鼠标焦点
    WindowLostMouse,    // 窗口失去鼠标焦点
    WindowGainKeyboard, // 窗口获得键盘焦点
	WindowLostKeyboard, // 窗口失去键盘焦点
    WindowMinimize,     // 窗口最小化
	WindowMaximize,     // 窗口最大化

// 键盘事件
	KeyboardDown,    // 键盘按下
    KeyboardUp,			    // 键盘抬起

// 鼠标事件
	MouseDown,          // 鼠标按下
	MouseUp,            // 鼠标抬起
	MouseMotion,          // 鼠标移动
	MouseWheel,         // 鼠标滚轮
	MouseAdd,           // 新鼠标加入
	MouseRemove,        // 鼠标移除

// 手柄事件
	JoystickAxisMotion,    // 手柄轴移动
	JoystickBallMotion,    // 手柄球移动
	JoystickHatMotion,     // 手柄帽移动
	JoystickButtonDown,  // 手柄按键按下
	JoystickButtonUp,    // 手柄按键抬起
	JoystickAdd,         // 手柄加入
	JoystickRemove,      // 手柄移除

	GamepadAxisMotion,    // 游戏手柄轴移动
	GamepadButtonDown,  // 游戏手柄按键按下
	GamepadButtonUp,    // 游戏手柄按键抬起
	GamepadAdd,         // 游戏手柄加入
	GamepadRemove,      // 游戏手柄移除
    GamepadTouchpadDown,    // 游戏手柄触摸板按下
	GamepadTouchpadUp,      // 游戏手柄触摸板抬起
	GamepadTouchpadMotion,    // 游戏手柄触摸板移动

// 触摸板
	FingerDown,    // 手指按下
	FingerUp,          // 手指抬起
	FingerMotion,      // 手指移动

// 剪切板
	ClipboardUpdate,    // 剪切板更新

	DropFile,		  // 文件拖拽
	DropText,			  // 文本拖拽
    DropBegin,			  // 拖拽开始
	DropEnd,			  // 拖拽结束

// 音频
	AudioDeviceAdded,    // 音频设备添加
	AudioDeviceRemoved,            // 音频设备移除
	AudioDeviceFormatChanged,            // 音频设备格式改变

// 压感笔
	PenProximityIn = 0x1300,    // 压感笔可用
	PenProximityOut,           // 压感笔不可用
	PenDown,                // 压感笔按下在表面绘制
	PenUp,                  // 压感笔抬起停止绘制
	PenButtonDown,          // 压感笔按键按下
	PenButtonUp,            // 压感笔按键抬起
	PenMotion,              // 压感笔移动
	PenAxis,                // 压感笔角度, 压力变化

// 渲染设备
	RenderTargetReset,    // 渲染目标充值, 以及其内容更新
	RenderDeviceReset,    // 渲染设备重置
	RenderDeviceLost,     // 渲染设备丢失

	Count
};
enum class EKeyType : std::int16_t
{
	None = 0,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	Digit_One, Digit_Two, Digit_Three, Digit_Four, Digit_Five, Digit_Six, Digit_Seven, 
	Digit_Eight, Digit_Nine, Digit_Zero,
	Num_One, Num_Two, Num_Three, Num_Four, Num_Five, Num_Six, Num_Seven, 
	Num_Eight, Num_Nine, Num_Zero, Num_Div, Num_Mul, Num_Sub, Num_Add, Num_Decimal, Num_Enter,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, ESC, 
	BackQuote, /* ~ 键 */
	Backsapce,
	Tab,
	LeftShift, RightShift,
	LeftControl, RightControl,
	LeftAlt, RightAlt,
	LeftOption, RightOption,
	LeftCommand, RightCommand,
	LeftFn, RightFn,
	LeftBracket, /* [ 键 */
	RightBracket, /* ] 键 */
	Semicolon, /* ; 键 */
	Apostrophe, /* ' 键 */
	Enter,
	Backslash, /* \ 键 */
	Comma, /* , 键 */
	Period, /* . 键 */
	Slash, /* / 键 */
	Minus, /* - 键 */
	Equals,  /* = 键 */
	Space,
	Menu, /* Win 右Ctrl 之间*/
	Win,
	PrintScreen, ScrollLock, PauseBreak,
	Insert, Home, PageUp, 
	Delete, End, PageDown,
	Up, Down, Left, Right,
};

enum class EKeyModifierType : uint16_t
{
	None         = 0,
	LeftShift    = 1 << 0,
	RightShift   = 1 << 1,
	LeftControl  = 1 << 2,
	RightControl = 1 << 3,
	LeftAlt      = 1 << 4,
	RightAlt     = 1 << 5,
	LeftCommand  = 1 << 6,
	RightCommand = 1 << 7,
	LeftOption   = 1 << 8,
	RightOption  = 1 << 9,
	CapsLock     = 1 << 10,
	NumLock      = 1 << 11,
	Fn           = 1 << 12,
};

using EKeyModifier = core::wrappers::Flags<EKeyModifierType>;

enum class EMouseButton : uint8_t
{
	None,
	Left, // 鼠标左键
	Right, // 鼠标右键
	Middle, // 鼠标中键
	Thumb01, // 拇指侧键1
	Thumb02  // 拇指侧键2
};

struct KeyEvent
{
	WindowId_t WindowId;
	InputDeviceId_t DeviceId;


	EKeyModifier Modifiers;
	bool IsRepeat;
};

}