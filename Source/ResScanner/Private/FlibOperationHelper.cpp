// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibOperationHelper.h"

#include "SocketSubsystem.h"
#include "GameFramework/WorldSettings.h"

TSubclassOf<class AGameModeBase> UFlibOperationHelper::GetMapGameModeClass(UWorld* World)
{
	TSubclassOf<class AGameModeBase> GameMode;
	if (World)
	{
		AWorldSettings*  Settings = World->GetWorldSettings();
		if (Settings)
		{
			GameMode = Settings->DefaultGameMode;
		}
	}
	return GameMode;
}

TSubclassOf<AGameModeBase> UFlibOperationHelper::GetMapGameModeClassByAsset(UObject* World)
{
	return UFlibOperationHelper::GetMapGameModeClass(Cast<UWorld>(World));
}

FString UFlibOperationHelper::GetMachineHostName()
{
	static FString HostName;
	if(HostName.IsEmpty())
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetHostName(HostName);
	}
	return HostName;
}

TArray<FString> UFlibOperationHelper::GetMachineHostIPs()
{
	TArray<FString> IpAddrs = { TEXT("127.0.0.1") };
	TArray<TSharedPtr<FInternetAddr>> AdapterAddresses;
	if(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalAdapterAddresses(AdapterAddresses))
	{
		for(auto& Address:AdapterAddresses)
		{
			if(Address->IsValid())
			{
				IpAddrs.AddUnique(Address->ToString(false));
			}
		}
	}
	return IpAddrs;
}
