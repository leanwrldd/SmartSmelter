# Smart Smelter

A [Satisfactory](https://www.satisfactorygame.com/) mod that lets Smelters choose their own recipe.

Built with [SML](https://ficsit.app/mod/SML) (Satisfactory Mod Loader) 3.12 for Satisfactory 1.2.

## Features

- A Smelter sets its recipe from the ore waiting on its input belt
- Switch a belt from Iron Ore to Copper Ore and the Smelter finishes what is inside, lets it
  drain out, then switches to Copper Ingot
- A Smelter with no recipe starts as soon as ore reaches it
- Never deletes items: a Smelter only switches once it is empty and idle
- Smelters on standby are left alone
- Only unlocked recipes are used; standard recipes are preferred over alternates

## Not yet tested

- Everything in game. See [PLAN.md](PLAN.md) for the test list.
- Multiplayer and dedicated servers. The mod runs on the host or server; the recipe replicates
  to clients as it does when a player sets it by hand.

## Building

Junction the repo into an SML project's `Mods/GameFeatures/SmartSmelter`, build the editor
target, then package with Alpakit. `Scripts/create_game_feature_data.py` recreates the
`FGGameFeatureData` asset through the editor's Python commandlet if it is ever lost.
