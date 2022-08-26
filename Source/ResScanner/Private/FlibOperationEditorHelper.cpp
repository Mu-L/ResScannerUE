// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibOperationEditorHelper.h"

#include "FlibAssetParseHelper.h"
#include "Engine/TextureCube.h"
#if WITH_EDITOR
	#include "BlueprintEditorSettings.h"
	#include "Kismet2/CompilerResultsLog.h"
	#include "Kismet2/KismetEditorUtilities.h"
#endif

bool URule_CheckBlueprintError::Match_Implementation(UObject* Object, const FString& Asset)
{
	return UFlibOperationEditorHelper::BlueprintHasError(Object,false);
}

URule_IsAllowCommiterChecker::URule_IsAllowCommiterChecker(const FObjectInitializer& Initializer):Super(Initializer),RepoDir(SC_PROJECT_CONTENT_DIR_MARK)
{
}



bool URule_IsAllowCommiterChecker::Match_Implementation(UObject* Object, const FString& AssetType)
{
	bool bIsAllow = true;
	FString RepoRootDir = UFlibAssetParseHelper::ReplaceMarkPath(RepoDir);
	
	if(FPaths::DirectoryExists(RepoRootDir))
	{
		FString LongPackageName;
		
		if(UFlibAssetParseHelper::GetLongPackageNameByObject(Object,LongPackageName))
		{
			FFileCommiter FileCommiter;
			bool bGetStatus = UFlibAssetParseHelper::GetLocalEditorByLongPackageName(RepoDir,LongPackageName,FileCommiter);
     
			if(!bGetStatus)
			{
				bGetStatus = UFlibAssetParseHelper::GetGitCommiterByLongPackageName(RepoDir,LongPackageName,FileCommiter);
			}
     
			if(bGetStatus)
			{
				bIsAllow = AllowCommiters.Contains(FileCommiter.Commiter);
			}
		}
	}
	return !bIsAllow;
}

bool UFlibOperationEditorHelper::CompileBlueprint(UObject* Blueprint, int32& OutNumError, int32& OutNumWarning)
{
	bool bBlueprintHasError = false;
#if WITH_EDITOR
	if(Blueprint)
	{
		UBlueprint* BlueprintIns = Cast<UBlueprint>(Blueprint);
		if(BlueprintIns)
		{
			FCompilerResultsLog LogResults;
			LogResults.SetSourcePath(BlueprintIns->GetPathName());
			LogResults.BeginEvent(TEXT("Compile"));
			LogResults.bLogDetailedResults = GetDefault<UBlueprintEditorSettings>()->bShowDetailedCompileResults;
			LogResults.EventDisplayThresholdMs = GetDefault<UBlueprintEditorSettings>()->CompileEventDisplayThresholdMs;
			EBlueprintCompileOptions CompileOptions = EBlueprintCompileOptions::BatchCompile | EBlueprintCompileOptions::SkipSave;
			bool bSaveIntermediateBuildProducts = true;
			if( bSaveIntermediateBuildProducts )
			{
				CompileOptions |= EBlueprintCompileOptions::SaveIntermediateProducts;
			}
			FKismetEditorUtilities::CompileBlueprint(BlueprintIns, CompileOptions, &LogResults);

			LogResults.EndEvent();
			OutNumError = LogResults.NumErrors;
			OutNumWarning = LogResults.NumWarnings;
		
			if(BlueprintIns->Status == EBlueprintStatus::BS_Error)
			{
				bBlueprintHasError = true;
			}
		}
	}

#endif
	return bBlueprintHasError;
}

bool UFlibOperationEditorHelper::BlueprintHasError(UObject* Blueprint,bool bWarningAsError)
{
	int32 OutNumError = 0;
	int32 OutNumWarning = 0;
	bool bHasError = false;
	UBlueprint* BlueprintIns = Cast<UBlueprint>(Blueprint);
	if(BlueprintIns)
	{
		bHasError = UFlibOperationEditorHelper::CompileBlueprint(BlueprintIns,OutNumError,OutNumWarning);
	}
	
	bool bRetHasError = bWarningAsError ? (bHasError || OutNumWarning > 0) : bHasError;
	return bRetHasError;
}

FVector2D UFlibOperationEditorHelper::GetTextureCubeSize(UTextureCube* TextureCube)
{
	FVector2D result;
	if(TextureCube)
	{
		result.X = TextureCube->GetSizeX();
		result.Y = TextureCube->GetSizeY();
	}
	return result;
}

bool UFlibOperationEditorHelper::IsAllowCommiterChanged(const FString& LongPackageName, const FString& RepoDir,
	const TArray<FString>& AllowCommiter)
{
	bool bIsAllow = true;
	FFileCommiter FileCommiter;
	bool bGetStatus = UFlibAssetParseHelper::GetLocalEditorByLongPackageName(RepoDir,LongPackageName,FileCommiter);

	if(!bGetStatus)
	{
		bGetStatus = UFlibAssetParseHelper::GetGitCommiterByLongPackageName(RepoDir,LongPackageName,FileCommiter);
	}

	if(bGetStatus)
	{
		bIsAllow = AllowCommiter.Contains(FileCommiter.Commiter);
	}
	return bIsAllow;
}