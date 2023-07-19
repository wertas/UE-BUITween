#include "BUITweenInstance.h"

#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogBUITween);

void FBUITweenInstance::Begin()
{
	bShouldUpdate = true;
	bHasPlayedStartEvent = false;
	bHasPlayedCompleteEvent = false;

	if ( !pWidget.IsValid() )
	{
		UE_LOG( LogBUITween, Warning, TEXT( "Trying to start invalid widget" ) );
		return;
	}

	// Set all the props to the existng state
	TranslationProp.OnBegin( pWidget->GetRenderTransform().Translation );
	ScaleProp.OnBegin( pWidget->GetRenderTransform().Scale );
	RotationProp.OnBegin( pWidget->GetRenderTransform().Angle );
	OpacityProp.OnBegin( pWidget->GetRenderOpacity() );
	VisibilityProp.OnBegin( pWidget->GetVisibility() );

	if ( UUserWidget* UW = Cast<UUserWidget>( pWidget ) )
	{
		ColorProp.OnBegin( UW->GetColorAndOpacity() );
	}
	else if ( UImage* UI = Cast<UImage>( pWidget ) )
	{
		ColorProp.OnBegin( UI->GetColorAndOpacity() );
	}
	else if ( UBorder* Border = Cast<UBorder>( pWidget ) )
	{
		ColorProp.OnBegin( Border->GetContentColorAndOpacity() );
	}
	else
	{
		UE_LOG(LogBUITween, Error, TEXT("FBUITweenInstance::Begin: Wrong setup or unimplemented, ColorProp in %s"), *pWidget->GetName());
		ColorProp.Unset();
	}

	if ( UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>( pWidget->Slot ) )
	{
		CanvasPositionProp.OnBegin( CanvasSlot->GetPosition() );
	}

	const auto GetPaddingAsVector4 = [](auto Slot) -> FVector4
	{
		return FVector4(
			Slot->GetPadding().Left,
			Slot->GetPadding().Top,
			Slot->GetPadding().Bottom,
			Slot->GetPadding().Right );
	};

	if ( UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>( pWidget->Slot ) )
	{
		PaddingProp.OnBegin( GetPaddingAsVector4( OverlaySlot ) );
	}
	else if ( UHorizontalBoxSlot* HorizontalBoxSlot = Cast<UHorizontalBoxSlot>( pWidget->Slot ) )
	{
		PaddingProp.OnBegin( GetPaddingAsVector4( HorizontalBoxSlot ) );
	}
	else if ( UVerticalBoxSlot* VerticalBoxSlot = Cast<UVerticalBoxSlot>( pWidget->Slot ) )
	{
		PaddingProp.OnBegin( GetPaddingAsVector4( VerticalBoxSlot ) );
	}
	else
	{
		UE_LOG(LogBUITween, Error, TEXT("FBUITweenInstance::Begin: Wrong setup or unimplemented, PaddingProp in %s"), *pWidget->GetName());
		PaddingProp.Unset();
	}

	if ( USizeBox* SizeBox = Cast<USizeBox>( pWidget ) )
	{
		MaxDesiredHeightProp.OnBegin( SizeBox->GetMaxDesiredHeight() );
		WidthOverrideProp.OnBegin( SizeBox->GetWidthOverride() );
	}

	// Apply the starting conditions, even if we delay
	Apply( 0 );
}

void FBUITweenInstance::Update( float DeltaTime )
{
	if ( !bShouldUpdate && !bIsComplete )
	{
		return;
	}
	if ( !pWidget.IsValid() )
	{
		bIsComplete = true;
		return;
	}

	if ( Delay > 0 )
	{
		// TODO could correctly subtract from deltatime and use rmaining on alpha but meh
		Delay -= DeltaTime;
		return;
	}

	if ( !bHasPlayedStartEvent )
	{
		OnStartedDelegate.ExecuteIfBound( pWidget.Get() );
		OnStartedBPDelegate.ExecuteIfBound( pWidget.Get() );
		bHasPlayedStartEvent = true;
	}

	// Tween each thingy
	Alpha += DeltaTime;
	if ( Alpha >= Duration )
	{
		Alpha = Duration;
		bIsComplete = true;
	}

	const float EasedAlpha = EasingParam.IsSet()
		? FBUIEasing::Ease( EasingType, Alpha, Duration, EasingParam.GetValue() )
		: FBUIEasing::Ease( EasingType, Alpha, Duration );

	Apply( EasedAlpha );
}

