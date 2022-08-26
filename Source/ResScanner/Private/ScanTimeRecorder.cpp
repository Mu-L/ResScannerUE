#include "ScanTimeRecorder.h"
#include "ResScannerProxy.h"

FScanTimeRecorder::FScanTimeRecorder(const FString& InDisplay, bool InAuto):Display(InDisplay),bAuto(InAuto)
{
	if(bAuto)
	{
		Begin(InDisplay);
	}
        
}

FScanTimeRecorder::~FScanTimeRecorder()
{
	if(bAuto)
	{
		End();
	}
}

void FScanTimeRecorder::Begin(const FString& InDisplay)
{
	Display = InDisplay;
	BeginTime = FDateTime::Now();
}

void FScanTimeRecorder::End()
{
	CurrentTime = FDateTime::Now();
	UsedTime = CurrentTime-BeginTime;
	UE_LOG(LogResScannerProxy,Display,TEXT("----Time Recorder----: %s %s"),*Display,*UsedTime.ToString());
}
