#include "BUITween.h"

#include "BUITweenInstanceNew.h"

#if 1
TArray< FBUITweenInstance2 > UBUITween::ActiveInstances = TArray< FBUITweenInstance2 >();
TArray< FBUITweenInstance2 > UBUITween::InstancesToAdd = TArray< FBUITweenInstance2 >();
#endif

FBUIPoolManager UBUITween::PoolManager{};

bool UBUITween::bIsInitialized = false;

void UBUITween::Startup()
{
	PoolManager.RemoveAll();
	ActiveInstances.Empty();
	InstancesToAdd.Empty();

	bIsInitialized = true;

	check([]()
	{
		FBUIPoolManager::EntityHandle Handle = PoolManager.GetEntityHandle();

		struct Bla
		{
			void Apply() {}
			void Begin() {}
			void SetActive(const bool) {}
			void SetData(void*) {}
		};
		PoolManager.Add(Handle, Bla{});
		PoolManager.Add(Handle, BUITweenComponents::BUITranslationTween()
			.From(FVector2D(5, 5))
			.To(FVector2D(10, 10)));
		PoolManager.RemoveEntity(Handle);

		return true;
	}
	());
}


void UBUITween::Shutdown()
{
	PoolManager.RemoveAll();
	ActiveInstances.Empty();
	InstancesToAdd.Empty();

	bIsInitialized = false;
}



FBUITweenInstance2& UBUITween::Create( UWidget* pInWidget, float InDuration, float InDelay, bool bIsAdditive )
{
	// By default let's kill any existing tweens
	if ( !bIsAdditive )
	{
		Clear( pInWidget );
	}

	InstancesToAdd.Emplace( pInWidget, InDuration, InDelay );

	return InstancesToAdd.Last();
}


int32 UBUITween::Clear( UWidget* pInWidget )
{
	int32 NumRemoved = 0;

	const auto DoesTweenMatchWidgetFn = [pInWidget](const FBUITweenInstance2& CurTweenInstance) -> bool
	{
		return CurTweenInstance.GetWidget().IsValid() && CurTweenInstance.GetWidget() == pInWidget;
	};

	NumRemoved += ActiveInstances.RemoveAll(DoesTweenMatchWidgetFn);
	NumRemoved += InstancesToAdd.RemoveAll(DoesTweenMatchWidgetFn);

	return NumRemoved;
}


void UBUITween::Update( float DeltaTime )
{
	// Reverse it so we can remove
	//for ( int32 i = ActiveInstances.Num()-1; i >= 0; --i )


	for (auto& ActiveInst: ActiveInstances)
	{
		ActiveInst.PreComponentsUpdate(DeltaTime);
	}

	PoolManager.ApplyAll();

	// Reverse it so we can remove
	for ( int32 i = ActiveInstances.Num()-1; i >= 0; --i )
	{
		FBUITweenInstance2& ActiveInst = ActiveInstances[ i ];
		ActiveInst.PostComponentsUpdate();

		if ( ActiveInst.IsComplete() )
		{
			FBUITweenInstance2 CompleteInst = MoveTemp(ActiveInst);
			ActiveInstances.RemoveAtSwap( i, 1, false );

			// We do this here outside of the instance update and after removing from active instances because we
			// don't know if the callback in the cleanup is going to trigger adding more events
			CompleteInst.DoCompleteCleanup();
		}
	}

	for (int32 i = 0; i < InstancesToAdd.Num(); ++i)
	{
		ActiveInstances.Add(MoveTemp(InstancesToAdd[i]));
	}
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
