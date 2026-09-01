
#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <generic_application/UICommon.hpp>
#include <generic_application/GenericApplication.hpp>
namespace app
{

[[nodiscard]] std::unique_ptr<ui::IGenericApplication> createGenericApplication();


class Application final
	: public ui::IApplicationMessageHandler
	, std::enable_shared_from_this<Application>
{
public:
	static std::shared_ptr<Application> create(ui::GenericWindowPointer PlatformRelatedWindow);

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
	ui::GenericWindowPointer PlatformRelatedWindow;
	bool bExitRequested = false;
};
}