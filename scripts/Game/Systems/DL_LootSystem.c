class DL_LootSystem : WorldSystem
{
	float lastTickTime = 0;
	[Attribute("0.1", UIWidgets.Auto, desc: "Dynamic Loot system tick rate, setting this too low may cause performance issues. Too high may cause delays in loot processing.", category: "Dynamic Loot")]
	float tickInterval;
	
	[Attribute("false", UIWidgets.Auto, desc: "Enable creation of dynamic loot spawners. With this off the loot EntityCatalogs will still be processed for anything else that needs them, but loot spawners will not be added to prefabs", category: "Dynamic Loot/Loot")]
	bool enableLootSpawning;
	
	[Attribute("5.0", UIWidgets.Auto, desc: "Amplifies scarcity gap between low and high value gear, 5.0 works well for vanilla where few items are >200 supply, if high-end modded gear is too common try increasing this value", category: "Dynamic Loot/Loot")]
	float scarcityMultiplier;
	
	[Attribute("20", UIWidgets.Auto, desc: "Max items to spawn per container, setting this too high may cause performance issues.", category: "Dynamic Loot/Loot")]
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
	
		
	[Attribute("1.5", UIWidgets.Auto, desc: "Multiplies spawn rate of common items (generally clothes and equipment) be?", category: "Dynamic Loot")]
	float commonItemTypesMultiplier;
	SCR_EArsenalItemType commonItemTypes = SCR_EArsenalItemType.HEAL
		| SCR_EArsenalItemType.WEAPON_ATTACHMENT
		| SCR_EArsenalItemType.TORSO
		| SCR_EArsenalItemType.LEGS
		| SCR_EArsenalItemType.FOOTWEAR
		| SCR_EArsenalItemType.HANDWEAR
		| SCR_EArsenalItemType.EQUIPMENT;
	
	[Attribute("0.5", UIWidgets.Auto, desc: "Multiplies spawn rate of uncommon items (generally guns and explosives) be?", category: "Dynamic Loot")]
	float uncommonItemTypesMultiplier;
	SCR_EArsenalItemType uncommonItemTypes = SCR_EArsenalItemType.RIFLE
		| SCR_EArsenalItemType.WEAPON_ATTACHMENT
		| SCR_EArsenalItemType.EXPLOSIVES
		| SCR_EArsenalItemType.ROCKET_LAUNCHER
		| SCR_EArsenalItemType.SNIPER_RIFLE
		| SCR_EArsenalItemType.MACHINE_GUN
		| SCR_EArsenalItemType.LETHAL_THROWABLE;
	
	SCR_EArsenalItemType itemBlacklist = SCR_EArsenalItemType.MORTARS | SCR_EArsenalItemType.HELICOPTER | SCR_EArsenalItemType.VEHICLE;
	
	// list of spawn components in the world, each will be checked if it should be a "lootable" and spawn a container prefab if so
	ref array<DL_LootSpawnComponent> spawnComponents = {};
	ref array<DL_LootSpawn> spawns = {};
	ref SCR_WeightedArray<SCR_EntityCatalogEntry> lootData = new SCR_WeightedArray<SCR_EntityCatalogEntry>();
	ref array<SCR_EntityCatalogEntry> vehicleData = {};
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
		GetGame().GetCallqueue().Call(ReadLootCatalogs, lootData);
		GetGame().GetCallqueue().CallLater(ReadVehicleCatalogs, 5000, false, vehicleData);
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
		
		float time = GetGame().GetWorld().GetFixedTimeSlice();
		lastTickTime += time;
		if (lastTickTime < tickInterval)
			return;
		
		lastTickTime = 0;
		
		if (!enableLootSpawning)
			return;
		
		// create spawner entity prefabs for each eligible spawner component
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
		
		array<string> segments = {};
		string resource = owner.ToString();
		resource.Split("/", segments, true);
		bool allowed = false;
		foreach (string type : whitelist)
		{
			foreach (string segment : segments)
			{
				if (resource.Contains(type))
				{
					allowed = true;
					break 2;
				}
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
		
		// set spawn volume based on parent char collision volume which should represent its physical space in world
		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(spawn.FindComponent(SCR_UniversalInventoryStorageComponent));
		float parentVolume = Math.Max(10, ((maxes[0] - mins[0]) * (maxes[1] - mins[1]) * (maxes[2] - mins[2])) * 100);
		spawn.maxVolume = parentVolume;
		storage.SetAdditionalVolume(parentVolume);
		
		SCR_UniversalInventoryStorageComponent inv = SCR_UniversalInventoryStorageComponent.Cast(spawn.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!inv)
			return;
		
		OnContainerToggled(spawn, false);
		
		if (!spawn.categoryFilter && resource.Contains("FirstAidBox"))
		{
			spawn.categoryFilter = SCR_EArsenalItemType.HEAL;
			spawn.minLootItems = 2;
		}
		
		if (!spawn.categoryFilter && resource.Contains("Kitchen"))
			spawn.categoryFilter = SCR_EArsenalItemType.HEAL | SCR_EArsenalItemType.EQUIPMENT | SCR_EArsenalItemType.HANDWEAR;
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
	
	bool ReadLootCatalogs(out SCR_WeightedArray<SCR_EntityCatalogEntry> data)
	{
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions); //{"US", "USSR", "FIA", "CIV"};
		
		PrintFormat("DL_LootSystem: Reading EntityCatalogs for %1 factions", factions.Count());
		
		array<SCR_EntityCatalogEntry> entries = {};
		foreach(Faction f : factions)
		{
			SCR_Faction fact = SCR_Faction.Cast(f);
			if (!fact)
				continue;
			 
			foreach(EEntityCatalogType label : labels)
			{
				SCR_EntityCatalog factionCatalog = fact.GetFactionEntityCatalogOfType(label);
				if (!factionCatalog)
				{
					PrintFormat("DL_LootSystem: Unable to find catalog of type %1 for %2", label, f);
					continue;
				}
				
				factionCatalog.MergeEntityList(entries);
			}
		}
		
		foreach(SCR_EntityCatalogEntry entry : entries)
		{
			float value = 1;
			SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
			if (item)
				value = Math.Max(item.GetSupplyCost(SCR_EArsenalSupplyCostType.GADGET_ARSENAL), 1);
			
			if (!item)
				continue;
			
			SCR_EArsenalItemType itemType = item.GetItemType();
			if (!itemType)
				continue;
			
			if (itemType == itemBlacklist)
				continue;
						
			if (itemType == commonItemTypes)
				value = value * commonItemTypesMultiplier;
			
			if (itemType == uncommonItemTypes)
				value = value / uncommonItemTypesMultiplier;
			
			// base item rarity of inverse of supply cost % of 1k
			// e.g.
			// 100 - (40 supply) / 1000 * 100 = 96 weight
			// 100 - (120 supply) / 1000 * 100 = 88 weight
			// 100 - (320 supply) / 1000 * 100 = 67 weight
			float weight = 100 - (Math.Min(Math.Max(value, 1) * scarcityMultiplier, 999) / 1000 * 100);
			data.Insert(entry, weight);
		}

		if (data.IsEmpty())
			return false;
		
		PrintFormat("DL_LootSystem: Found %1 items in %2 EntityCatalogs", data.Count(), factions.Count());
		
		return true;
	}
	
	bool ReadVehicleCatalogs(out array<SCR_EntityCatalogEntry> data)
	{
		SCR_EntityCatalogManagerComponent comp = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!comp)
			return false;
		
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);
		foreach(Faction f : factions)
		{
			SCR_Faction fact = SCR_Faction.Cast(f);
			if (!fact)
				continue;
			
			SCR_EntityCatalog factionCatalog = fact.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
			if (factionCatalog)
				factionCatalog.MergeEntityList(data);
		}

		if (data.IsEmpty())
			return false;
		
		return true;
	}
}
