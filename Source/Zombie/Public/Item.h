// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Item.generated.h"

UENUM(BlueprintType)
enum  class EItemRarity : uint8
{
	EIR_Damaged UMETA(DisplayName = "Damaged"),
	EIR_Common UMETA(DisplayName = "Common"),
	EIR_Uncommon UMETA(DisplayName = "Uncommon"),
	EIR_Rare UMETA(DisplayName = "Rare"),
	EIR_Legendary UMETA(DisplayName = "Legendary"),

	EIR_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum  class EItemState : uint8
{
	EIS_Pickup UMETA(DisplayName = "Pickup"),
	EIS_EqupInterping UMETA(DisplayName = "EqupInterping"),
	EIS_PickedUp UMETA(DisplayName = "PickedUp"),
	EIS_Equipped UMETA(DisplayName = "Equipped"),
	EIS_Falling UMETA(DisplayName = "Falling"),

	EIS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum  class EItemType : uint8
{
	EIT_Ammo UMETA(DisplayName = "Ammo"),
	EIT_Weapon UMETA(DisplayName = "Weapon"),

	EIT_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FItemRatityTable : public  FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor GlowColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LightColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor DarkColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumberOfStars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* IconBackground;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CustomDepthStencil;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AddDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AddMagazineCapacity;
};

UCLASS()
class ZOMBIE_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void SetActiveStars();

	virtual void SetItemPropeties(EItemState State);

	void FinishInterping();

	void ItemInterp(float DeltaTime);

	FVector GetInterpLocation();

	void PlayPickupSound(bool bForcePlaySound = false);

	virtual void InitializeCustomDepth();

	virtual void OnConstruction(const FTransform& Transform) override;

	void EnableGlowMaterial();

	void UpdatePulse();
	void ResetPulseTimer();
	void StartPulseTimer();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ItemMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* AreaSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	FString ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	int32 ItemCount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	EItemRarity ItemRarity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	TArray<bool> ActiveStars;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	EItemState ItemState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	class UCurveFloat* ItemZCurve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	FVector ItemInterpStartLocation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	FVector CameraTargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	bool bInterping;

	FTimerHandle ItemInterpTimer;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	float ZCurveTime;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	class AZHeroCharacter* Charapter;

	float ItemInterpX;
	float ItemInterpY;

	float InterpInitialYawOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ItemScaleCurve;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	class USoundCue* PickupSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	USoundCue* EquipSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	EItemType ItemType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	int32 InterpLocIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	int32 MaterialIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* MaterialInstance;

	bool bCanChangeCustomDepth;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	class UCurveVector* PulseCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	UCurveVector* InterpPulseCurve;
	
	FTimerHandle PulseTimer;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	float PulseCurveTime;
	
	UPROPERTY(VisibleAnywhere, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	float GlowAmount;
	UPROPERTY(VisibleAnywhere, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	float FresnelExponent;
	UPROPERTY(VisibleAnywhere, Category = "Item Propeties", meta = (AllowPrivateAccess = "true"))
	float FresnelReflectFraction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	UTexture2D* IconItem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	UTexture2D* AmmoIcon;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	int32 SlotIndex;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool bCharapterInventoryFull;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	class UDataTable* ItemRarityDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	FLinearColor GlowColor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	FLinearColor LightColor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	FLinearColor DarkColor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	int32 NumberOfStars;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	UTexture2D* IconBackground;
public:
	FORCEINLINE UWidgetComponent* GetPickupWidget() const {return  PickupWidget;}
	FORCEINLINE USphereComponent* GetAreaSphere() const {return  AreaSphere;}
	FORCEINLINE UBoxComponent* GetCollisionBox() const {return  CollisionBox;}
	FORCEINLINE USkeletalMeshComponent* GetItemMesh() const {return ItemMesh;}
	FORCEINLINE EItemState GetItemState() const {return  ItemState;}
	FORCEINLINE int32 GetItemCount() const {return ItemCount;}
	FORCEINLINE int32 GetSlotIndex() const {return SlotIndex;}
	FORCEINLINE void SetSlotIndex(int32 Index) {SlotIndex = Index;}
	FORCEINLINE USoundCue* GetPickupSound() const { return PickupSound;}
	FORCEINLINE void SetPickupSound(USoundCue* Sound) { PickupSound = Sound;}
	FORCEINLINE USoundCue* GetEquipSound() const {return  EquipSound;}
	FORCEINLINE void SetEquipSound(USoundCue* Sound) { EquipSound = Sound;}
	FORCEINLINE void SetCharatper(AZHeroCharacter* Hero) {Charapter = Hero;}
	FORCEINLINE void SetCharapterInventoryFull(bool bFull) {bCharapterInventoryFull = bFull;}
	FORCEINLINE void SetItemName(FString Name) {ItemName = Name;}
	FORCEINLINE void SetItemIcon(UTexture2D* Icon) {IconItem = Icon;}
	FORCEINLINE void SetAmmoIcon(UTexture2D* Icon) {AmmoIcon = Icon;}
	FORCEINLINE UMaterialInstance* GetMaterialInstance() const {return  MaterialInstance;}
	FORCEINLINE void SetMaterialInstance(UMaterialInstance* Instance) {MaterialInstance = Instance;}
	FORCEINLINE UMaterialInstanceDynamic* GetDynamicMaterialInstance() const {return  DynamicMaterialInstance;}
	FORCEINLINE void SetDynamicMaterialInstance(UMaterialInstanceDynamic* Instance) {DynamicMaterialInstance = Instance;}
	FORCEINLINE FLinearColor GetGlowColor() const {return GlowColor;}
	FORCEINLINE int32 GetMaterialIndex() const {return MaterialIndex;}
	FORCEINLINE void SetMaterialIndex(int32 Index) {MaterialIndex = Index;}
	FORCEINLINE EItemRarity GetItemRarity() const {return  ItemRarity;}
	
	void SetItemState(EItemState State);
	void StartItemCurve(AZHeroCharacter* Hero, bool bForcePlaySound = false);

	void PlayEquipSound(bool bForcePlaySound = false);
	
	virtual void EnableCustomDepth();
	virtual void DisableCustomDepth();
	void DisableGlowMaterial();
};
