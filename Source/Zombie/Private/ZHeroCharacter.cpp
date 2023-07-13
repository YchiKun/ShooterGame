// Fill out your copyright notice in the Description page of Project Settings.


#include "ZHeroCharacter.h"

#include "Ammo.h"
#include "BulletHitInterface.h"
#include "Enemy.h"
#include "EnemyController.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Item.h"
#include "Weapon.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Zombie/Zombie.h"

// Sets default values
AZHeroCharacter::AZHeroCharacter():
BaseTurnRate(45.0f), BaseLookUpRate(45.0f),
HipTurnRate(90.0f), HipLookUpRate(90.0f), AimingTurnRate(20.0f), AimingLookUpRate(20.0f),
MouseHipTurnRate(1.0f), MouseHipLookUpRate(1.0f), MouseAimingTurnRate(0.2f), MouseAimingLookUpRate(0.2f),
bAiming(false),
CameraDefaultFOV(0.0f), CameraZoomedFOV(25.0f), CameraCurrentFOV(0.0f), ZoomInterpSpeed(20.0f),
CrosshairSpreadMultiplier(0.0f), CrosshairVelocityFactor(0.0f),
CrosshairInAirFactor(0.0f), CrosshairAimFactor(0.0f), CrosshairShootingFactor(0.0f),
bFireButtonPressed(false), bShouldFire(true),
ShootTimeDuration(0.05f), bFiringBullet(false),
bShouldTraceForItems(false),
CameraInterpDistance(250.0f), CameraInterpElevation(65.0f),
Starting9mmAmmo(85), StartingARAmmo(120),
CombatState(ECombatState::ECS_Unoccupied),
bCrouching(false),
BaseMovementSpeed(650.0f), CrouchMovementSpeed(300.0f),
StandingCapsuleHalfHeight(88.0f), CrouchingCapsuleHalfHeight(44.0f),
BaseGroundFriction(2.0f), CrouchingGroundFriction(100.0f),
bAimingButtonPressed(false),
bShouldPlayPickupSound(true), bShouldPlayEquipSound(true),
PickupSoundResetTime(0.2f), EquipSoundResetTime(0.2f),
HighlightedSlot(-1),
Health(100.0f), MaxHealth(100.f),
StunChance(0.25f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 240.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.0f, 35.0f, 85.0f);
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f;;
	GetCharacterMovement()->AirControl = 0.2f;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());
	
	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComp1->SetupAttachment(GetFollowCamera());
	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComp2->SetupAttachment(GetFollowCamera());
	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComp3->SetupAttachment(GetFollowCamera());
	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComp4->SetupAttachment(GetFollowCamera());
	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComp5->SetupAttachment(GetFollowCamera());
	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComp6->SetupAttachment(GetFollowCamera());
}

float AZHeroCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if(Health - DamageAmount <= 0.0f)
	{
		Health = 0.0f;
		Die();

		auto EnemyController = Cast<AEnemyController>(EventInstigator);
		if(EnemyController)
		{
			EnemyController->GetBlackboardComponent()->SetValueAsBool(FName("CharapterDeath"), true);
		}
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AZHeroCharacter::Die()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void AZHeroCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if(PC)
	{
		DisableInput(PC);
	}
}

// Called when the game starts or when spawned
void AZHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);
	EquippedWeapon->SetSlotIndex(0);
	EquippedWeapon->DisableCustomDepth();
	EquippedWeapon->DisableGlowMaterial();
	EquippedWeapon->SetCharatper(this);

	InitializeAmmoMap();
	
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

	InitializeInterpLocations();
}


// Called every frame
void AZHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraInterpZoom(DeltaTime);

	SetLookRates();

	CalculateCrosshairSpread(DeltaTime);

	TraceForItems();

	InterpCapsuleHalfHeight(DeltaTime);
}


