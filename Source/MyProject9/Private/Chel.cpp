// Fill out your copyright notice in the Description page of Project Settings.


#include "Chel.h"

#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine.h"
#include "Engine/EngineTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/SkinnedMeshComponent.h"
#include "AreaCollision.h"
#include "BP_PlayerController.h"
#include "Cache_Key.h"
#include "CanalizationDamageCollision.h"
#include "Code_Note.h"

enum PickUpType {
	Boltorez,
	KeyShelter,
	Otvertka,
	OpenArea,
	Cache,
	CacheKey,
	GeneratorArea,
	CanalizationButton,
	WebCamLocker,
	InvisiblePotion,
	CodeNote,
	ClickButton,
};

enum CacheType {
	KeyBronze,
	KeySilver,
	KeyGold
};

// Sets default values
AChel::AChel()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PoseableMeshComp = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableMeshComp"));
	PoseableMeshComp->SetupAttachment(RootComponent);

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	Scene->SetupAttachment(PoseableMeshComp, TEXT("Bone_002_end"));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(Scene);

	DamageCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DamageCollision"));
	DamageCollision->SetupAttachment(RootComponent);

	Stone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));
	Stone->SetupAttachment(CameraComp);

	TimeLine_Stone_First = CreateDefaultSubobject<UTimelineComponent>(TEXT("ThrowStoneFirst"));
	TimeLine_Stone_Second = CreateDefaultSubobject<UTimelineComponent>(TEXT("ThrowStoneSecond"));
	TimeLine_FOV_WebCam = CreateDefaultSubobject<UTimelineComponent>(TEXT("FOV_WebCam"));

	//������� ���������, ����� ������� ��������� ��� Update � ��� OnFinished
	InterpFunction_Stone.BindUFunction(this, FName("TimelineVectorReturn_Stone"));
	InterpFunction_FOV_WebCam.BindUFunction(this, FName("TimelineFloatReturn_FOV_WebCam"));
	TimelineFinished_Stone_First.BindUFunction(this, FName("OnTimelineFinished_Stone_First"));
	TimelineFinished_Stone_Second.BindUFunction(this, FName("OnTimelineFinished_Stone_Second"));

	DamageCollision->OnComponentBeginOverlap.AddDynamic(this, &AChel::OnOverlapBegin);

	Health = 0;
	bIsAlreadyThrowing = false;
	IsInGame = false;
	Ammo = 10;
	bLineTrace_is_need_refresh = false;
	DoesHave_Owner = false;
	ItemCodePickUp = -1;
	IsEnableInput = true;

	CanalizationDamage = 1.0f;

	KillerIndex = -1;
	IsSuccessOpening = false;
	AreaCode = -1;
	bCanPossessWebCam = true;
	bCanWalkingAndWatching = true;

//	OpenAreaObj = nullptr;

	KeysCount.Init(0, 3);
}

// Called when the game starts or when spawned
void AChel::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() != ROLE_Authority)
		MyBeginPlay();

	MeshTrans = PoseableMeshComp->GetRelativeTransform();
}

void AChel::MyBeginPlay()
{
	GS = Cast<AGS>(GetWorld()->GetGameState());

	IsServerAuth = GetLocalRole() == ROLE_Authority;
	World = GetWorld();
	IsPlayerOwner = UGameplayStatics::GetPlayerController(GetWorld(), 0) == GetController();
	StonePosition = Stone->GetRelativeLocation();

	GI = World->GetGameInstance<UGI>();

	if (vCurveStone) {
		//���������� ��� ������� ��� TimeLine
		TimeLine_Stone_First->AddInterpVector(vCurveStone, InterpFunction_Stone);
		TimeLine_Stone_First->SetTimelineFinishedFunc(TimelineFinished_Stone_First);

		TimeLine_Stone_First->SetLooping(false);
		TimeLine_Stone_First->SetIgnoreTimeDilation(true);

		TimeLine_Stone_Second->AddInterpVector(vCurveStone, InterpFunction_Stone);
		TimeLine_Stone_Second->SetTimelineFinishedFunc(TimelineFinished_Stone_Second);

		TimeLine_Stone_Second->SetLooping(false);
		TimeLine_Stone_Second->SetIgnoreTimeDilation(true);

		TimeLine_FOV_WebCam->AddInterpFloat(vCurveFOV_WebCam, InterpFunction_FOV_WebCam);

		TimeLine_FOV_WebCam->SetLooping(false);
		TimeLine_FOV_WebCam->SetIgnoreTimeDilation(true);
	}

	if (vCurveFOV_WebCam)
	{
	}

	if (IsPlayerOwner) {
		UserView = Cast<UUserView>(CreateWidget(World, UserViewClass));
		GeneratorView = Cast<UGeneratorWidget>(CreateWidget(World, GeneratorView_class));
		KillFeed = Cast<UKillFeed>(CreateWidget(World, KillFeed_class));
		Widget_Note = Cast<UNoteWidget>(CreateWidget(World, NoteWidget_class));
		UserView->AddToViewport();
		KillFeed->AddToViewport();
		GeneratorView->AddToViewport();
		GeneratorView->SetVisibility(ESlateVisibility::Hidden);
		Widget_Note->AddToViewport();
		Widget_Note->SetVisibility(ESlateVisibility::Hidden);
		KillFeed->AddToViewport();

		UserView->Player = this;
		UserView->AmmoLabel->SetText(FText::AsNumber((int32)Ammo));

		if (GI != nullptr) {
			//Sensetivity = GI->SGF->Sensetivity;
			//DeliverNicknameToServer(GI->SGF->NickName);
		}
	}

	if (IsServerAuth) {
		Index = GS->AmountOfPlayers++;
		UE_LOG(LogTemp, Warning, TEXT("AmountOfPlayers increase Chel"))
		for (int i = 0; i < 2; ++i)
		{
			ASpectator* spec = World->SpawnActorDeferred<ASpectator>(SpectatorClass, CameraComp->GetComponentTransform());
			if (spec != nullptr) {
				spec->Player = this;
				spec->Index = Index;
				spec->Layer = i;
				UGameplayStatics::FinishSpawningActor(spec, CameraComp->GetComponentTransform());
			}
			if (GS != nullptr)
				GS->Spectators.Add(spec);
		}

		DoesHave.Init(false, 3);
	}

	if (IsServerAuth && IsPlayerOwner)
		Cast<ABP_PlayerController>(GetController())->IsHost = true;
}

void AChel::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	MyBeginPlay();
}

