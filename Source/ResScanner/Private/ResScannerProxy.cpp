#include "ResScannerProxy.h"

#include "FlibSourceControlHelper.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY(LogResScannerProxy);
#define LOCTEXT_NAMESPACE "UResScannerProxy"

void UResScannerProxy::Init()
{
	SCOPED_NAMED_EVENT_TEXT("UResScannerProxy::Init",FColor::Red);
	if(!GetScannerConfig().IsValid())
	{
		ScannerConfig = MakeShareable(new FScannerConfig);
	}
	MatchOperators.Add(TEXT("NameMatchRule"),MakeShareable(new NameMatchOperator));
	MatchOperators.Add(TEXT("PathMatchRule"),MakeShareable(new PathMatchOperator));
	MatchOperators.Add(TEXT("PropertyMatchRule"),MakeShareable(new PropertyMatchOperator));
	MatchOperators.Add(TEXT("CustomMatchRule"),MakeShareable(new CustomMatchOperator));
}

void UResScannerProxy::Shutdown()
{
}

FRuleMatchedInfo UResScannerProxy::ScanSingleRule(const TArray<FAssetData>& GlobalAssets,const FScannerMatchRule& ScannerRule,int32 RuleID/* = 0*/)
{
	FScopedNamedEventStatic ScanSingleRule(FColor::Red,*ScannerRule.RuleName);
	FRuleMatchedInfo RuleMatchedInfo;
	// FScannerMatchRule& ScannerRule = GetScannerConfig()->ScannerRules[RuleID];
	if(!ScannerRule.bEnableRule)
	{
		UE_LOG(LogResScannerProxy,Warning,TEXT("rule %s is missed!"),*ScannerRule.RuleName);
		return RuleMatchedInfo;
	}
	if(!ScannerRule.ScanFilters.Num() && !ScannerConfig->bByGlobalScanFilters)
	{
		UE_LOG(LogResScannerProxy,Warning,TEXT("rule %s not contain any filters!"),*ScannerRule.RuleName);
		return RuleMatchedInfo;
	}
	if(!ScannerRule.HasValidRules())
	{
		UE_LOG(LogResScannerProxy,Warning,TEXT("rule %s not contain any rules!"),*ScannerRule.RuleName);
		return RuleMatchedInfo;
	}
	
	FString RuleConfig;
	TemplateHelper::TSerializeStructAsJsonString(ScannerRule,RuleConfig);

	if(GetScannerConfig()->bVerboseLog)
	{
		UE_LOG(LogResScannerProxy,Display,TEXT("RuleName %s is Scanning."),*ScannerRule.RuleName);
		UE_LOG(LogResScannerProxy,Display,TEXT("RuleName %s is Scanning. config:\n%s"),*ScannerRule.RuleName,*RuleConfig);
	}
	
	TArray<FAssetData> FilterAssets;
	if(GetScannerConfig()->bByGlobalScanFilters || GetScannerConfig()->GitChecker.bGitCheck)
	{
		FilterAssets = UFlibAssetParseHelper::GetAssetsWithCachedByTypes(GlobalAssets,TArray<UClass*>{ScannerRule.ScanAssetType},ScannerRule.bGlobalAssetMustMatchFilter,ScannerRule.ScanFilters,ScannerRule.RecursiveClasses);
	}
	if(!GetScannerConfig()->bBlockRuleFilter)
	{
		FilterAssets.Append(UFlibAssetParseHelper::GetAssetsByFiltersByClass(TArray<UClass*>{ScannerRule.ScanAssetType},ScannerRule.ScanFilters,ScannerRule.RecursiveClasses));
	}
	RuleMatchedInfo.RuleName = ScannerRule.RuleName;
	RuleMatchedInfo.RuleDescribe = ScannerRule.RuleDescribe;
	RuleMatchedInfo.RuleID  = RuleID;
	TArray<FAssetFilters> FinalIgnoreFilters;
	FinalIgnoreFilters.Add(GetScannerConfig()->GlobalIgnoreFilters);
	
	FinalIgnoreFilters.Add(ScannerRule.IgnoreFilters);
	
	for(const auto& Asset:FilterAssets)
	{
		if(!UFlibAssetParseHelper::IsIgnoreAsset(Asset,FinalIgnoreFilters))
		{
			bool bMatchAllRules = !!GetMatchOperators().Num() ? true : false;
			for(const auto& Operator:GetMatchOperators())
			{
				bMatchAllRules = Operator.Value->Match(Asset,ScannerRule);
				if(!bMatchAllRules)
				{
					break;
				}
			}
			if(bMatchAllRules)
			{
				RuleMatchedInfo.Assets.AddUnique(Asset);
				RuleMatchedInfo.AssetPackageNames.AddUnique(Asset.PackageName.ToString());
			}
			if(GetScannerConfig()->bVerboseLog && bMatchAllRules)
			{
				UE_LOG(LogResScannerProxy,Display,TEXT("\t%s"),*Asset.GetFullName());
			}
		}
	}
	
	if(!!RuleMatchedInfo.Assets.Num() && ScannerRule.bEnablePostProcessor)
	{
		// 对扫描之后的资源进行后处理（可以执行自动化处理操作）
		PostProcessorMatchRule(ScannerRule,RuleMatchedInfo);
	}
	return RuleMatchedInfo;
}

