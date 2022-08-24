// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResScannerEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "LevelEditor.h"
#include "ResScannerCommands.h"
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION>=26
	#define InvokeTab TryInvokeTab
#endif
#include "EditorModeRegistry.h"
#include "Misc/FileHelper.h"
#include "ISettingsModule.h"
#include "ResScannerEditorSettings.h"
#include "ResScannerProxy.h"
#include "SResScanner.h"
#include "DetailCustomization/CustomPropertyMatchMappingDetails.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "SVersionUpdater/VersionUpdaterStyle.h"

static const FName ResScannerTabName("ResScanner");

#define LOCTEXT_NAMESPACE "FResScannerEditorModule"

DEFINE_LOG_CATEGORY(LogResScannerEditor);


FString LongPackageNameToPackagePath(const FString& InLongPackageName)
{
	SCOPED_NAMED_EVENT_TEXT("LongPackageNameToPackagePath",FColor::Red);
	if(InLongPackageName.Contains(TEXT(".")))
	{
		return InLongPackageName;
	}
	FString AssetName;
	{
		int32 FoundIndex;
		InLongPackageName.FindLastChar('/', FoundIndex);
		if (FoundIndex != INDEX_NONE)
		{
			AssetName = UKismetStringLibrary::GetSubstring(InLongPackageName, FoundIndex + 1, InLongPackageName.Len() - FoundIndex);
		}
	}
	FString OutPackagePath = InLongPackageName + TEXT(".") + AssetName;
	return OutPackagePath;
}


void UResScannerRegister::OpenResScannerEditor()
{
	FResScannerEditorModule& ResScannerModule = FModuleManager::LoadModuleChecked<FResScannerEditorModule>(ResScannerTabName);
	ResScannerModule.PluginButtonClicked();
}

FResScannerEditorModule& FResScannerEditorModule::Get()
{
	FResScannerEditorModule& Module = FModuleManager::GetModuleChecked<FResScannerEditorModule>("ResScannerEditor");
	return Module;
}

void FResScannerEditorModule::StartupModule()
{
	if(IsRunningCommandlet())
	{
		GConfig->SetBool(TEXT("/Script/LiveCoding.LiveCodingSettings"),TEXT("bEnabled"),false,GEditorPerProjectIni);
	}
	FResScannerStyle::Initialize();
	FResScannerStyle::ReloadTextures();
	FResScannerCommands::Register();
	
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.RegisterCustomPropertyTypeLayout("PropertyMatchMapping", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCustomPropertyMatchMappingDetails::MakeInstance));
	}
	
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FResScannerCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FResScannerEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ResScannerTabName, FOnSpawnTab::CreateRaw(this, &FResScannerEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FArtPathEditorTabTitle", "ArtPathEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FResScannerEditorModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

		// settings
		// TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		// ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FResScannerEditorModule::AddToolbarExtension));
		// 
		// LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}


	MissionNotifyProay = NewObject<UScannerNotificationProxy>();
	MissionNotifyProay->AddToRoot();

	CreateExtensionSettings();

	ScannerProxy = NewObject<UResScannerProxy>();
	ScannerProxy->AddToRoot();
	ScannerProxy->Init();
	
	ScannerProxy->SetScannerConfig(GetDefault<UResScannerEditorSettings>()->EditorScannerConfig);

	UPackage::PackageSavedEvent.AddRaw(this,&FResScannerEditorModule::PackageSaved);

	const UResScannerEditorSettings* EditorSetting = GetDefault<UResScannerEditorSettings>();
	if(::IsRunningCommandlet() && EditorSetting->bEnableCookingCheck)
	{
		ScannerPackageTracker = MakeShareable(new FScannerPackageTracker);
		FCoreDelegates::OnEnginePreExit.AddRaw(this,&FResScannerEditorModule::OnEnginePreExit);
	}
}

void FResScannerEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

}

void FResScannerEditorModule::OnEnginePreExit()
{
	const UResScannerEditorSettings* EditorSetting = GetDefault<UResScannerEditorSettings>();
    if(::IsRunningCommandlet() && EditorSetting->bEnableCookingCheck && ScannerPackageTracker.IsValid())
    {
    	UResScannerProxy* CookingScannerProxy = NewObject<UResScannerProxy>();
    	CookingScannerProxy->SetScannerConfig(GetDefault<UResScannerEditorSettings>()->CookingScannerConfig);
    	CookingScannerProxy->Init();
    		
    	TArray<FAssetData> LoadedPackageDatas;
    
    	for(const auto& PackageName:ScannerPackageTracker->GetLoadedPackageNames())
    	{
    		FSoftObjectPath ObjectPath(LongPackageNameToPackagePath(PackageName.ToString()));
    		FAssetData AssetData;
    		if(UAssetManager::Get().GetAssetDataForPath(ObjectPath,AssetData))
    		{
    			LoadedPackageDatas.AddUnique(AssetData);
    		}
    	}	
    	const auto& ScanResult = ScannerProxy->ScanAssets(LoadedPackageDatas);
    
    	if(ScanResult.HasValidResult())
    	{
    		FString ScanResultStr = ScanResult.SerializeResult(true);
    		TSharedPtr<FScannerConfig> Config = CookingScannerProxy->GetScannerConfig();
    		FString ConfigName = FString::Printf(TEXT("%s.txt"),*Config->ConfigName);
    		FString SaveTo = UFlibAssetParseHelper::ReplaceMarkPath(FPaths::Combine(Config->SavePath.Path,ConfigName));
    		if(FFileHelper::SaveStringToFile(ScanResultStr,*SaveTo,FFileHelper::EEncodingOptions::ForceUTF8))
    		{
    			UE_LOG(LogResScannerEditor,Display,TEXT("Saveing %s Scanner result to %s.\n%s"),*Config->ConfigName,*SaveTo,*ScanResultStr);
    		}
    	}
    	
    	ScannerPackageTracker = nullptr;
    }
}