void AZHeroCharacter::CameraInterpZoom(float DeltaTime)
{
	
	if(bAiming)
	{
		CameraCurrentFOV = FMath::FInterpTo(
			CameraCurrentFOV,
			CameraZoomedFOV,
			DeltaTime,
			ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(
			CameraCurrentFOV,
			CameraDefaultFOV,
			DeltaTime,
			ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AZHeroCharacter::SetLookRates()
{
	if(bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AZHeroCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2d WalkSpeedRange{ 0.0f, 600.0f};
	FVector2d VelocityMultiplierRange {0.0f, 1.0f};
	FVector Velocity{ GetVelocity()};
	Velocity.Z = 0.0f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange,
		VelocityMultiplierRange,
		Velocity.Size());

	if(GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.0f, DeltaTime, 30.0f);
	}

	if(bAiming)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.0f);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.0f);
	}

	if(bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.0f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 60.0f);
	}
	
	CrosshairSpreadMultiplier = 0.5f +
		CrosshairVelocityFactor +
			CrosshairInAirFactor -
				CrosshairAimFactor +
					CrosshairShootingFactor;
}

void AZHeroCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(
		CrosshairShootTimer,
		this,
		&AZHeroCharacter::FinishCrosshairBulletFire,
		ShootTimeDuration);
}

void AZHeroCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AZHeroCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void AZHeroCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AZHeroCharacter::StartFireTimer()
{
	if(EquippedWeapon == nullptr) return;
	
	CombatState = ECombatState::ECS_FireTimerInProgress;
	
	GetWorldTimerManager().SetTimer(
		AutoFireTimer,
		this,
		&AZHeroCharacter::AutoFireReset,
		EquippedWeapon->GetAutoFireRate());
}

void AZHeroCharacter::AutoFireReset()
{
	if(CombatState == ECombatState::ECS_Stunned) return;
	
	CombatState = ECombatState::ECS_Unoccupied;
	if(EquippedWeapon == nullptr) return;
	if(WeaponHasAmmo())
	{
		if(bFireButtonPressed && EquippedWeapon->GetAutomatic())
		{
			FireWeapon();
		}
	}
	else
	{
		ReloadWeapon();
	}
}

float AZHeroCharacter::GetCrosshairSpreadMultiplier() const
{
	return  CrosshairSpreadMultiplier;
}

