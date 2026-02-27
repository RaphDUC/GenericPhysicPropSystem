// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsPropComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DamageType.h"
#include "PhysicsPropDamageType.h"
#include "GenericDamageType.h"
#include "PhysicalMaterials/PhysicalMaterial.h" // Necessary for audio interactions

// Sets default values for this component's properties
UPhysicsPropComponent::UPhysicsPropComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Enable tick only when the object moves

	// Default values
	PhysicsCullDistance = 3000.0f;
	DistanceCheckInterval = 1.0f;
	CCDSpeedThreshold = 500.0f;
}


void UPhysicsPropComponent::Physicalize(UStaticMeshComponent* TargetMesh)
{
	if (!TargetMesh) return;
	ManagedMesh = TargetMesh;

	// Source Engine style configuration
	ManagedMesh->SetSimulatePhysics(true);
	ManagedMesh->SetNotifyRigidBodyCollision(true); // Required for impacts
	ManagedMesh->SetGenerateOverlapEvents(false);   // Optimization: no overlaps if unnecessary
    
	// Prevent actor from ticking needlessly
	GetOwner()->SetActorTickEnabled(false);

	// Bind sleep events
	ManagedMesh->OnComponentSleep.AddDynamic(this, &UPhysicsPropComponent::OnPhysicsComponentSleep);
	ManagedMesh->OnComponentWake.AddDynamic(this, &UPhysicsPropComponent::OnPhysicsComponentWake);
	
	// Bind collision events for audio
	ManagedMesh->OnComponentHit.AddDynamic(this, &UPhysicsPropComponent::OnComponentHit);

	// Aggressive sleep threshold (Source style)
	// Using BodyInstance. 'Sensitive' = falls asleep faster (lower threshold).
	if (FBodyInstance* BodyInst = ManagedMesh->GetBodyInstance())
	{
		BodyInst->SleepFamily = ESleepFamily::Sensitive;

		// Can also increase damping to help object reach rest state faster
		ManagedMesh->SetLinearDamping(0.5f); 
		ManagedMesh->SetAngularDamping(0.5f);
	}

	// Start optimization timer if necessary
	if (PhysicsCullDistance > 0.0f)
	{
		// Randomize timer start to spread CPU load if many objects spawn at once
		float RandomVariability = FMath::RandRange(0.0f, 0.5f);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_DistanceCheck, this, &UPhysicsPropComponent::CheckDistanceToPlayer, DistanceCheckInterval + RandomVariability, true);
	}

	// Bind damage events
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UPhysicsPropComponent::OnTakeAnyDamage);
		Owner->OnTakePointDamage.AddDynamic(this, &UPhysicsPropComponent::OnTakePointDamage);
		Owner->OnTakeRadialDamage.AddDynamic(this, &UPhysicsPropComponent::OnTakeRadialDamage);
	}
}

void UPhysicsPropComponent::BeginPlay()
{
	Super::BeginPlay();
	// If Physicalize is not called manually, try finding the first StaticMesh
	if (!ManagedMesh)
	{
		if (UStaticMeshComponent* FoundMesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
		{
			if (FoundMesh->IsSimulatingPhysics())
			{
				Physicalize(FoundMesh);
			}
		}
	}
}

void UPhysicsPropComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ManagedMesh || CCDSpeedThreshold <= 0.0f) return;

	// Dynamic CCD management to avoid tunneling at high speeds
	const float SpeedSq = ManagedMesh->GetComponentVelocity().SizeSquared();
	const bool bShouldUseCCD = SpeedSq > (CCDSpeedThreshold * CCDSpeedThreshold);

	// Only call function if state changes to avoid overhead
	if (ManagedMesh->BodyInstance.bUseCCD != bShouldUseCCD)
	{
		ManagedMesh->SetUseCCD(bShouldUseCCD);
	}

	// --- Grab Logic ---
	if (bIsGrabbed && CurrentHolder)
	{
		UpdateGrabbedPosition();
	}
}

