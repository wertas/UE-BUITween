#include "BUITween.h"

TArray< FBUITweenInstance > UBUITween::ActiveInstances = TArray< FBUITweenInstance >();
TArray< FBUITweenInstance > UBUITween::InstancesToAdd = TArray< FBUITweenInstance >();
bool UBUITween::bIsInitialized = false;

void UBUITween::Startup()
{
	bIsInitialized = true;
	ActiveInstances.Empty();
	InstancesToAdd.Empty();
}


void UBUITween::Shutdown()
{
	ActiveInstances.Empty();
	InstancesToAdd.Empty();
	bIsInitialized = false;
}


FBUITweenInstance& UBUITween::Create( UWidget* pInWidget, float InDuration, float InDelay, bool bIsAdditive )
{
	// By default let's kill any existing tweens
	if ( !bIsAdditive )
	{
		Clear( pInWidget );
	}

	FBUITweenInstance Instance( pInWidget, InDuration, InDelay );

	InstancesToAdd.Add( Instance );

	return InstancesToAdd.Last();
}


int32 UBUITween::Clear( UWidget* pInWidget )
{
	int32 NumRemoved = 0;

	auto DoesTweenMatchWidgetFn = [pInWidget](const FBUITweenInstance& CurTweenInstance) -> bool {
		return ( CurTweenInstance.GetWidget().IsValid() && CurTweenInstance.GetWidget() == pInWidget );
	};

	NumRemoved += ActiveInstances.RemoveAll(DoesTweenMatchWidgetFn);
	NumRemoved += InstancesToAdd.RemoveAll(DoesTweenMatchWidgetFn);

	return NumRemoved;
}


void UBUITween::Update( float DeltaTime )
{
	// Reverse it so we can remove
	for ( int32 i = ActiveInstances.Num()-1; i >= 0; --i )
	{
		FBUITweenInstance& Inst = ActiveInstances[ i ];
		Inst.Update( DeltaTime );
		if ( Inst.IsComplete() )
		{
			FBUITweenInstance CompleteInst = Inst;
			ActiveInstances.RemoveAtSwap( i );

			// We do this here outside of the instance update and after removing from active instances because we
			// don't know if the callback in the cleanup is going to trigger adding more events
			CompleteInst.DoCompleteCleanup();
		}
	}

	ActiveInstances.Append(MoveTemp(InstancesToAdd));
	InstancesToAdd.Empty();
}


bool UBUITween::GetIsTweening( UWidget* pInWidget )
{
	for ( int32 i = 0; i < ActiveInstances.Num(); ++i )
	{
		if ( ActiveInstances[ i ].GetWidget() == pInWidget )
		{
			return true;
		}
	}
	return false;
}


void UBUITween::CompleteAll()
{
	// Very hacky way to make sure all Tweens complete immediately.
	// First Update clears ActiveTweens, second clears "InstancesToAdd".
	Update( 100000 );
	Update( 100000 );
}

#define BUI_MAKE_INTERFACE_DEFINITION_OneParam(FunctionName, InType, OptionalModifier)											\
UBUIParamChain* UBUITweenBlueprintFunctionLibrary::FunctionName(UBUIParamChain* Previous, const InType##OptionalModifier Value)	\
{																																\
	check(Previous && Previous->TweenInstance);																					\
	Previous->TweenInstance->FunctionName(Value);																				\
	return Previous;																											\
}

#define BUI_MAKE_INTERFACE_DEFINITION_ZeroParam(FunctionName)								\
UBUIParamChain* UBUITweenBlueprintFunctionLibrary::FunctionName(UBUIParamChain* Previous)	\
{																							\
	check(Previous && Previous->TweenInstance);												\
	Previous->TweenInstance->FunctionName();												\
	return Previous;																		\
}

BUI_MAKE_INTERFACE_DEFINITION_OneParam(Easing, EBUIEasingType,);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(SetEasingOptionalParam, float,);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToTranslation, FVector2D, &);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromTranslation, FVector2D, &);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToScale, FVector2D, &);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromScale, FVector2D, &);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToOpacity, float,);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromOpacity, float,);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToColor, FLinearColor, &);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromColor, FLinearColor, &);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToRotation, float,);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromRotation, float,);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToSizeBoxMaxDesiredHeight, float,);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromSizeBoxMaxDesiredHeight, float,);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToSizeBoxWidthOverride, float,);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromSizeBoxWidthOverride, float,);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToCanvasPosition, FVector2D, &);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromCanvasPosition, FVector2D, &);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToPadding, FMargin, &);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromPadding, FMargin, &);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(ToVisibility, ESlateVisibility,);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(FromVisibility, ESlateVisibility,);

BUI_MAKE_INTERFACE_DEFINITION_OneParam(OnStart, FBUITweenBPSignature, &);
BUI_MAKE_INTERFACE_DEFINITION_OneParam(OnComplete, FBUITweenBPSignature, &);

BUI_MAKE_INTERFACE_DEFINITION_ZeroParam(ToReset);

#undef BUI_MAKE_INTERFACE_DEFINITION_OneParam
#undef BUI_MAKE_INTERFACE_DEFINITION_ZeroParam



#define BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS(Type, OptionalReference) \
void UBUITweenBlueprintFunctionLibrary::SelectConditional_##Type(const Type##OptionalReference InFirst, const Type##OptionalReference InSecond, const bool bIsForward, Type& OutFirst, Type& OutSecond) \
{									\
	if (bIsForward)					\
	{								\
		OutFirst = InFirst;			\
		OutSecond = InSecond;		\
	}								\
	else							\
	{								\
		Type Temp = InFirst; 		\
		OutFirst = InSecond;		\
		OutSecond = Temp;			\
	}								\
}

BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS(float,)
BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS(FVector2D, &)
BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS(FLinearColor, &)
BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS(FVector4, &)
BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS(ESlateVisibility,)

#undef BUI_CREATE_HELPER_CONDITIONAL_DEFINITIONS
