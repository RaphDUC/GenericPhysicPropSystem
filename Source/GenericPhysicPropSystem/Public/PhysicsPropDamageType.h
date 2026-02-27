// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "PhysicsPropDamageType.generated.h"

/**
 * Custom DamageType for the Jam Physics System.
 * Allows defining an impact force independent of damage (HP).
 */
UCLASS()
class GENERICPHYSICPROPSYSTEM_API UPhysicsPropDamageType : public UDamageType
{
	GENERATED_BODY()
	
public:
	/** 
	 * Impulse multiplier applied to the physics object.
	 * 0 = No movement. 1000 = Standard push.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Jam Physics")
	float ImpulsePower = 1000.0f;

	/** If true, the object will be woken up even if it was asleep. */
	UPROPERTY(EditDefaultsOnly, Category = "Jam Physics")
	bool bForceWake = true;
};
