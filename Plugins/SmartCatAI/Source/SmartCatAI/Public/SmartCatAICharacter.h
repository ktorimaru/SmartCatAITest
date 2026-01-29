// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SmartCatAICharacter.generated.h"

UCLASS()
class SMARTCATAI_API ASmartCatAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASmartCatAICharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "SmartCatAI")
	USkeletalMeshComponent* GetCatMesh() const { return GetMesh(); }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Mesh")
	TObjectPtr<USkeletalMesh> CatSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Animation")
	TSubclassOf<UAnimInstance> CatAnimClass;
};
