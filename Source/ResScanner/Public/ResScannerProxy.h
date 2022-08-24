#pragma once
#include "FMatchRuleTypes.h"
#include "FlibAssetParseHelper.h"
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "ResScannerProxy.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogResScannerProxy, Log, All);

UCLASS(BlueprintType)
class RESSCANNER_API UResScannerProxy:public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    virtual void Init();
    UFUNCTION(BlueprintCallable)
    virtual void Shutdown();
    UFUNCTION(BlueprintCallable)
    virtual FMatchedResult DoScan();
    UFUNCTION(BlueprintCallable)
    virtual void SetScannerConfig(FScannerConfig InConfig);

    FRuleMatchedInfo ScanSingleRule(const TArray<FAssetData>& GlobalAssets,const FScannerMatchRule& ScannerRule,int32 RuleID = 0);
    
    virtual TSharedPtr<FScannerConfig> GetScannerConfig(){return ScannerConfig;}
    virtual TMap<FString,TSharedPtr<IMatchOperator>>& GetMatchOperators(){return MatchOperators;}
    
    FMatchedResult ScanAssets(const TArray<FAssetData>& Assets);
    
protected:
    virtual void PostProcessorMatchRule(const FScannerMatchRule& Rule,const FRuleMatchedInfo& RuleMatchedInfo);    
private:
    TSharedPtr<FScannerConfig> ScannerConfig;
    TMap<FString,TSharedPtr<IMatchOperator>> MatchOperators;
};
