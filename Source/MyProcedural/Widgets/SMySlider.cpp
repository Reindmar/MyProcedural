// Copyright Epic Games, Inc. All Rights Reserved.

#include "SMySlider.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#if WITH_ACCESSIBILITY
#include "Widgets/Accessibility/SlateAccessibleWidgets.h"
#endif

SMySlider::SMySlider()
{
#if WITH_ACCESSIBILITY
	AccessibleBehavior = EAccessibleBehavior::Summary;
	bCanChildrenBeAccessible = false;
#endif
}

void SMySlider::Construct( const SMySlider::FArguments& InDeclaration )
{
	check(InDeclaration._Style);

	Style = InDeclaration._Style;

	IndentHandle = InDeclaration._IndentHandle;
	bMouseUsesStep = InDeclaration._MouseUsesStep;
	bRequiresControllerLock = InDeclaration._RequiresControllerLock;
	LockedAttribute = InDeclaration._Locked;
	Orientation = InDeclaration._Orientation;
	StepSize = InDeclaration._StepSize;
	ValueAttribute = InDeclaration._Value;
	MinValue = InDeclaration._MinValue;
	MaxValue = InDeclaration._MaxValue;
	SliderBarColor = InDeclaration._SliderBarColor;
	SliderHandleColor = InDeclaration._SliderHandleColor;
	bIsFocusable = InDeclaration._IsFocusable;
	OnMouseCaptureBegin = InDeclaration._OnMouseCaptureBegin;
	OnMouseCaptureEnd = InDeclaration._OnMouseCaptureEnd;
	OnControllerCaptureBegin = InDeclaration._OnControllerCaptureBegin;
	OnControllerCaptureEnd = InDeclaration._OnControllerCaptureEnd;
	OnValueChanged = InDeclaration._OnValueChanged;

	bControllerInputCaptured = false;
	SetCanTick(false);
}

int32 SMySlider::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// we draw the slider like a horizontal slider regardless of the orientation, and apply a render transform to make it display correctly.
	// However, the AllottedGeometry is computed as it will be rendered, so we have to use the "horizontal orientation" when doing drawing computations.
	const float AllottedWidth = Orientation == Orient_Horizontal ? AllottedGeometry.GetLocalSize().X : AllottedGeometry.GetLocalSize().Y;
	const float AllottedHeight = Orientation == Orient_Horizontal ? AllottedGeometry.GetLocalSize().Y : AllottedGeometry.GetLocalSize().X;

	float HandleRotation;
	FVector2D HandleTopLeftPoint;
	FVector2D SliderStartPoint;
	FVector2D SliderEndPoint;

	// calculate slider geometry as if it's a horizontal slider (we'll rotate it later if it's vertical)
	const FVector2D HandleSize = GetThumbImage()->ImageSize;
	const FVector2D HalfHandleSize = 0.5f * HandleSize;
	const float Indentation = IndentHandle.Get() ? HandleSize.X : 0.0f;

	// We clamp to make sure that the slider cannot go out of the slider Length.
	const float SliderPercent = FMath::Clamp(GetNormalizedValue(), 0.0f, 1.0f); 
	const float SliderLength = AllottedWidth - (Indentation + HandleSize.X);
	const float SliderHandleOffset = SliderPercent * SliderLength;
	const float SliderY = 0.5f * AllottedHeight;

	HandleRotation = 0.0f;
	HandleTopLeftPoint = FVector2D(SliderHandleOffset + (0.5f * Indentation), SliderY - HalfHandleSize.Y);

	SliderStartPoint = FVector2D(HalfHandleSize.X, SliderY);
	SliderEndPoint = FVector2D(AllottedWidth - HalfHandleSize.X, SliderY);

	FGeometry SliderGeometry = AllottedGeometry;
	
	// rotate the slider 90deg if it's vertical. The 0 side goes on the bottom, the 1 side on the top.
	if (Orientation == Orient_Vertical)
	{
		// Do this by translating along -X by the width of the geometry, then rotating 90 degreess CCW (left-hand coords)
		FSlateRenderTransform SlateRenderTransform = TransformCast<FSlateRenderTransform>(Concatenate(Inverse(FVector2D(AllottedWidth, 0)), FQuat2D(FMath::DegreesToRadians(-90.0f))));
		// create a child geometry matching this one, but with the render transform.
		SliderGeometry = AllottedGeometry.MakeChild(
			FVector2D(AllottedWidth, AllottedHeight), 
			FSlateLayoutTransform(), 
			SlateRenderTransform, FVector2D::ZeroVector);
	}

	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	// draw slider bar
	auto BarTopLeft = FVector2D(SliderStartPoint.X, SliderStartPoint.Y - Style->BarThickness * 0.5f);
	auto BarSize = FVector2D(SliderEndPoint.X - SliderStartPoint.X, Style->BarThickness);
	auto BarImage = GetBarImage();
	auto ThumbImage = GetThumbImage();
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		SliderGeometry.ToPaintGeometry(BarTopLeft, BarSize),
		BarImage,
		DrawEffects,
		BarImage->GetTint(InWidgetStyle) * SliderBarColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
		);

	++LayerId;

	// draw slider thumb
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId,
		SliderGeometry.ToPaintGeometry(HandleTopLeftPoint, GetThumbImage()->ImageSize),
		ThumbImage,
		DrawEffects,
		ThumbImage->GetTint(InWidgetStyle) * SliderHandleColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
	);

	return LayerId;
}