// Called every frame
void AChel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsInGame == true) {
		if (IsServerAuth) {
			DeltaTime *= 2 * 0.01f * RadCoeff * CanalizationDamage;
			Health += DeltaTime;
			if (Health > 1.0f) {
				if (DoesHave[Boltorez])
					GS->CurrentBoltorez--;
				if (DoesHave[KeyShelter])
					GS->CurrentKeyShelter--;
				if (DoesHave[Otvertka])
					GS->CurrentOtvertka--;

				TArray<AActor*>Players;
				UGameplayStatics::GetAllActorsOfClass(World, StaticClass(), Players);
				for (auto& Player : Players) {
					Cast<AChel>(Player)->RefreshWidgets(DoesHave, KillerIndex, Index);
				}
				DoesHave.Init(false, 3);
				KillerIndex = -1;
				bCanWalkingAndWatching = true;
				KillPlayer();
			}
		}
	}

	if (IsPlayerOwner) {
		UserView->RadiationPoints->SetPercent(Health);
		UserView->DarkScreen->SetRenderOpacity(Health);

		FHitResult OutHit;

		FVector StartLocation = CameraComp->GetComponentLocation();
		FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

		FCollisionQueryParams CollisionParams;

		//DrawDebugLine(World, StartLocation, EndLocation, FColor::Red, false, 1, 0, 1);

		isTracedBad = false; //�����������, ��� �� �� ����� ���� ������� ������ ��� ��������
		if (World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams))
		{
			if (OutHit.bBlockingHit)
			{
				AActor* HittableActor = OutHit.GetActor();
				if (HittableActor) { //���� �� ���������� � �����-�� �����, � �� � �������
					APickableItem* TracedItem = Cast<APickableItem>(HittableActor);
					if (TracedItem) { //���� �� ���������� � ������ ��� �������
						ItemCodePickUp = TracedItem->Type;
						UserView->E_Mark->SetVisibility(ESlateVisibility::Visible);
						isTracedBad = false;
						bLineTrace_is_need_refresh = true;  //�������, ��� � ������� ����� �� ��������� � ������ �������
						if (LastItem)
							LastItem->Item->SetCustomDepthStencilValue(0);
						LastItem = TracedItem;
						if (TracedItem->Type == ClickButton)
						{
							if (GS->IsCodeTerminalAvaliable)
								LastItem->Item->SetCustomDepthStencilValue(2);
						}
						else if (TracedItem->Type == OpenArea) {
							AOpenArea* MyOpenArea = Cast<AOpenArea>(TracedItem);
							if (!MyOpenArea->bIsAvaliable) {
								UserView->E_Mark->SetVisibility(ESlateVisibility::Hidden);
							}
							else {
								LastItem->Item->SetCustomDepthStencilValue(2);
							}
						}
						else
							LastItem->Item->SetCustomDepthStencilValue(2);
					}
					else
					{
						ACache* TracedCache = Cast<ACache>(HittableActor);
						if (TracedCache) {
							if (TracedCache->IsEnabled && KeysCount[TracedCache->CacheType] > 0) {
								isTracedBad = false;
								bLineTrace_is_need_refresh = true;
								ItemCodePickUp = Cache;
								UserView->E_Mark->SetVisibility(ESlateVisibility::Visible);

								LastCache = TracedCache;
							}
						}
						else
						{
							ACanalizationButton* TracedButton = Cast<ACanalizationButton>(HittableActor);
							if (TracedButton) {
								bLineTrace_is_need_refresh = true;
								ItemCodePickUp = CanalizationButton;
								UserView->E_Mark->SetVisibility(ESlateVisibility::Visible);

								LastButton = TracedButton;
							}
							else
							{
								AWebCamLocker* TracedWebCamLocker = Cast<AWebCamLocker>(HittableActor);
								if (TracedWebCamLocker)
								{
									bLineTrace_is_need_refresh = true;
									ItemCodePickUp = WebCamLocker;
									UserView->E_Mark->SetVisibility(ESlateVisibility::Visible);

									LastWebCamLocker = TracedWebCamLocker;
								}
								else
									isTracedBad = true; //�� �� ������ � ������ ��� �������
								
							}
						}
					}
				}
				else
					isTracedBad = true;  //�� �� ������ � ������ ��� �������
			}
			else
				isTracedBad = true;  //�� �� ������ � ������ ��� �������
		}
		else
			isTracedBad = true;   //�� �� ������ � ������ ��� �������

		if (isTracedBad && bLineTrace_is_need_refresh) { 
			//���� �� �� ������ � ������ ��� �������, �� �� ������� ����� �� ���������� ������,
			// �� ��� ����������� �� �������� ��� ������� �� ������� �������� ������������ � ������ ���������
			bLineTrace_is_need_refresh = false;

			ItemCodePickUp = -1;
			if (LastItem)
			{
				LastItem->Item->SetCustomDepthStencilValue(0);
			}


			if (LastItem && Cast<AOpenArea>(LastItem)) 
			{
				PickUp_Released();
			}
			UserView->StopAllAnimations();
			UserView->E_Mark->SetVisibility(ESlateVisibility::Hidden);

		}
	}
}

//SetupReplicationVariables----
void AChel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChel, Index);
	DOREPLIFETIME(AChel, Health);
	DOREPLIFETIME(AChel, NickName);
	DOREPLIFETIME(AChel, Ammo);
	DOREPLIFETIME(AChel, Death);
	DOREPLIFETIME(AChel, Kills);
	DOREPLIFETIME(AChel, IsEnableInput);
	DOREPLIFETIME(AChel, DoesHave);
	DOREPLIFETIME(AChel, KeysCount);
	DOREPLIFETIME(AChel, bCanWalkingAndWatching);
}
//-----------------------------

//NicknameSetup----------------
void AChel::DeliverNicknameToServer_Implementation(const FText& newNickName) {
	GS->NickNames[Index] = newNickName;
}

