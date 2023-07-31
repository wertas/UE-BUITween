#pragma once

#include "Delegates/Delegate.h"
#include "BUIEasing.h"
#include "Components/Widget.h"

#include "BUIPoolManager.h"
#include "BUITween.h"
#include "BUITweenInstance.generated.h"


BUITWEEN_API DECLARE_LOG_CATEGORY_EXTERN(LogBUITween, Log, All);

DECLARE_DELEGATE_OneParam(FBUITweenSignature, UWidget* /*Owner*/);
UDELEGATE() DECLARE_DYNAMIC_DELEGATE_OneParam(FBUITweenBPSignature, UWidget*, Owner);


template<typename T>
class TBUITweenProp
{
public:

	inline bool IsSet() const 
	{
		ensure(false); // todo: add checks on this, or remove this
		return bHasStart || bHasTarget;
	}

	template<typename CallerT>
	void SetStart( CallerT&& InStart )
	{
		static_assert(std::is_same_v<std::decay_t<CallerT>, T>);
		bHasStart = true;
		StartValue = std::forward<CallerT>(InStart);
	}

	template<typename CallerT>
	void SetTarget( CallerT&& InTarget )
	{
		static_assert(std::is_same_v<std::decay_t<CallerT>, T>);
		bHasTarget = true;
		TargetValue = std::forward<CallerT>(InTarget);
	}

	bool IsNeedToApply()
	{
		return bNeedApply;
	}

	void SetNeedApply(const bool Value)
	{
		bNeedApply = Value;
	}

	void OnBegin( T InCurrentValue )
	{
		if ( !bHasStart )
		{
			StartValue = InCurrentValue;
		}
	}

	bool IsFirstTime() const
	{
		return CurrentAlpha == 0.f;
	}

	T GetCurrentValue() const
	{
		return FMath::Lerp<T>( StartValue, TargetValue, CurrentAlpha );
	}

	[[nodiscard]]
	bool Update( float Alpha )
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("TBUITweenProp::Update");

		if (!IsSet() || !bNeedApply)
		{
			return false;
		}

		const bool bShouldUpdateWidget = IsFirstTime() || !FMath::IsNearlyEqual(Alpha, CurrentAlpha, KINDA_SMALL_NUMBER);

		//UE_LOG(LogBUITween, Warning, TEXT("Alpha: %f"), Alpha);
		if (bShouldUpdateWidget)
		{
			CurrentAlpha = Alpha;
		}
		return bShouldUpdateWidget;
	}

private:

	T StartValue;
	T TargetValue;

	float CurrentAlpha = 0.f;

	bool bNeedApply = false; // Used in Tween component
	bool bHasStart = false;
	bool bHasTarget = false;
};

template<typename T>
class TBUITweenInstantProp
{
public:
	bool bNeedUpdate = false; // Todo: this was added, but not implemented
	bool bHasStart = false;
	bool bHasTarget = false;
	bool bIsFirstTime = true;
	inline bool IsSet() const { return bHasStart || bHasTarget; }
	T StartValue;
	T TargetValue;
	T CurrentValue;
	void SetStart( T InStart )
	{
		bHasStart = true;
		StartValue = InStart;
		CurrentValue = StartValue;
	}
	void SetTarget( T InTarget )
	{
		bHasTarget = true;
		TargetValue = InTarget;
	}
	void OnBegin( T InCurrentValue )
	{
		if ( !bHasStart )
		{
			StartValue = InCurrentValue;
			CurrentValue = InCurrentValue;
		}
	}
	bool Update( float Alpha )
	{
		check(false); // todo: rewrite TBUITweenInstantProp
		const T OldValue = CurrentValue;
		if ( Alpha >= 1 && bHasTarget )
		{
			CurrentValue = TargetValue;
		}
		else
		{
			CurrentValue = StartValue;
		}
		const bool bShouldChange = bIsFirstTime || OldValue != CurrentValue;
		bIsFirstTime = false;
		return bShouldChange;
	}
};

class BUIPostActionsBase
{
public:

	BUIPostActionsBase()
		: WidgetPtr(nullptr),
		  bIsValid(false)
	{}

	BUIPostActionsBase(TWeakObjectPtr<UWidget> InWidget)
		: WidgetPtr(InWidget),
		  bIsValid(WidgetPtr.IsValid())
	{}

