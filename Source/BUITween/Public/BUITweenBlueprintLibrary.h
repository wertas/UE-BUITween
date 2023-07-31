#pragma once

#include "BUITween.h"
#include "BUITweenInstance.h"
#include "BUITweenComponents.h"
#include "BUITweenBlueprintLibrary.generated.h"


UCLASS(BlueprintType)
class BUITWEEN_API UBUIParamChain : public UObject
{
	GENERATED_BODY()

public:

	FBUITweenInstance* TweenInstance;
};


UCLASS()
class BUITWEEN_API UBUITweenBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = UITween)
	static void RunTweenAnimation(UBUIParamChain* Params)
	{
		Params->TweenInstance->Begin();
		Params->ConditionalBeginDestroy();
	}

	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* CreateTweenAnimationParams(UWidget* InWidget, const float InDuration = 1.0f, const float InDelay = 0.0f, const bool bIsAdditive = false)
	{
		FBUITweenInstance& Tween = UBUITween::Create(InWidget, InDuration, InDelay, bIsAdditive);
		UBUIParamChain* Params = NewObject<UBUIParamChain>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		Params->TweenInstance = &Tween;
		return Params;
	}

	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* SetTranslationTween(UBUIParamChain* Previous, const FVector2D& InStart, const FVector2D& InTarget)
	{
		Previous->TweenInstance->SetTween(BUITweenComponents::BUITranslationTween().From(InStart).To(InTarget));
		return Previous;
	}

	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* SetRotationTween(UBUIParamChain* Previous, const float InStart, const float InTarget)
	{
		Previous->TweenInstance->SetTween(BUITweenComponents::BUIRotationTween().From(InStart).To(InTarget));
		return Previous;
	}

	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* SetScaleTween(UBUIParamChain* Previous, const FVector2D& InStart, const FVector2D& InTarget)
	{
		Previous->TweenInstance->SetTween(BUITweenComponents::BUIScaleTween().From(InStart).To(InTarget));
		return Previous;
	}

	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* SetSizeBoxWidthOverrideTween(UBUIParamChain* Previous, const float InStart, const float InTarget)
	{
		Previous->TweenInstance->SetTween(BUITweenComponents::BUISizeBoxWidthOverrideTween().From(InStart).To(InTarget));
		return Previous;
	}






	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* SetOnStartDelegate(UBUIParamChain* Previous, const FBUITweenBPSignature& InOnStart)
	{
		Previous->TweenInstance->SetOnStartDelegate(InOnStart);
		return Previous;
	}

	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* SetOnCompleteDelegate(UBUIParamChain* Previous, const FBUITweenBPSignature& InOnComplete)
	{
		Previous->TweenInstance->SetOnCompleteDelegate(InOnComplete);
		return Previous;
	}

	// Helper functions to reduce boilerplate on forward - reverse animations
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_float(				const float				InFirst, const float			InSecond, const bool bIsForward, float&				OutFirst, float&			OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_FVector2D(			const FVector2D&		InFirst, const FVector2D&		InSecond, const bool bIsForward, FVector2D&			OutFirst, FVector2D&		OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_FLinearColor(		const FLinearColor&		InFirst, const FLinearColor&	InSecond, const bool bIsForward, FLinearColor&		OutFirst, FLinearColor&		OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_FVector4(			const FVector4&			InFirst, const FVector4&		InSecond, const bool bIsForward, FVector4&			OutFirst, FVector4&			OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_ESlateVisibility(	const ESlateVisibility	InFirst, const ESlateVisibility	InSecond, const bool bIsForward, ESlateVisibility&	OutFirst, ESlateVisibility&	OutSecond);

private:

};
