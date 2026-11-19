#include "SmartSmelterSubsystem.h"

#include "SmartSmelter.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "FGFactoryConnectionComponent.h"
#include "FGInventoryComponent.h"
#include "FGRecipe.h"
#include "Resources/FGItemDescriptor.h"

namespace
{
	const TCHAR* SmelterClassPath = TEXT("/Game/FactoryGame/Buildable/Factory/SmelterMk1/Build_SmelterMk1.Build_SmelterMk1_C");

	/// How often every smelter is checked. Belts move slowly enough that a second is invisible.
	constexpr float ScanInterval = 1.f;

	bool RecipeUsesItem(TSubclassOf<UFGRecipe> recipe, TSubclassOf<UFGItemDescriptor> item)
	{
		if (recipe == nullptr)
			return false;
		for (const FItemAmount& ingredient : recipe.GetDefaultObject()->GetIngredients())
		{
			if (ingredient.ItemClass == item)
				return true;
		}
		return false;
	}

	bool IsAlternateRecipe(TSubclassOf<UFGRecipe> recipe)
	{
		return recipe->GetName().Contains(TEXT("Alternate"));
	}
}

bool USmartSmelterSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
	const UWorld* world = Cast<UWorld>(outer);
	return world != nullptr && world->IsGameWorld() && Super::ShouldCreateSubsystem(outer);
}

void USmartSmelterSubsystem::OnWorldBeginPlay(UWorld& world)
{
	Super::OnWorldBeginPlay(world);

	// The recipe is replicated, so only the server decides it.
	if (world.GetNetMode() == NM_Client)
		return;

	mSmelterClass = LoadClass<AFGBuildable>(nullptr, SmelterClassPath);
	if (mSmelterClass == nullptr)
	{
		UE_LOG(LogSmartSmelter, Error, TEXT("Could not load %s, Smart Smelter is inactive"), SmelterClassPath);
		return;
	}

	mBuildableSubsystem = AFGBuildableSubsystem::Get(&world);
	if (mBuildableSubsystem == nullptr)
	{
		UE_LOG(LogSmartSmelter, Error, TEXT("No buildable subsystem, Smart Smelter is inactive"));
		return;
	}

	mBuildableSubsystem->AddFactoryTickHandler(this);
	UE_LOG(LogSmartSmelter, Log, TEXT("Smart Smelter active, watching %s"), *mSmelterClass->GetName());
}

void USmartSmelterSubsystem::Deinitialize()
{
	if (IsValid(mBuildableSubsystem))
		mBuildableSubsystem->RemoveFactoryTickHandler(this);
	mBuildableSubsystem = nullptr;

	Super::Deinitialize();
}

void USmartSmelterSubsystem::PreFactoryTick(AFGBuildableSubsystem* subsystem, float deltaTime)
{
	mTimeSinceScan += deltaTime;
	if (mTimeSinceScan < ScanInterval)
		return;
	mTimeSinceScan = 0.f;

	ScanSmelters(subsystem);
}

void USmartSmelterSubsystem::ScanSmelters(AFGBuildableSubsystem* subsystem)
{
	TArray<AFGBuildable*> smelters;
	subsystem->GetTypedBuildable(mSmelterClass, smelters);

	for (AFGBuildable* buildable : smelters)
	{
		if (AFGBuildableManufacturer* smelter = Cast<AFGBuildableManufacturer>(buildable))
			UpdateSmelter(smelter);
	}
}

void USmartSmelterSubsystem::UpdateSmelter(AFGBuildableManufacturer* smelter)
{
	// Standby is the player's call; leave it alone.
	if (!IsValid(smelter) || smelter->IsProductionPaused())
		return;

	const TSubclassOf<UFGItemDescriptor> item = PeekInputItem(smelter);
	if (item == nullptr)
		return;

	const TSubclassOf<UFGRecipe> current = smelter->GetCurrentRecipe();
	if (RecipeUsesItem(current, item))
		return;

	// SetRecipe on the server has no player to return leftovers to, so a switch with anything
	// inside would delete it. Wait until the last batch has smelted and drained out.
	const UFGInventoryComponent* input = smelter->GetInputInventory();
	const UFGInventoryComponent* output = smelter->GetOutputInventory();
	if (smelter->IsProducing() || (input != nullptr && !input->IsEmpty()) || (output != nullptr && !output->IsEmpty()))
		return;

	const TSubclassOf<UFGRecipe> recipe = FindRecipeFor(smelter, item);
	if (recipe == nullptr || recipe == current)
		return;

	UE_LOG(LogSmartSmelter, Log, TEXT("%s: %s on the belt, switching %s -> %s"),
		*smelter->GetName(), *item->GetName(),
		current != nullptr ? *current->GetName() : TEXT("no recipe"), *recipe->GetName());

	smelter->SetRecipe(recipe);
}

TSubclassOf<UFGItemDescriptor> USmartSmelterSubsystem::PeekInputItem(const AFGBuildableManufacturer* smelter)
{
	TInlineComponentArray<UFGFactoryConnectionComponent*> connections;
	smelter->GetComponents(connections);

	for (const UFGFactoryConnectionComponent* connection : connections)
	{
		if (connection->GetDirection() != EFactoryConnectionDirection::FCD_INPUT || !connection->IsConnected())
			continue;

		// Peek from the belt's side: it offers only items that have reached its end.
		const UFGFactoryConnectionComponent* feeder = connection->GetConnection();
		if (feeder == nullptr)
			continue;

		TArray<FInventoryItem> items;
		if (feeder->Factory_PeekOutput(items) && items.Num() > 0 && items[0].GetItemClass() != nullptr)
			return items[0].GetItemClass();
	}
	return nullptr;
}

TSubclassOf<UFGRecipe> USmartSmelterSubsystem::FindRecipeFor(const AFGBuildableManufacturer* smelter, TSubclassOf<UFGItemDescriptor> item)
{
	TArray<TSubclassOf<UFGRecipe>> recipes;
	smelter->GetAvailableRecipes(recipes);

	TSubclassOf<UFGRecipe> fallback = nullptr;
	for (const TSubclassOf<UFGRecipe>& recipe : recipes)
	{
		if (recipe == nullptr)
			continue;
		const TArray<FItemAmount>& ingredients = recipe.GetDefaultObject()->GetIngredients();
		if (ingredients.Num() != 1 || ingredients[0].ItemClass != item)
			continue;

		if (!IsAlternateRecipe(recipe))
			return recipe;
		if (fallback == nullptr)
			fallback = recipe;
	}
	return fallback;
}