void FBUITweenInstance::Apply( float EasedAlpha )
{
	UWidget* Target = pWidget.Get();

	if ( ColorProp.IsSet() && ColorProp.Update( EasedAlpha ) )
	{
		if ( UUserWidget* UW = Cast<UUserWidget>( Target ) )
		{
			UW->SetColorAndOpacity( ColorProp.CurrentValue );
		}
		else if ( UImage* UI = Cast<UImage>( Target ) )
		{
			UI->SetColorAndOpacity( ColorProp.CurrentValue );
		}
		else if ( UBorder* Border = Cast<UBorder>( Target ) )
		{
			Border->SetContentColorAndOpacity( ColorProp.CurrentValue );
		}
	}

	if ( OpacityProp.IsSet() && OpacityProp.Update( EasedAlpha ) )
	{
		Target->SetRenderOpacity( OpacityProp.CurrentValue );
	}

	// Only apply visibility changes at 0 or 1
	if ( VisibilityProp.IsSet() && VisibilityProp.Update( EasedAlpha ) )
	{
		Target->SetVisibility( VisibilityProp.CurrentValue );
	}

	bool bChangedRenderTransform = false;
	FWidgetTransform CurrentTransform = Target->GetRenderTransform();

	if ( TranslationProp.IsSet() && TranslationProp.Update( EasedAlpha ) )
	{
		CurrentTransform.Translation = TranslationProp.CurrentValue;
		bChangedRenderTransform = true;
	}

	if ( ScaleProp.IsSet() && ScaleProp.Update( EasedAlpha ) )
	{
		CurrentTransform.Scale = ScaleProp.CurrentValue;
		bChangedRenderTransform = true;
	}

	if ( RotationProp.IsSet() && RotationProp.Update( EasedAlpha ) )
	{
		CurrentTransform.Angle = RotationProp.CurrentValue;
		bChangedRenderTransform = true;
	}

	if ( CanvasPositionProp.IsSet() && CanvasPositionProp.Update( EasedAlpha ) )
	{
		if ( UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>( pWidget->Slot ) )
		{
			CanvasSlot->SetPosition( CanvasPositionProp.CurrentValue );
		}
	}

	if ( PaddingProp.IsSet() && PaddingProp.Update( EasedAlpha ) )
	{
		if ( UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(pWidget->Slot) )
		{
			OverlaySlot->SetPadding( PaddingProp.CurrentValue );
		}
		else if ( UHorizontalBoxSlot* HorizontalBoxSlot = Cast<UHorizontalBoxSlot>(pWidget->Slot) )
		{
			HorizontalBoxSlot->SetPadding( PaddingProp.CurrentValue );
		}
		else if ( UVerticalBoxSlot* VerticalBoxSlot = Cast<UVerticalBoxSlot>(pWidget->Slot) )
		{
			VerticalBoxSlot->SetPadding( PaddingProp.CurrentValue );
		}
	}

	if ( MaxDesiredHeightProp.IsSet() && MaxDesiredHeightProp.Update( EasedAlpha ) )
	{
		if ( USizeBox* SizeBox = Cast<USizeBox>( pWidget ) )
		{
			SizeBox->SetMaxDesiredHeight( MaxDesiredHeightProp.CurrentValue );
		}
	}

	if ( WidthOverrideProp.IsSet() && WidthOverrideProp.Update( EasedAlpha ) )
	{
		if ( USizeBox* SizeBox = Cast<USizeBox>( pWidget ) )
		{
			SizeBox->SetWidthOverride( WidthOverrideProp.CurrentValue );
		}
	}

	if ( bChangedRenderTransform )
	{
		Target->SetRenderTransform( CurrentTransform );
	}
}