FVector2D SMySlider::ComputeDesiredSize( float ) const
{
	static const FVector2D SMySliderDesiredSize(16.0f, 16.0f);

	if ( Style == nullptr )
	{
		return SMySliderDesiredSize;
	}

	const float Thickness = FMath::Max(Style->BarThickness, 
		FMath::Max(Style->NormalThumbImage.ImageSize.Y, Style->HoveredThumbImage.ImageSize.Y));

	if (Orientation == Orient_Vertical)
	{
		return FVector2D(Thickness, SMySliderDesiredSize.Y);
	}

	return FVector2D(SMySliderDesiredSize.X, Thickness);
}

bool SMySlider::IsLocked() const
{
	return LockedAttribute.Get();
}

bool SMySlider::IsInteractable() const
{
	return IsEnabled() && !IsLocked() && SupportsKeyboardFocus();
}

bool SMySlider::SupportsKeyboardFocus() const
{
	return bIsFocusable;
}

void SMySlider::ResetControllerState()
{
	if (bControllerInputCaptured)
	{
		OnControllerCaptureEnd.ExecuteIfBound();
		bControllerInputCaptured = false;
	}
}

FNavigationReply SMySlider::OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent)
{
	if (bControllerInputCaptured || !bRequiresControllerLock)
	{
		FNavigationReply Reply = FNavigationReply::Escape();

		float NewValue = ValueAttribute.Get();
		if (Orientation == EOrientation::Orient_Horizontal)
		{
			if (InNavigationEvent.GetNavigationType() == EUINavigation::Left)
			{
				NewValue -= StepSize.Get();
				Reply = FNavigationReply::Stop();
			}
			else if (InNavigationEvent.GetNavigationType() == EUINavigation::Right)
			{
				NewValue += StepSize.Get();
				Reply = FNavigationReply::Stop();
			}
		}
		else
		{
			if (InNavigationEvent.GetNavigationType() == EUINavigation::Down)
			{
				NewValue -= StepSize.Get();
				Reply = FNavigationReply::Stop();
			}
			else if (InNavigationEvent.GetNavigationType() == EUINavigation::Up)
			{
				NewValue += StepSize.Get();
				Reply = FNavigationReply::Stop();
			}
		}
		if (ValueAttribute.Get() != NewValue)
		{
			CommitValue(FMath::Clamp(NewValue, MinValue, MaxValue));
			return Reply;
		}
	}

	return SLeafWidget::OnNavigation(MyGeometry, InNavigationEvent);
}

FReply SMySlider::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FReply::Unhandled();

	if (IsInteractable())
	{
		// The controller's bottom face button must be pressed once to begin manipulating the slider's value.
		// Navigation away from the widget is prevented until the button has been pressed again or focus is lost.
		// The value can be manipulated by using the game pad's directional arrows ( relative to slider orientation ).
		if (FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept && bRequiresControllerLock)
		{
			if (bControllerInputCaptured == false)
			{
				// Begin capturing controller input and allow user to modify the slider's value.
				bControllerInputCaptured = true;
				OnControllerCaptureBegin.ExecuteIfBound();
				Reply = FReply::Handled();
			}
			else
			{
				ResetControllerState();
				Reply = FReply::Handled();
			}
		}
		else
		{
			Reply = SLeafWidget::OnKeyDown(MyGeometry, InKeyEvent);
		}
	}
	else
	{
		Reply = SLeafWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

	return Reply;
}

