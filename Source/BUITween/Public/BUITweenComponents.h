#pragma once

#include "Components/Widget.h"
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


namespace BUITweenComponents
{

template<typename DerivedT, typename ValueT, typename PropertyT = TBUITweenProp<ValueT>>
struct BUITweenComponentBase
{
public:
	template<typename CallerT>
	DerivedT& From(CallerT&& InStart) { Property.SetStart(std::forward<CallerT>(InStart)); return static_cast<DerivedT&>(*this); }
	template<typename CallerT>
	DerivedT& To(CallerT&& InTarget) { Property.SetTarget(std::forward<CallerT>(InTarget)); return static_cast<DerivedT&>(*this); }

	void SetActive(const bool Value)
	{
		Property.SetNeedApply(Value);
	}

	void SetData(void* Data)
	{
		Instance = static_cast<FBUITweenInstance*>(Data);
	}

protected:

	template<typename ActionT>
	ActionT& GetTypedAction()
	{
		BUIPostActionsBase& Action = [this]() -> BUIPostActionsBase&
		{
			const auto TypeIndex = BUIDetails::TypeIndexer::GetTypeIndex<ActionT>();
			if (auto* ActionPtr = Instance->PostActionsMap.Find(TypeIndex))
			{
				return **ActionPtr;
			}
			return *Instance->PostActionsMap.Add(TypeIndex, MakeUnique<ActionT>());
		}();
		return static_cast<ActionT&>(Action);
	}
protected:

	friend FBUITweenInstance;

	BUITweenComponentBase() = default;

	PropertyT Property;
	FBUITweenInstance* Instance;
};

struct BUITranslationTween : BUITweenComponentBase<BUITranslationTween, FVector2D>
{
	void Begin();
	void Apply();
};

struct BUIScaleTween : BUITweenComponentBase<BUIScaleTween, FVector2D>
{
	void Begin();
	void Apply();
};

struct BUIRotationTween : BUITweenComponentBase<BUIRotationTween, float>
{
	void Begin();
	void Apply();
};

struct BUISizeBoxWidthOverrideTween : BUITweenComponentBase<BUISizeBoxWidthOverrideTween, float>
{
	void Begin();
	void Apply();
};



class BUIPostActionTransform final: public BUIPostActionsBase
{
public:
	BUIPostActionTransform() = default;

	BUIPostActionTransform(TWeakObjectPtr<UWidget> WidgetPtr)
		: BUIPostActionsBase(WidgetPtr)
	{}

	virtual void ApplyAction() noexcept override
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("BUIPostActionTransform::ApplyAction");
		if (WidgetPtr.IsValid())
		{
			WidgetPtr->SetRenderTransform(TransformToSet);
		}
	}

	FWidgetTransform& GetTransform(TWeakObjectPtr<UWidget> InWidget)
	{
		if (IsSet())
		{
			return TransformToSet;
		}
		SetWidget(InWidget);
		if (IsSet())
		{
			TransformToSet = WidgetPtr->GetRenderTransform();
		}
		return TransformToSet;
	}

private:
	FWidgetTransform TransformToSet;
};

} // namespace BUITweenComponents

/*
static const int bla = alignof(BUITweenComponents::BUITranslationTween);
static const int bla2 = sizeof(BUITweenComponents::BUITranslationTween);
char (*__kaboom1)[alignof(BUITweenComponents::BUITranslationTween)] = 1;
char (*__kaboom2)[sizeof(BUITweenComponents::BUITranslationTween)] = 1;
*/



// BUITranslationTween

void BUITweenComponents::BUITranslationTween::Begin()
{
	if (Instance->WidgetPtr.IsValid())
	{
		Property.OnBegin( Instance->WidgetPtr->GetRenderTransform().Translation );
	}
}

void BUITweenComponents::BUITranslationTween::Apply()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("BUITranslationTween::Apply");
	if (!Property.Update( Instance->EasedAlpha ))
	{
		return;
	}
	GetTypedAction<BUIPostActionTransform>().GetTransform(Instance->WidgetPtr).Translation = Property.GetCurrentValue();
}

// BUIScaleTween

void BUITweenComponents::BUIScaleTween::Begin()
{
	if (Instance->WidgetPtr.IsValid())
	{
		Property.OnBegin( Instance->WidgetPtr->GetRenderTransform().Scale );
	}
}

void BUITweenComponents::BUIScaleTween::Apply()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("BUITranslationTween::Apply");
	if (!Property.Update( Instance->EasedAlpha ))
	{
		return;
	}
	GetTypedAction<BUIPostActionTransform>().GetTransform(Instance->WidgetPtr).Scale = Property.GetCurrentValue();
}

// BUIRotationTween

void BUITweenComponents::BUIRotationTween::Begin()
{
	if (Instance->WidgetPtr.IsValid())
	{
		Property.OnBegin( Instance->WidgetPtr->GetRenderTransform().Angle );
	}
}

void BUITweenComponents::BUIRotationTween::Apply()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("BUITranslationTween::Apply");
	if (!Property.Update( Instance->EasedAlpha ))
	{
		return;
	}
	GetTypedAction<BUIPostActionTransform>().GetTransform(Instance->WidgetPtr).Angle = Property.GetCurrentValue();
}

// BUISizeBoxWidthOverrideTween

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