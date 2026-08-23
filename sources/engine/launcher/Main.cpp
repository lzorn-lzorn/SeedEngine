#include <memory>
#include <new>

#include "RTGlobal.hpp"
#include "CommandLine.hpp"
#include "ModuleLoader.hpp"

int main(int argc, char** argv)
{
	CommandLine cmd(argc, argv);
	// 初始化内存分配器

	// 初始化模块加载器
	ModuleLoader::self();
	
	// 初始化引擎
	GSeedEngine.preinitialize();

	GSeedEngine.initialize();
	
	GSeedEngine.run();
	
	GSeedEngine.destroy();
	return 0;
}