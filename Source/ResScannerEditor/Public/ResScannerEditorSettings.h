#pragma once

// Engine Header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Uobject/NoExportTypes.h"
#include "FMatchRuleTypes.h"
#include "FlibAssetParseHelper.h"

#include "ResScannerEditorSettings.generated.h"


UCLASS(BlueprintType, Blueprintable, config = GameUserSettings, defaultconfig)
class UResScannerEditorSettings : public UObject
{
    GENERATED_BODY()
public:
    UResScannerEditorSettings();
    
    UPROPERTY(config, EditAnywhere)
    bool bEnableEditorCheck = false;
    
    UPROPERTY(config, EditAnywhere)
    FScannerConfig EditorScannerConfig;

    UPROPERTY(config, EditAnywhere)
    bool bEnableCookingCheck = false;

    UPROPERTY(config, EditAnywhere)
    FScannerConfig CookingScannerConfig;

    void InitializeConfig(FScannerConfig& ScannerConfig);
    
};


inline UResScannerEditorSettings::UResScannerEditorSettings()
{
    InitializeConfig(EditorScannerConfig);
    EditorScannerConfig.ConfigName = TEXT("Editor");
    InitializeConfig(CookingScannerConfig);
    CookingScannerConfig.ConfigName = TEXT("Cooking");   
}

inline void UResScannerEditorSettings::InitializeConfig(FScannerConfig& ScannerConfig)
{
    ScannerConfig.ScanRulesType = EScanRulesType::All;
    ScannerConfig.bByGlobalScanFilters = true;
    ScannerConfig.bBlockRuleFilter = true;
    ScannerConfig.GitChecker.bGitCheck = false;
    ScannerConfig.bSaveConfig = false;
    ScannerConfig.bSaveResult = false;
    ScannerConfig.bStandaloneMode = false;
    ScannerConfig.bNoShaderCompile = false;
    ScannerConfig.bSavaeLiteResult = false;
    ScannerConfig.bUseRulesTable = true;
    ScannerConfig.bVerboseLog = false;
    ScannerConfig.SavePath.Path = FPaths::Combine(SC_PROJECT_SAVED_DIR_MARK,TEXT("ResScanner"));
}
