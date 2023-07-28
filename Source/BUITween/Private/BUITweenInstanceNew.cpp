#include "BUITweenInstanceNew.h"

#include "BUITween.h"

#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/UserWidget.h"


const char* const BUITweenComponents::BUIPostActionTransform::Name = "BUITweens::PostActionTransform";

static_assert(false, "Profile using insights");

void BUITweenComponents::BUISizeBoxWidthOverrideTween::Begin()
{
	if (Instance->WidgetPtr.IsValid())
	{
		if (USizeBox* SizeBox = Cast<USizeBox>(Instance->WidgetPtr.Get()))
		{
			Property.OnBegin(SizeBox->GetWidthOverride());
		}
	}
}

void BUITweenComponents::BUISizeBoxWidthOverrideTween::Apply()
{
	if (!Property.IsNeedToApply())
	{
		return;
	}

	if (Property.Update(Instance->EasedAlpha))
	{
		if (USizeBox* SizeBox = Cast<USizeBox>(Instance->WidgetPtr.Get()))
		{
			SizeBox->SetWidthOverride(Property.GetCurrentValue());
		}
	}
}
