# Smart Smelter: plan

## What it does
A Smelter picks its own recipe from whatever arrives on its input belt. Feed it Iron Ore and it
runs Iron Ingot; switch the belt to Copper Ore and, once the iron already inside has finished
smelting and drained out, it switches to Copper Ingot. A Smelter with no recipe set starts
working as soon as ore reaches it.

## How it works
- `USmartSmelterSubsystem` is a world subsystem that only exists on the server (single player
  host, listen server host, dedicated server). Clients get the new recipe through the
  replicated `mCurrentRecipe`, as with a recipe a player picked by hand.
- It registers as an `IFGFactoryTickHandlerInterface` on `AFGBuildableSubsystem`.
  `PreFactoryTick` runs sequentially on the game thread before the parallel factory tick,
  which the SDK documents as the safe place to read and write any buildable. Peeking a belt
  from anywhere else could race the conveyor chain workers.
- Once a second it walks every `Build_SmelterMk1_C` (`GetTypedBuildable`), and for each one:
  1. Finds its connected input port and peeks the item waiting at the end of the belt
     (`UFGFactoryConnectionComponent::Factory_PeekOutput` on the belt's side). A belt only
     offers items that have reached its end, so this sees the next item, not one further back.
  2. Skips it if the current recipe already uses that item.
  3. Skips it unless both the input and output inventories are empty. `SetRecipe` on the
     server has no player to hand leftovers to, so switching with items inside would delete
     them. Waiting also means a mixed belt finishes each batch before flipping.
  4. Picks, from `GetAvailableRecipes` (unlocked recipes only), one whose single ingredient
     is the peeked item. Recipes that are not alternates win; otherwise the first match.
  5. Calls `SetRecipe`.
- Items with no smelter recipe (say, Limestone) leave the Smelter alone.

## Scope for 1.0
- Smelters only. Each smelter ingredient maps to one recipe, so the choice is never a guess.
  Constructors would be ambiguous (Iron Ingot makes rods, plates or screws).
- No config menu. Possible later: an on/off toggle, and a per-building opt-out.

## Risks / to test in game
- Belt peek through 1.2 conveyor chain actors (the chain-owned branch of peek is native and
  should work; unverified in game).
- Overclocked/somerslooped smelters: `SetRecipe` should keep potential and boost.
- Multiplayer client sees the new recipe in the UI (should, `mCurrentRecipe` replicates).
- Smelter with its UI open while the recipe flips.
- Lift (vertical) input instead of a belt.
- Blueprint designer smelters (expected harmless).
