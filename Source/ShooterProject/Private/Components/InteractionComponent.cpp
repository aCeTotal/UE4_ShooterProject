// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"
#include "Components/ActorComponent.h"
#include "Player/ShooterProjectCharacter.h"
#include "UI/InteractionWidget.h"

UInteractionComponent::UInteractionComponent()
{
	SetComponentTickEnabled(false);

	InteractionTime = 0.f;
	InteractionDistance = 200.f;
	InteractibleNameText = FText::FromString("Interactable Object");
	InteractibleActionText = FText::FromString("Interact");
	bAllowMultipleInteractors = true;

	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(600, 100);
	bDrawAtDesiredSize = true;

	SetActive(true);
	SetHiddenInGame(true);
}


void UInteractionComponent::SetInteractableText(const FText& NewNameText)
{
	InteractibleNameText = NewNameText;
	RefreshWidget();
}


void UInteractionComponent::SetInteractableActionText(const FText& NewActionText)
{
	InteractibleActionText = NewActionText;
	RefreshWidget();
}


void UInteractionComponent::Deactivate()
{
	Super::Deactivate();

	for (int32 i = Interactors.Num() - 1; i >= 0; --i )
	{
		if (AShooterProjectCharacter* Interactor = Interactors[i])
		{
			EndFocus(Interactor);
			EndInteract(Interactor);
		}
	}
	Interactors.Empty();
}


bool UInteractionComponent::CanInteract(class AShooterProjectCharacter* Character) const
{
	const bool bPlayerAlreadyInteracting = !bAllowMultipleInteractors && Interactors.Num() >= 1;
	return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}


void UInteractionComponent::RefreshWidget()
{
	if (!bHiddenInGame && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		if (UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject()))
		{
			InteractionWidget->UpdateInteractionWidget(this);
		}
	}
}


void UInteractionComponent::BeginFocus(class AShooterProjectCharacter* Character)
{
	if (!IsActive() || !GetOwner()  || !Character)
	{
		return;
	}

	OnBeginFocus.Broadcast(Character);

	SetHiddenInGame(false);

	//Object outliner
	if (!GetOwner()->HasAuthority())
	{
		for (auto& VisualComp : GetOwner()->GetComponents())
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			{
				Prim->SetRenderCustomDepth(true);
			}
		}
	}
	RefreshWidget();
}


void UInteractionComponent::EndFocus(class AShooterProjectCharacter* Character)
{
	OnEndFocus.Broadcast(Character);

	SetHiddenInGame(true);

	if (!GetOwner()->HasAuthority())
	{
		for (auto& VisualComp : GetOwner()->GetComponents())
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			{
				Prim->SetRenderCustomDepth(false);
			}
		}
	}
}


void UInteractionComponent::BeginInteract(class AShooterProjectCharacter* Character)
{
	if (CanInteract(Character))
	{
		Interactors.AddUnique(Character);
		OnBeginInteract.Broadcast(Character);
	}
}


void UInteractionComponent::EndInteract(class AShooterProjectCharacter* Character)
{
	Interactors.RemoveSingle(Character);
	OnEndInteract.Broadcast(Character);
}


void UInteractionComponent::Interact(class AShooterProjectCharacter* Character)
{
	if (CanInteract(Character))
	{
		OnInteract.Broadcast(Character);
	}
}


float UInteractionComponent::GetInteractPercentage()
{
	if (Interactors.IsValidIndex(0))
	{
		if (AShooterProjectCharacter* Interactor = Interactors[0])
		{
			if (Interactor && Interactor->IsInteracting())
			{
				return 1.f - FMath::Abs(Interactor->GetRemainingInteractTime() / InteractionTime);
			}
		}
	}
	return 0.f;
}