void UPhysicsPropComponent::Grab(USceneComponent* Holder)
{
	if (!Holder || !ManagedMesh) return;

	CurrentHolder = Holder;
	bIsGrabbed = true;

	// 1. Disable physics for direct control
	ManagedMesh->SetSimulatePhysics(false);
	
	// 2. Ignore Player Pawn to avoid pushing oneself (Source-like physics)
	// Assuming Holder belongs to the Pawn
	if (AActor* OwnerActor = Holder->GetOwner())
	{
		ManagedMesh->IgnoreActorWhenMoving(OwnerActor, true);
		
		if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
		{
			OwnerPawn->MoveIgnoreActorAdd(GetOwner());
		}
	}

	// 3. Calculate current ideal hold distance
	HoldDistance = FVector::Distance(CurrentHolder->GetComponentLocation(), ManagedMesh->GetComponentLocation());

	// Min/Max limits (arbitrary, to avoid grabbing something 1km away and keeping it there)
	HoldDistance = FMath::Clamp(HoldDistance, 50.0f, 250.0f);

	// Wake object to be sure
	SetComponentTickEnabled(true);
}

void UPhysicsPropComponent::Drop()
{
	if (!bIsGrabbed || !ManagedMesh) return;

	// Reactivate physics
	ManagedMesh->SetSimulatePhysics(true);
	
	// Preserve inertia (if player runs and drops, object continues)
	if (CurrentHolder)
	{
		AActor* HolderOwner = CurrentHolder->GetOwner();
		// Try to get player velocity if possible
		if (HolderOwner)
		{
			ManagedMesh->SetPhysicsLinearVelocity(HolderOwner->GetVelocity());
			
			// Restore collisions with player
			ManagedMesh->IgnoreActorWhenMoving(HolderOwner, false);
			
			if (APawn* OwnerPawn = Cast<APawn>(HolderOwner))
			{
				OwnerPawn->MoveIgnoreActorRemove(GetOwner());
			}
		}
	}
	ManagedMesh->WakeAllRigidBodies();

	// Cleanup
	bIsGrabbed = false;
	CurrentHolder = nullptr;
}

void UPhysicsPropComponent::Throw(FVector Direction, float Force)
{
	if (!bIsGrabbed || !ManagedMesh) return;

	// First drop properly to reactivate physics
	Drop(); 

	// Then propel
	ManagedMesh->AddImpulse(Direction.GetSafeNormal() * Force, NAME_None, true); // true = Velocity Change (ignoring mass for arcade feel)
}

void UPhysicsPropComponent::UpdateGrabbedPosition()
{
	if (!CurrentHolder || !ManagedMesh) return;

	FVector TargetLocation = CurrentHolder->GetComponentLocation() + (CurrentHolder->GetForwardVector() * HoldDistance);
	FRotator TargetRotation = CurrentHolder->GetComponentRotation(); // Alternatively keep object rot. Here we lock like an FPS.

	// --- ANTI-CLIPPING ---
	// Cast a ray (Sweep) from player to target object position.
	// If hitting a wall, pull object closer so it doesn't clip through.
	
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner()); // Ignore self
	Params.AddIgnoredActor(CurrentHolder->GetOwner()); // Ignore player

	FVector StartTrace = CurrentHolder->GetComponentLocation();
	
	// Sweep with a sphere slightly smaller than object (approx 10cm here)
	// Ideally use object collision box but that is heavy.
	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, 
		StartTrace, 
		TargetLocation, 
		FQuat::Identity, 
		ECC_Visibility, // Visibility channel or Camera to detect walls
		FCollisionShape::MakeSphere(10.0f), 
		Params
	);

	if (bHit)
	{
		// If wall hit, place object just BEFORE impact point
		// Add small offset (Normal * Radius) to not stick origin into wall
		TargetLocation = Hit.Location + (Hit.ImpactNormal * 12.0f);
	}

	// Smooth movement (VInterp) to avoid stuttering if moving fast
	FVector NewLoc = FMath::VInterpTo(GetOwner()->GetActorLocation(), TargetLocation, GetWorld()->GetDeltaSeconds(), 20.0f);
	FRotator NewRot = FMath::RInterpTo(GetOwner()->GetActorRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 20.0f);

	GetOwner()->SetActorLocationAndRotation(NewLoc, NewRot, true); // true = Sweep (tries to slide along walls too)
}