	virtual ~BUIPostActionsBase() = default;

	virtual void ApplyAction() noexcept = 0;


	void SetWidget(TWeakObjectPtr<UWidget> InWidget)
	{
		WidgetPtr = InWidget;
		bIsValid = InWidget.IsValid();
	}

	// Rely on this with caution
	bool IsSet() const
	{
		return bIsValid;
	}

	void Reset()
	{
		WidgetPtr.Reset();
		bIsValid = false;
	}
protected:
	TWeakObjectPtr<UWidget> WidgetPtr;

private:
	bool bIsValid;
};




struct BUITWEEN_API FBUITweenInstance
{

public:
	FBUITweenInstance() = delete;
	~FBUITweenInstance()
	{
		if (Entity != FBUIPoolManager::INVALID_ENTITY)
		{
			UBUITween::GetPoolManager().RemoveEntity(Entity);
		}
	}


	FBUITweenInstance(const FBUITweenInstance&) = delete;
	FBUITweenInstance& operator=(const FBUITweenInstance&) = delete;

	FBUITweenInstance(FBUITweenInstance&& Other)
		: PostActionsMap(MoveTemp(Other.PostActionsMap)),
		bShouldUpdateInstance(Other.bShouldUpdateInstance),
		bShouldUpdateComponents(Other.bShouldUpdateComponents),
		bIsComplete(Other.bIsComplete),
		bHasPlayedStartEvent(Other.bHasPlayedStartEvent),
		bHasPlayedCompleteEvent(Other.bHasPlayedCompleteEvent),
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
		Other.Entity = FBUIPoolManager::INVALID_ENTITY;
	}

	FBUITweenInstance& operator=(FBUITweenInstance&&) = default;

	FBUITweenInstance(UWidget* Widget, float InDuration, float InDelay = 0.f)
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
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("FBUITweenInstance::PreComponentsUpdate");
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
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("FBUITweenInstance::PostComponentsUpdate");
		for (auto& Action : PostActionsMap)
		{
			TUniquePtr<BUIPostActionsBase>& PostAction = Action.Value;
			if (PostAction->IsSet())
			{
				Action.Value->ApplyAction();
				Action.Value->Reset();
			}
		}
	}

	inline bool operator==( const FBUITweenInstance& other) const
	{
		return WidgetPtr == other.WidgetPtr;
	}
	bool IsComplete() const { return bIsComplete; }

	// EasingParam is used for easing functions that have a second parameter, like Elastic
	FBUITweenInstance& Easing( EBUIEasingType InType, TOptional<float> InEasingParam = TOptional<float>() )
	{
		EasingType = InType;
		EasingParam = InEasingParam;
		return *this;
	}

	template<typename TweenComponent>
	FBUITweenInstance& SetTween(TweenComponent&& Component)
	{
		Component.Instance = this;
		UBUITween::GetPoolManager().Add(Entity, std::forward<TweenComponent>(Component));
		return *this;
	}

	FBUITweenInstance& SetOnStartDelegate( const FBUITweenSignature& InOnStart )
	{
		OnStartedDelegate = InOnStart;
		return *this;
	}

	FBUITweenInstance& SetOnCompleteDelegate(const FBUITweenSignature& InOnComplete)
	{
		OnCompleteDelegate = InOnComplete;
		return *this;
	}

	FBUITweenInstance& SetOnStartDelegate( const FBUITweenBPSignature& InOnStart )
	{
		OnStartedBPDelegate = InOnStart;
		return *this;
	}
	FBUITweenInstance& SetOnCompleteDelegate( const FBUITweenBPSignature& InOnComplete )
	{
		OnCompleteBPDelegate = InOnComplete;
		return *this;
	}

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
		Entity = FBUIPoolManager::INVALID_ENTITY;
	}

public:
	using TypeIndex = BUIDetails::TypeIndexer::TypeIndex;
	TMap<TypeIndex, TUniquePtr<BUIPostActionsBase>> PostActionsMap;

	bool bShouldUpdateInstance = false;
	bool bShouldUpdateComponents = false;
	bool bIsComplete = false;
	bool bHasPlayedStartEvent = false;
	bool bHasPlayedCompleteEvent = false;

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
