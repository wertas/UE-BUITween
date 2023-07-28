#include "BUITweenBlueprintLibrary.h"


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
		Type Temp = InFirst;		\
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