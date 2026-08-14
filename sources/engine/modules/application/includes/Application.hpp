
#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <AppCommon.hpp>
#include <GenericApplication.hpp>
namespace app
{


class Application final
	: public IApplicationMessageHandler
	, std::enable_shared_from_this<Application>
{
public:
	static std::shared_ptr<Application> create(GenericWindowPointer PlatformRelatedWindow);

	~Application();

	void tick(float DeltaTime);

	[[nodiscard]]
	bool isExitRequested() const { return bExitRequested; }

	void onQuitRequested() override;
	void onAppTermination() override;
	void onWindowClose() override;
private:
	Application();

	void initialize();
private:
	GenericWindowPointer PlatformRelatedWindow;
	bool bExitRequested = false;
};
}