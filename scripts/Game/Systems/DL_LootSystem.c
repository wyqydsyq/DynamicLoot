class DL_LootSystem : WorldSystem
{
	float lastTickTime = 0;
	[Attribute("0.1", UIWidgets.Auto, desc: "Dynamic Loot system tick rate, setting this too low may cause performance issues. Too high may cause delays in loot processing.", category: "Dynamic Loot - Core")]
	float tickInterval;
	
	[Attribute("false", UIWidgets.Auto, desc: "Enable creation of dynamic loot spawners. With this off the loot EntityCatalogs will still be processed for anything else that needs them, but loot spawners will not be added to prefabs", category: "Dynamic Loot - Loot Spawning")]
	bool enableLootSpawning;
	
	[Attribute("3600", UIWidgets.Auto, desc: "Time (in seconds) after spawning loot that a spawner should auto-despawn so it can spawn new items when next opened", category: "Dynamic Loot - Loot Spawning")]
	float lootDespawnTime;
	
	[Attribute("2.5", UIWidgets.Auto, desc: "Amplifies scarcity gap between low and high value gear, 2.5 works well for vanilla where few items are >200 supply, if high-end modded gear is too common try increasing this value", category: "Dynamic Loot - Loot Spawning")]
	float scarcityMultiplier;
	
	[Attribute("20", UIWidgets.Auto, desc: "Max items to spawn per container, setting this too high may cause performance issues.", category: "Dynamic Loot - Loot Spawning")]
	int maxLootItemsPerContainer;
	
	//[Attribute(desc: "ResourceName whitelist, entities with the DL_LootSpawnComponent attached and containing one of these as a substring of its ResourceName will become loot container", category: "Dynamic Loot/Loot")]
	ref array<string> whitelist = {
		"Cupboard",
		"Cabinet",
		"Dresser",
		"Wardrobe",
		"Bookshelf",
		"Desk_",
		"DeskSchool",
		"Fridge",
		"Pantry",
		"Nightstand",
		"Safe",
		"Bin",
		"Basket",
		"Bucket",
		"Crate",
		"CardboardBox",
		"BoxWooden",
		"Chest",
		"Kitchen",
		"FirstAidBox",
		"Workbench",
		"MarsBox",
		"CargoContainer",
		"GarbageContainer",
		"TrashBin",
		"Suitcase",
		"CashierShop",
		"CounterShop",
		"CoolingBoxShop",
		"CabinetCardFile",
		"BeerKeg",
		"ShelfShop",
		"ShelfUniversal",
		"ShoppingCart",
		"Caravan",
		"TentBig",
		"TentSmall",
		"Rack_",
		"WeaponRack"
	};
		
	ref array<string> blacklist = {
		"Vehicle",
		"Workbench_Vice",
		"KitchenHood",
		"CoatRack",
		"TowelRack"
	};
		
	[Attribute("3.0", UIWidgets.Auto, desc: "Multiplies spawn rate of ammo, should be at least be enough to negate uncommonItemTypesMultiplier so magazines are more common than scopes and suppressors", category: "Dynamic Loot - Loot Spawning")]
	float ammoMultiplier;	
		
	[Attribute("0.5", UIWidgets.Auto, desc: "Multiplies spawn rate of attachments, if modded attachments are too common try decreasing this or increasing their individual arsenal values", category: "Dynamic Loot - Loot Spawning")]
	float attachmentMultiplier;
	
	[Attribute("1.25", UIWidgets.Auto, desc: "Multiplies spawn rate of common items (generally clothes and equipment)", category: "Dynamic Loot - Loot Spawning")]
	float commonItemTypesMultiplier;
	ref array <SCR_EArsenalItemType> commonItemTypes = {
		SCR_EArsenalItemType.HEAL,
		SCR_EArsenalItemType.WEAPON_ATTACHMENT,
		SCR_EArsenalItemType.TORSO,
		SCR_EArsenalItemType.LEGS,
		SCR_EArsenalItemType.FOOTWEAR,
		SCR_EArsenalItemType.HANDWEAR,
		SCR_EArsenalItemType.EQUIPMENT,
		SCR_EArsenalItemType.PISTOL
	};
	
	[Attribute("0.75", UIWidgets.Auto, desc: "Multiplies spawn rate of uncommon items (generally guns and explosives)", category: "Dynamic Loot - Loot Spawning")]
	float uncommonItemTypesMultiplier;
	ref array<SCR_EArsenalItemType> uncommonItemTypes = {
		SCR_EArsenalItemType.RIFLE,
		SCR_EArsenalItemType.WEAPON_ATTACHMENT,
		SCR_EArsenalItemType.EXPLOSIVES,
		SCR_EArsenalItemType.ROCKET_LAUNCHER,
		SCR_EArsenalItemType.SNIPER_RIFLE,
		SCR_EArsenalItemType.MACHINE_GUN,
		SCR_EArsenalItemType.LETHAL_THROWABLE
	};	
	
	[Attribute("0.45", UIWidgets.Auto, desc: "Multiplies spawn rate of rare items (generally rocket launchers, NVGs/thermals etc)", category: "Dynamic Loot - Loot Spawning")]
	float rareItemTypesMultiplier;
	ref array<SCR_EArsenalItemType> rareItemTypes = {
		SCR_EArsenalItemType.ROCKET_LAUNCHER
	};
	
	ref array<SCR_EArsenalItemType> itemBlacklist = {
		SCR_EArsenalItemType.MORTARS,
		SCR_EArsenalItemType.HELICOPTER,
		SCR_EArsenalItemType.VEHICLE
	};
	
	// list of spawn components in the world, each will be checked if it should be a "lootable" and spawn a container prefab if so
	ref array<DL_LootSpawnComponent> spawnComponents = {};
	ref array<DL_LootSpawn> spawns = {};
	
	// raw weighted loot data lists of evaluated entity catalog items based on their arsenal supply costs
	bool lootDataReady = false;
	ref array<SCR_EntityCatalogEntry> lootData = {};
	ref SCR_WeightedArray<SCR_EntityCatalogEntry> lootDataWeighted = new SCR_WeightedArray<SCR_EntityCatalogEntry>();
	ref SCR_EntityCatalog lootCatalog;
	
	bool vehicleDataReady = false;
	ref array<SCR_EntityCatalogEntry> vehicleData = {};
	ref SCR_EntityCatalog vehicleCatalog;
	
	ref array<EEntityCatalogType> labels = {
		EEntityCatalogType.ITEM,
	};
	
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
            .AddPoint(ESystemPoint.Frame);
    }
	
	override void OnInit()
	{
		PrintFormat("DL_LootSystem: OnInit");
		
		GetGame().GetCallqueue().Call(ReadLootCatalogs);
		GetGame().GetCallqueue().Call(ReadVehicleCatalogs);
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
		if (!Replication.IsServer()) // only calculate updates on server, changes are broadcast to clients
			return;
		
		if (!enableLootSpawning)
			return;
		
		float time = GetGame().GetWorld().GetFixedTimeSlice();
		lastTickTime += time;
		if (lastTickTime < tickInterval)
			return;
		lastTickTime = 0;
		
		// create spawner entity prefabs for each eligible spawner component
		// limit to 250 per tick to avoid tanking server too hard at start
		// as most maps will easily have 10k+ spawns to process, doing all that at once is bad
		for(int i; i < Math.Min(100, spawnComponents.Count()); i++)
		{
			DL_LootSpawnComponent comp = spawnComponents[i];
			if (!comp)
				continue;
			
			CreateLootContainer(comp.GetOwner());
			spawnComponents.Remove(i);
		}
	}
	
	void CreateLootContainer(IEntity owner)
	{
		GenericEntity entity = GenericEntity.Cast(owner);
		if (!owner)
			return;
		
		ResourceName resource = owner.ToString();
		
		foreach(string blacklistType : blacklist)
		{
			if (resource.Contains(blacklistType))
				return;
		}
		
		bool allowed = false;
		foreach (string type : whitelist)
		{
			if (resource.Contains(type))
			{
				allowed = true;
				break;
			}
		}
		
		if (!allowed)
			return;
		
		vector mins;
		vector maxes;
		owner.GetBounds(mins, maxes);
		vector pos = Vector((mins[0] + maxes[0]) * 0.5, (mins[1] + maxes[1]) * 0.5, (mins[2] + maxes[2]) * 0.5);
		
		vector transform[4];
		owner.GetTransform(transform);

		EntitySpawnParams params = new EntitySpawnParams();
		params.Parent = owner;

		params.Transform[0] = transform[0];
		params.Transform[1] = transform[1];
		params.Transform[2] = transform[2];
		params.Transform[3] = pos;
		params.TransformMode = ETransformMode.LOCAL;
		
		DL_LootSpawn spawn = DL_LootSpawn.Cast(GetGame().SpawnEntityPrefabEx("{E00CB9FFFC6C9339}Prefabs/DL_LootSpawn.et", true, null, params));
		owner.AddChild(spawn, -1);
		
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
		
		SCR_UniversalInventoryStorageComponent inv = SCR_UniversalInventoryStorageComponent.Cast(spawn.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!inv)
			return;
		
		OnContainerToggled(spawn, false);
	}
	
	void HandleManagerChanged(InventoryStorageManagerComponent manager)
	{}
	
	void OnContainerToggled(DL_LootSpawn spawn, bool open = false)
	{
		SCR_UniversalInventoryStorageComponent inv = SCR_UniversalInventoryStorageComponent.Cast(spawn.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!inv)
			return;
		
		SCR_ItemAttributeCollection attr = SCR_ItemAttributeCollection.Cast(inv.GetAttributes());
		attr.SetIsVisible(open);
		
		if (open)
			inv.ShowOwner();
		else
			inv.HideOwner();
	}
	
	ref ScriptInvoker Event_LootCatalogsReady = new ScriptInvoker;
	SCR_EntityCatalog ReadLootCatalogs()
	{
		SCR_EntityCatalog entityCatalog = new SCR_EntityCatalog();
		entityCatalog.SetCatalogType(EEntityCatalogType.ITEM);
		entityCatalog.SetEntityList({});
		
		array<SCR_EntityCatalogEntry> entries = {};
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach(Faction f : factions)
		{
			SCR_Faction fact = SCR_Faction.Cast(f);
			if (!fact)
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
			entityCatalog.MergeEntityListRef(factionCatalog.GetEntityListRef());
		}
		
		entityCatalog.GetEntityList(lootData);
		if (lootData.IsEmpty())
			return null;
		
		PrintFormat("DL_LootSystem: Found %1 items in %2 EntityCatalogs", lootData.Count(), factions.Count());
		lootCatalog = entityCatalog;
		lootDataWeighted = CalculateEntryWeights(lootData);
		lootDataReady = true;
		Event_LootCatalogsReady.Invoke(lootDataWeighted);
		
		return entityCatalog;
	}
	
	SCR_WeightedArray<SCR_EntityCatalogEntry> CalculateEntryWeights(array<SCR_EntityCatalogEntry> entries)
	{
		ref SCR_WeightedArray<SCR_EntityCatalogEntry> data = new SCR_WeightedArray<SCR_EntityCatalogEntry>();
		foreach(SCR_EntityCatalogEntry entry : entries)
		{
			float value = 1;
			SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
			if (!item)
				continue;
			
			SCR_EArsenalItemType itemType = item.GetItemType();
			if (!itemType || itemBlacklist.Contains(itemType))
				continue;
			
			SCR_EArsenalItemMode itemMode = item.GetItemMode();
			if (
				itemMode == SCR_EArsenalItemMode.WEAPON
				|| itemMode == SCR_EArsenalItemMode.WEAPON_VARIANTS
				|| itemMode == SCR_EArsenalItemMode.ATTACHMENT
			)
				item.PostInitData(entry);
			
			value = Math.Max(item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT) + item.GetSupplyCost(SCR_EArsenalSupplyCostType.GADGET_ARSENAL), 1);
						
			if (commonItemTypes.Contains(itemType))
				value = value / commonItemTypesMultiplier;
			
			if (uncommonItemTypes.Contains(itemType))
				value = value / uncommonItemTypesMultiplier;
				
			// @TODO figure out a clean way to categorize stuff like NVGs/thermals as rare
			if (rareItemTypes.Contains(itemType))
				value = value / rareItemTypesMultiplier;
			
			if (
				itemMode == SCR_EArsenalItemMode.AMMUNITION
				&& itemType != SCR_EArsenalItemType.MACHINE_GUN // already common enough to find more than you can carry
				&& itemType != SCR_EArsenalItemType.ROCKET_LAUNCHER // same as above
			)
				value = value / ammoMultiplier;	
					
			if (itemMode == SCR_EArsenalItemMode.ATTACHMENT)
				value = value / attachmentMultiplier;
			
			// base item rarity on inverse of supply cost % of 1k
			// e.g.
			// 100 - (40 supply) / 1000 * 100 = 96 weight
			// 100 - (120 supply) / 1000 * 100 = 88 weight
			// 100 - (320 supply) / 1000 * 100 = 67 weight
			float weight = 100 - (Math.Min(Math.Max(value, 1) * scarcityMultiplier, 999) / 1000 * 100);
			data.Insert(entry, weight);
		}
		
		return data;
	}
	
	ref ScriptInvoker Event_VehicleCatalogsReady = new ScriptInvoker;
	SCR_EntityCatalog ReadVehicleCatalogs()
	{
		SCR_EntityCatalog entityCatalog = new SCR_EntityCatalog();
		entityCatalog.SetCatalogType(EEntityCatalogType.VEHICLE);
		entityCatalog.SetEntityList({});

		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach(Faction f : factions)
		{
			SCR_Faction fact = SCR_Faction.Cast(f);
			if (!fact)
				continue;
			
			SCR_EntityCatalog factionCatalog = fact.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
			if (factionCatalog)
			{
				// merge catalogs
				entityCatalog.MergeEntityListRef(factionCatalog.GetEntityListRef());
			}
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
