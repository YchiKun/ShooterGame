// Fill out your copyright notice in the Description page of Project Settings.


#include "ZHeroAnimInstance.h"

#include "Weapon.h"
#include "WeaponType.h"
#include "ZHeroCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UZHeroAnimInstance::UZHeroAnimInstance():
Speed(0.0f), bIsInAir(false), bIsAccelerating(false),
MovementOffsetYaw(0.0f), LastMovementOffsetYaw(0.0f),
bAiming(false),
TIPCharacterYaw(0.0f), TIPCharacterYawLastFrame(0.0f), RootYawOffset(0.0f),
Pitch(0.0f), bReloadoing(false), OffsetState(EOffsetState::EOS_Hip),
CharacterRotator(FRotator(0.0f)), CharacterRotatorLastFrame(FRotator(0.0f)), YawDelta(0.0f),
bCrouching(false), RecoilWeight(1.0f), bTurningInPlace(false),
EquippedWeaponType(EWeaponType::EWT_MAX), bShouldUseFABRIK(false)
{
}

void UZHeroAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if( ZHeroCharapter == nullptr)
	{
		ZHeroCharapter = Cast<AZHeroCharacter>(TryGetPawnOwner());
	}
	if (ZHeroCharapter)
	{
		bCrouching = ZHeroCharapter->GetCrounching();
		bReloadoing = ZHeroCharapter->GetCombatState() == ECombatState::ECS_Reloading;
		bEquipping = ZHeroCharapter->GetCombatState() == ECombatState::ECS_Equipping;
		bShouldUseFABRIK = ZHeroCharapter->GetCombatState() == ECombatState::ECS_Unoccupied ||
			ZHeroCharapter->GetCombatState() == ECombatState::ECS_FireTimerInProgress;
		
		FVector Velocity{ZHeroCharapter->GetVelocity()};
		Velocity.Z = 0;
		Speed = Velocity.Size();

		bIsInAir = ZHeroCharapter->GetCharacterMovement()->IsFalling();

		if(ZHeroCharapter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f)
		{
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}

		FRotator AimRotation = ZHeroCharapter->GetBaseAimRotation();
		FRotator MovementRotationYaw =
			UKismetMathLibrary::MakeRotFromX(ZHeroCharapter->GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(
			MovementRotationYaw,
			AimRotation).Yaw;

		if(ZHeroCharapter->GetVelocity().Size() > 0.0f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}

		bAiming = ZHeroCharapter->GetAiming();

		if(bReloadoing)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if(ZHeroCharapter->GetAiming())
		{
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}
		if(ZHeroCharapter->GetEquippedWeapon())
		{
			EquippedWeaponType = ZHeroCharapter->GetEquippedWeapon()->GetWeaponType();
		}
	}
	TurnInPlace();
	Lean(DeltaTime);
}

void UZHeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ZHeroCharapter = Cast<AZHeroCharacter>(TryGetPawnOwner());
}

void UZHeroAnimInstance::TurnInPlace()
{
	if(ZHeroCharapter == nullptr) return;

	Pitch = ZHeroCharapter->GetBaseAimRotation().Pitch;
	
	
	if(Speed > 0 || bIsInAir)
	{
		RootYawOffset = 0.0f;
		TIPCharacterYaw = ZHeroCharapter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurveLastFrame = 0.0f;
		RotationCurve = 0.0f;
	}
	else
	{
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = ZHeroCharapter->GetActorRotation().Yaw;
		const float TIPYawDelta{ TIPCharacterYaw - TIPCharacterYawLastFrame};
		
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

		const float Turning{GetCurveValue(TEXT("Turning"))};
		if(Turning > 0)
		{
			bTurningInPlace = true;
			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation{RotationCurve - RotationCurveLastFrame};

			RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

			const float ABSRootYawOffset{ FMath::Abs(RootYawOffset)};
			if(ABSRootYawOffset > 90.0f)
			{
				const float YawExcess{ABSRootYawOffset - 90.0f};
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}
		else
		{
			bTurningInPlace = false;
		}

		if(bTurningInPlace)
		{
			if(bReloadoing || bEquipping)
			{
				RecoilWeight = 1.0f;
			}
			else
			{
				RecoilWeight = 0.0f;
			}
		}
		else
		{
			if(bCrouching)
			{
				if(bReloadoing)
				{
					RecoilWeight = 1.0f;
				}
				else
				{
					RecoilWeight = 0.1f;
				}
			}
			else
			{
				if(bAiming || bReloadoing || bEquipping)
				{
					RecoilWeight = 1.0f;
				}
				else
				{
					RecoilWeight = 0.5f;
				}
			}
		}
	}
}

void UZHeroAnimInstance::Lean(float DeltaTime)
{
	if(ZHeroCharapter == nullptr) return;
	CharacterRotatorLastFrame = CharacterRotator;
	CharacterRotator = ZHeroCharapter->GetActorRotation();

	FRotator Delta{UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotator, CharacterRotatorLastFrame)};

	const double Target{ Delta.Yaw / DeltaTime};
	const double Interp{FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.0f)};

	YawDelta = FMath::Clamp(Interp, -90.0, 90.0);
}