bool AChel::DeliverNicknameToServer_Validate(const FText& newNickName) {
	return true;
}
// Called to bind functionality to input
void AChel::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("GoForward", this, &AChel::GoForward);
	PlayerInputComponent->BindAxis("GoRight", this, &AChel::GoRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AChel::LookUp);
	PlayerInputComponent->BindAxis("LookRight", this, &AChel::LookRight);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AChel::MyJump);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AChel::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AChel::StopSprint);
	PlayerInputComponent->BindAction("KillFeed", IE_Pressed, this, &AChel::ShowKillFeed);
	PlayerInputComponent->BindAction("KillFeed", IE_Released, this, &AChel::UnShowKillFeed);
	PlayerInputComponent->BindAction("ThrowStone", IE_Pressed, this, &AChel::ThrowStone);
	PlayerInputComponent->BindAction("PickUp", IE_Pressed, this, &AChel::PickUp);
	PlayerInputComponent->BindAction("PickUp", IE_Released, this, &AChel::PickUp_Released);
	PlayerInputComponent->BindAction("Opening", IE_Pressed, this, &AChel::OpenAreaPressed);
	PlayerInputComponent->BindAction("Opening", IE_Released, this, &AChel::OpenAreaReleased);
	PlayerInputComponent->BindAction("UpdateSpectating_Left", IE_Released, this, &AChel::UpdateSpectating_Left);
	PlayerInputComponent->BindAction("UpdateSpectating_Right", IE_Released, this, &AChel::UpdateSpectating_Right);
}

void AChel::UpdateSpectating_Right()
{
	if (!IsEnableInput)
	{
		do
		{
			WebCamIterator += 1;
			if (WebCamIterator >= GS->WebCam_IsEnabled.Num())
			{
				WebCamIterator -= GS->WebCam_IsEnabled.Num();
			}
			UE_LOG(LogTemp, Warning, TEXT("WebCamIterator - %d"), WebCamIterator);
			UE_LOG(LogTemp, Warning, TEXT("Count - %d"), GS->WebCam_IsEnabled.Num());
		} while (!GS->WebCam_IsEnabled[WebCamIterator]);
		PoseableMeshComp->SetWorldRotation(GS->WebCam_Rotation[WebCamIterator]);
		PoseableMeshComp->SetWorldLocation(GS->WebCam_Location[WebCamIterator]);
		PoseableMeshComp->SetBoneRotationByName(TEXT("Bone_002"), { 0, 0, 0 }, EBoneSpaces::ComponentSpace);
		GoToWebCamServer(WebCamIterator);
	}
}

void AChel::UpdateSpectating_Left()
{
	if (!IsEnableInput)
	{
		do
		{
			WebCamIterator -= 1;
			if (WebCamIterator < 0)
			{
				WebCamIterator += GS->WebCam_IsEnabled.Num();
			}
			UE_LOG(LogTemp, Warning, TEXT("WebCamIterator - %d"), WebCamIterator);
			UE_LOG(LogTemp, Warning, TEXT("Count - %d"), GS->WebCam_IsEnabled.Num());
		} while (!GS->WebCam_IsEnabled[WebCamIterator]);
		PoseableMeshComp->SetWorldRotation(GS->WebCam_Rotation[WebCamIterator]);
		PoseableMeshComp->SetWorldLocation(GS->WebCam_Location[WebCamIterator]);
		PoseableMeshComp->SetBoneRotationByName(TEXT("Bone_002"), { 0, 0, 0 }, EBoneSpaces::ComponentSpace);
		GoToWebCamServer(WebCamIterator);
	}
}

void AChel::OpenAreaPressed() 
{
	if (AreaCode != -1)
	{
		IsSuccessOpening = true;
		if (AreaCode <= 2 && AreaCode >= 0) {
			if (!GS->AreaAvaliables[AreaCode] && DoesHave[AreaCode]) {
				UserView->PB_Opening->SetVisibility(ESlateVisibility::Visible);
				UserView->TimeLeft->SetVisibility(ESlateVisibility::Visible);
			}
		}
		switch (AreaCode)
		{
		case Boltorez:
		{
			if (GS->AreaAvaliables[AreaCode])
			{
				PlayerEscape(AreaCode);
				UserView->RemoveFromParent();
			}
			else if (DoesHave[AreaCode])
			{
				UserView->PlayAnimation(UserView->CanalizaciaAnim);
			}
			break;
		}
		case KeyShelter:
		{
			if (GS->AreaAvaliables[AreaCode])
			{
				PlayerEscape(AreaCode);
				UserView->RemoveFromParent();
			}
			else if (DoesHave[AreaCode])
			{
				UserView->PlayAnimation(UserView->ShelterAnim);
			}
			break;
		}
		case Otvertka:
		{
			if (GS->AreaAvaliables[AreaCode])
			{
				PlayerEscape(AreaCode);
				UserView->RemoveFromParent();
			}
			else if (DoesHave[AreaCode])
			{
				UserView->PlayAnimation(UserView->VentilaciaAnim);
			}
			break;
		}
		case GeneratorArea:
		{
			if (GenAreaObj) {
				if (GenAreaObj->IsAvalible) {
					if (GeneratorView->IsVisible()) {
						if (GeneratorView->Corretca->Value <= GeneratorView->PB_Repair->Percent) {
							ChangeGeneratorStas();
						}
						else {
							GeneratorView->ChangeCorretcaPosition(GenAreaObj->Stadiya);
							OutlineBad_Server();//����� ����� ���� ���� ������ � �������� ����������� ������ ��� ���� ���������, �� ��� 12 ����� ����, ����, ��
						}
					}
					else {
						UserView->HoldText->SetVisibility(ESlateVisibility::Hidden);
						GeneratorView->ChangeCorretcaPosition(GenAreaObj->Stadiya);
						GeneratorView->SetVisibility(ESlateVisibility::Visible);
					}
				}
			}
			break;
		}
		}
	}
}

void AChel::OpenAreaReleased()
{
	if (AreaCode != -1) {
		if (IsSuccessOpening) {
			IsSuccessOpening = false;
			UserView->StopAllAnimations();
		}
	}
}

//TimelineAnimation
void AChel::OnTimelineFinished_Stone_First() {
	TimeLine_Stone_Second->ReverseFromEnd();

	if (IsServerAuth) {
		--Ammo;
		StoneCountUpdate(Ammo);

		FTransform trans;
		trans.SetLocation(FVector(GetActorLocation().X, GetActorLocation().Y, Stone->GetComponentLocation().Z));
		trans.SetRotation(FQuat(Stone->GetComponentRotation()));

		AStone* NewStone = World->SpawnActorDeferred<AStone>(StoneClass, trans, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		NewStone->Index = Index;
		UGameplayStatics::FinishSpawningActor(NewStone, trans);
		if (Ammo == 0) {
			HideStoneMulticast();
		}
	}
}

void AChel::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (GetLocalRole() == ROLE_Authority) {
		AStone* OverStone = Cast<AStone>(OtherActor);
		if (OverStone) {
			if (OverStone->Index != Index)
			{
				StoneAttack(OverStone->Index);
				TArray<AActor*>Players;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChel::StaticClass(), Players);
				for (int i = 0; i < Players.Num(); i++) {
					AChel* Chel = Cast<AChel>(Players[i]);
					if (Chel->Index == OverStone->Index)
					{
						Chel->AddHitMarker();
						break;
					}
				}
				OverStone->Destroy();
			}
		}
	}
}