FReply SMySlider::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FReply::Unhandled();
	if (bControllerInputCaptured)
	{
		Reply = FReply::Handled();
	}
	return Reply;
}

void SMySlider::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	if (bControllerInputCaptured)
	{
		// Commit and reset state
		CommitValue(ValueAttribute.Get());
		ResetControllerState();
	}
}
FReply SMySlider::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ((MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && !IsLocked())
	{
		OnMouseCaptureBegin.ExecuteIfBound();
		CommitValue(PositionToValue(MyGeometry, MouseEvent.GetScreenSpacePosition()));
		SetCanTick(true);
		// Release capture for controller/keyboard when switching to mouse.
		ResetControllerState();
		
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

void SMySlider::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	CommitValue(PositionToValue(AllottedGeometry, FSlateApplication::Get().GetCursorPos()));
		
	// Release capture for controller/keyboard when switching to mouse
	ResetControllerState();
}

FReply SMySlider::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ((MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && HasMouseCaptureByUser(MouseEvent.GetUserIndex(), MouseEvent.GetPointerIndex()))
	{
		// Release capture for controller/keyboard when switching to mouse.
		ResetControllerState();
		SetCanTick(false);
		return FReply::Handled().ReleaseMouseCapture();	
	}

	return FReply::Unhandled();
}

void SMySlider::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	OnMouseCaptureEnd.ExecuteIfBound();
	SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
}

FReply SMySlider::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

FReply SMySlider::OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	if (!IsLocked())
	{
		// Release capture for controller/keyboard when switching to mouse.
		ResetControllerState();

		PressedScreenSpaceTouchDownPosition = InTouchEvent.GetScreenSpacePosition();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SMySlider::OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	if (HasMouseCaptureByUser(InTouchEvent.GetUserIndex(), InTouchEvent.GetPointerIndex()))
	{
		CommitValue(PositionToValue(MyGeometry, InTouchEvent.GetScreenSpacePosition()));

		// Release capture for controller/keyboard when switching to mouse
		ResetControllerState();

		return FReply::Handled();
	}
	else if (!HasMouseCapture())
	{
		if (FSlateApplication::Get().HasTraveledFarEnoughToTriggerDrag(InTouchEvent, PressedScreenSpaceTouchDownPosition, Orientation))
		{
			OnMouseCaptureBegin.ExecuteIfBound();

			CommitValue(PositionToValue(MyGeometry, InTouchEvent.GetScreenSpacePosition()));

			// Release capture for controller/keyboard when switching to mouse
			ResetControllerState();

			return FReply::Handled().CaptureMouse(SharedThis(this));
		}
	}

	return FReply::Unhandled();
}

