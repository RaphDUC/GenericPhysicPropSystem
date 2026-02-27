// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PropPhysicsImpactData.generated.h"

USTRUCT(BlueprintType)
struct FImpactSoundEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* ImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	float VolumeMultiplier = 1.0f;
};

/**
 * Data Asset to configure physics impact sounds.
 */
UCLASS(BlueprintType)
class UPropPhysicsImpactData : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Map associating a Physical Material with a sound entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impacts")
	TMap<UPhysicalMaterial*, FImpactSoundEntry> ImpactMap;
	
	/** Fallback sound if no Physical Material matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impacts")
	FImpactSoundEntry DefaultSound;
};