void UPhysicsPropComponent::CheckDistanceToPlayer()
{
	if (!ManagedMesh || !GetWorld()) return;

	// Basic safety: If object falls too low (under map), destroy it to save perfs
	if (GetOwner()->GetActorLocation().Z < -20000.0f) // -200 meters
	{
		GetOwner()->Destroy();
		return;
	}

	// Find camera or player pawn (generic method without cast)
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	FVector ObserverLoc;
	FRotator ObserverRot;
	PC->GetPlayerViewPoint(ObserverLoc, ObserverRot);

	const float DistSq = FVector::DistSquared(ObserverLoc, GetOwner()->GetActorLocation());
	const float CullDistSq = PhysicsCullDistance * PhysicsCullDistance;

	// If far away
	if (DistSq > CullDistSq)
	{
		// If object is awake, force sleep to save CPU
		if (ManagedMesh->IsSimulatingPhysics() && ManagedMesh->GetBodyInstance()->IsInstanceAwake())
		{
			ManagedMesh->PutAllRigidBodiesToSleep();
		}
		
		// Reduce mesh tick frequency if active
		ManagedMesh->SetComponentTickInterval(1.0f);
	}
	else
	{
		// Close by: restore normal tick rate for max smoothness
		ManagedMesh->SetComponentTickInterval(0.0f);
	}
}

void UPhysicsPropComponent::OnPhysicsComponentSleep(UPrimitiveComponent* SleepingComponent, FName BoneName)
{
	// Object asleep: cut consumption
	SetComponentTickEnabled(false); // No need to check speed
	
	GetOwner()->SetActorTickEnabled(false);
    
	// Optional: Disable movement replication to save bandwidth
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetOwner()->SetReplicateMovement(false);
	}
}

void UPhysicsPropComponent::OnPhysicsComponentWake(UPrimitiveComponent* WakingComponent, FName BoneName)
{
	// Object moving: reactivate needs
	SetComponentTickEnabled(true); // Start monitoring speed for CCD

	GetOwner()->SetActorTickEnabled(true);
    
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetOwner()->SetReplicateMovement(true);
	}
}

void UPhysicsPropComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!ManagedMesh || !ManagedMesh->IsSimulatingPhysics()) return;

	// Any damage wakes object, even if "lulled"
	ManagedMesh->SetComponentTickInterval(0.0f); // Reset normal tick rate immediately

	// Unified DamageType handling
	float ForceMultiplier = 500.0f;
	bool bShouldWake = true;

	if (const UPhysicsPropDamageType* JamDmg = Cast<UPhysicsPropDamageType>(DamageType))
	{
		bShouldWake = JamDmg->bForceWake;
		ForceMultiplier = JamDmg->ImpulsePower;
	}
	else if (const UGenericDamageType* GenDmg = Cast<UGenericDamageType>(DamageType))
	{
		// Use GenericDamageType modifier on standard base
		ForceMultiplier = 500.0f * GenDmg->ImpulseModifier;
	}

	if (bShouldWake)
	{
		ManagedMesh->WakeAllRigidBodies();
	}

	// Calculate impulse direction
	FVector ImpulseDir = FVector::ZeroVector;

	// If we have an instigator (Controller), use their aim
	if (InstigatedBy)
	{
		ImpulseDir = InstigatedBy->GetControlRotation().Vector();
	}
	// Else if we have a Causer, use direction from causer to object
	else if (DamageCauser)
	{
		ImpulseDir = (ManagedMesh->GetComponentLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	}
	// Else, push up arbitrarily
	else
	{
		ImpulseDir = FVector::UpVector;
	}

	const FVector Impulse = ImpulseDir * (Damage * ForceMultiplier);
	// Apple to center of mass because we don't have HitLocation in AnyDamage
	ManagedMesh->AddImpulse(Impulse, NAME_None, true); // true = Vel Change? No, standard Impulse.
}