void AChel::StoneCountUpdate_Implementation(int32 Count)
{
	UserView->AmmoLabel->SetText(FText::AsNumber(Count));
}

void AChel::TimelineVectorReturn_Stone(FVector value) {
	Stone->SetRelativeLocation(StonePosition + value);
}

void AChel::TimelineFloatReturn_FOV_WebCam(float value)
{
	CameraComp->SetFieldOfView(value);
}

void AChel::OnTimelineFinished_Stone_Second() {
	bIsAlreadyThrowing = false;
}
//-----------------------------

//AttackInput------------------
void AChel::ThrowStone() {
	if (Ammo > 0 && IsEnableInput) {
		if (!bIsAlreadyThrowing) {
			bIsAlreadyThrowing = true;

			ThrowStoneServer();
		}
	}
}

void AChel::ThrowStoneMulticast_Implementation()
{
	TimeLine_Stone_First->PlayFromStart();
}

void AChel::ThrowStoneServer_Implementation()
{
	ThrowStoneMulticast();
}

bool AChel::ThrowStoneServer_Validate()
{
	return true;
}

void AChel::HideStoneMulticast_Implementation() {
	Stone->SetHiddenInGame(true);
}

void AChel::ShowStoneMulticast_Implementation() {
	Stone->SetHiddenInGame(false);
}
//----------------------------

//KeyBoardInput----------------
void AChel::GoForward(float input) {
	if (bCanWalkingAndWatching) {
		if (input != 0.0f && IsEnableInput) {
			AddMovementInput(GetActorForwardVector(), input * MoveCoeff);
		}
	}
}

void AChel::GoRight(float input) {
	if (bCanWalkingAndWatching) {
		if (input != 0.0f && IsEnableInput) {
			AddMovementInput(GetActorRightVector(), input * MoveCoeff);
		}
	}
}
//-----------------------------

//ReplicationMouseInput--------
void AChel::MeshCompRepMulticast_Implementation(float RotationRoll)
{
	PoseableMeshComp->SetBoneRotationByName(TEXT("Bone_002"), { 0, 0, RotationRoll }, EBoneSpaces::ComponentSpace);
}


void AChel::MeshCompRepServer_Implementation(float RotationRoll)
{
	MeshCompRepMulticast(RotationRoll);
}

bool AChel::MeshCompRepServer_Validate(float RotationRoll)
{
	return true;
}
//-----------------------------

//MouseInput-------------------
void AChel::LookUp(float input)
{
	if (bCanWalkingAndWatching) {
		if (IsEnableInput) {
			if (input != 0.0f)
			{
				input *= Sensetivity;
				float RotationRoll = PoseableMeshComp->GetBoneRotationByName(TEXT("Bone_002"), EBoneSpaces::ComponentSpace).Roll;
				if (RotationRoll + input >= 0.f && RotationRoll + input <= 179.f) {
					RotationRoll += input;
				}
				else if (RotationRoll + input > 179.f) {
					RotationRoll = 179.f;
				}
				else
					RotationRoll = 0;
				MeshCompRepServer(RotationRoll);
			}
		}
		else
		{
			if (input != 0.0f) {
				input *= Sensetivity * WebCamSensetivity;
				float NewPitchRot = CameraComp->GetRelativeRotation().Pitch;

				NewPitchRot = FMath::Clamp<float>(NewPitchRot + input, -MaxPitchAngle, MaxPitchAngle);

				CameraComp->SetRelativeRotation(FRotator(NewPitchRot,
					CameraComp->GetRelativeRotation().Yaw, CameraComp->GetRelativeRotation().Roll));
			}
		}
	}
}


void AChel::LookRight(float input)
{
	if (bCanWalkingAndWatching) {
		if (IsEnableInput) {
			if (input != 0.0f) {
				input *= Sensetivity;
				AddControllerYawInput(input);
			}
		}
		else
		{
			if (input != 0.0f) {
				input *= Sensetivity * WebCamSensetivity;
				float NewYawRot = CameraComp->GetRelativeRotation().Yaw;

				NewYawRot = FMath::Clamp<float>(NewYawRot + input, -MaxYawAngle, MaxYawAngle);

				CameraComp->SetRelativeRotation(FRotator(CameraComp->GetRelativeRotation().Pitch, NewYawRot,
					CameraComp->GetRelativeRotation().Roll));
			}
		}
	}
}
//-----------------------------

//Jump-------------------------
void AChel::MyJump()
{
	if (bCanWalkingAndWatching) {
		if (IsEnableInput)
			Jump();
	}
}
//-----------------------------


//Sprint-----------------------
void AChel::StartSprint_Server_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = 1200.f;
}

bool AChel::StartSprint_Server_Validate()
{
	return true;
}

void AChel::StartSprint() {
	GetCharacterMovement()->MaxWalkSpeed = 1200.f;
	StartSprint_Server();
}

void AChel::StopSprint_Server_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = 800.f;
}

bool AChel::StopSprint_Server_Validate()
{
	return true;
}

void AChel::StopSprint() {
	GetCharacterMovement()->MaxWalkSpeed = 800.f;
	StopSprint_Server();
}
//-----------------------------


//KillFeed --------------------
void AChel::ShowKillFeed()
{
	KillFeed->SetVisibility(ESlateVisibility::Visible);
}

void AChel::UnShowKillFeed()
{
	KillFeed->SetVisibility(ESlateVisibility::Hidden);
}

//PlayStartingAnimation---------------------
void AChel::PlaySpawnAnimationSleep_Implementation() {
	UserView->PlayAnimation(UserView->Shading);
}

//PlayStartingAnimation---------------------
void AChel::PlaySpawnAnimationAwake_Implementation() {
	PoseableMeshComp->SetRelativeTransform(MeshTrans);
	UserView->SetVisibility(ESlateVisibility::Visible);
	UserView->PlayAnimation(UserView->Shading, 0, 1, EUMGSequencePlayMode::Type::Reverse);
}

