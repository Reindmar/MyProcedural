#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "MyPawn.generated.h"

UCLASS()
class MYPROCEDURAL_API AMyPawn : public APawn
{
	GENERATED_BODY()

public:
	AMyPawn();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UFloatingPawnMovement* FloatingPawnMovement;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USpringArmComponent* SpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
//	Moving
	//receive inputs from Keyboard+Joystick for NextMove
	void CameraMoveForward(float value);
	void CameraMoveRight(float value);
	FVector2D NextMove;

	//receive inputs from Mouse	for MousePosition
	void MovingMouseX(float value);
	void MovingMouseY(float value);
	FVector2D MousePosition;

	//receive inputs from Mouse	for bDragging, bShouldSlide, DraggingStartPosition and reset DraggingTarget
	void MovingCameraWithMousePressed();
	void MovingCameraWithMouseReleased();
	bool bDragging = false; //true when Pressed, false when Released
	bool bShouldSlide = false; //true when Pressed, can be stopped (in Tick right before Move())
	FVector DraggingStartPosition;
	FVector DraggingTarget;

	//calculate BorderAxis
	void SetUpBorderAxis();
	FVector2D BorderAxis;

	//move pawn using NextMove and BorderAxis
	void Move();

	//calculate DraggingTarget (using MousePosition)
	void UpdateSlideTarget();
	//move using mouse DraggingTarget if not moving using Keyboard, Joystick
	void Slide(); //is only used when (NextMove.IsZero() AND (bDragging=1 OR BorderAxis.IsZero()))
	
//	Rotating
	//receive inputs from Mouse for bRotating and bResetRotation
	void RotatingCameraWithMousePressed();
	void RotatingCameraWithMouseReleased();
	bool bRotating = false;
	bool bResetRotation = false;
	//receive inputs from Joystick for JoyStickPosition
	void RotatingCameraWithJoystickX(float value);
	void RotatingCameraWithJoystickY(float value);
	FVector2D JoyStickPosition;

	//receive input from Keyboard and Joystick for RotationZ
	void CameraRotateZ(float value);
	//calculate RotationZ using input and maxAngleZ
	void UpdateRotationZ(float value);
	float RotationZ;

	//calculate FVector2D RotatingTarget using (using MousePosition and JoyStickPosition)
	void UpdateRotatingTarget(float X, float Y);
	FVector2D RotatingTarget;

//	Zooming
	//receive input from Keyboard and Joystick which is used to calculate ZoomingTarget
	void CameraZoom(float value);
	float ZoomingTarget;

//	receive inputs from Keyboard and Joystick for bResetRotation and zoomingTarget
	void ResetCamera();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Move", Meta = (ToolTip = "Speed multiplier for Keyboard and Joystick"))
	float NextMoveStrength = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Move", Meta = (ToolTip = "Number of pixels to count as viewport border"))
	float BorderSize = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Dragging", Meta = (ToolTip = "Speed multiplier for mouse-near-border camera movement"))
	float BorderStrength = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Dragging", Meta = (ToolTip = "Lower numbers make camera reach target later"))
	float SlideInterpSpeed = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Dragging", Meta = (ToolTip = "Speed multiplier for mouse dragging"))
	float DraggingStrength = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Rotating", Meta = (ToolTip = "Lower numbers make camera reach target later"))
	float RotatingInterpSpeed = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Rotating", Meta = (ToolTip = "Lower numbers make camera reach target later"))
	float RotationZInterpSpeed = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Rotating", Meta = (ToolTip = "Max angle for rotating with CameraRotateZ input"))
	float maxAngleZ = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Zooming")
	float ZoomingDefaultDistance=700.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Zooming", Meta = (ToolTip = "Speed multiplier for zooming"))
	float ZoomingStrength=100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Zooming")
	float ZoomingMinDistance=500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Zooming")
	float ZoomingMaxDistance=1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Zooming", Meta = (ToolTip = "Lower numbers make camera reach target later"))
	float ZoomingInterpSpeed=8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Reset", Meta = (ToolTip = "Lower numbers make camera reach target later"))
	float ResetCameraInterpSpeed = 5.f;
};
