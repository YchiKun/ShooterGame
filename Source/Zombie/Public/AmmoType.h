﻿#pragma once

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm UMETA(DisplayName = "9mm"),
	EAT_AR UMETA(DisplayName = "AssautRifle"),

	EAT_MAX UMETA(DisplayName = "DefaultMax")
};