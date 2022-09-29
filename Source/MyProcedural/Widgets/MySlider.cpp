// Copyright Epic Games, Inc. All Rights Reserved.

#include "MySlider.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SMySlider.h"
#include "Styling/UMGCoreStyle.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UMySlider

static FSliderStyle* DefaultSliderStyle = nullptr;

#if WITH_EDITOR
static FSliderStyle* EditorSliderStyle = nullptr;
#endif 

UMySlider::UMySlider(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MinValue = 0.0f;
	MaxValue = 1.0f;
	Orientation = EOrientation::Orient_Horizontal;
	SliderBarColor = FLinearColor::White;
	SliderHandleColor = FLinearColor::White;
	StepSize = 0.01f;
	IsFocusable = true;
	MouseUsesStep = false;
	RequiresControllerLock = true;

	if (DefaultSliderStyle == nullptr)
	{
		DefaultSliderStyle = new FSliderStyle(FUMGCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider"));

		// Unlink UMG default colors.
		DefaultSliderStyle->UnlinkColors();
	}

	WidgetStyle = *DefaultSliderStyle;

#if WITH_EDITOR 
	if (EditorSliderStyle == nullptr)
	{
		EditorSliderStyle = new FSliderStyle(FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider"));

		// Unlink UMG Editor colors from the editor settings colors.
		EditorSliderStyle->UnlinkColors();
	}
	
	if (IsEditorWidget())
	{
		WidgetStyle = *EditorSliderStyle;

		// The CDO isn't an editor widget and thus won't use the editor style, call post edit change to mark difference from CDO
		PostEditChange();
	}
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
	AccessibleBehavior = ESlateAccessibleBehavior::Summary;
	bCanChildrenBeAccessible = false;
#endif
}

TSharedRef<SWidget> UMySlider::RebuildWidget()
{
	MySlider = SNew(SMySlider)
		.Style(&WidgetStyle)
		.IsFocusable(IsFocusable)
		.OnMouseCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnMouseCaptureBegin))
		.OnMouseCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnMouseCaptureEnd))
		.OnControllerCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnControllerCaptureBegin))
		.OnControllerCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnControllerCaptureEnd))
		.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnValueChanged));

	return MySlider.ToSharedRef();
}

void UMySlider::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<float> ValueBinding = PROPERTY_BINDING(float, Value);
	
	MySlider->SetOrientation(Orientation);
	MySlider->SetMouseUsesStep(MouseUsesStep);
	MySlider->SetRequiresControllerLock(RequiresControllerLock);
	MySlider->SetSliderBarColor(SliderBarColor);
	MySlider->SetSliderHandleColor(SliderHandleColor);
	MySlider->SetValue(ValueBinding);
	MySlider->SetMinAndMaxValues(MinValue, MaxValue);
	MySlider->SetLocked(Locked);
	MySlider->SetIndentHandle(IndentHandle);
	MySlider->SetStepSize(StepSize);
}

void UMySlider::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MySlider.Reset();
}

void UMySlider::HandleOnValueChanged(float InValue)
{
	OnValueChanged.Broadcast(InValue);
}

void UMySlider::HandleOnMouseCaptureBegin()
{
	OnMouseCaptureBegin.Broadcast();
}

void UMySlider::HandleOnMouseCaptureEnd()
{
	OnMouseCaptureEnd.Broadcast();
}

void UMySlider::HandleOnControllerCaptureBegin()
{
	OnControllerCaptureBegin.Broadcast();
}

void UMySlider::HandleOnControllerCaptureEnd()
{
	OnControllerCaptureEnd.Broadcast();
}

float UMySlider::GetValue() const
{
	if ( MySlider.IsValid() )
	{
		return MySlider->GetValue();
	}

	return Value;
}

float UMySlider::GetNormalizedValue() const
{
	if (MySlider.IsValid())
	{
		return MySlider->GetNormalizedValue();
	}

	if (MinValue == MaxValue)
	{
		return 1.0f;
	}
	else
	{
		return (Value - MinValue) / (MaxValue - MinValue);
	}
}

void UMySlider::SetValue(float InValue)
{
	Value = InValue;
	if ( MySlider.IsValid() )
	{
		MySlider->SetValue(InValue);
	}
}

void UMySlider::SetMinValue(float InValue)
{
	MinValue = InValue;
	if (MySlider.IsValid())
	{
		// Because SMySlider clamps min/max values upon setting them,
		// we have to send both values together to ensure that they
		// don't get out of sync.
		MySlider->SetMinAndMaxValues(MinValue, MaxValue);
	}
}

void UMySlider::SetMaxValue(float InValue)
{
	MaxValue = InValue;
	if (MySlider.IsValid())
	{
		MySlider->SetMinAndMaxValues(MinValue, MaxValue);
	}
}

void UMySlider::SetIndentHandle(bool InIndentHandle)
{
	IndentHandle = InIndentHandle;
	if ( MySlider.IsValid() )
	{
		MySlider->SetIndentHandle(InIndentHandle);
	}
}

void UMySlider::SetLocked(bool InLocked)
{
	Locked = InLocked;
	if ( MySlider.IsValid() )
	{
		MySlider->SetLocked(InLocked);
	}
}

void UMySlider::SetStepSize(float InValue)
{
	StepSize = InValue;
	if (MySlider.IsValid())
	{
		MySlider->SetStepSize(InValue);
	}
}

void UMySlider::SetSliderHandleColor(FLinearColor InValue)
{
	SliderHandleColor = InValue;
	if (MySlider.IsValid())
	{
		MySlider->SetSliderHandleColor(InValue);
	}
}

void UMySlider::SetSliderBarColor(FLinearColor InValue)
{
	SliderBarColor = InValue;
	if (MySlider.IsValid())
	{
		MySlider->SetSliderBarColor(InValue);
	}
}

#if WITH_ACCESSIBILITY
TSharedPtr<SWidget> UMySlider::GetAccessibleWidget() const
{
	return MySlider;
}
#endif

#if WITH_EDITOR

const FText UMySlider::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