void AZHeroCharacter::IncrementOvelappedItemCount(int8 Amount)
{
	if(OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

/*
FVector AZHeroCharacter::GetCameraIterpLocation()
{
	const FVector CameraWorldLocation{FollowCamera->GetComponentLocation()};
	const FVector CameraForward{FollowCamera->GetForwardVector()};
	return  CameraWorldLocation + CameraForward * CameraInterpDistance
	+ FVector(0.0f, 0.0f, CameraInterpElevation);
	
}
*/

void AZHeroCharacter::GetPickupItem(AItem* Item)
{
	Item->PlayEquipSound();
	
	auto Weapon = Cast<AWeapon>(Item);
	if(Weapon)
	{
		if(Inventory.Num() < INVENTORY_CAPACITY)
		{
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else
		{
			SwapWeapon(Weapon);
		}
	}

	auto Ammo = Cast<AAmmo>(Item);
	if(Ammo)
	{
		PickupAmmo(Ammo);
	}
}

FInterpLocation AZHeroCharacter::GetInterpLocation(int32 Index)
{
	if(Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}


AWeapon* AZHeroCharacter::SpawnDefaultWeapon()
{
	if(DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return  nullptr;
}

void AZHeroCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwapping)
{
	if(WeaponToEquip)
	{		
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if(HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		if(EquippedWeapon == nullptr)
		{
			EquipItemDelefate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else if (!bSwapping)
		{
			EquipItemDelefate.Broadcast(
				EquippedWeapon->GetSlotIndex(),
				WeaponToEquip->GetSlotIndex());
		}
		
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AZHeroCharacter::DropWeapon()
{
	if(EquippedWeapon)
	{
		FDetachmentTransformRules DetachmetTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmetTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

// Called to bind functionality to input
void AZHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &AZHeroCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AZHeroCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AZHeroCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("TurnRate", this, &AZHeroCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AZHeroCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &AZHeroCharacter::Turn);
	
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this,
		&AZHeroCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this,
		&ACharacter::StopJumping);
	
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this,
		&AZHeroCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this,
		&AZHeroCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this,
		&AZHeroCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this,
		&AZHeroCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this,
		&AZHeroCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this,
		&AZHeroCharacter::SelectButtonReleased);
	
	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this,
		&AZHeroCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this,
		&AZHeroCharacter::CrouchButtonPressed);

	PlayerInputComponent->BindAction("FKey", IE_Pressed, this,
		&AZHeroCharacter::FKeyPressed);
	PlayerInputComponent->BindAction("1Key", IE_Pressed, this,
		&AZHeroCharacter::OneKeyPressed);
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this,
		&AZHeroCharacter::TwoKeyPressed);
	PlayerInputComponent->BindAction("3Key", IE_Pressed, this,
		&AZHeroCharacter::ThreeKeyPressed);
	PlayerInputComponent->BindAction("4Key", IE_Pressed, this,
		&AZHeroCharacter::FourKeyPressed);
	PlayerInputComponent->BindAction("5Key", IE_Pressed, this,
		&AZHeroCharacter::FiveKeyPressed);
}


void AZHeroCharacter::SelectButtonPressed()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if(TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this, true);
		TraceHitItem = nullptr;
	}
}

void AZHeroCharacter::SelectButtonReleased()
{
}

void AZHeroCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AZHeroCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if(CombatState != ECombatState::ECS_Reloading
		&& CombatState != ECombatState::ECS_Equipping
		&& CombatState != ECombatState::ECS_Stunned)
	{
		Aim();
	}
}

void AZHeroCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AZHeroCharacter::CrouchButtonPressed()
{
	if(!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}
	if(bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AZHeroCharacter::FKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 0) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AZHeroCharacter::OneKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 1) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AZHeroCharacter::TwoKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 2) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AZHeroCharacter::ThreeKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 3) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AZHeroCharacter::FourKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 4) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AZHeroCharacter::FiveKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 5) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AZHeroCharacter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex)
{
	if((CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		(CombatState == ECombatState::ECS_Unoccupied || CombatState == ECombatState::ECS_Equipping))
	{
		if(bAiming)
		{
			StopAiming();
		}
		auto OldEquippedWeapon = EquippedWeapon;
		auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
		EquipWeapon(NewWeapon);

		OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);
	
		CombatState = ECombatState::ECS_Equipping;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.0f);
			AnimInstance->Montage_JumpToSection(FName("Equip"));
		}
		NewWeapon->PlayEquipSound(true);
	}
}

int32 AZHeroCharacter::GetEmptyInventorySlot()
{
	for(int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] == nullptr)
		{
			return i;
		}
	}
	if(Inventory.Num() < INVENTORY_CAPACITY)
	{
		return Inventory.Num();
	}
	return -1;
}

void AZHeroCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot{GetEmptyInventorySlot()};
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

void AZHeroCharacter::UnHighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

void AZHeroCharacter::Stun()
{
	if(Health <= 0.0f) return;
	
	CombatState = ECombatState::ECS_Stunned;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

void AZHeroCharacter::Jump()
{
	if(bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		Super::Jump();
	}
}

EPhysicalSurface AZHeroCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start{GetActorLocation()};
	const FVector End {Start + FVector(0.0f, 0.0f, -400.0f)};
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start, End,
		ECollisionChannel::ECC_Visibility,
		QueryParams);
	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AZHeroCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if(bAimingButtonPressed)
	{
		Aim();
	}
}

void AZHeroCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight{};
	if(bCrouching)
	{
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else
	{
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}
	const float InterpHalfHeight{ FMath::FInterpTo(
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
		TargetCapsuleHalfHeight,
		DeltaTime,
		20.0f)};

	const float DeltaCapsuleHalfHeight{InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight()};

	const FVector MeshOffset{0.0f, 0.0f, -DeltaCapsuleHalfHeight};
	GetMesh()->AddLocalOffset(MeshOffset);
	
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AZHeroCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AZHeroCharacter::StopAiming()
{
	bAiming = false;
	if(!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AZHeroCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}

void AZHeroCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}


void AZHeroCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(
		PickupSoundTimer,
		this,
		&AZHeroCharacter::ResetPickupSoundTimer,
		PickupSoundResetTime);
}

void AZHeroCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(
		EquipSoundTimer,
		this,
		&AZHeroCharacter::ResetEquipSoundTimer,
		EquipSoundResetTime);
}

void AZHeroCharacter::PickupAmmo(AAmmo* Ammo)
{
	if(AmmoMap.Find(Ammo->GetAmmoType()))
	{
		int32 AmmoCount{AmmoMap[Ammo->GetAmmoType()]};
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
		
		if(EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
		{
			if(EquippedWeapon->GetAmmo() == 0)
			{
				ReloadWeapon();
			}
		}

		Ammo->Destroy();
	}
}

void AZHeroCharacter::InitializeInterpLocations()
{
	InterpLocations.Add(FInterpLocation{WeaponInterpComp, 0});
	InterpLocations.Add(FInterpLocation{InterpComp1, 0});
	InterpLocations.Add(FInterpLocation{InterpComp2, 0});
	InterpLocations.Add(FInterpLocation{InterpComp3, 0});
	InterpLocations.Add(FInterpLocation{InterpComp4, 0});
	InterpLocations.Add(FInterpLocation{InterpComp5, 0});
	InterpLocations.Add(FInterpLocation{InterpComp6, 0});
}

int32 AZHeroCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for(int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if(InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}
	
	return  LowestIndex;
}

void AZHeroCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if(Amount < -1 || Amount > 1) return;

	if (InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AZHeroCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{

	if(Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}
	
	DropWeapon();
	EquipWeapon(WeaponToSwap, true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AZHeroCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AZHeroCharacter::WeaponHasAmmo()
{
	if(EquippedWeapon == nullptr) return false;

	return EquippedWeapon->GetAmmo() > 0;
}

void AZHeroCharacter::TraceForItems()
{
	if(bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if(ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			
			const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if(TraceHitWeapon)
			{
				if(HighlightedSlot == -1)
				{
					HighlightInventorySlot();
				}
			}
			else
			{
				if(HighlightedSlot != -1)
				{
					UnHighlightInventorySlot();
				}
			}
			
			if(TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EqupInterping)
			{
				TraceHitItem = nullptr;
			}
			if(TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();

				if(Inventory.Num() >= INVENTORY_CAPACITY)
				{
					TraceHitItem->SetCharapterInventoryFull(true);
				}
				else
				{
					TraceHitItem->SetCharapterInventoryFull(false);
				}
			}

			if(TraceHitItemLastFrame)
			{
				if(TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

void AZHeroCharacter::TurnAtRate(float Value)
{
	AddControllerYawInput(Value * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AZHeroCharacter::LookUpAtRate(float Value)
{
	AddControllerPitchInput(Value * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AZHeroCharacter::Turn(float Value)
{
	float TurnSkaleFactor {0.0f};
	if(bAiming)
	{
		TurnSkaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnSkaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(Value * TurnSkaleFactor);
}

void AZHeroCharacter::LookUp(float Value)
{
	float LookUpSkaleFactor{0.0f};
	if(bAiming)
	{
		LookUpSkaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpSkaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value * LookUpSkaleFactor);
}


void AZHeroCharacter::MoveForward(float Value)
{
	if((Controller != nullptr) && (Value != 0.0f))
	{
		const FRotator Rotation{ Controller->GetControlRotation()};
		const FRotator YawRotation { 0, Rotation.Yaw, 0};

		const FVector Direction { FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X)};
		AddMovementInput(Direction, Value);
	}
}

void AZHeroCharacter::MoveRight(float Value)
{
	if((Controller != nullptr) && (Value != 0.0f))
	{
		const FRotator Rotation{ Controller->GetControlRotation()};
		const FRotator YawRotation { 0, Rotation.Yaw, 0};

		const FVector Direction { FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y)};
		AddMovementInput(Direction, Value);
	}
}


bool AZHeroCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
	UGameplayStatics::GetPlayerController(this, 0),
	CrosshairLocation,
	CrosshairWorldPosition,
	CrosshairWorldDirection);

	if(bScreenToWorld)
	{
		const FVector Start{CrosshairWorldPosition};
		const FVector End{Start + CrosshairWorldDirection * 50'000.0f};
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);
		if(OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}
	return false;
}


bool AZHeroCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation,
	 FHitResult& OutHitResult)
{
	FVector OutBeamLocation;
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if(bCrosshairHit)
	{
		OutBeamLocation = CrosshairHitResult.Location;
	}
	
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector WeaponTraceStart{ MuzzleSocketLocation};
	const FVector StartToEnd{ OutBeamLocation - MuzzleSocketLocation};
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f};
			
	GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);
			
	if(!OutHitResult.bBlockingHit)
	{
		OutHitResult.Location = OutBeamLocation;
		return  false;
	}
	return true;
}


void AZHeroCharacter::FireWeapon()
{
	if(EquippedWeapon == nullptr) return;

	if(CombatState != ECombatState::ECS_Unoccupied) return;

	if(WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunFireMontage();
		EquippedWeapon->DecrementAmmo();
		StartCrosshairBulletFire();
		StartFireTimer();

		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol)
		{
			EquippedWeapon->StartSlideTimer();
		}
	}	

}

void AZHeroCharacter::PlayFireSound()
{
	if(EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AZHeroCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket =
		EquippedWeapon->GetItemMesh()->GetSocketByName("barr_socket_r");
	if(BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

		if(EquippedWeapon->GetMuzzleFlash())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
		}

		FHitResult BeamHitResult;
		bool bBeamEnd = GetBeamEndLocation(
			SocketTransform.GetLocation(),
			BeamHitResult);

		if(bBeamEnd)
		{
			if(BeamHitResult.GetActor())
			{
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.GetActor());
				if(BulletHitInterface)
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResult, this, GetController());
				}
				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.GetActor());
				if(HitEnemy)
				{
					int32 Damage{0};
					if(BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadBone())
					{
						Damage = EquippedWeapon->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.GetActor(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, true);
					}
					else
					{
						Damage = EquippedWeapon->GetDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.GetActor(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, false);
					}
					
				}
			}
			else
			{
				if(ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						BeamHitResult.Location);
				}
			}

			
			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					BeamParticles,
					SocketTransform);
				if(Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
				}
			}
		}
	}
}

