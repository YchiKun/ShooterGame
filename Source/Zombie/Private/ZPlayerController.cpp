// Fill out your copyright notice in the Description page of Project Settings.


#include "ZPlayerController.h"

#include "Blueprint/UserWidget.h"

AZPlayerController::AZPlayerController()
{
	
}

void AZPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if(HUDOverlayClass)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if(HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}
