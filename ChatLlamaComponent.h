#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "ChatLlamaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLlamaResponse, const FString&, ResponseText);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEST_API UChatLlamaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UChatLlamaComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category = "ChatLlama")
    void SendPromptToLlama(const FString& Prompt);

    UPROPERTY(BlueprintAssignable, Category = "ChatLlama")
    FOnLlamaResponse OnLlamaResponseEvent;

private:
    void OnLlamaResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    FString RemoveEmojisFromString(const FString& Input);
};