//-----------------------------
void AChel::PickUp() {
	if (bCanWalkingAndWatching) {
		if (IsEnableInput) {
			if (ItemCodePickUp != -1) {
				IsSuccessOpening = true;
				switch (ItemCodePickUp) {
				case Boltorez:
				{
					UserView->WS_Boltorez->SetActiveWidgetIndex(1);
					DoesHave_Owner = true;
					NewHaveItemServer(ItemCodePickUp);
					break;
				}
				case KeyShelter:
				{
					UserView->WS_KeyShelter->SetActiveWidgetIndex(1);
					DoesHave_Owner = true;
					NewHaveItemServer(ItemCodePickUp);
					break;
				}
				case Otvertka:
				{
					UserView->WS_Otvertka->SetActiveWidgetIndex(1);
					DoesHave_Owner = true;
					NewHaveItemServer(ItemCodePickUp);
					break;
				}
				case CacheKey:
				{
					int KeyType = Cast<ACache_Key>(LastItem)->KeyType;
					switch (KeyType)
					{
					case KeyBronze:
					{
						UserView->KeyLeft_Bronze->SetText(FText::AsNumber(KeysCount[KeyBronze] + 1));
						break;
					}
					case KeySilver:
					{
						UserView->KeyLeft_Silver->SetText(FText::AsNumber(KeysCount[KeySilver] + 1));
						break;
					}
					case KeyGold:
					{
						UserView->KeyLeft_Gold->SetText(FText::AsNumber(KeysCount[KeyGold] + 1));
						break;
					}
					}
					KeysCount[KeyType]++;
					DoesHave_Owner = true;
					NewHaveItemServer(ItemCodePickUp);
					break;
				}
				case Cache:
				{
					if (LastCache != nullptr) {
						if (KeysCount[LastCache->CacheType] > 0)
						{
							switch (LastCache->CacheType)
							{
							case KeyBronze:
							{
								UserView->KeyLeft_Bronze->SetText(FText::AsNumber(KeysCount[KeyBronze] - 1));
								break;
							}
							case KeySilver:
							{
								UserView->KeyLeft_Silver->SetText(FText::AsNumber(KeysCount[KeySilver] - 1));
								break;
							}
							case KeyGold:
							{
								UserView->KeyLeft_Gold->SetText(FText::AsNumber(KeysCount[KeyGold] - 1));
								break;
							}
							}
							KeysCount[LastCache->CacheType]--;
							UserView->E_Mark->SetVisibility(ESlateVisibility::Hidden);
							ChangeIsAvaliableCache();
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("LastCache is nullptr"))
					}
					break;

				}
				case CanalizationButton:
				{
					UserView->E_Mark->SetVisibility(ESlateVisibility::Hidden);
					ChangeButtonCount_Server();
					break;
				}
				case WebCamLocker:
				{
					LockWebCam_Server();
					break;
				}
				case InvisiblePotion:
				{
					AddInvisibleServer();
					break;
				}
				case CodeNote:
				{
					if (Widget_Note->IsVisible())
					{
						Widget_Note->SetVisibility(ESlateVisibility::Hidden);
						bCanWalkingAndWatching = true;
					}
					else
					{
						Widget_Note->ChangeText(GS->CodeGenerator);
						Widget_Note->SetVisibility(ESlateVisibility::Visible);
						bCanWalkingAndWatching = false;
					}
					break;
				}
				case ClickButton:
				{
					AClickButton* CurButton = Cast<AClickButton>(LastItem);
					if (GS->IsCodeTerminalAvaliable && !GS->ButtonPlayAnim && CurButton)
					{
						if (CurButton->ButtonType <= 9) {
							if (GS->NumbersOnPanel.Num() <= 4) {
								AddNumToTerminalServer(CurButton->ButtonType);
								ButtonPressAnimationServer();
							}
						}
						else {
							if (CurButton->ButtonType == 10) {
								if (GS->NumbersOnPanel.Num() > 0) {
									DeleteLAstNumServer();
									ButtonPressAnimationServer();
								}
							}
							else
							{
								CheckCodeServer();
								ButtonPressAnimationServer();
							}
						}
					}
					break;
				}
				case OpenArea:
				{

					AOpenArea* CurOpArea = Cast<AOpenArea>(LastItem);
					if (CurOpArea != nullptr && CurOpArea->bIsAvaliable) {
						UserView->PB_Opening->SetVisibility(ESlateVisibility::Visible);
						UserView->TimeLeft->SetVisibility(ESlateVisibility::Visible);
					}
					UserView->PlayAnimation(UserView->OpenAreaAnim);
					break;
				}
				}
			}
		}
		else
		{
			TimeLine_FOV_WebCam->Play();
		}
	}
}

void AChel::PickUp_Released()
{
	if (bCanWalkingAndWatching) {
		if (IsEnableInput) {
			IsSuccessOpening = false;
			UserView->StopAllAnimations();
		}
		else
		{
			TimeLine_FOV_WebCam->Reverse();
		}
	}
}

void AChel::ChangeIsAvaliableCache_Implementation()
{
	FHitResult OutHit;

	FVector StartLocation = CameraComp->GetComponentLocation();
	FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

	FCollisionQueryParams CollisionParams;

	World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams);
	if (OutHit.GetActor()) {
		ACache* TempItem = Cast<ACache>(OutHit.GetActor());
		if (TempItem)
		{
			TempItem->IsEnabled = false;
			TempItem->Opening();
			return;
		}
	}
}

bool AChel::ChangeIsAvaliableCache_Validate()
{
	return true;
}

void AChel::NewHaveItemServer_Implementation(int32 ItemType)
{
	if (ItemType >= 0 && ItemType <= 2) {
		DoesHave[ItemType] = true;
		TArray<AActor*>Players;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChel::StaticClass(), Players);
		for (auto Player : Players)
			Cast<AChel>(Player)->NewHaveItemClient(ItemType);
	}

	FHitResult OutHit;

	FVector StartLocation = CameraComp->GetComponentLocation();
	FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

	FCollisionQueryParams CollisionParams;

	World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams);
	if (OutHit.GetActor()) {
		APickableItem* TempItem = Cast<APickableItem>(OutHit.GetActor());
		if (TempItem)
		{
			ACache_Key* CacheKey = Cast<ACache_Key>(TempItem);
			if (CacheKey)
			{
				CacheKey->RemoveAndRefreshTimer();
			}
			else
			{
				TempItem->Destroy();
			}
		}
	}
	
}

bool AChel::NewHaveItemServer_Validate(int32 ItemType)
{
	return true;
}

void AChel::NewHaveItemClient_Implementation(int32 ItemType)
{
	if (!DoesHave_Owner)
		switch (ItemType) {
		case Boltorez:
		{
			UserView->WS_Boltorez->SetRenderOpacity(0.5f);
			break;
		}
		case KeyShelter:
		{
			UserView->WS_KeyShelter->SetRenderOpacity(0.5f);
			break;
		}
		case Otvertka:
		{
			UserView->WS_Otvertka->SetRenderOpacity(0.5f);
			break;
		}
		default:
			break;
		}
	DoesHave_Owner = false;
}

