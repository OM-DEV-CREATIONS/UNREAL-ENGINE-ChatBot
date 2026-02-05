#include "ChatLlamaComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "HttpManager.h"

UChatLlamaComponent::UChatLlamaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UChatLlamaComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UChatLlamaComponent::SendPromptToLlama(const FString& Prompt)
{
    // Create HTTP request
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://localhost:11434/api/generate"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build JSON payload
    FString Payload = FString::Printf(TEXT("{\"model\": \"gemma3:1b\", \"prompt\": \"%s\"}"), *Prompt);
    Request->SetContentAsString(Payload);
                                                                                  
    // Bind response
    Request->OnProcessRequestComplete().Unbind();
    Request->OnProcessRequestComplete().BindUObject(this, &UChatLlamaComponent::OnLlamaResponse);
    Request->ProcessRequest();

    FHttpModule::Get().GetHttpManager().Flush(EHttpFlushReason::FullFlush);
}

void UChatLlamaComponent::OnLlamaResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Llama request failed"));
        return;
    }

    FString ResponseStr = Response->GetContentAsString();

    // Parse JSON
    TArray<FString> Lines;
    ResponseStr.ParseIntoArrayLines(Lines);

    FString FullText;

    for (const FString& Line : Lines)
    {
        TSharedPtr<FJsonObject> JsonObject;
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Line), JsonObject) && JsonObject->HasField("response"))
        {
            FullText += JsonObject->GetStringField("response");
        }
    }

    FString CleanText = RemoveEmojisFromString(FullText);
    
    CleanText.ReplaceInline(TEXT("**"), TEXT(""));
    CleanText.ReplaceInline(TEXT("*"), TEXT(""));
    CleanText.ReplaceInline(TEXT("_"), TEXT(""));

    UE_LOG(LogTemp, Display, TEXT("Llama says: %s"), *CleanText);
    OnLlamaResponseEvent.Broadcast(CleanText);
}

static bool IsEmojiCodepoint(uint32 CodePoint)
{
    return
        (CodePoint >= 0x1F300 && CodePoint <= 0x1F5FF) || // Misc Symbols and Pictographs
        (CodePoint >= 0x1F600 && CodePoint <= 0x1F64F) || // Emoticons
        (CodePoint >= 0x1F680 && CodePoint <= 0x1F6FF) || // Transport & Map
        (CodePoint >= 0x1F700 && CodePoint <= 0x1F77F) || // Alchemical Symbols
        (CodePoint >= 0x1F780 && CodePoint <= 0x1F7FF) || // Geometric Ext
        (CodePoint >= 0x1F800 && CodePoint <= 0x1F8FF) || // Supplemental
        (CodePoint >= 0x1F900 && CodePoint <= 0x1F9FF) || // Supplemental Symbols & Pictographs
        (CodePoint >= 0x1FA00 && CodePoint <= 0x1FA6F) || // Symbols Extended-A
        (CodePoint >= 0x1FA70 && CodePoint <= 0x1FAFF) || // Symbols Extended-B
        (CodePoint >= 0x2600 && CodePoint <= 0x26FF) || // Misc symbols (some dingbats)
        (CodePoint >= 0x2700 && CodePoint <= 0x27BF) || // Dingbats
        (CodePoint >= 0xFE00 && CodePoint <= 0xFE0F) || // Variation Selectors
        (CodePoint >= 0x1F1E6 && CodePoint <= 0x1F1FF);    // Regional Indicator Symbols (flags)
}

// Remove emojis by decoding UTF-16 code units and rebuilding the string
FString UChatLlamaComponent::RemoveEmojisFromString(const FString& Input)
{
    const TCHAR* Ptr = *Input;
    int32 Len = Input.Len();
    FString Out;
    Out.Reserve(Len);

    for (int32 i = 0; i < Len; ++i)
    {
        uint32 CodePoint = (uint32)Ptr[i];

        if (CodePoint >= 0xD800 && CodePoint <= 0xDBFF && i + 1 < Len)
        {
            uint32 High = CodePoint;
            uint32 Low = (uint32)Ptr[i + 1];
            if (Low >= 0xDC00 && Low <= 0xDFFF)
            {
                uint32 U = ((High - 0xD800) << 10) + (Low - 0xDC00) + 0x10000;
                if (IsEmojiCodepoint(U))
                {
                    ++i;
                    continue;
                }
                Out.AppendChar((TCHAR)High);
                Out.AppendChar((TCHAR)Low);
                ++i;
                continue;
            }
        }

        if (!IsEmojiCodepoint(CodePoint))
        {
            Out.AppendChar((TCHAR)CodePoint);
        }
    }

    return Out;
}