FReply SMySlider::OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	if (HasMouseCaptureByUser(InTouchEvent.GetUserIndex(), InTouchEvent.GetPointerIndex()))
	{
		OnMouseCaptureEnd.ExecuteIfBound();

		CommitValue(PositionToValue(MyGeometry, InTouchEvent.GetScreenSpacePosition()));

		// Release capture for controller/keyboard when switching to mouse.
		ResetControllerState();

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

void SMySlider::CommitValue(float NewValue)
{
	const float OldValue = GetValue();

	if (!ValueAttribute.IsBound())
	{
		ValueAttribute.Set(NewValue);
	}

	Invalidate(EInvalidateWidgetReason::Paint);

	OnValueChanged.ExecuteIfBound(NewValue);
}

float SMySlider::PositionToValue( const FGeometry& MyGeometry, const FVector2D& AbsolutePosition )
{
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(AbsolutePosition);

	float RelativeValue;
	float Denominator;
	// Only need X as we rotate the thumb image when rendering vertically
	const float Indentation = GetThumbImage()->ImageSize.X * (IndentHandle.Get() ? 2.f : 1.f);
	const float HalfIndentation = 0.5f * Indentation;

	if (Orientation == Orient_Horizontal)
	{
		Denominator = MyGeometry.Size.X - Indentation;
		RelativeValue = (Denominator != 0.f) ? (FMath::RoundToFloat(LocalPosition.X) - HalfIndentation) / Denominator : 0.f;
	}
	else
	{
		Denominator = MyGeometry.Size.Y - Indentation;
		// Inverse the calculation as top is 0 and bottom is 1
		RelativeValue = (Denominator != 0.f) ? ((MyGeometry.Size.Y - FMath::RoundToFloat(LocalPosition.Y)) - HalfIndentation) / Denominator : 0.f;
	}
	RelativeValue = FMath::Clamp(RelativeValue, 0.0f, 1.0f) * (MaxValue - MinValue) + MinValue;
	if (bMouseUsesStep)
	{
		float direction = ValueAttribute.Get() - RelativeValue;
		if (direction > StepSize.Get() / 2.0f)
		{
			return FMath::Clamp(ValueAttribute.Get() - StepSize.Get(), MinValue, MaxValue);
		}
		else if (direction < StepSize.Get() / -2.0f)
		{
			return FMath::Clamp(ValueAttribute.Get() + StepSize.Get(), MinValue, MaxValue);
		}
		else
		{
			return ValueAttribute.Get();
		}
	}
	return RelativeValue;
}

const FSlateBrush* SMySlider::GetBarImage() const
{
	if (!IsEnabled() || LockedAttribute.Get())
	{
		return &Style->DisabledBarImage;
	}
	else if (IsHovered())
	{
		return &Style->HoveredBarImage;
	}
	else
	{
		return &Style->NormalBarImage;
	}
}

const FSlateBrush* SMySlider::GetThumbImage() const
{
	if (!IsEnabled() || LockedAttribute.Get())
	{
		return &Style->DisabledThumbImage;
	}
	else if (IsHovered())
	{
		return &Style->HoveredThumbImage;
	}
	else
	{
		return &Style->NormalThumbImage;
	}
}

float SMySlider::GetValue() const
{
	return ValueAttribute.Get();
}

float SMySlider::GetNormalizedValue() const
{
	if (MaxValue == MinValue)
	{
		return 1.0f;
	}
	else
	{
		return (ValueAttribute.Get() - MinValue) / (MaxValue - MinValue);
	}
}

void SMySlider::SetValue(const TAttribute<float>& InValueAttribute)
{
	SetAttribute(ValueAttribute, InValueAttribute, EInvalidateWidgetReason::Paint);
}

void SMySlider::SetMinAndMaxValues(float InMinValue, float InMaxValue)
{
	MinValue = InMinValue;
	MaxValue = InMaxValue;
	if (MinValue > MaxValue)
	{
		MaxValue = MinValue;
	}
}

void SMySlider::SetIndentHandle(const TAttribute<bool>& InIndentHandle)
{
	SetAttribute(IndentHandle, InIndentHandle, EInvalidateWidgetReason::Paint);
}

void SMySlider::SetLocked(const TAttribute<bool>& InLocked)
{
	SetAttribute(LockedAttribute, InLocked, EInvalidateWidgetReason::Paint);
}

void SMySlider::SetOrientation(EOrientation InOrientation)
{
	if (Orientation != InOrientation)
	{
		Orientation = InOrientation;
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SMySlider::SetSliderBarColor(FSlateColor InSliderBarColor)
{
	SetAttribute(SliderBarColor, TAttribute<FSlateColor>(InSliderBarColor), EInvalidateWidgetReason::Paint);
}

void SMySlider::SetSliderHandleColor(FSlateColor InSliderHandleColor)
{
	SetAttribute(SliderHandleColor, TAttribute<FSlateColor>(InSliderHandleColor), EInvalidateWidgetReason::Paint);
}

float SMySlider::GetStepSize() const
{
	return StepSize.Get();
}

void SMySlider::SetStepSize(const TAttribute<float>& InStepSize)
{
	StepSize = InStepSize;
}

void SMySlider::SetMouseUsesStep(bool MouseUsesStep)
{
	bMouseUsesStep = MouseUsesStep;
}

void SMySlider::SetRequiresControllerLock(bool RequiresControllerLock)
{
	bRequiresControllerLock = RequiresControllerLock;
}

#if WITH_ACCESSIBILITY
TSharedRef<FSlateAccessibleWidget> SMySlider::CreateAccessibleWidget()
{
	return MakeShareable<FSlateAccessibleWidget>(new FSlateAccessibleSlider(SharedThis(this)));
}
#endif