void AChel::AddHitMarker_Implementation()
{
	UserView->Marker->SetVisibility(ESlateVisibility::Visible);
	FTimerHandle FuzeTimerHandle;
	World->GetTimerManager().SetTimer(FuzeTimerHandle, this, &AChel::RemoveHitMarker, 0.05, false);
}

void AChel::RemoveHitMarker()
{
	UserView->Marker->SetVisibility(ESlateVisibility::Hidden);
}

void AChel::StoneAttack(int StoneIndex)
{
	if (IsInGame) 
	{
		if (STONE_DAMAGE + Health + DeltaRadiation >= 1.0f)
		{
			KillerIndex = StoneIndex;
		}

		Health += STONE_DAMAGE;

		if (Health >= 1.0f)
		{

			if (DoesHave[Boltorez])
				GS->CurrentBoltorez--;
			if (DoesHave[KeyShelter])
				GS->CurrentKeyShelter--;
			if (DoesHave[Otvertka])
				GS->CurrentOtvertka--;

			TArray<AActor*>Players;
			UGameplayStatics::GetAllActorsOfClass(World, StaticClass(), Players);
			for (auto& Player : Players)
					Cast<AChel>(Player)->RefreshWidgets(DoesHave, KillerIndex, Index);
			DoesHave.Init(false, 3);
			bCanWalkingAndWatching = true;
			KillPlayer();
		}
	}
}

void AChel::RefreshWidgets_Implementation(const TArray<bool>& whatToUpdate, int KillerNickIndex, int VictimNickIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("WidgetUpdates"))
	if (whatToUpdate[Boltorez])
	{
		UserView->WS_Boltorez->SetActiveWidgetIndex(0);
		UserView->WS_Boltorez->SetRenderOpacity(1.f);
	}
	if (whatToUpdate[Otvertka])
	{
		UserView->WS_Otvertka->SetActiveWidgetIndex(0);
		UserView->WS_Otvertka->SetRenderOpacity(1.f);
	}
	if (whatToUpdate[KeyShelter])
	{
		UserView->WS_KeyShelter->SetActiveWidgetIndex(0);
		UserView->WS_KeyShelter->SetRenderOpacity(1.f);
	}

	if (KillerNickIndex == -1) {
		UPlayerSuicide* PS_Widget = Cast<UPlayerSuicide>(CreateWidget(World, PlayerSuicide_class));
		PS_Widget->Player->SetText(GS->NickNames[VictimNickIndex]);
		KillFeed->VB_KillFeed->AddChild(PS_Widget);
	}
	else
	{
		UPlayerKillPlayer* PKP_Widget = Cast<UPlayerKillPlayer>(CreateWidget(World, PlayerKillPlayer_class));
		PKP_Widget->Killer->SetText(GS->NickNames[KillerNickIndex]);
		PKP_Widget->Victim->SetText(GS->NickNames[VictimNickIndex]);
		KillFeed->VB_KillFeed->AddChild(PKP_Widget);
	}

	if (whatToUpdate[Boltorez] || whatToUpdate[Otvertka] || whatToUpdate[KeyShelter])
	{
		UPlayerLostItem* PLI_Widget = Cast<UPlayerLostItem>(CreateWidget(World, PlayerLostItem_class));
		PLI_Widget->Player->SetText(GS->NickNames[VictimNickIndex]);
		if (whatToUpdate[Boltorez])
			PLI_Widget->IMG_Boltorez->SetVisibility(ESlateVisibility::Visible);
		if (whatToUpdate[Otvertka])
			PLI_Widget->IMG_Otvertka->SetVisibility(ESlateVisibility::Visible);
		if (whatToUpdate[KeyShelter])
			PLI_Widget->IMG_KeyShelter->SetVisibility(ESlateVisibility::Visible);

		KillFeed->VB_KillFeed->AddChild(PLI_Widget);
	}

}

void AChel::KillPlayer()
{
	GS->Deaths[Index] = FText::AsNumber(++Death);
	TArray<AActor*>Players;
	UGameplayStatics::GetAllActorsOfClass(World, AChel::StaticClass(), Players);
	for (auto Player : Players)
	{
		AChel* Chel = Cast<AChel>(Player);
		if (Chel->Index == KillerIndex)
		{
			GS->Kills[KillerIndex] = FText::AsNumber(++(Chel->Kills));
			break;
		}
	}
	DisableCollisionEverywhere();
	SetActorHiddenInGame(true);
	IsInGame = false;
	PlaySpawnAnimationSleep();
	IsEnableInput = false;

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, this, &AChel::SpawnPlayer, SPAWN_TIME, false);
}

void AChel::GoToWebCam()
{
	int32 Iterator = FMath::Rand() % GS->WebCam_IsEnabled.Num();

	while (!GS->WebCam_IsEnabled[Iterator])
	{
		Iterator += 1;

		if (Iterator >= GS->WebCam_IsEnabled.Num())
		{
			Iterator -= GS->WebCam_IsEnabled.Num();
		}
	}
	PoseableMeshComp->SetWorldRotation(GS->WebCam_Rotation[Iterator]);
	PoseableMeshComp->SetWorldLocation(GS->WebCam_Location[Iterator]);
	PoseableMeshComp->SetBoneRotationByName(TEXT("Bone_002"), { 0, 0, 0 }, EBoneSpaces::ComponentSpace);
	WebCamIterator = Iterator;
	GoToWebCamServer(Iterator);
}

void AChel::GoToWebCamServer_Implementation(int32 Iterator)
{
	PoseableMeshComp->SetWorldRotation(GS->WebCam_Rotation[Iterator]);
	PoseableMeshComp->SetWorldLocation(GS->WebCam_Location[Iterator]);
	PoseableMeshComp->SetBoneRotationByName(TEXT("Bone_002"), { 0, 0, 0 }, EBoneSpaces::ComponentSpace);
	UE_LOG(LogTemp, Warning, TEXT("Staying on webcam"));
}

bool AChel::GoToWebCamServer_Validate(int32 Iterator)
{
	return true;
}

void AChel::UpdatePositionClient_Implementation(FTransform NewTrans)
{
	PoseableMeshComp->SetWorldTransform(NewTrans);
}

