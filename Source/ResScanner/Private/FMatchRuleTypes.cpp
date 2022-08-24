#include "FMatchRuleTypes.h"
#include "FlibAssetParseHelper.h"

FString FGitChecker::GetRepoDir() const
{
	return UFlibAssetParseHelper::ReplaceMarkPath(RepoDir.Path);
}

FScannerConfig::FScannerConfig()
{
	SavePath.Path = TEXT("[PROJECT_SAVED_DIR]/ResScanner");
}

bool FScannerConfig::IsAllowRule(const FScannerMatchRule& Rule, int32 RuleID) const
{
	bool bIsAllow = false;
	switch (ScanRulesType)
	{
		case EScanRulesType::All: {
				bIsAllow = true;
				break;
		}
		case EScanRulesType::RuleIDs: {
				bool bCheck = RuleWhileListIDs.Num() ? RuleWhileListIDs.Contains(RuleID) : true;
				bCheck = RuleBlockListIDs.Contains(RuleID) ? false : bCheck;
				bIsAllow = bCheck;
				break;
		}
		case EScanRulesType::Prioritys: {
				bIsAllow = Prioritys.Contains(Rule.Priority);
				break;
		}
	}
	return bIsAllow;
}

TArray<FScannerMatchRule> FScannerConfig::GetTableRules() const
{
	TArray<FScannerMatchRule> result;
	if(ImportRulesTable.IsValid())
	{
		UDataTable* RulesTable = Cast<UDataTable>(ImportRulesTable.TryLoad());
		if(RulesTable)
		{
			TArray<FName> RowNames = RulesTable->GetRowNames();
			FString ContextString;
			for (auto& name : RowNames)
			{
				FScannerMatchRule* pRow = RulesTable->FindRow<FScannerMatchRule>(name, ContextString);
				result.Add(*pRow);
			}
		}
	}
	
	return result;
}

void FScannerConfig::HandleImportRulesTable()
{
	for (auto& Rule : GetTableRules())
	{
		ScannerRules.Add(Rule);
	}
}

void FMatchedResult::RecordGitCommiter(bool bInRecordCommiter, const FString& RepoDir)
{
	FRuleMatchedInfo::ResetTransient();
	FRuleMatchedInfo::SetSerializeTransient(bInRecordCommiter); // GetScannerConfig()->GitChecker.bGitCheck && GetScannerConfig()->GitChecker.bRecordCommiter);
	if(bInRecordCommiter)
	{
		UFlibAssetParseHelper::CheckMatchedAssetsCommiter(*this,RepoDir);
		bRecordCommiter = bInRecordCommiter;
	}
}

FString FMatchedResult::SerializeLiteResult()const
{
	FString Result;
	// bool bRecordCommiter = GetScannerConfig()->GitChecker.bGitCheck && GetScannerConfig()->GitChecker.bRecordCommiter;
	for(const auto& RuleMatchedInfo:MatchedAssets)
	{
		Result += TEXT("-------------------------------------------\n");
		if(RuleMatchedInfo.AssetPackageNames.Num() || RuleMatchedInfo.AssetsCommiter.Num())
		{
			FString Describle = RuleMatchedInfo.RuleDescribe.IsEmpty() ? TEXT(""):FString::Printf(TEXT("(%s)"),*RuleMatchedInfo.RuleDescribe);
			Result += FString::Printf(TEXT("规则名: %s (%d) %s\n"),*RuleMatchedInfo.RuleName,RuleMatchedInfo.AssetPackageNames.Num(),*Describle);
		}
		
		if(bRecordCommiter)
		{
			if(RuleMatchedInfo.AssetsCommiter.Num())
			{
				for(const auto& AssetCommiter:RuleMatchedInfo.AssetsCommiter)
				{
					Result += FString::Printf(TEXT("\t%s, %s\n"),*AssetCommiter.File,*AssetCommiter.Commiter);
				}
			}
		}
		else
		{
			if(RuleMatchedInfo.AssetPackageNames.Num())
			{
				for(const auto& AssetPackageName:RuleMatchedInfo.AssetPackageNames)
				{
					Result += FString::Printf(TEXT("\t%s\n"),*AssetPackageName);
				}
			}
		}
	}
	Result += TEXT("-------------------------------------------\n");
	return Result;
}

FString FMatchedResult::SerializeResult(bool Lite)const
{
	FString Result;
	if(Lite)
	{
		Result = SerializeLiteResult();
	}
	else
	{
		TemplateHelper::TSerializeStructAsJsonString(*this,Result);
	}
	return Result;
}

bool FMatchedResult::HasValidResult() const
{
	return !!MatchedAssets.Num();
}
