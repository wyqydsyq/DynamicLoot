class DL_LootSystem : WorldSystem
{
	ref ScriptCallQueue callQueue = new ScriptCallQueue();
	
	float lastTickTime = 0;
	float lootSupplyValueCap = 10000;
	
	[Attribute("0.1", UIWidgets.Auto, desc: "Dynamic Loot system tick rate, setting this too low may cause performance issues. Too high may cause delays in loot processing.", category: "Dynamic Loot - Core")]
	float tickInterval;
	
	[Attribute(desc: "FactionKey blacklist, matches will be skipped from entity catalog evaluation (useful for skipping factions with duplicated catalogs)", category: "Dynamic Loot - Entity Catalogs")]
	ref array<FactionKey> factionBlacklist;
	
	[Attribute("10.0", UIWidgets.Auto, desc: "Multiplies weighting of ammo, should be at least be enough to negate uncommonItemTypesMultiplier so magazines are more common than scopes and suppressors", category: "Dynamic Loot - Entity Catalogs")]
	float ammoMultiplier;	
	
	[Attribute("10.0", UIWidgets.Auto, desc: "Multiplies weighting of consumables", category: "Dynamic Loot - Entity Catalogs")]
	float consumableMultiplier;
		
	[Attribute("0.5", UIWidgets.Auto, desc: "Multiplies weighting of attachments, if modded attachments are too common try decreasing this or increasing their individual arsenal values", category: "Dynamic Loot - Entity Catalogs")]
	float attachmentMultiplier;
		
	[Attribute("25", UIWidgets.Auto, desc: "Multiplies all base weights, useful to increase scarcity gap between low and high value gear. If expensive items are too common try increasing it, if they are too rare try decreasing it", category: "Dynamic Loot - Entity Catalogs")]
	float scarcityMultiplier;
	
	[Attribute("false", UIWidgets.Auto, desc: "Enable creation of dynamic loot spawners. With this off the loot EntityCatalogs will still be processed for anything else that needs them, but loot spawners will not be added to prefabs", category: "Dynamic Loot - Loot Spawning")]
	bool enableLootSpawning;
	
	[Attribute("3600", UIWidgets.Auto, desc: "Time (in seconds) after spawning loot that a spawner should auto-despawn so it can spawn new items when next opened", category: "Dynamic Loot - Loot Spawning")]
	float lootDespawnTime;
	
	[Attribute("20", UIWidgets.Auto, desc: "Max items to spawn per container, setting this too high may cause performance issues.", category: "Dynamic Loot - Loot Spawning")]
	int maxLootItemsPerContainer;
	
	[Attribute("10", UIWidgets.Auto, desc: "Maximum supply value a single loot container can accumulate", category: "Dynamic Loot - Loot Spawning")]
	float maxContainerValue;
	
	[Attribute("0.05", UIWidgets.Auto, params: "0 1", desc: "Chance for a loot container to be a jackpot", category: "Dynamic Loot - Loot Spawning")]
	float jackpotContainerRate;
	
	[Attribute("10", UIWidgets.Auto, desc: "Multiplier applied to max value of jackpot containers", category: "Dynamic Loot - Loot Spawning")]
	float jackpotContainerValueMultiplier;
	
	[Attribute(desc: "ResourceName whitelist, entities with the DL_LootSpawnComponent attached and containing one of these as a substring of its ResourceName will become loot container", category: "Dynamic Loot - Loot Spawning")]
	ref array<string> whitelist;
		
	[Attribute(desc: "ResourceName blacklist, matches will be excluded from becoming loot containers even if whitelisted", category: "Dynamic Loot - Loot Spawning")]
	ref array<string> blacklist;

	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Categorizes included arsenal item types as military items", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> militaryItemTypes;
	
	[Attribute("", desc: "Gear considered MILITARY but belonging to included factions and in civilianGearTypes will not be *exclusively* MILITARY, so e.g. pistols and rifles belonging to CIV or FIA can also spawn in URBAN or RURAL areas", category: "Dynamic Loot - Loot Categorization")]
	ref array<FactionKey> civilianGearFactions;	
	
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Gear types considered MILITARY but included here and belonging to a civilian gear faction will not be *exclusively* MILITARY, so e.g. pistols and rifles belonging to CIV or FIA can also spawn in URBAN or RURAL areas", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> civilianGearTypes;
	
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Categorizes included arsenal item types as police items", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> policeItemTypes;
	
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Categorizes included arsenal item types as medical items", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> medicalItemTypes;	
	
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Categorizes included arsenal item types as industrial items", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> industrialItemTypes;	
	
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Categorizes included arsenal item types as commercial items", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> commercialItemTypes;
		
	[Attribute("1.25", UIWidgets.Auto, desc: "Multiplies spawn rate of common items (generally clothes and equipment)", category: "Dynamic Loot - Entity Catalogs")]
	float commonItemTypesMultiplier;
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Common item multiplier is applied to included types (RURAL and URBAN areas can spawn common)", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> commonItemTypes;
	
	[Attribute("0.75", UIWidgets.Auto, desc: "Multiplies spawn rate of uncommon items (generally civilian weapons and surplus military gear)", category: "Dynamic Loot - Entity Catalogs")]
	float uncommonItemTypesMultiplier;
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Uncommon item multiplier is applied to included types (URBAN areas can spawn uncommon)", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> uncommonItemTypes;	
	
	[Attribute("0.45", UIWidgets.Auto, desc: "Multiplies spawn rate of rare items (generally modern weapons, NVGs/thermals etc)", category: "Dynamic Loot - Entity Catalogs")]
	float rareItemTypesMultiplier;
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType), desc: "Rare item multiplier is applied to included types", category: "Dynamic Loot - Loot Categorization")]
	ref array<SCR_EArsenalItemType> rareItemTypes;
	
	ref array<SCR_EArsenalItemType> itemBlacklist = {
		SCR_EArsenalItemType.MORTARS,
		SCR_EArsenalItemType.HELICOPTER,
		SCR_EArsenalItemType.VEHICLE
	};
	
	// list of spawn components in the world, each will be checked if it should be a "lootable" and spawn a container prefab if so
	ref array<DL_LootSpawnComponent> spawnComponents = {};
	ref array<DL_LootSpawn> spawns = {};
	
	// list of processed resource names, grouped by FactionKey of the catalog they originated from
	ref map<ResourceName, FactionKey> processedResources = new map<ResourceName, FactionKey>();
	
	// raw weighted loot data lists of evaluated entity catalog items based on their arsenal supply costs
	bool lootDataReady = false;
	ref array<SCR_EntityCatalogEntry> lootData = {};
	ref SCR_WeightedArray<SCR_EntityCatalogEntry> lootDataWeighted = new SCR_WeightedArray<SCR_EntityCatalogEntry>();
	ref map<DL_LootCategory, ref SCR_WeightedArray<SCR_EntityCatalogEntry>> categoryLootData = new map<DL_LootCategory, ref SCR_WeightedArray<SCR_EntityCatalogEntry>>();
	ref SCR_EntityCatalog lootCatalog;
	
	bool vehicleDataReady = false;
	ref array<SCR_EntityCatalogEntry> vehicleData = {};
	ref SCR_EntityCatalog vehicleCatalog;
	
	ref array<EEntityCatalogType> labels = {
		EEntityCatalogType.ITEM,
	};
	
	ref array<DL_LootSpawnCategoryProviderComponent> categoryProviders = {};
	ref array<DL_LootSpawnRarityProviderComponent> rarityProviders = {};
	
	vector worldMins;
	vector worldMaxs;
	
	void DL_LootSystem()
	{
		PrintFormat("DL_LootSystem: Constructed");
	}
	
    override static void InitInfo(WorldSystemInfo outInfo)
    {
		super.InitInfo(outInfo);
        outInfo
            .SetAbstract(false)
            .SetLocation(ESystemLocation.Both)
            .AddPoint(ESystemPoint.FixedFrame);
    }
	
	override void OnInit()
	{
		// init category-specific loot data sets
		array<DL_LootCategory> categories = {};
		SCR_Enum.GetEnumValues(DL_LootCategory, categories);
		foreach (DL_LootCategory category : categories)
			categoryLootData.Insert(category, new SCR_WeightedArray<SCR_EntityCatalogEntry>());
		
		
		GetGame().GetWorld().GetBoundBox(worldMins, worldMaxs);
		callQueue.Call(ReadLootCatalogs);
		callQueue.Call(ReadVehicleCatalogs);
	}
	
	static DL_LootSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;

		return DL_LootSystem.Cast(world.FindSystem(DL_LootSystem));
	}
	
	override void OnUpdate(ESystemPoint point)
	{
		float time = GetGame().GetWorld().GetFixedTimeSlice();
		callQueue.Tick(0.001 * time);
		lastTickTime += time;
		if (lastTickTime < tickInterval)
			return;
		lastTickTime = 0;
		
		if (!Replication.IsServer())
			return;
		
		if (!enableLootSpawning)
			return;
		
		// create trigger prefabs for each eligible spawner component
		// limit per tick to avoid tanking server too hard at start
		// as some maps will easily have 10k+ spawns to process, doing all that at once is bad
		for (int i; i < Math.Min(100, spawnComponents.Count()); i++)
		{
			DL_LootSpawnComponent comp = spawnComponents[i];
			if (!comp)
			{
				spawnComponents.Remove(i);
				continue;
			}
			
			if (CanSpawnLoot(comp.GetOwner().ToString()))
				CreateContainerTrigger(comp.GetOwner());
			
			spawnComponents.Remove(i);
		}
	}
	
	bool CanSpawnLoot(ResourceName resource)
	{
		bool allowed = false;
		
		foreach (string blacklistType : blacklist)
		{
			if (resource.Contains(blacklistType))
				return false;
				
		}
		
		foreach (string type : whitelist)
		{
			if (resource.Contains(type))
			{
				allowed = true;
				break;
			}
		}
		
		return allowed;
	}
	
	DL_LootContainerTrigger CreateContainerTrigger(IEntity owner)
	{
		vector transform[4];
		owner.GetTransform(transform);
		EntitySpawnParams params = new EntitySpawnParams();
		params.Parent = owner;
		DL_LootContainerTrigger trigger = DL_LootContainerTrigger.Cast(GetGame().SpawnEntityPrefabEx("{A4F6AB6D2E1B668C}Prefabs/DL_LootContainerTrigger.et", false, params: params));
		return trigger;
	}
	
	// owner is parent of trigger/container
	void CreateLootContainer(IEntity owner)
	{
		vector mins;
		vector maxes;
		owner.GetBounds(mins, maxes);
		vector pos = Vector((mins[0] + maxes[0]) * 0.5, (mins[1] + maxes[1]) * 0.5, (mins[2] + maxes[2]) * 0.5);
		
		vector transform[4];
		owner.GetWorldTransform(transform);

		EntitySpawnParams params = new EntitySpawnParams();
		params.Parent = owner;
		params.Transform[3] = pos;
		params.TransformMode = ETransformMode.LOCAL;
		
		ref DL_LootSpawn spawn = DL_LootSpawn.Cast(GetGame().SpawnEntityPrefabEx("{1DAAEE444AEA4BB5}Prefabs/DL_LootContainer.et", true, null, params));
		spawns.Insert(spawn);
		
		DL_LootSpawnComponent comp = DL_LootSpawnComponent.Cast(owner.FindComponent(DL_LootSpawnComponent));
		
		// copy category filter specified directly on component
		if (comp && !spawn.categoryFilter.Count() && comp.categoryFilter)
			spawn.categoryFilter.Copy(comp.categoryFilter);
		
		// otherwise evaluate category filter based on providers in proximity
		if (!spawn.categoryFilter.Count())
		{
			// check registered category providers and apply categories of nearest/most direct
			DL_LootSpawnCategoryProviderComponent categoryProvider = GetSpawnCategoryProvider(spawn);
			if (categoryProvider && categoryProvider.categories.Count())
				spawn.categoryFilter.Copy(categoryProvider.categories);
			// fallback to rural category when no providers in proximity
			else
				spawn.categoryFilter = {DL_LootCategory.RURAL};
		}
		
		// look for nearest rarity provider and multiply max supply value by its multiplier / distance from provider
		DL_LootSpawnRarityProviderComponent rarityProvider = GetSpawnRarityProvider(spawn);
		if (rarityProvider)
			spawn.maxSupplyValue *= rarityProvider.multiplier / Math.Max(1, vector.Distance(rarityProvider.GetOwner().GetOrigin(), spawn.GetOrigin()) / rarityProvider.radius);
		else
		{
			spawn.maxSupplyValue *= 20 / Math.Max(1, vector.Distance((worldMins + worldMaxs) * 0.5, spawn.GetOrigin()) / 1000);
		}
		
		// set spawn volume based on parent bbox volume which should roughly represent its physical space in world
		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(spawn.FindComponent(SCR_UniversalInventoryStorageComponent));
		float parentVolume = Math.Max(10, ((maxes[0] - mins[0]) * (maxes[1] - mins[1]) * (maxes[2] - mins[2])) * 100);
		spawn.maxVolume = parentVolume;
		storage.SetAdditionalVolume(parentVolume);
		
		// how to apply parent mesh dimensions to spawn for action contexts w/o rendering xob?? :(
		// if we can do this in a way detected by storage component's capacity coefficient mode would be ideal
		// and allow removing the above manual bbox calculation
		//VObject vobj = owner.GetVObject();
		//spawn.SetObject(vobj, "");
		//spawn.SetFlags(~EntityFlags.VISIBLE);
		//spawn.SetOrigin(owner.GetOrigin());
	}
	
	DL_LootSpawnCategoryProviderComponent GetSpawnCategoryProvider(DL_LootSpawn spawn)
	{
		vector position = spawn.GetOrigin();
		ref DL_LootSpawnCategoryProviderComponent closestGenericProvider;
		ref DL_LootSpawnCategoryProviderComponent closest;
		float closestDistance = float.MAX;

		foreach (DL_LootSpawnCategoryProviderComponent provider : categoryProviders)
		{
			// skip providers that didn't detect any categories for themselves
			if (!provider.categories.Count())
				continue;
			
			float distance = vector.Distance(provider.GetOwner().GetOrigin(), position);
			if (distance < closestDistance && distance <= provider.radius)
			{
				closestDistance = distance;
				closest = provider;
				
				if (provider.categories.Contains(DL_LootCategory.URBAN) || provider.categories.Contains(DL_LootCategory.RURAL))
					closestGenericProvider = provider;
			}
		}
		
		// copy closest generic provider category to closest if not generic
		// to combine e.g. a city's URBAN category with the COMMERCIAL category of a shop in the city
		if (closest != closestGenericProvider && closestGenericProvider && closestGenericProvider.categories.Count())
			closest.categories.Insert(closestGenericProvider.categories[0]);
		
		//if (closest)
		//	PrintFormat("DL_LootSystem: %1 found category provider %2 (%3m) with categories %4", spawn, closest.GetOwner(), closestDistance, closest.categories);
		
		return closest;
	}
	
	DL_LootSpawnRarityProviderComponent GetSpawnRarityProvider(DL_LootSpawn spawn)
	{
		if (!rarityProviders.Count())
		{
			PrintFormat("DL_LootSystem: Defaulting to 20x rarity multiplier at world center due to no rarity providers found in-world, add instances of `DL_LootSpawnProvider.et` to your world to suppress this warning.", level: LogLevel.WARNING);
			return null;
		}
		
		vector position = spawn.GetOrigin();
		ref DL_LootSpawnRarityProviderComponent closest;
		float closestDistance = float.MAX;

		foreach (DL_LootSpawnRarityProviderComponent provider : rarityProviders)
		{
			float distance = vector.Distance(provider.GetOwner().GetOrigin(), position);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closest = provider;
			}
		}
		
		//if (closest)
		//	PrintFormat("DL_LootSystem: %1 found rarity provider %2 (%3m) with max mult = %4", spawn, closest.GetOwner(), closestDistance, closest.multiplier);

		return closest;
	}
	
	ref ScriptInvoker Event_LootCatalogsReady = new ScriptInvoker;
	SCR_EntityCatalog ReadLootCatalogs()
	{
		SCR_EntityCatalog entityCatalog = new SCR_EntityCatalog();
		entityCatalog.SetCatalogType(EEntityCatalogType.ITEM);
		entityCatalog.SetEntityList({});
		
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach (Faction f : factions)
		{
			SCR_Faction fact = SCR_Faction.Cast(f);
			if (!fact || factionBlacklist.Contains(f.GetFactionKey()))
				continue;

			SCR_EntityCatalog factionCatalog = fact.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM);
			if (!factionCatalog)
			{
				PrintFormat("DL_LootSystem: Unable to find catalog of type %1 for %2", EEntityCatalogType.ITEM, f, LogLevel.ERROR);
				continue;
			}
			
			if (factionCatalog.GetCatalogType() == EEntityCatalogType.WEAPONS_TRIPOD)
				continue;
			
			// merge faction catalog into global catalog by ref
			entityCatalog.MergeEntityListRef(factionCatalog.GetEntityListRef(), factionCatalog.GetCatalogType(), f.GetFactionKey());
		}
		
		entityCatalog.GetEntityList(lootData);
		if (lootData.IsEmpty())
			return null;
		
		PrintFormat("DL_LootSystem: Found %1 items in EntityCatalogs from %2 factions, calculating entry weights...", lootData.Count(), factions.Count());
		lootCatalog = entityCatalog;
		
		// @TODO add a means for weighting Vehicles based on SCR_EditableVehicleComponent's "CAMPAIGN" budget cost
		lootDataWeighted = new SCR_WeightedArray<SCR_EntityCatalogEntry>();
		foreach (SCR_EntityCatalogEntry entry : lootData)
		{
			float weight = CalculateEntryWeight(entry);
			if (!weight)
				continue;
			
		 	lootDataWeighted.Insert(entry, weight);
			
			array<string> categoryNames = {};
			array<DL_LootCategory> categories = CategorizeEntry(entry);
			if (categories.Count())
			{
				foreach (DL_LootCategory category : categories)
				{
					SCR_WeightedArray<SCR_EntityCatalogEntry> categoryData = categoryLootData.Get(category);
					if (!categoryData)
					{
						PrintFormat("DL_LootSystem: Unable to find category %1 in categoryLootData for %2!", SCR_Enum.GetEnumName(DL_LootCategory, category), entry, level: LogLevel.ERROR);
						categoryLootData.Get(DL_LootCategory.RURAL).Insert(entry, weight);
						break;
					}
					else
					{
						categoryNames.Insert(SCR_Enum.GetEnumName(DL_LootCategory, category));
						categoryData.Insert(entry, weight);
					}
				}
			}
			
			PrintFormat("DL_LootSystem: [%3](%4) Item %1 = %2 weight", entry.GetPrefab(), weight, processedResources.Get(entry.GetPrefab()), categoryNames);
		}
		
		lootDataReady = true;
		Event_LootCatalogsReady.Invoke(lootDataWeighted);
		
		return entityCatalog;
	}
	
	array<DL_LootCategory> CategorizeEntry(SCR_EntityCatalogEntry entry)
	{
		ref array<DL_LootCategory> categories = {};
		SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!item)
			return {DL_LootCategory.RURAL};
		
		SCR_EArsenalItemType itemType = item.GetItemType();
		SCR_EArsenalItemMode itemMode = item.GetItemMode();
		FactionKey factionKey = processedResources.Get(entry.GetPrefab());
		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
		
		if (
			(
				faction.IsMilitary()
				&& militaryItemTypes.Contains(itemType)
			)
			|| item.GetItemResourceName().Contains("Equipment/Tripods")
			|| item.GetItemResourceName().Contains("Equipment/Mortars")
			|| item.GetItemResourceName().Contains("Items/NVG")
		)
		{
			categories.Insert(DL_LootCategory.MILITARY);
			
			// restrict items tagged as MILITARY but not considered "civilian" to military spawns
			// e.g. old/basic weapons beloning to FIA should also be findable in POLICE, URBAN or RURAL areas
			// otherwise return out early with only MILITARY category for all other military items
			// so they are exclusive to areas categorized as military
			if (!civilianGearTypes.Contains(itemType) && !civilianGearFactions.Contains(factionKey))
				return categories;
		}
		
		if (policeItemTypes.Contains(itemType))
			categories.Insert(DL_LootCategory.POLICE);
		
		if (medicalItemTypes.Contains(itemType))
			categories.Insert(DL_LootCategory.MEDICAL);
		
		if (industrialItemTypes.Contains(itemType))
			categories.Insert(DL_LootCategory.INDUSTRIAL);
		
		if (commercialItemTypes.Contains(itemType))
			categories.Insert(DL_LootCategory.COMMERCIAL);
		
		if (
			commonItemTypes.Contains(itemType)
			|| (
				uncommonItemTypes.Contains(itemType)
				&& (civilianGearFactions.Contains(factionKey) && civilianGearTypes.Contains(itemType))
			)
		)
			categories.Insert(DL_LootCategory.URBAN);
		
		if (
			commonItemTypes.Contains(itemType)
			|| (civilianGearFactions.Contains(factionKey) && civilianGearTypes.Contains(itemType))
		)
			categories.Insert(DL_LootCategory.RURAL);
		
		// fallback for gear that doesn't fit into any specific category, allow spawning in urban and rural
		if (!categories.Count())
		{
			categories.Insert(DL_LootCategory.URBAN);
			categories.Insert(DL_LootCategory.RURAL);
		}
		
		return categories;
	}
	
	float CalculateEntryWeight(SCR_EntityCatalogEntry entry)
	{
		float value = 1;
		SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!item)
			return 0;
		
		SCR_EArsenalItemType itemType = item.GetItemType();
		if (!itemType || itemBlacklist.Contains(itemType))
			return 0;
		
		SCR_EArsenalItemMode itemMode = item.GetItemMode();
		if (
			itemMode == SCR_EArsenalItemMode.WEAPON
			|| itemMode == SCR_EArsenalItemMode.WEAPON_VARIANTS
			|| itemMode == SCR_EArsenalItemMode.ATTACHMENT
		)
			item.PostInitData(entry);
		
		value = Math.Max(item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT), 1);
					
		if (commonItemTypes.Contains(itemType))
			value = value / commonItemTypesMultiplier;
		
		if (uncommonItemTypes.Contains(itemType))
			value = value / uncommonItemTypesMultiplier;

		if (rareItemTypes.Contains(itemType) || item.GetItemResourceName().Contains("Items/NVG"))
			value = value / rareItemTypesMultiplier;
		
		if (itemMode == SCR_EArsenalItemMode.CONSUMABLE)
			value = value / consumableMultiplier;	
				
		if (
			itemMode == SCR_EArsenalItemMode.AMMUNITION
			&& itemType != SCR_EArsenalItemType.MACHINE_GUN // already common enough to find more than you can carry
			&& itemType != SCR_EArsenalItemType.ROCKET_LAUNCHER // same as above
		)
			value = value / ammoMultiplier;	
				
		if (itemMode == SCR_EArsenalItemMode.ATTACHMENT || itemType == SCR_EArsenalItemType.WEAPON_ATTACHMENT)
			value = value / attachmentMultiplier;
		
		// base item rarity on inverse of supply cost of 10k to create some headroom
		// for multiplier differences
		// e.g.
		// 10000 - (60 supply) = 9940 weight
		// 10000 - (320 supply) = 9680 weight
		// 10000 - (470 supply) = 9530 weight
		// 10000 - (1240 supply) = 8760 weight
		float weight = lootSupplyValueCap - Math.Min(Math.Max(value, 1) * scarcityMultiplier, lootSupplyValueCap);
		
		return weight;
	}
	
	ref ScriptInvoker Event_VehicleCatalogsReady = new ScriptInvoker;
	SCR_EntityCatalog ReadVehicleCatalogs()
	{
		ref SCR_EntityCatalog entityCatalog = new SCR_EntityCatalog();
		entityCatalog.SetCatalogType(EEntityCatalogType.VEHICLE);
		entityCatalog.SetEntityList({});

		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach (Faction f : factions)
		{
			SCR_Faction fact = SCR_Faction.Cast(f);
			if (!fact)
				continue;
			
			SCR_EntityCatalog factionCatalog = fact.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
			if (factionCatalog)
				entityCatalog.MergeEntityListRef(factionCatalog.GetEntityListRef(), factionCatalog.GetCatalogType(), f.GetFactionKey());
		}

		entityCatalog.GetEntityList(vehicleData);
		if (vehicleData.IsEmpty())
			return null;
		
		vehicleCatalog = entityCatalog;
		vehicleDataReady = true;
		Event_VehicleCatalogsReady.Invoke(vehicleData);
		
		return entityCatalog;
	}
}

enum DL_LootCategory {
	// RURAL and URBAN are generic area categories that can apply to a spawn in combination with one of the specific categories below
	// at least one of these categories will always be set for an item and both always include common item types + items matching "civilian" faction and type whitelists
	RURAL,
	URBAN,
	
	// optional specific area category that can be combined with generic categories above
	// e.g. factory in a city will be URBAN + INDUSTRIAL, farms will be RURAL + INDUSTRIAL
	// military bases will generally always be URBAN + MILITARY
	MILITARY,
	POLICE,
	MEDICAL,
	INDUSTRIAL,
	COMMERCIAL
}