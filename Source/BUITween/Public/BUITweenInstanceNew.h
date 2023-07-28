#pragma once

#include "BUIEasing.h"
#include "Components/Widget.h"
#include "BUITweenInstance.h" // Temporary, remove after deprecation removal

#include "BUIPoolManager.h"
#include "BUITween.h"


struct BUITWEEN_API FBUITweenInstance2
{

public:
	FBUITweenInstance2() = delete;

	FBUITweenInstance2(const FBUITweenInstance2&) = delete;
	FBUITweenInstance2& operator=(const FBUITweenInstance2&) = delete;

	FBUITweenInstance2(FBUITweenInstance2&& Other)
		: PostActionsMap(MoveTemp(Other.PostActionsMap)),
		bShouldUpdateInstance(Other.bShouldUpdateInstance),
		bShouldUpdateComponents(Other.bShouldUpdateComponents),
		bIsComplete(Other.bIsComplete),
		bHasPlayedStartEvent(Other.bHasPlayedStartEvent),
		bHasPlayedCompleteEvent(Other.bHasPlayedCompleteEvent),
		bCleanedUp(Other.bCleanedUp),
		EasingType(Other.EasingType),
		WidgetPtr(Other.WidgetPtr),
		Delay(Other.Delay),
		EasedAlpha(Other.EasedAlpha),
		Alpha(Other.Alpha),
		Duration(Other.Duration),
		Entity(Other.Entity),
		EasingParam(Other.EasingParam),
		OnStartedDelegate(Other.OnStartedDelegate),
		OnCompleteDelegate(Other.OnCompleteDelegate),
		OnStartedBPDelegate(Other.OnStartedBPDelegate),
		OnCompleteBPDelegate(Other.OnCompleteBPDelegate)
	{
		UBUITween::GetPoolManager().SetEntityData(Entity, this);
	}

	FBUITweenInstance2& operator=(FBUITweenInstance2&&) = default;

	FBUITweenInstance2(UWidget* Widget, float InDuration, float InDelay = 0.f)
		: WidgetPtr(Widget),
		Delay(InDelay),
		Duration(InDuration),
		Entity(UBUITween::GetPoolManager().GetEntityHandle())
	{
		ensure(Widget != nullptr);
	}

	void Begin()
	{
		bShouldUpdateInstance = true;
		SetShouldUpdateComponents(true);

		UBUITween::GetPoolManager().EntityBegin(Entity);
		UBUITween::GetPoolManager().Apply(Entity);
	}

	void SetShouldUpdateComponents(const bool NewValue)
	{
		if (bShouldUpdateComponents != NewValue)
		{
			bShouldUpdateComponents = NewValue;
			UBUITween::GetPoolManager().SetEntityActive(Entity, NewValue);
		}
	}

	void PreComponentsUpdate(float InDeltaTime)
	{
		// Not called Begin() yet
		if ( !bShouldUpdateInstance && !bIsComplete )
		{
			return;
		}
		// Widget became invalid - no things to do
		if ( !WidgetPtr.IsValid() )
		{
			bIsComplete = true;
			SetShouldUpdateComponents(false);
			return;
		}
		// If completed but occasionally called again
		if (bIsComplete)
		{
			ensure(false);
			SetShouldUpdateComponents(false);
			return;
		}

		if ( Delay > 0 )
		{
			// TODO could correctly subtract from deltatime and use remaining on alpha but meh
			Delay -= InDeltaTime;
			SetShouldUpdateComponents(false);
			return;
		}
		else if (bShouldUpdateComponents == false && Delay <= 0)
		{
			SetShouldUpdateComponents(true);
		}

		if ( !bHasPlayedStartEvent )
		{
			OnStartedDelegate.ExecuteIfBound( WidgetPtr.Get() );
			OnStartedBPDelegate.ExecuteIfBound( WidgetPtr.Get() );
			bHasPlayedStartEvent = true;
		}

		Alpha += InDeltaTime;
		if ( Alpha >= Duration )
		{
			Alpha = Duration;
			bIsComplete = true;
		}

		EasedAlpha = EasingParam.IsSet()
			? FBUIEasing::Ease( EasingType, Alpha, Duration, EasingParam.GetValue() )
			: FBUIEasing::Ease( EasingType, Alpha, Duration );

		return;
	}

	void PostComponentsUpdate()
	{
		for (const auto& Action : PostActionsMap)
		{
			Action.Value->ApplyAction();
		}
	}

	inline bool operator==( const FBUITweenInstance2& other) const
	{
		return WidgetPtr == other.WidgetPtr;
	}
	bool IsComplete() const { return bIsComplete; }

	// EasingParam is used for easing functions that have a second parameter, like Elastic
	FBUITweenInstance2& Easing( EBUIEasingType InType, TOptional<float> InEasingParam = TOptional<float>() )
	{
		EasingType = InType;
		EasingParam = InEasingParam;
		return *this;
	}

	template<typename TweenComponent>
	FBUITweenInstance2& SetTween(TweenComponent&& Component)
	{
		ensure(!bCleanedUp);
		Component.Instance = this;
		UBUITween::GetPoolManager().Add(Entity, std::forward<TweenComponent>(Component));
		return *this;
	}

	FBUITweenInstance2& SetOnStartDelegate( const FBUITweenSignature& InOnStart )
	{
		OnStartedDelegate = InOnStart;
		return *this;
	}

	FBUITweenInstance2& SetOnCompleteDelegate(const FBUITweenSignature& InOnComplete)
	{
		OnCompleteDelegate = InOnComplete;
		return *this;
	}