void AZHeroCharacter::PlayGunFireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AZHeroCharacter::ReloadWeapon()
{
	if(CombatState != ECombatState::ECS_Unoccupied) return;
	
	if(EquippedWeapon == nullptr) return;
	
	if (CarryingAmmo() && !EquippedWeapon->ClipIsFull())
	{
		if(bAiming)
		{
			StopAiming();
		}
		
		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

void AZHeroCharacter::FinishReloading()
{
	if(CombatState == ECombatState::ECS_Stunned) return;
	
	CombatState = ECombatState::ECS_Unoccupied;

	if(bAimingButtonPressed)
	{
		Aim();
	}
	
	if(EquippedWeapon == nullptr) return;

	const auto AmmoType = EquippedWeapon->GetAmmoType();
	
	if(AmmoMap.Contains(AmmoType))
	{
		int32 CarriedAmmo = AmmoMap[AmmoType];

		const int32 MagEmptySpace =
			EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if(MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AZHeroCharacter::FinishEquipping()
{
	if(CombatState == ECombatState::ECS_Stunned) return;
	
	CombatState = ECombatState::ECS_Unoccupied;
	if(bAimingButtonPressed)
	{
		Aim();
	}
}

bool AZHeroCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr) return false;

	auto AmmoType = EquippedWeapon->GetAmmoType();

	if(AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}

	return false;
}

void AZHeroCharacter::GrabClip()
{
	if(EquippedWeapon == nullptr) return;
	if(HandSceneComponent == nullptr) return;

	int32 ClipBoneIndex{EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName())};
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("hand_l")));
	HandSceneComponent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

void AZHeroCharacter::ReplaceClip()
{
	EquippedWeapon->SetMovingClip(false);
}




