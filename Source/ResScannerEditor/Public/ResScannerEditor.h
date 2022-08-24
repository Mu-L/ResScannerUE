// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThreadUtils/ScannerNotificationProxy.h"
#include "Modules/ModuleManager.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "FScannerPackageTracker.h"
#include "ResScannerEditor.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogResScannerEditor,All,All);

UCLASS()
class RESSCANNEREDITOR_API UResScannerRegister : public UObject
{
	GENERATED_BODY()
	UFUNCTION(meta=(QEToolBar="FFUtils/SystemUtils.ResScanner", QEIcon="ResScanner/Resources/Icon128.png"))
	static void OpenResScannerEditor();
};

class FResScannerEditorModule : public IModuleInterface
{
public:
	static FResScannerEditorModule& Get();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void PluginButtonClicked();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& InSpawnTabArgs);
	void OnTabClosed(TSharedRef<SDockTab> InTab);
	void AddMenuExtension(FMenuBuilder& Builder);
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void RunProcMission(const FString& Bin, const FString& Command, const FString& MissionName);
	void OnAssetUpdate(const FAssetData& NewAsset);
	void PackageSaved(const FString& PacStr,UObject* PackageSaved);
	void CreateExtensionSettings();
	void OnEnginePreExit();
	
private:
	mutable TSharedPtr<FProcWorkerThread> mProcWorkingThread;
	UScannerNotificationProxy* MissionNotifyProay = NULL;
	class UResScannerProxy* ScannerProxy = NULL;
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<SDockTab> DockTab;

	TSharedPtr<FScannerPackageTracker> ScannerPackageTracker;
};