void FResScannerEditorModule::CreateExtensionSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		// ClassViewer Editor Settings
		SettingsModule->RegisterSettings("Project", "Game", "ResScannerSettings",
										LOCTEXT("ResScannerSettingsDisplayName", "Res Scanner Settings"),
										LOCTEXT("ResScannerSettingsDescription", "Res Scanner Settings."),
										GetMutableDefault<UResScannerEditorSettings>()
		);
	}
}
void FResScannerEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FResScannerCommands::Get().PluginAction);
}

void FResScannerEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FResScannerCommands::Get().PluginAction);
}


void FResScannerEditorModule::PluginButtonClicked()
{
	if(!DockTab.IsValid())
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ResScannerTabName, FOnSpawnTab::CreateRaw(this, &FResScannerEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FResScannerTabTitle", "ResScanner"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	}
	FGlobalTabmanager::Get()->InvokeTab(ResScannerTabName);
}

TSharedRef<class SDockTab> FResScannerEditorModule::OnSpawnPluginTab(const class FSpawnTabArgs& InSpawnTabArgs)
{
	return SAssignNew(DockTab,SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("ResScannerTab", "Res Scanner"))
		.ToolTipText(LOCTEXT("ResScannerTabTextToolTip", "Res Scanner"))
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this,&FResScannerEditorModule::OnTabClosed))
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SResScanner)
		];
}

void FResScannerEditorModule::OnTabClosed(TSharedRef<SDockTab> InTab)
{
	DockTab.Reset();
}

void FResScannerEditorModule::RunProcMission(const FString& Bin, const FString& Command, const FString& MissionName)
{
	if (mProcWorkingThread.IsValid() && mProcWorkingThread->GetThreadStatus()==EThreadStatus::Busy)
	{
		mProcWorkingThread->Cancel();
	}
	else
	{
		mProcWorkingThread = MakeShareable(new FProcWorkerThread(*FString::Printf(TEXT("PakPresetThread_%s"),*MissionName), Bin, Command));
		mProcWorkingThread->ProcOutputMsgDelegate.AddUObject(MissionNotifyProay,&UScannerNotificationProxy::ReceiveOutputMsg);
		mProcWorkingThread->ProcBeginDelegate.AddUObject(MissionNotifyProay,&UScannerNotificationProxy::SpawnRuningMissionNotification);
		mProcWorkingThread->ProcSuccessedDelegate.AddUObject(MissionNotifyProay,&UScannerNotificationProxy::SpawnMissionSuccessedNotification);
		mProcWorkingThread->ProcFaildDelegate.AddUObject(MissionNotifyProay,&UScannerNotificationProxy::SpawnMissionFaildNotification);
		MissionNotifyProay->SetMissionName(*FString::Printf(TEXT("%s"),*MissionName));
		MissionNotifyProay->SetMissionNotifyText(
			FText::FromString(FString::Printf(TEXT("%s in progress"),*MissionName)),
			LOCTEXT("RunningCookNotificationCancelButton", "Cancel"),
			FText::FromString(FString::Printf(TEXT("%s Mission Finished!"),*MissionName)),
			FText::FromString(FString::Printf(TEXT("%s Failed!"),*MissionName))
		);
		MissionNotifyProay->MissionCanceled.AddLambda([this]()
		{
			if (mProcWorkingThread.IsValid() && mProcWorkingThread->GetThreadStatus() == EThreadStatus::Busy)
			{
				mProcWorkingThread->Cancel();
			}
		});
		
		mProcWorkingThread->Execute();
	}
}

void FResScannerEditorModule::PackageSaved(const FString& PacStr,UObject* PackageSaved)
{
	bool bEnable = GetDefault<UResScannerEditorSettings>()->bEnableEditorCheck;
	if(bEnable)
	{
		static UResScannerEditorSettings* ResScannerEditorSettings = GetMutableDefault<UResScannerEditorSettings>();
		ScannerProxy->SetScannerConfig(ResScannerEditorSettings->EditorScannerConfig);

		FString PackageName = LongPackageNameToPackagePath(FPackageName::FilenameToLongPackageName(PacStr));
		FSoftObjectPath ObjectPath(PackageName);
		FAssetData AssetData;
		if(UAssetManager::Get().GetAssetDataForPath(ObjectPath,AssetData))
		{
			const auto& ScanResult = ScannerProxy->ScanAssets(TArray<FAssetData>{AssetData});

			if(ScanResult.HasValidResult())
			{
				FString ScanResultStr = ScanResult.SerializeResult(true);
				UE_LOG(LogResScannerEditor,Warning,TEXT("\n%s"),*ScanResultStr);
				FText DialogText = UKismetTextLibrary::Conv_StringToText(ScanResultStr);
				FText DialogTitle = UKismetTextLibrary::Conv_StringToText(TEXT("ResScanner Message"));
				FMessageDialog::Open(EAppMsgType::Ok, DialogText,&DialogTitle);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FResScannerEditorModule, ResScannerEditor)