#pragma once
#include "CoreMinimal.h"
#include "UObject/UObjectArray.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"

struct FScannerPackageTracker : public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
	FScannerPackageTracker()
	{
		for (TObjectIterator<UPackage> It; It; ++It)
		{
			UPackage* Package = *It;
		
			if (Package->GetOuter() == nullptr)
			{
				FScannerPackageTracker::OnPackageCreated(Package);
			}
		}

		GUObjectArray.AddUObjectDeleteListener(this);
		GUObjectArray.AddUObjectCreateListener(this);
	}

	virtual ~FScannerPackageTracker()
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}

	virtual void NotifyUObjectCreated(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if (Package->GetOuter() == nullptr && !Package->GetFName().IsNone())
			{	
				OnPackageCreated(Package);
			}
		}
	}

	virtual void NotifyUObjectDeleted(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if(!Package->GetFName().IsNone())
			{
				
				OnPackageDeleted(Package);
			}
		}
	}

	virtual void OnUObjectArrayShutdown()
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
	virtual void OnPackageCreated(UPackage* Package)
	{
		FString AssetPathName = *Package->GetPathName();
        if(!AssetPathName.StartsWith(TEXT("/Script/")))
        {
        	LoadedPackages.Add(*AssetPathName);
        }	
	};
	virtual void OnPackageDeleted(UPackage* Package){};
	virtual const TSet<FName>& GetLoadedPackageNames()const{ return LoadedPackages; }
	
protected:
	TSet<FName>		LoadedPackages;
};