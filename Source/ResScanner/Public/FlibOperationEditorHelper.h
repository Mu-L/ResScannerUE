// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FMatchRuleTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibOperationEditorHelper.generated.h"


UCLASS()
class URule_CheckBlueprintError:public UOperatorBase
{
	GENERATED_BODY()
public:
	virtual bool Match_Implementation(UObject* Object,const FString& AssetType) override;
};


UCLASS()
class URule_IsAllowCommiterChecker:public UOperatorBase
{
	GENERATED_BODY()
public:
	URule_IsAllowCommiterChecker(const FObjectInitializer& Initializer);
	
	virtual bool Match_Implementation(UObject* Object,const FString& AssetType) override;
	virtual bool MatchFast_Implementation(const FString& LongPackagePath,const FString& AssetType) override;
protected:
	// [PROJECT_CONTENT_DIR] or [PROJECT_DIR]
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString RepoDir;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FString> AllowCommiters;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bUseGitUserName = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bUseHostName = false;
};

/**
 * 
 */
UCLASS()
class RESSCANNER_API UFlibOperationEditorHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable,meta=(AutoCreateRefTerm="OutNumError,OutNumWarning"))
	static bool CompileBlueprint(UObject* Blueprint,int32& OutNumError,int32& OutNumWarning);
	UFUNCTION(BlueprintCallable)
	static bool BlueprintHasError(UObject* Blueprint,bool bWarningAsError = false);
	
	UFUNCTION(BlueprintCallable,BlueprintPure)
	static FVector2D GetTextureCubeSize(UTextureCube* TextureCube);

	UFUNCTION(BlueprintCallable,BlueprintPure)
	static bool IsAllowCommiterChanged(const FString& LongPackageName,const FString& RepoDir,const TArray<FString>& AllowCommiter);
};
