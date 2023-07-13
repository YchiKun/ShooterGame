// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

AWeapon::AWeapon(): AItem(),
ThrowWeaponTime(0.7f), bFalling(false), Ammo(0), MagazineCapacity(30),
WeaponType(EWeaponType::EWT_SubmachineGun), AmmoType(EAmmoType::EAT_9mm),
ReloadMontageSection(FName(TEXT("Reload SMG"))),
ClipBoneName(TEXT("smg_clip")),
SlideDisplacement(0.0f),
SlideDisplacementTime(0.2f), bMovingSlide(false), MaxSlideDisplacement(4.0f),
MaxRecoilRotation(20.0f), RecoilRotation(0.0f),
bAutomatic(true)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		const FRotator MeshRotation {0.0f, GetItemMesh()->GetComponentRotation().Yaw, 0.0f};
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	UpdateSlideDisplacement();
}

void AWeapon::ThrowWeapon()
{
	FRotator MeshRotation(0.0f, GetItemMesh()->GetComponentRotation().Yaw, 0.0f);
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector MeshForward{GetItemMesh()->GetForwardVector()};
	const FVector MeshRight {GetItemMesh()->GetRightVector()};
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.0f, MeshForward);

	float RandomRotation{FMath::FRandRange(10.0f, 60.0f)};
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.0f,0.0f,1.0f));
	ImpulseDirection *= 2'000.0f;
	GetItemMesh()->AddImpulse(ImpulseDirection);

	bFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);

	EnableGlowMaterial();
}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	StartPulseTimer();
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const FString WeaponTablePath(TEXT("/Script/Engine.DataTable'/Game/_Game/DataTable/WeaponDataTable.WeaponDataTable'"));
	UDataTable* WeaponTableObject = Cast<UDataTable>(
		StaticLoadObject(UDataTable::StaticClass() ,nullptr, *WeaponTablePath));

	if(WeaponTableObject)
	{
		FWeaponDataTable* WeaponDataRow = nullptr;
		switch (WeaponType)
		{
		case EWeaponType::EWT_SubmachineGun:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
			break;
		case EWeaponType::EWT_AssultRifle:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
			break;
			case EWeaponType::EWT_Pistol:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
			break;
		default:
			break;
		}
		if (WeaponDataRow)
		{
			AmmoType = WeaponDataRow->AmmoType;
			Ammo = WeaponDataRow->WeaponAmmo;
			MagazineCapacity = WeaponDataRow->MagazineCapacity;
			SetPickupSound(WeaponDataRow->PickupSound);
			SetEquipSound(WeaponDataRow->EquipSound);
			GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
			SetItemName(WeaponDataRow->ItemName);
			SetItemIcon(WeaponDataRow->InventoryIcon);
			SetAmmoIcon(WeaponDataRow->AmmoIcon);

			SetMaterialInstance(WeaponDataRow->MaterialInstance);
			PreviousMaterialIndex = GetMaterialIndex();
			GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
			SetMaterialIndex(WeaponDataRow->MaterialIndex);
			SetClipBoneName(WeaponDataRow->ClipBoneName);
			SetReloadMontageSection(WeaponDataRow->ReloadMontageSection);
			GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);
			CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
			CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
			CrosshairsRight = WeaponDataRow->CrosshairsRight;
			CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
			CrosshairsTop = WeaponDataRow->CrosshairsTop;
			AutoFireRate = WeaponDataRow->AutoFireRate;
			MuzzleFlash = WeaponDataRow->MuzzleFlash;
			FireSound = WeaponDataRow->FireSound;
			BoneToHide = WeaponDataRow->BoneToHide;
			bAutomatic = WeaponDataRow->bAutomatic;
			Damage = WeaponDataRow->Damage;
			HeadShotDamage = WeaponDataRow->HeadShotDamage;
		}
		if(GetMaterialInstance())
		{
			SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
			GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
			GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
		
			EnableGlowMaterial();
		}
		
		FString RarityTablePath(TEXT("/Script/Engine.DataTable'/Game/_Game/DataTable/ItemRarityDataTable.ItemRarityDataTable'"));
		UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
		if(RarityTableObject)
		{
			FItemRatityTable* RarityRow = nullptr;
			switch (GetItemRarity())
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
				MagazineCapacity += RarityRow->AddMagazineCapacity;
				Damage += RarityRow->AddDamage;
				HeadShotDamage += RarityRow->AddDamage;
			}
		}
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if(BoneToHide != FName(""))
	{
		GetItemMesh()->HideBoneByName(BoneToHide, PBO_None);
	}
}

void AWeapon::FinishMovingSlide()
{
	bMovingSlide = false;
}

void AWeapon::UpdateSlideDisplacement()
{
	if(SlideDisplacementCurve && bMovingSlide)
	{
		const float ElapsedTime(GetWorldTimerManager().GetTimerElapsed(SlideTimer));
		const float CurveValue{SlideDisplacementCurve->GetFloatValue(ElapsedTime)};
		SlideDisplacement = CurveValue * MaxSlideDisplacement;
		RecoilRotation = CurveValue * MaxRecoilRotation;
	}
}

void AWeapon::DecrementAmmo()
{
	if(Ammo - 1 <= 0)
	{
		Ammo = 0;
	}
	else
	{
		--Ammo;
	}
}

void AWeapon::StartSlideTimer()
{
	bMovingSlide = true;
	GetWorldTimerManager().SetTimer(
		SlideTimer,
		this,
		&AWeapon::FinishMovingSlide,
		SlideDisplacementTime);
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attemped to reload with more than magazine capacity"));
	Ammo += Amount;
}

bool AWeapon::ClipIsFull()
{
	return Ammo >= MagazineCapacity;
}

