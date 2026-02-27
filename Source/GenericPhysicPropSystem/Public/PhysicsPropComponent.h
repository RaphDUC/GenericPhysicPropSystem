// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PropPhysicsImpactData.h"
#include "PhysicsPropComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GENERICPHYSICPROPSYSTEM_API UPhysicsPropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPhysicsPropComponent();

	/** Distance in centimeters at which physics (and ticking) logic is optimized/disabled. 0 = Infinite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jam Physics|Optimization")
	float PhysicsCullDistance = 3000.0f; // Default 30m

	/** Frequency in seconds to check the distance between the player and this object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jam Physics|Optimization")
	float DistanceCheckInterval = 1.0f;

	/** 
	 * Speed threshold (cm/s) to enable CCD (Continuous Collision Detection). 0 = Disabled.
	 * Useful to prevent fast objects from tunneling through walls.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jam Physics|Collision")
	float CCDSpeedThreshold = 500.0f; 

	// Audio Configuration
	
	/** Data Asset containing impact sounds per Physical Material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jam Physics|Audio")
	UPropPhysicsImpactData* ImpactTable;

	/** 
	 * Minimum impulse threshold required to trigger a sound.
	 * High value because impulse is Mass * Velocity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jam Physics|Audio")
	float MinImpactThreshold = 10000.0f; 

	/** Minimum time in seconds between two impact sounds to avoid audio spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jam Physics|Audio")
	float ImpactCooldown = 0.1f;

	// --- Interaction Mechanics (Grab/Throw) ---

	/** 
	 * Grabs the object and attaches it to the specified component (e.g., player hand or SceneComponent in front of camera).
	 * @param Holder The scene component that will hold the object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Jam Physics|Interaction")
	void Grab(USceneComponent* Holder);

	/** Gently drops the object. */
	UFUNCTION(BlueprintCallable, Category = "Jam Physics|Interaction")
	void Drop();

	/** 
	 * Throws the object in a direction with a specific force.
	 * @param Direction The direction vector to throw the object.
	 * @param Force The magnitude of the throw force.
	 */
	UFUNCTION(BlueprintCallable, Category = "Jam Physics|Interaction")
	void Throw(FVector Direction, float Force);

	/** Returns true if the object is currently being held. */
	UFUNCTION(BlueprintPure, Category = "Jam Physics|Interaction")
	bool IsGrabbed() const { return bIsGrabbed; }

	/** 
	 * Initializes physics on the target mesh.
	 * Sets up collision notifications, sleep events, and optimization settings.
	 * @param TargetMesh The static mesh component to physicalize.
	 */
	UFUNCTION(BlueprintCallable, Category = "Jam Physics")
	void Physicalize(UStaticMeshComponent* TargetMesh);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Timer function to check distance to player for optimization. */
	void CheckDistanceToPlayer();

	FTimerHandle TimerHandle_DistanceCheck;

	/** Callback when the physics component falls asleep. */
	UFUNCTION()
	void OnPhysicsComponentSleep(UPrimitiveComponent* SleepingComponent, FName BoneName);

	/** Callback when the physics component wakes up. */
	UFUNCTION()
	void OnPhysicsComponentWake(UPrimitiveComponent* WakingComponent, FName BoneName);

	/** Callback for AnyDamage to wake up the object and apply generic forces. */
	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	/** Callback for PointDamage to apply directional impulse. */
	UFUNCTION()
	void OnTakePointDamage(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	/** Callback for RadialDamage to apply explosion impulses. */
	UFUNCTION()
	void OnTakeRadialDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser);

	/** Callback for collision hits to trigger audio effects. */
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// --- Clipping Management ---
	
	/** Updates the position of the grabbed object, handling collision with walls to prevent clipping. */
	void UpdateGrabbedPosition();

private:
	UPROPERTY()
	UStaticMeshComponent* ManagedMesh;

	float LastImpactTime = 0.0f;

	// Grab State
	bool bIsGrabbed = false;

	UPROPERTY()
	USceneComponent* CurrentHolder = nullptr;

	/** Ideal hold distance calculated during Grab. */
	float HoldDistance = 0.0f;
};
