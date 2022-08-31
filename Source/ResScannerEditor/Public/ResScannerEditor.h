// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataTableEditorUtils.h"
#include "Kismet2/StructureEditorUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogResScannerEditor,All,All);

class FScannerDataTableListener:
	public FNotifyHook,
	public FStructureEditorUtils::INotifyOnStructChanged,
	public FDataTableEditorUtils::INotifyOnDataTableChanged
{
	// FNotifyHook
	virtual void NotifyPreChange( FProperty* PropertyAboutToChange ) override{}
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged ) override{}

	// INotifyOnStructChanged
	virtual void PreChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override{}
	virtual void PostChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override{}
	
	// INotifyOnDataTableChanged
	virtual void PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override{}
	virtual void PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;
};

class RESSCANNEREDITOR_API FResScannerEditorModule : public IModuleInterface
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
	void PackageSaved(const FString& PacStr,UObject* PackageSaved);
	void CreateExtensionSettings();
	void OnEnginePreExit();
	
private:
	mutable TSharedPtr<class FProcWorkerThread> mProcWorkingThread;
	class UScannerNotificationProxy* MissionNotifyProay = NULL;
	class UResScannerProxy* ScannerProxy = NULL;
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<SDockTab> DockTab;

	TSharedPtr<struct FScannerPackageTracker> ScannerPackageTracker;
	TSharedPtr<FScannerDataTableListener> ScannerDataTableListener;
};
