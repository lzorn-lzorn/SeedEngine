
#pragma once

#include <cstdint>
namespace app
{

enum class EAppEvent : uint32_t
{
	None = 0x00,
// App 事件
	Quit = 0x100,               // App 退出
    Close,			    // App 关闭
	Termination,        // App 终止
    AppFirstEvent = Quit,
	AppLastEvent = Termination,

	LocaleChange,       // 语言环境改变
	ThemeChange,        // 主题改变
// Display 事件
	DisplayOrientationChange = 0x150, // 显示器方向改变
	DisplayAdd,                     // 显示器加入
	DisplayRemove,                  // 显示器移除
	DisplayFirstEvent = DisplayOrientationChange,
	DisplayLastEvent = DisplayRemove,
// 窗口事件
	WindowResize = 0x200,       // 窗口大小改变
    WindowMotion,     // 窗口位置改变
    WindowGainMouse,    // 窗口获得鼠标焦点
    WindowLostMouse,    // 窗口失去鼠标焦点
    WindowGainKeyboard, // 窗口获得键盘焦点
	WindowLostKeyboard, // 窗口失去键盘焦点
    WindowMinimize,     // 窗口最小化
	WindowMaximize,     // 窗口最大化
	WindowFirstEvent = WindowResize,
	WindowLastEvent = WindowMaximize,

// 键盘事件
	KeyboardDown = 0x300,    // 键盘按下
    KeyboardUp,			    // 键盘抬起
    KeyboardFirstEvent = KeyboardDown,
    KeyboardLastEvent = KeyboardUp,

// 鼠标事件
	MouseDown = 0x400,          // 鼠标按下
	MouseUp,            // 鼠标抬起
	MouseMotion,          // 鼠标移动
	MouseWheel,         // 鼠标滚轮
	MouseAdd,           // 新鼠标加入
	MouseRemove,        // 鼠标移除
	MouseFirstEvent = MouseDown,
	MouseLastEvent = MouseRemove,

// 手柄事件
	JoystickAxisMotion = 0x600,    // 手柄轴移动
	JoystickBallMotion,    // 手柄球移动
	JoystickHatMotion,     // 手柄帽移动
	JoystickButtonDown,  // 手柄按键按下
	JoystickButtonUp,    // 手柄按键抬起
	JoystickAdd,         // 手柄加入
	JoystickRemove,      // 手柄移除
	JoystickFirstEvent = JoystickAxisMotion,
	JoystickLastEvent = JoystickRemove,

	GamepadAxisMotion = 0x700,    // 游戏手柄轴移动
	GamepadButtonDown,  // 游戏手柄按键按下
	GamepadButtonUp,    // 游戏手柄按键抬起
	GamepadAdd,         // 游戏手柄加入
	GamepadRemove,      // 游戏手柄移除
    GamepadTouchpadDown,    // 游戏手柄触摸板按下
	GamepadTouchpadUp,      // 游戏手柄触摸板抬起
	GamepadTouchpadMotion,    // 游戏手柄触摸板移动
	GamepadFirstEvent = GamepadAxisMotion,
	GamepadLastEvent = GamepadTouchpadMotion,

// 触摸板
	FingerDown = 0x800,    // 手指按下
	FingerUp,          // 手指抬起
	FingerMotion,      // 手指移动
	FingerFirstEvent = FingerDown,
	FingerLastEvent = FingerMotion,

// 剪切板
	ClipboardUpdate = 0x900,    // 剪切板更新
    ClipboardFirstEvent = ClipboardUpdate,
    ClipboardLastEvent = ClipboardUpdate,

	DropFile = 0x1000,		  // 文件拖拽
	DropText,			  // 文本拖拽
    DropBegin,			  // 拖拽开始
	DropEnd,			  // 拖拽结束
	DropFirstEvent = DropFile,
	DropLastEvent = DropEnd,

// 音频
	AudioDeviceAdded = 0x1100,    // 音频设备添加
	AudioDeviceRemoved,            // 音频设备移除
	AudioDeviceFormatChanged,            // 音频设备格式改变
	AudioDeviceFirstEvent = AudioDeviceAdded,
	AudioDeviceLastEvent = AudioDeviceFormatChanged,

// 压感笔
	PenProximityIn = 0x1300,    // 压感笔可用
	PenProximityOut,           // 压感笔不可用
	PenDown,                // 压感笔按下在表面绘制
	PenUp,                  // 压感笔抬起停止绘制
	PenButtonDown,          // 压感笔按键按下
	PenButtonUp,            // 压感笔按键抬起
	PenMotion,              // 压感笔移动
	PenAxis,                // 压感笔角度, 压力变化
	PenFirstEvent = PenProximityIn,
	PenLastEvent = PenAxis,

// 渲染设备
	RenderTargetReset = 0x2000,    // 渲染目标充值, 以及其内容更新
	RenderDeviceReset,    // 渲染设备重置
	RenderDeviceLost,     // 渲染设备丢失
	RenderTargetFirstEvent = RenderTargetReset,
	RenderTargetLastEvent = RenderDeviceLost,

	LastEvent = 0xFFFF

};

class Application
{
public:
	Application();
	~Application();

	void tick(float DeltaTime);
};
}