void AChel::SpawnPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("Setting camera from webcam to player"));
	PoseableMeshComp->SetRelativeTransform(MeshTrans);
	CameraComp->SetRelativeLocation( { 0,0,0 } );
	CameraComp->SetRelativeRotation( { 0,0,0 } );

	CameraComp->SetFieldOfView(90.0f);

	EnableCollisionEverywhere();
	SetActorHiddenInGame(false);
	IsEnableInput = true;
	IsInGame = true;
	Health = 0;
	Ammo = 10;

	TArray<AActor*> TargetPoints;
	UGameplayStatics::GetAllActorsOfClassWithTag(World, ATargetPoint::StaticClass(), TEXT("spawn"), TargetPoints);
	SetActorLocation(TargetPoints[FMath::Rand() % 4]->GetActorLocation());

	PlaySpawnAnimationAwake();
}

void AChel::DisableCollisionEverywhere_Implementation()
{
	SetActorEnableCollision(false);
	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->StopMovementImmediately();
}

void AChel::EnableCollisionEverywhere_Implementation()
{
	SetActorEnableCollision(true);
	GetCharacterMovement()->GravityScale = 1.2f;
	GetCharacterMovement()->StopMovementImmediately();
}

void AChel::PlayerOpenAreaUpdate_Implementation(int32 EscapeWay)
{
	TArray<AActor*>Players;
	UGameplayStatics::GetAllActorsOfClass(World, AChel::StaticClass(), Players);
	for (auto Player : Players) {
		AChel* Chel = Cast<AChel>(Player);
		Chel->RefreshWidgets_Winner(EscapeWay);
	}

	TArray<AActor*>Chels;
	GS->Areas[EscapeWay]->GetOverlappingActors(Chels);
	for (auto Player : Chels)
	{
		Cast<AChel>(Player)->ExitAvaliableUpdate(EscapeWay);
	}
}

bool AChel::PlayerOpenAreaUpdate_Validate(int32 EscapeWay)
{
	return true;
}

void AChel::RefreshWidgets_Winner_Implementation(int32 EscapeWay)
{
	switch (EscapeWay)
	{
	case Boltorez:
	{
		UserView->WS_Boltorez->SetVisibility(ESlateVisibility::Hidden);
		break;
	}
	case Otvertka:
	{
		UserView->WS_Otvertka->SetVisibility(ESlateVisibility::Hidden);
		break;
	}
	case KeyShelter:
	{
		UserView->WS_KeyShelter->SetVisibility(ESlateVisibility::Hidden);
		break;
	}
	}
}

void AChel::StuffAvaliableUpdate_Implementation(int32 EscapeWay)
{
	GS->AreaAvaliables[EscapeWay] = true;
}

bool AChel::StuffAvaliableUpdate_Validate(int32 EscapeWay)
{
	return true;
}

void AChel::ExitAvaliableUpdate_Implementation(int32 EscapeWay)
{
	UserView->EscapeText->SetVisibility(ESlateVisibility::Visible);
}

void AChel::PlayerEscape_Implementation(int32 EscapeWay)
{
	GS->AreaClosed[EscapeWay] = true;
	GS->Winners[EscapeWay] = GS->NickNames[Index];
	GS->EscapeTime[EscapeWay] = GS->CurrentTime;

	if (GS->GeneralLayer == 2) {
		TArray<AActor*>PlayerControllers;
		TArray<AActor*>Players;
		TArray<AActor*>Spectators;

		UGameplayStatics::GetAllActorsOfClass(World, ABP_PlayerController::StaticClass(), PlayerControllers);
		UGameplayStatics::GetAllActorsOfClass(World, AChel::StaticClass(), Players);
		UGameplayStatics::GetAllActorsOfClass(World, ASpectator::StaticClass(), Spectators);

		for (auto PlController : PlayerControllers)
		{
			Cast<ABP_PlayerController>(PlController)->AddFinalMenu();
		}

		for (auto Player : Players) {
			Cast<AChel>(Player)->DeleteGameHUD();
			Player->Destroy();
		}

		for (auto Spec : Spectators) {
			Spec->Destroy();
		}
	}
	else
	{
		TArray<AActor*>Chels;
		GS->Areas[EscapeWay]->GetOverlappingActors(Chels);
		for (auto Player : Chels)
		{
			Cast<AChel>(Player)->AreaClosedUpdate(EscapeWay);
		}

		PossessToSpectator();
	}
}

bool AChel::PlayerEscape_Validate(int32 EscapeWay)
{
	return true;
}

void AChel::AreaClosedUpdate_Implementation(int32 EscapeWay)
{
	AreaCode = -1;
	UserView->EscapeText->SetVisibility(ESlateVisibility::Hidden);
}


void AChel::PossessToSpectator()
{
	int iteration = 0;
	for (iteration = 0; iteration < 8; iteration += 2)
	{
		if (GS->Spectators[iteration]->Index == Index)
		{
			if (GS->Spectators[iteration]->IsPawnControlled())
				GS->Spectators[iteration]->UpdateSpectating_Right();
			if (GS->Spectators[iteration + 1]->IsPawnControlled())
				GS->Spectators[iteration + 1]->UpdateSpectating_Right();

			GS->Spectators[iteration]->IsKilled = true;
			GS->Spectators[iteration + 1]->IsKilled = true;
			break;
		}
	}

	iteration += 2;
	iteration += GS->GeneralLayer;
	GS->GeneralLayer++;
	while (true)
	{
		if (iteration >= 8)
		{
			iteration -= 8;
		}
		if (!GS->Spectators[iteration]->IsKilled)
		{
			GetController()->Possess(GS->Spectators[iteration]);
			Destroy();
			break;
		}
		iteration += 2;
	}
}

void AChel::ChangeGeneratorStas_Implementation()
{
	
	GenAreaObj->Stadiya++;
	if (GenAreaObj->Stadiya >= 3) {
		GenAreaObj->Stadiya = 0;
		GenAreaObj->DoSomethinkGen();
		ShowRandomItem();

		TArray<AActor*> Players;
		GenAreaObj->GetOverlappingActors(Players, AChel::StaticClass());
		for (auto& it : Players) {
			Cast<AChel>(it)->HideWidgetStas();
			Cast<AChel>(it)->ChangeCorretca_Client(0);
		}
		GS->EventSpawnNote();
	}
	else {
		TArray<AActor*> Players;
		GenAreaObj->GetOverlappingActors(Players, AChel::StaticClass());
		for (auto& it: Players) {
			Cast<AChel>(it)->ChangeCorretca_Client(GenAreaObj->Stadiya);
		}
	}
}

bool AChel::ChangeGeneratorStas_Validate()
{
	return true;
}

