#pragma once

class EngineCycle final
{
public:
	EngineCycle() = default;
	~EngineCycle() = default;

	/**
	 * @brief 负责初始化基础设置和与平台相关的设置
	 * 	- 设置平台相关系统
	 * 		- RHI
	 *		- Application 底层 UI 系统
	 * 	- 日志系统
	 * 	- 内存分配器
	 * 	- 反射系统
	 */
	void preinitialize();

	/**
	 * @brief 正式初始化引擎各个子系统
	 */
	void initialize();
	
	/**
	 * @brief 本质上就是引擎执行的第一帧
	 */
	void postinitialize();
	void run();
	void processEvents();
	void render();
	void quit();
	bool shouldQuit() const { return bShouldQuit; }
	void destroy();
private:
	void tick(float DeltaTime);
	bool bShouldQuit { false };
};