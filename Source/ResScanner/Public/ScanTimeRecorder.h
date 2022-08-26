#pragma once

#include "CoreMinimal.h"

struct RESSCANNER_API FScanTimeRecorder
{
    FScanTimeRecorder(const FString& InDisplay=TEXT(""),bool InAuto = true);
    ~FScanTimeRecorder();
    void Begin(const FString& InDisplay);
    void End();
public:
    FDateTime BeginTime;
    FDateTime CurrentTime;
    FTimespan UsedTime;
    FString Display;
    bool bAuto = false;
};
