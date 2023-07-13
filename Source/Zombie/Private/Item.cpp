// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "ZHeroCharacter.h"
#include "Camera/CameraComponent.h"
#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
AItem::AItem():
ItemName(FString("Default")), ItemCount(0),
ItemRarity(EItemRarity::EIR_Common), ItemState(EItemState::EIS_Pickup),
ItemInterpStartLocation(FVector(0.0f)), CameraTargetLocation(FVector(0.0f)),
bInterping(false), ZCurveTime(0.7f),
ItemInterpX(0.0f), ItemInterpY(0.0f),
InterpInitialYawOffset(0.0f),
ItemType(EItemType::EIT_MAX),
InterpLocIndex(0), MaterialIndex(0),
bCanChangeCustomDepth(true),
PulseCurveTime(5.0f),
GlowAmount(150.0f), FresnelExponent(3.0f), FresnelReflectFraction(4.0f),
SlotIndex(0), bCharapterInventoryFull(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Visibility,
		ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();

	if(PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
	
	SetActiveStars();
	
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);

	SetItemPropeties(ItemState);

	InitializeCustomDepth();

	StartPulseTimer();
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);

	UpdatePulse();
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor)
	{
		AZHeroCharacter* ZHeroCharapter = Cast<AZHeroCharacter>(OtherActor);
		if(ZHeroCharapter)
		{
			ZHeroCharapter->IncrementOvelappedItemCount(1);
		}
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor)
	{
		AZHeroCharacter* ZHeroCharapter = Cast<AZHeroCharacter>(OtherActor);
		if(ZHeroCharapter)
		{
			ZHeroCharapter->IncrementOvelappedItemCount(-1);
			ZHeroCharapter->UnHighlightInventorySlot();
		}
	}
}

void AItem::SetActiveStars()
{
	for(int32 i = 0; i < 5; i++)
	{
		ActiveStars.Add(false);
	}

	switch (ItemRarity)
	{
	case EItemRarity::EIR_Damaged:
		ActiveStars[0] = true;
		break;
	case EItemRarity::EIR_Common:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		break;
	case EItemRarity::EIR_Uncommon:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		break;
	case EItemRarity::EIR_Rare:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		break;
	case EItemRarity::EIR_Legendary:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		break;
	default:
		break;
	}
}

void AItem::SetItemPropeties(EItemState State)
{
	switch (State)
	{
	case EItemState::EIS_Pickup:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	case EItemState::EIS_Equipped:
		PickupWidget->SetVisibility(false);
		
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_Falling:
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_EqupInterping:
		PickupWidget->SetVisibility(false);

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_PickedUp:
		PickupWidget->SetVisibility(false);

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	default:
		break;
	}
}

void AItem::StartPulseTimer()
{
	if(ItemState == EItemState::EIS_Pickup)
	{
		GetWorldTimerManager().SetTimer(PulseTimer, this, &AItem::ResetPulseTimer, PulseCurveTime);
	}
}

void AItem::ResetPulseTimer()
{
	StartPulseTimer();
}

void AItem::SetItemState(EItemState State)
{
	ItemState = State;
	SetItemPropeties(State);
}

void AItem::StartItemCurve(AZHeroCharacter* Hero, bool bForcePlaySound)
{
	Charapter = Hero;

	InterpLocIndex = Charapter->GetInterpLocationIndex();
	Charapter->IncrementInterpLocItemCount(InterpLocIndex, 1);
	
	PlayPickupSound(bForcePlaySound);
	
	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	SetItemState(EItemState::EIS_EqupInterping);
	GetWorldTimerManager().ClearTimer(PulseTimer);

	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItem::FinishInterping, ZCurveTime);

	const double CameraRotationYaw{ Charapter->GetFollowCamera()->GetComponentRotation().Yaw};
	const double ItemRotationYaw{GetActorRotation().Yaw};
	InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;
	
	bCanChangeCustomDepth = false;
}

void AItem::FinishInterping()
{
	bInterping = false;
	if(Charapter)
	{
		Charapter->IncrementInterpLocItemCount(InterpLocIndex, -1);
		Charapter->GetPickupItem(this);

		Charapter->UnHighlightInventorySlot();
	}
	
	SetActorScale3D(FVector(1.0f));

	DisableGlowMaterial();
	bCanChangeCustomDepth = true;
	DisableCustomDepth();
}