FMatchedResult UResScannerProxy::DoScan()
{
	SCOPED_NAMED_EVENT_TEXT("UResScannerProxy::DoScan",FColor::Red);
	FString ScanConfigContent;
	TemplateHelper::TSerializeStructAsJsonString(*GetScannerConfig(),ScanConfigContent);
	UE_LOG(LogResScannerProxy, Display, TEXT("%s"), *ScanConfigContent);

	TArray<FAssetData> GlobalAssets;
	if(GetScannerConfig()->bByGlobalScanFilters)
	{
		 GlobalAssets = UFlibAssetParseHelper::GetAssetsByObjectPath(GetScannerConfig()->GlobalScanFilters.Assets);
		 GlobalAssets.Append(UFlibAssetParseHelper::GetAssetsByFiltersByClass(TArray<UClass*>{},GetScannerConfig()->GlobalScanFilters.Filters, true));
		
		UE_LOG(LogResScannerProxy,Display,TEXT("assets by global config"));
		for(const auto& Asset:GlobalAssets)
		{
			UE_LOG(LogResScannerProxy,Display,TEXT("\t%s"),*Asset.AssetName.ToString());
		}
	}
	if(GetScannerConfig()->GitChecker.bGitCheck)
	{
		FString OutRepoDir;
		if(UFlibSourceControlHelper::FindRootDirectory(GetScannerConfig()->GitChecker.GetRepoDir(),OutRepoDir))
		{
			TArray<FSoftObjectPath> ObjectPaths = UFlibAssetParseHelper::GetAssetsByGitChecker(GetScannerConfig()->GitChecker);
			GlobalAssets.Append(UFlibAssetParseHelper::GetAssetsByObjectPath(ObjectPaths));
			UE_LOG(LogResScannerProxy,Display,TEXT("assets by git repo %s:"),*OutRepoDir);
			for(const auto& ObjectPath:ObjectPaths)
			{
				UE_LOG(LogResScannerProxy,Display,TEXT("\t%s"),*ObjectPath.GetLongPackageName());
			}
		}
		else
		{
			UE_LOG(LogResScannerProxy,Display,TEXT("%s is not a valid git repo."),*OutRepoDir);
		}
	}

	FMatchedResult MatchedResult = ScanAssets(GlobalAssets);
	
	bool bRecordCommiter = GetScannerConfig()->GitChecker.bGitCheck && GetScannerConfig()->GitChecker.bRecordCommiter;
	MatchedResult.RecordGitCommiter(bRecordCommiter,GetScannerConfig()->GitChecker.GetRepoDir());
	
	FString SaveBasePath = UFlibAssetParseHelper::ReplaceMarkPath(GetScannerConfig()->SavePath.Path);


	FString Name = GetScannerConfig()->ConfigName;
	if(Name.IsEmpty())
	{
		Name = FDateTime::UtcNow().ToString();
	}
	
	// serialize config
	if(GetScannerConfig()->bSaveConfig)
	{
		FString SerializedJsonStr;
		TemplateHelper::TSerializeStructAsJsonString(*ScannerConfig,SerializedJsonStr);
		
		FString SavePath = FPaths::Combine(SaveBasePath,FString::Printf(TEXT("%s_config.json"),*Name));
		if(FFileHelper::SaveStringToFile(SerializedJsonStr, *SavePath,FFileHelper::EEncodingOptions::ForceUTF8) && !IsRunningCommandlet())
		{
			FText Msg = LOCTEXT("SavedScanConfigMag", "Successd to Export the Config.");
			UFlibAssetParseHelper::CreateSaveFileNotify(Msg,SavePath,SNotificationItem::CS_Success);
		}
	}
	
	FString ResultSavePath = FPaths::Combine(SaveBasePath,FString::Printf(TEXT("%s_result.json"),*Name));
	IFileManager::Get().Delete(*ResultSavePath);
	
	// serialize matched assets
	if(GetScannerConfig()->bSaveResult && MatchedResult.HasValidResult())
	{
		FString SerializedJsonStr = MatchedResult.SerializeResult(GetScannerConfig()->bSavaeLiteResult);
		
		if(FFileHelper::SaveStringToFile(SerializedJsonStr, *ResultSavePath,FFileHelper::EEncodingOptions::ForceUTF8) && !IsRunningCommandlet())
		{
			FText Msg = LOCTEXT("SavedScanResultMag", "Successd to Export the scan result.");
			if(::IsRunningCommandlet())
			{
				UE_LOG(LogResScannerProxy,Log,TEXT("%s"),*SerializedJsonStr);	
			}
			else
			{
				UFlibAssetParseHelper::CreateSaveFileNotify(Msg,ResultSavePath,SNotificationItem::CS_Success);
			}
		}
	}
	return MatchedResult;
}

