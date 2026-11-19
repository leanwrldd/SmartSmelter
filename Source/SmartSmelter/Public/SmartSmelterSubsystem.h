#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FGBuildableSubsystem.h"
#include "Buildables/FGBuildable.h"
#include "SmartSmelterSubsystem.generated.h"

class AFGBuildableManufacturer;
class UFGItemDescriptor;
class UFGRecipe;

/**
 * Server-only. Once a second, looks at the item waiting on each Smelter's input belt and, when
 * the Smelter is empty and its recipe does not use that item, switches to the recipe that does.
 *
 * Runs from PreFactoryTick, which the buildable subsystem calls sequentially on the game thread
 * before the parallel factory tick: the only point where reading a belt and changing a building
 * cannot race the conveyor workers.
 */
UCLASS()
class SMARTSMELTER_API USmartSmelterSubsystem : public UWorldSubsystem, public IFGFactoryTickHandlerInterface
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* outer) const override;
	virtual void OnWorldBeginPlay(UWorld& world) override;
	virtual void Deinitialize() override;

	// IFGFactoryTickHandlerInterface
	virtual void PreFactoryTick(AFGBuildableSubsystem* subsystem, float deltaTime) override;

private:
	void ScanSmelters(AFGBuildableSubsystem* subsystem);
	void UpdateSmelter(AFGBuildableManufacturer* smelter);

	/// The item sitting at the end of the belt feeding this smelter, or null.
	static TSubclassOf<UFGItemDescriptor> PeekInputItem(const AFGBuildableManufacturer* smelter);

	/// An unlocked recipe for this smelter whose only ingredient is item, or null.
	static TSubclassOf<UFGRecipe> FindRecipeFor(const AFGBuildableManufacturer* smelter, TSubclassOf<UFGItemDescriptor> item);

	UPROPERTY()
	TObjectPtr<AFGBuildableSubsystem> mBuildableSubsystem;

	UPROPERTY()
	TSubclassOf<AFGBuildable> mSmelterClass;

	float mTimeSinceScan = 0.f;
};