void AItem::ItemInterp(float DeltaTime)
{
	if(!bInterping) return;

	if(Charapter && ItemZCurve)
	{
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

		FVector ItemLocation = ItemInterpStartLocation;
		const FVector CameraInterpLocation{GetInterpLocation()};

		const FVector ItemToCamera{FVector(0.0f, 0.0f, (CameraInterpLocation - ItemLocation).Z)};
		const float DeltaZ = ItemToCamera.Size();

		const FVector CurrentLocation{GetActorLocation()};
		const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 30.0f);
		const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.0f);

		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;
		
		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

		const FRotator CameraRotation{Charapter->GetFollowCamera()->GetComponentRotation()};
		FRotator ItemRotation{0.0f, CameraRotation.Yaw + InterpInitialYawOffset, 0.0f};
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

		if(ItemScaleCurve)
		{
			const float ScaleCurveValue =  ItemScaleCurve->GetFloatValue(ElapsedTime);
			SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue,ScaleCurveValue));
		}
	}
}

FVector AItem::GetInterpLocation()
{
	if(Charapter == nullptr) return FVector(0.0f);

	switch (ItemType)
	{
	case EItemType::EIT_Ammo:
		return  Charapter->GetInterpLocation(InterpLocIndex).SceneComponent->GetComponentLocation();
		break;
	case EItemType::EIT_Weapon:
		return  Charapter->GetInterpLocation(0).SceneComponent->GetComponentLocation();
		break;
	default:
		break;
	}
	return  FVector(0.0f);
}

void AItem::PlayPickupSound(bool bForcePlaySound)
{
	if(Charapter)
	{
		if(bForcePlaySound)
		{
			if(PickupSound)
			{
				UGameplayStatics::PlaySound2D(this, PickupSound);
			}
		}
		else if(Charapter->GetShouldPlayPickupSound())
		{
			Charapter->StartPickupSoundTimer();
			if(PickupSound)
			{
				UGameplayStatics::PlaySound2D(this, PickupSound);
			}
		}
	}
}

void AItem::EnableCustomDepth()
{
	if(bCanChangeCustomDepth)
	{
		ItemMesh->SetRenderCustomDepth(true);
	}
}

void AItem::DisableCustomDepth()
{
	if(bCanChangeCustomDepth)
	{
		ItemMesh->SetRenderCustomDepth(false);
	}
}

void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
}

void AItem::OnConstruction(const FTransform& Transform)
{
	FString RarityTablePath(TEXT("/Script/Engine.DataTable'/Game/_Game/DataTable/ItemRarityDataTable.ItemRarityDataTable'"));
	UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
	if(RarityTableObject)
	{
		FItemRatityTable* RarityRow = nullptr;
		switch (ItemRarity)
		{
		case EItemRarity::EIR_Damaged:
			RarityRow = RarityTableObject->FindRow<FItemRatityTable>(FName("Damaged"), TEXT(""));
			break;
		case EItemRarity::EIR_Common:
			RarityRow = RarityTableObject->FindRow<FItemRatityTable>(FName("Common"), TEXT(""));
			break;
		case EItemRarity::EIR_Uncommon:
			RarityRow = RarityTableObject->FindRow<FItemRatityTable>(FName("Uncommon"), TEXT(""));
			break;
		case EItemRarity::EIR_Rare:
			RarityRow = RarityTableObject->FindRow<FItemRatityTable>(FName("Rare"), TEXT(""));
			break;
		case EItemRarity::EIR_Legendary:
			RarityRow = RarityTableObject->FindRow<FItemRatityTable>(FName("Legendary"), TEXT(""));
			break;
		default:
				break;
		}

		if(RarityRow)
		{
			GlowColor = RarityRow->GlowColor;
			LightColor = RarityRow->LightColor;
			DarkColor = RarityRow->DarkColor;
			NumberOfStars = RarityRow->NumberOfStars;
			IconBackground = RarityRow->IconBackground;
			if(GetItemMesh())
			{
				GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
			}
		}
	}
	
	if(MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
		
		EnableGlowMaterial();
	}
}

void AItem::EnableGlowMaterial()
{
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
	}
}

void AItem::UpdatePulse()
{
	float ElapsedTime{};
	FVector CurveValue{};
	switch (ItemState)
	{
	case EItemState::EIS_Pickup:
		if(PulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
			CurveValue = PulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	case EItemState::EIS_EqupInterping:
		if(InterpPulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
			CurveValue = InterpPulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
		default:
			break;
	}
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
	}
}

void AItem::DisableGlowMaterial()
{
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
	}
}

void AItem::PlayEquipSound(bool bForcePlaySound)
{
	if(Charapter)
	{
		if(bForcePlaySound)
		{
			if(EquipSound)
			{
				UGameplayStatics::PlaySound2D(this, EquipSound);
			}
		}
		else if(Charapter->GetShouldPlayEquipSound())
		{
			Charapter->StartEquipSoundTimer();
			if(EquipSound)
			{
				UGameplayStatics::PlaySound2D(this, EquipSound);
			}
		}
	}
}

