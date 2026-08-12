#pragma once

namespace app
{

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
}