void AChel::CallDoThomethinkArea_Implementation()
{
	Cast<AOpenArea>(LastItem)->DoSomethink();
}
bool AChel::CallDoThomethinkArea_Validate()
{
	return true;
}
void AChel::DeleteGameHUD_Implementation()
{
	UserView->RemoveFromParent();
	UserView->Destruct();

	KillFeed->RemoveFromParent();
	KillFeed->Destruct();
	
	Destroy();
}

void AChel::AddDoubleRadiationWidget_Implementation()
{
	UserView->AddDoubleRadiationEffect();
}

void AChel::DisableDoubleRadiationWidget_Implementation()
{
	UserView->DisableDoubleRadiationEffect();
}

void AChel::ChangeCorretca_Client_Implementation(int32 ValueV)
{
	GeneratorView->ChangeCorretcaPosition(ValueV);
}

void AChel::HideWidgetStas_Implementation()
{
	UserView->AreaUsedText->SetVisibility(ESlateVisibility::Visible);
	GeneratorView->SetVisibility(ESlateVisibility::Hidden);
}

void AChel::ChangeButtonCount_Server_Implementation()
{
	FHitResult OutHit;

	FVector StartLocation = CameraComp->GetComponentLocation();
	FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

	FCollisionQueryParams CollisionParams;

	World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams);
	if (OutHit.GetActor()) {
		ACanalizationButton* TempItem = Cast<ACanalizationButton>(OutHit.GetActor());
		if (TempItem)
		{
			if (!TempItem->DoesRefresh)
			{
				TempItem->Open();
				GS->CurrentButtonCount++;
				if (GS->CurrentButtonCount == 3)
				{
					TArray<AActor*>CanalAreas;
					UGameplayStatics::GetAllActorsOfClass(World, ACanalizationDamageCollision::StaticClass(), CanalAreas);
					if (CanalAreas.Num() != 0)
					{
						Cast<ACanalizationDamageCollision>(CanalAreas[0])->StopRadiation();
					}
				}
			}
		}
	}
}

bool AChel::ChangeButtonCount_Server_Validate()
{
	return true;
}

void AChel::RefreshOutline()
{
	PoseableMeshComp->SetCustomDepthStencilValue(0);
}

void AChel::OutlineBad_Multicast_Implementation()
{
	PoseableMeshComp->SetCustomDepthStencilValue(1);

	FTimerHandle FuzeTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(FuzeTimerHandle, this, &AChel::RefreshOutline, 2, false);
}


void AChel::OutlineBad_Server_Implementation()
{
	OutlineBad_Multicast();
}

bool AChel::OutlineBad_Server_Validate()
{
	return true;
}

void AChel::ShowRandomItem_Implementation() {
	TArray<AActor*> Items;
	UGameplayStatics::GetAllActorsOfClass(World, APickableItem::StaticClass(), Items);
	TArray<APickableItem*> ImportantItems;
	ImportantItems.Init(nullptr, 0);
	for (auto& it : Items) 
	{
		if (Cast<APickableItem>(it)->Type <= 2) 
		{
			ImportantItems.Add(Cast<APickableItem>(it));
		}
	}
	if (ImportantItems.Num() != 0)
	{
		LastOutlineItem = ImportantItems[FMath::Rand() % ImportantItems.Num()];
		LastOutlineItem->Item->SetCustomDepthStencilValue(2);

		FTimerHandle FuzeTimerHandle;
		World->GetTimerManager().SetTimer(FuzeTimerHandle, this, &AChel::HideRandomItem, 5, false);

	}
}

void AChel::HideRandomItem() {
	if (LastOutlineItem) 
	{
		LastOutlineItem->Item->SetCustomDepthStencilValue(0);
		LastOutlineItem = nullptr;
	}
}

void AChel::LockWebCam_Server_Implementation()
{
	FHitResult OutHit;

	FVector StartLocation = CameraComp->GetComponentLocation();
	FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

	FCollisionQueryParams CollisionParams;

	World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams);
	if (OutHit.GetActor()) {
		AWebCamLocker* TempItem = Cast<AWebCamLocker>(OutHit.GetActor());
		if (TempItem)
		{
			if (TempItem->DoesLock)
			{
				TempItem->Close();
				TempItem->DoesLock = false;
				GS->WebCam_IsEnabled[TempItem->Index] = true;
			}
			else
			{
				TempItem->Open();
				TempItem->DoesLock = true;
				GS->WebCam_IsEnabled[TempItem->Index] = false;
			}
		}
	}
}

bool AChel::LockWebCam_Server_Validate()
{
	return true;
}

void AChel::AddInvisibleServer_Implementation()
{
	InvisibleEverywhere();
}

bool AChel::AddInvisibleServer_Validate()
{
	return true;
}

void AChel::HideNoteWidget_Implementation()
{
	Widget_Note->SetVisibility(ESlateVisibility::Hidden);
	bCanWalkingAndWatching = true;
}

void AChel::RefreshGeneratorArea_Implementation()
{
	UserView->AreaUsedText->SetVisibility(ESlateVisibility::Hidden);
	UserView->HoldText->SetVisibility(ESlateVisibility::Visible);
}

void AChel::AddNumToTerminalServer_Implementation(int32 ButtonType)
{
	UE_LOG(LogTemp, Warning, TEXT("%d"), ButtonType);
	GS->AddNumToTerminal(ButtonType);
}

bool AChel::AddNumToTerminalServer_Validate(int32 ButtonType)
{
	return true;
}

void AChel::DeleteLAstNumServer_Implementation()
{
	GS->DeleteLastNumber();
}

bool AChel::DeleteLAstNumServer_Validate()
{
	return true;
}

void AChel::CheckCodeServer_Implementation()
{
	GS->CheckCode(Index);
}

bool AChel::CheckCodeServer_Validate()
{
	return true;
}

void AChel::ButtonPressAnimationServer_Implementation() {
	FHitResult OutHit;

	FVector StartLocation = CameraComp->GetComponentLocation();
	FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

	FCollisionQueryParams CollisionParams;

	if (World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams))
	{		
		Cast<AClickButton>(OutHit.GetActor())->ButtonPressAnimation();
	}
	
}

bool AChel::ButtonPressAnimationServer_Validate()
{
	return true;
}

void AChel::DoTraceOpenArea_Implementation() 
{
	FHitResult OutHit;

	FVector StartLocation = CameraComp->GetComponentLocation();
	FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * 300;

	FCollisionQueryParams CollisionParams;

	if (World->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, CollisionParams))
	{
		LastItem = Cast<APickableItem>(OutHit.GetActor());
	}

}

bool AChel::DoTraceOpenArea_Validate() 
{
	return true;
}