	FBUITweenInstance2& SetOnStartDelegate( const FBUITweenBPSignature& InOnStart )
	{
		OnStartedBPDelegate = InOnStart;
		return *this;
	}
	FBUITweenInstance2& SetOnCompleteDelegate( const FBUITweenBPSignature& InOnComplete )
	{
		OnCompleteBPDelegate = InOnComplete;
		return *this;
	}

	//FBUITweenInstance2& ToReset()
	//{
	//	ScaleProp.SetTarget( FVector2D::UnitVector );
	//	OpacityProp.SetTarget( 1 );
	//	TranslationProp.SetTarget( FVector2D::ZeroVector );
	//	ColorProp.SetTarget( FLinearColor::White );
	//	RotationProp.SetTarget( 0 );
	//	return *this;
	//}

	TWeakObjectPtr<UWidget> GetWidget() const { return WidgetPtr; }

	void DoCompleteCleanup()
	{
		if ( !bHasPlayedCompleteEvent )
		{
			OnCompleteDelegate.ExecuteIfBound( WidgetPtr.Get() );
			OnCompleteBPDelegate.ExecuteIfBound( WidgetPtr.Get() );
			bHasPlayedCompleteEvent = true;
		}
		UBUITween::GetPoolManager().RemoveEntity(Entity);
		bCleanedUp = true;
	}

	struct PostActionsBase
	{
		virtual ~PostActionsBase() = default;
		virtual void ApplyAction() = 0;
	};

public:
	TMap<const char*, TUniquePtr<PostActionsBase>> PostActionsMap;

	bool bShouldUpdateInstance = false;
	bool bShouldUpdateComponents = false;
	bool bIsComplete = false;
	bool bHasPlayedStartEvent = false;
	bool bHasPlayedCompleteEvent = false;

private:
	bool bCleanedUp = false;

public:

	EBUIEasingType EasingType = EBUIEasingType::InOutQuad;

	TWeakObjectPtr<UWidget> WidgetPtr = nullptr;

	float Delay = 0.f;
	float EasedAlpha = 0.f;
	float Alpha = 0.f;
	float Duration = 1.f;
	FBUIPoolManager::EntityHandle Entity;
	TOptional<float> EasingParam;


	FBUITweenSignature OnStartedDelegate;
	FBUITweenSignature OnCompleteDelegate;

	FBUITweenBPSignature OnStartedBPDelegate;
	FBUITweenBPSignature OnCompleteBPDelegate;
};




namespace BUITweenComponents
{

struct BUIPostActionTransform: FBUITweenInstance2::PostActionsBase
{
	static const char* const Name;

	BUIPostActionTransform(TWeakObjectPtr<UWidget> WidgetPtr)
		: WidgetPtr(WidgetPtr)
	{}

	virtual void ApplyAction() override
	{
		if (WidgetPtr.IsValid())
		{
			WidgetPtr->SetRenderTransform(CurrentTransform);
		}
	}

	TWeakObjectPtr<UWidget> WidgetPtr;
	FWidgetTransform CurrentTransform;
};



template<typename DerivedT, typename ValueT, typename PropertyT = TBUITweenProp<ValueT>>
struct BUITweenComponentBase
{
public:

	DerivedT& From(const ValueT& InStart) { Property.SetStart(InStart); return static_cast<DerivedT&>(*this); }
	DerivedT& To(const ValueT& InTarget) { Property.SetTarget(InTarget); return static_cast<DerivedT&>(*this); }

	void SetActive(const bool Value)
	{
		Property.SetNeedApply(Value);
	}

	void SetData(void* Data)
	{
		Instance = static_cast<FBUITweenInstance2*>(Data);
	}

protected:

	template<typename ActionT>
	ActionT& GetTypedAction()
	{
		FBUITweenInstance2::PostActionsBase& Action = [this]() -> FBUITweenInstance2::PostActionsBase&
		{
			if (auto* ActionPtr = Instance->PostActionsMap.Find(ActionT::Name))
			{
				return **ActionPtr;
			}
			return *Instance->PostActionsMap.Add(ActionT::Name, MakeUnique<ActionT>(Instance->WidgetPtr));
		}();
		return static_cast<ActionT&>(Action);
	}
protected:

	friend FBUITweenInstance2;

	BUITweenComponentBase() = default;

	PropertyT Property;
	FBUITweenInstance2* Instance;
};



struct BUITranslationTween : BUITweenComponentBase<BUITranslationTween, FVector2D>
{
	void Begin()
	{
		if (Instance->WidgetPtr.IsValid())
		{
			Property.OnBegin( Instance->WidgetPtr->GetRenderTransform().Translation );
		}
	}

	void Apply()
	{
		if (!Property.IsNeedToApply())
		{
			return;
		}

		const bool bDummy = Property.Update( Instance->EasedAlpha );
		GetTypedAction<BUIPostActionTransform>().CurrentTransform.Translation = Property.GetCurrentValue();
	}
};


struct BUISizeBoxWidthOverrideTween : BUITweenComponentBase<BUISizeBoxWidthOverrideTween, float>
{
	void Begin();
	void Apply();
};



} // namespace BUITweenComponents

/*
static const int bla = alignof(BUITweenComponents::BUITranslationTween);
static const int bla2 = sizeof(BUITweenComponents::BUITranslationTween);
char (*__kaboom1)[alignof(BUITweenComponents::BUITranslationTween)] = 1;
char (*__kaboom2)[sizeof(BUITweenComponents::BUITranslationTween)] = 1;
*/