void UPhysicsPropComponent::OnTakePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if (!ManagedMesh || !ManagedMesh->IsSimulatingPhysics()) return;

	float ForceMultiplier = 500.0f;
	bool bShouldWake = true;

	if (const UPhysicsPropDamageType* JamDmg = Cast<UPhysicsPropDamageType>(DamageType))
	{
		bShouldWake = JamDmg->bForceWake;
		ForceMultiplier = JamDmg->ImpulsePower;
	}
	else if (const UGenericDamageType* GenDmg = Cast<UGenericDamageType>(DamageType))
	{
		ForceMultiplier = 500.0f * GenDmg->ImpulseModifier;
	}

	if (bShouldWake)
	{
		ManagedMesh->WakeAllRigidBodies();
	}
	
	const FVector Impulse = ShotFromDirection * (Damage * ForceMultiplier);
	
	ManagedMesh->AddImpulseAtLocation(Impulse, HitLocation, BoneName);
}

void UPhysicsPropComponent::OnTakeRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!ManagedMesh || !ManagedMesh->IsSimulatingPhysics()) return;

	float ForceMultiplier = 2000.0f;
	bool bShouldWake = true;

	if (const UPhysicsPropDamageType* JamDmg = Cast<UPhysicsPropDamageType>(DamageType))
	{
		bShouldWake = JamDmg->bForceWake;
		ForceMultiplier = JamDmg->ImpulsePower * 4.0f;
	}
	else if (const UGenericDamageType* GenDmg = Cast<UGenericDamageType>(DamageType))
	{
		ForceMultiplier = 2000.0f * GenDmg->ImpulseModifier;
	}

	if (bShouldWake)
	{
		ManagedMesh->WakeAllRigidBodies();
	}

	const float ImpulseStrength = Damage * ForceMultiplier;
	
	ManagedMesh->AddRadialImpulse(Origin, 500.0f, ImpulseStrength, ERadialImpulseFalloff::RIF_Linear, true);
}

void UPhysicsPropComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!ImpactTable || !GetWorld()) return;

	// 1. Anti-spam: Time cooldown
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastImpactTime < ImpactCooldown) return;

	// 2. Calculate impact force (Normal Impulse)
	// NormalImpulse depends on mass. To get a "normalized" value close to velocity, divide by mass.
	float ImpactIntensity = NormalImpulse.Size();
    
	// If other object also simulates physics, energy is shared, impact is often felt stronger
	// Try to normalize to consistent unit (pseudo-velocity)
	if (ManagedMesh && ManagedMesh->GetMass() > 0.0f)
	{
		ImpactIntensity /= ManagedMesh->GetMass(); 
	}
	
	// Min threshold to play sound (avoid noise when rolling gently or vibrating)
	if (ImpactIntensity < MinImpactThreshold) return;

	// 3. Get Physical Material of touched surface
	UPhysicalMaterial* HitPhysMat = Hit.PhysMaterial.Get();
    
	// 4. Find sound in table
	const FImpactSoundEntry* SoundEntry = ImpactTable->ImpactMap.Find(HitPhysMat);
	
	// If no specific sound, use default
	if (!SoundEntry) 
	{
		SoundEntry = &ImpactTable->DefaultSound;
	}

	if (SoundEntry && SoundEntry->ImpactSound)
	{
		// 5. Volume Normalization (Source style)
		// Map intensity (e.g., 100 to 1500) to volume (0.2 to 1.0)
		// These "Magic Numbers" (1500.0f) depend on game scale, need tuning.
		const float Volume = FMath::GetMappedRangeValueClamped(FVector2D(MinImpactThreshold, MinImpactThreshold * 5.0f), FVector2D(0.2f, 1.0f), ImpactIntensity) * SoundEntry->VolumeMultiplier;		
		// Pitch variation to avoid robotic repetition (0.85 - 1.1)
		const float Pitch = FMath::RandRange(0.85f, 1.1f); 

		UGameplayStatics::PlaySoundAtLocation(this, SoundEntry->ImpactSound, Hit.ImpactPoint, Volume, Pitch);
        
		LastImpactTime = CurrentTime;
	}
}