void UResScannerProxy::SetScannerConfig(FScannerConfig InConfig)
{
	if(!ScannerConfig.IsValid())
	{
		ScannerConfig = MakeShareable(new FScannerConfig);
	}
	*ScannerConfig = InConfig;
}

FMatchedResult UResScannerProxy::ScanAssets(const TArray<FAssetData>& Assets)
{
	UE_LOG(LogResScannerProxy,Display,TEXT("Asset Scanning"));
	
	FMatchedResult ScanResult;
	auto ScanAssetByRule = [this](const TArray<FAssetData>& Assets,const FScannerMatchRule& Rule,int32 RuleID,FRuleMatchedInfo& OutResult)->bool
	{
		bool bResult = false;
		bool bIsAllowRule = GetScannerConfig()->IsAllowRule(Rule,RuleID);
		if(bIsAllowRule)
		{
			OutResult = ScanSingleRule(Assets,Rule,RuleID);
			bResult = !!OutResult.Assets.Num();
		}
		UE_LOG(LogResScannerProxy,Display,TEXT("Rule \"%s\" is %s!"),*Rule.RuleName,bIsAllowRule ? TEXT("enabled"):TEXT("disabled"));
		return bResult;
	};
	
	if(GetScannerConfig()->bUseRulesTable)
	{
		TArray<FScannerMatchRule> ImportRules = GetScannerConfig()->GetTableRules();
		UE_LOG(LogResScannerProxy,Display,TEXT("Total Rules: %d!"),ImportRules.Num());
		
		for(int32 RuleID = 0;RuleID < ImportRules.Num();++RuleID)
		{
			const auto& ScanRule = ImportRules[RuleID];
			FRuleMatchedInfo RuleMatchedInfo;
			if(ScanAssetByRule(Assets,ScanRule,RuleID,RuleMatchedInfo))
			{
				ScanResult.GetMatchedInfo().Add(RuleMatchedInfo);
			}
			
		}
	}
	
	for(int32 RuleID = 0;RuleID < GetScannerConfig()->ScannerRules.Num();++RuleID)
	{
		const auto& ScanRule = GetScannerConfig()->ScannerRules[RuleID];
		FRuleMatchedInfo RuleMatchedInfo;
		if(ScanAssetByRule(Assets,ScanRule,RuleID,RuleMatchedInfo))
		{
			ScanResult.GetMatchedInfo().Add(RuleMatchedInfo);
		}
	}
	return ScanResult;
}



void UResScannerProxy::PostProcessorMatchRule(const FScannerMatchRule& Rule,const FRuleMatchedInfo& RuleMatchedInfo)
{
	for(const auto& PostProcessorClass:Rule.PostProcessors)
	{
		if(IsValid(PostProcessorClass))
		{
			UScannnerPostProcessorBase* PostProcessorIns = Cast<UScannnerPostProcessorBase>(PostProcessorClass->GetDefaultObject());
			if(PostProcessorIns)
			{
				PostProcessorIns->Processor(RuleMatchedInfo,Rule.ScanAssetType->GetName());
			}
		}
	}
}
#undef LOCTEXT_NAMESPACE
