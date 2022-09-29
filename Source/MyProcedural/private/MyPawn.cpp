#include "MyPawn.h"
#include "Slate/SGameLayerManager.h"

AMyPawn::AMyPawn(){
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingPawnMovement"));
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Root);
	SpringArm->SetupAttachment(Root);
	Camera->SetupAttachment(SpringArm);
	SpringArm->TargetArmLength=ZoomingDefaultDistance;
	SpringArm->bDoCollisionTest=false;
	SpringArm->SetRelativeRotation(FRotator(-70,0,0));//pitch
	FloatingPawnMovement->MaxSpeed=3000;
	FloatingPawnMovement->Acceleration=20000;
	FloatingPawnMovement->Deceleration=20000;
}
void AMyPawn::BeginPlay(){
	Super::BeginPlay();
	ZoomingTarget=ZoomingDefaultDistance;
}
void AMyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("StartMovingCameraWithMouse",IE_Pressed,this,&AMyPawn::MovingCameraWithMousePressed);
	PlayerInputComponent->BindAction("StartMovingCameraWithMouse",IE_Released,this,&AMyPawn::MovingCameraWithMouseReleased).bExecuteWhenPaused = true;
	PlayerInputComponent->BindAction("StartRotatingCameraWithMouse",IE_Pressed,this,&AMyPawn::RotatingCameraWithMousePressed);
	PlayerInputComponent->BindAction("StartRotatingCameraWithMouse",IE_Released,this,&AMyPawn::RotatingCameraWithMouseReleased).bExecuteWhenPaused = true;
	PlayerInputComponent->BindAction("ResetCamera",IE_Pressed,this,&AMyPawn::ResetCamera);
	PlayerInputComponent->BindAxis("MovingMouseX",this,&AMyPawn::MovingMouseX);
	PlayerInputComponent->BindAxis("MovingMouseY",this,&AMyPawn::MovingMouseY);
	PlayerInputComponent->BindAxis("RotatingCameraWithJoystickX",this,&AMyPawn::RotatingCameraWithJoystickX);
	PlayerInputComponent->BindAxis("RotatingCameraWithJoystickY",this,&AMyPawn::RotatingCameraWithJoystickY);
	PlayerInputComponent->BindAxis("CameraMoveForward",this,&AMyPawn::CameraMoveForward);
	PlayerInputComponent->BindAxis("CameraMoveRight",this,&AMyPawn::CameraMoveRight);
	PlayerInputComponent->BindAxis("CameraZoom",this,&AMyPawn::CameraZoom);
	PlayerInputComponent->BindAxis("CameraRotateZ",this,&AMyPawn::CameraRotateZ);
}
void AMyPawn::MovingMouseX(float value){
	MousePosition.X=value;
}
void AMyPawn::MovingMouseY(float value){
	MousePosition.Y=value;
}
void AMyPawn::RotatingCameraWithJoystickX(float value){
	JoyStickPosition.X=value;
}
void AMyPawn::RotatingCameraWithJoystickY(float value){
	JoyStickPosition.Y=value;
}
void AMyPawn::CameraMoveForward(float value){
	NextMove.X=value;
}
void AMyPawn::CameraMoveRight(float value){
	NextMove.Y=value;
}
void AMyPawn::MovingCameraWithMousePressed(){
	bDragging=true;
	DraggingStartPosition=GetActorLocation();
	DraggingTarget=FVector(0.f,0.f,0.f);
	bShouldSlide=true;
}
void AMyPawn::MovingCameraWithMouseReleased(){
	bDragging=false;
}
void AMyPawn::RotatingCameraWithMousePressed(){
	bRotating=true;
	bResetRotation=false;
}
void AMyPawn::RotatingCameraWithMouseReleased(){
	bRotating=false;
}
void AMyPawn::ResetCamera(){
	bResetRotation=true;
	ZoomingTarget=ZoomingDefaultDistance;
}
void AMyPawn::CameraZoom(float value){
	ZoomingTarget = FMath::Clamp(value * ZoomingStrength + ZoomingTarget, ZoomingMinDistance, ZoomingMaxDistance);
}
void AMyPawn::CameraRotateZ(float value){
	UpdateRotationZ(value);
}



void AMyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//	Move() when Keyboard and Joystick are used OR when mouse is not dragged but is at the border of the window
	// 	!STOP sliding! (bShouldSlide=false) with Move()

	//	UpdateSlideTarget() if Mouse is currently being dragged
	//	Slide() to destination even with no input if not !STOPPED!

	((NextMove.IsZero()&&bDragging)||bRotating) ? BorderAxis=FVector2D(0.f,0.f) : SetUpBorderAxis();
	// 	no need to use BorderAxis while Dragging with no input from Keyboard or Joystick OR while Rotating

	if(!NextMove.IsZero()){ 				//	if NextMove
		bShouldSlide=false;					//		!STOP sliding!
		Move();								//		move with NextMove and BorderAxis
	}
	else if (bDragging){					//	else if Dragging ("StartMovingCameraWithMouse" is still pressed)
		UpdateSlideTarget();				//		update SlideTarget
		if (bShouldSlide) Slide();			//		if (sliding is not !STOPPED! since initial input) Slide
	}
	else{									//	else
		if (!BorderAxis.IsZero()){			//		if BorderAxis
			bShouldSlide=false;				//			!STOP sliding!
			Move();							//			move with BorderAxis
		}else if (bShouldSlide) Slide();	//		else if (sliding is not !STOPPED! since initial input) Slide
	}

	//Rotate:
	if (bRotating){
		UpdateRotatingTarget(MousePosition.X,MousePosition.Y);	//use mouse and joystick
	}else{
		UpdateRotatingTarget(0.f,0.f);							//use only joystick
	}
	if (!JoyStickPosition.IsZero()) bResetRotation=false;

	//Rotate around Y (Pitch) with RotatingTarget.Y, rotate around Z (Yaw) with RotatingTarget.X and RotationZ, X (Roll) = 0.f
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),											//from
		FRotator(RotatingTarget.Y,RotatingTarget.X+RotationZ,0.f),	//to
		GetWorld()->GetDeltaSeconds(), 
		RotatingInterpSpeed));

	//Zoom:
	SpringArm->TargetArmLength=FMath::FInterpTo(
		SpringArm->TargetArmLength, 		//from
		ZoomingTarget,						//to
		GetWorld()->GetDeltaSeconds(),
		ZoomingInterpSpeed);
}

void AMyPawn::SetUpBorderAxis()
{
	FGeometry ViewportGeometry = GetWorld()->GetGameViewport()->GetGameLayerManager()->GetViewportWidgetHostGeometry();
	FVector2D LocalPosition = FSlateApplication::Get().GetCursorPos();
	LocalPosition = ViewportGeometry.AbsoluteToLocal(LocalPosition);

	if (LocalPosition.X<=BorderSize){
		BorderAxis.Y=-1.f;
	}
	else if (LocalPosition.X>=(ViewportGeometry.Size.X-BorderSize)){
		BorderAxis.Y=1.f;
	}else{
		BorderAxis.Y=0.f;
	}
	if (LocalPosition.Y<=BorderSize){
		BorderAxis.X=1.f;
	}
	else if (LocalPosition.Y>=(ViewportGeometry.Size.Y-BorderSize)){
		BorderAxis.X=-1.f;
	}else{
		BorderAxis.X=0.f;
	}
}

void AMyPawn::Move()
{
	float moveForward = NextMove.X * NextMoveStrength + BorderAxis.X * BorderStrength;
	float moveRight = NextMove.Y * NextMoveStrength + BorderAxis.Y * BorderStrength;
	float clamping = FMath::Max(NextMoveStrength, BorderStrength);
	moveForward=FMath::Clamp(moveForward, -clamping, clamping);
	moveRight=FMath::Clamp(moveRight, -clamping, clamping);
	AddMovementInput(FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0), moveForward* GetWorld()->GetDeltaSeconds(), false);
	AddMovementInput(FVector(GetActorRightVector().X, GetActorRightVector().Y, 0), moveRight* GetWorld()->GetDeltaSeconds(), false);
}

void AMyPawn::UpdateSlideTarget(){
	//X (Right on screen) is Y (Right of Actor) and Y (Up on screen) is X (Forward of Actor)
	FVector SlideDirection = FVector(MousePosition.Y,MousePosition.X,0.f);
	FTransform Trans= FTransform(FRotator(0.f, GetTransform().Rotator().Yaw, 0.f)); // all zeros except Yaw
	DraggingTarget -= Trans.TransformVectorNoScale(SlideDirection); //rotate around Z (Yaw) and invert (-=)
}

void AMyPawn::Slide(){
	SetActorLocation(FMath::VInterpTo(
		GetActorLocation(), 										//from
		DraggingStartPosition+(DraggingTarget*DraggingStrength),	//to
		GetWorld()->GetDeltaSeconds(), 
		SlideInterpSpeed));
}

void AMyPawn::UpdateRotationZ(float value){
	RotationZ=FMath::FInterpTo(
		RotationZ, 								//from
		maxAngleZ*FMath::Clamp(value,-1.f,1.f),	//to
		GetWorld()->GetDeltaSeconds(), 
		RotationZInterpSpeed);
}

void AMyPawn::UpdateRotatingTarget(float X, float Y){
	RotatingTarget.X+=X+JoyStickPosition.X;
	if (bResetRotation){
		RotatingTarget.Y=FMath::FInterpTo(
			RotatingTarget.Y,				//from
			0.f,							//to
			GetWorld()->GetDeltaSeconds(),
			ResetCameraInterpSpeed);
	}else{
		RotatingTarget.Y=FMath::Clamp(RotatingTarget.Y+Y+JoyStickPosition.Y,-20.f,65.f);
		//clamp from -90 to -5 degrees (springarm is at -70 as default)
	}
}