#pragma once

#include "BUIEasing.h"
#include "Components/Widget.h"
#include "BUITweenInstance.h"
#include "BUITween.generated.h"

UCLASS()
class BUITWEEN_API UBUITween : public UObject
{
	GENERATED_BODY()

public:
	static void Startup();
	static void Shutdown();

	// Create a new tween on the target widget, does not start automatically
	static FBUITweenInstance& Create( UWidget* pInWidget, float InDuration = 1.0f, float InDelay = 0.0f, bool bIsAdditive = false );

	// Cancel all tweens on the target widget, returns the number of tween instances removed
	static int32 Clear( UWidget* pInWidget );

	static void Update( float InDeltaTime );

	static bool GetIsTweening( UWidget* pInWidget );

	static void CompleteAll();

protected:
	static bool bIsInitialized;

	static TArray< FBUITweenInstance > ActiveInstances;

	// We delay adding until the end of an update so we don't add to ActiveInstances within our update loop
	static TArray< FBUITweenInstance > InstancesToAdd;
};


// Helper object to pass parameter objects in Blueprints
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

	/**
	* Run BUI Tween Animation with given parameters.
	*
	* @param Params		Parameters created by CreateTweenAnimationParams and modified by To* From* On* functions
	*/
	UFUNCTION(BlueprintCallable, Category = UITween)
	static void RunTweenAnimation(UBUIParamChain* Params)
	{
		if (!Params)
		{
			UE_LOG(LogBUITween, Error, TEXT("RunTweenAnimation: Params not provided"));
			return;
		}
		Params->TweenInstance->Begin();
		Params->ConditionalBeginDestroy();
	}

	/**
	* Create BUI Tween Animation parameters.
	* Use To* From* On* on output object to modify paramerers as needed. 
	* Remember to always match From* parameters to To* ones.
	* 
	* Be sure to destroy return object (It happens in RunTweenAnimation) or you'll have memory leak
	*
	* @param InWidget		Widget to animate
	* @param InDuration		Animation duration
	* @param InDelay		Delay before animation start
	* @param bIsAdditive	if true, cancel previous running animation on the same widget
	* 
	* @return Object with parameters
	*/
	UFUNCTION(BlueprintPure, Category = UITween)
	static UBUIParamChain* CreateTweenAnimationParams(UWidget* InWidget, const float InDuration = 1.0f, const float InDelay = 0.0f, const bool bIsAdditive = false)
	{
		FBUITweenInstance& Tween = UBUITween::Create(InWidget, InDuration, InDelay, bIsAdditive);
		UBUIParamChain* Params = NewObject<UBUIParamChain>(GetTransientPackage(), TEXT(""), RF_MarkAsRootSet);
		Params->TweenInstance = &Tween;
		return Params;
	}

	// Helper functions to reduce boilerplate on forward - reverse animations
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_float(			  const float				InFirst, const float			InSecond, const bool bIsForward, float&				OutFirst, float&			OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_FVector2D(		  const FVector2D&			InFirst, const FVector2D&		InSecond, const bool bIsForward, FVector2D&			OutFirst, FVector2D&		OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_FLinearColor(	  const FLinearColor&		InFirst, const FLinearColor&	InSecond, const bool bIsForward, FLinearColor&		OutFirst, FLinearColor&		OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_FVector4(		  const FVector4&			InFirst, const FVector4&		InSecond, const bool bIsForward, FVector4&			OutFirst, FVector4&			OutSecond);
	UFUNCTION(BlueprintPure, Category = "UITween|Helpers") static void SelectConditional_ESlateVisibility(const ESlateVisibility	InFirst, const ESlateVisibility	InSecond, const bool bIsForward, ESlateVisibility&	OutFirst, ESlateVisibility&	OutSecond);

	// FBUITweenInstance interface

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* Easing(UBUIParamChain* Previous, const EBUIEasingType InType);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* SetEasingOptionalParam(UBUIParamChain* Previous, const float OptionalParam);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToTranslation(UBUIParamChain* Previous, const FVector2D& InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromTranslation(UBUIParamChain* Previous, const FVector2D& InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToScale(UBUIParamChain* Previous, const FVector2D& InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromScale(UBUIParamChain* Previous, const FVector2D& InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToOpacity(UBUIParamChain* Previous, const float InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromOpacity(UBUIParamChain* Previous, const float InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToColor(UBUIParamChain* Previous, const FLinearColor& InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromColor(UBUIParamChain* Previous, const FLinearColor& InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToRotation(UBUIParamChain* Previous, const float InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromRotation(UBUIParamChain* Previous, const float InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToSizeBoxMaxDesiredHeight(UBUIParamChain* Previous, const float InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromSizeBoxMaxDesiredHeight(UBUIParamChain* Previous, const float InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToSizeBoxWidthOverride(UBUIParamChain* Previous, const float InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromSizeBoxWidthOverride(UBUIParamChain* Previous, const float InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToCanvasPosition(UBUIParamChain* Previous, const FVector2D& InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromCanvasPosition(UBUIParamChain* Previous, const FVector2D& InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToPadding(UBUIParamChain* Previous, const FMargin& InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromPadding(UBUIParamChain* Previous, const FMargin& InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToVisibility(UBUIParamChain* Previous, const ESlateVisibility InTarget);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* FromVisibility(UBUIParamChain* Previous, const ESlateVisibility InStart);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* OnStart(UBUIParamChain* Previous, const FBUITweenBPSignature& InOnStart);
	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* OnComplete(UBUIParamChain* Previous, const FBUITweenBPSignature& InOnComplete);

	UFUNCTION(BlueprintPure, Category = UITween) static UBUIParamChain* ToReset(UBUIParamChain* Previous);

private:

};
