class DL_LootSpawnClass : GenericEntityClass
{
}

class DL_LootSpawn : GenericEntity
{
	ref array<DL_LootCategory> categoryFilter = {};
	
	DL_LootSystem lootSystem;
	bool spawned = false;
	
	int minLootItems = 1;
	float accumulatedSupplyValue = 0;
	float maxSupplyValue = 0;
	
	float maxVolume = 100;
	float accumulatedSpawnedVolume = 0;
	
	float preventDespawnDistanceSq = 50 * 50;
	
	void DL_LootSpawn(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		lootSystem = DL_LootSystem.GetInstance();
		if (lootSystem)
			maxSupplyValue = lootSystem.maxContainerValue;
	}
	
	void SpawnLoot()
	{
		int attemptLimit = 5;
		int attempts = 0;
		float maxValue = maxSupplyValue;
		bool isJackpot = Math.RandomFloat(0, 1) <= lootSystem.jackpotContainerRate;
		if (isJackpot)
			maxValue *= lootSystem.jackpotContainerValueMultiplier;
		
		//PrintFormat("DL_LootSystem: Spawning loot for %1 from %2 categories (%3)", this, categoryFilter.Count(), categoryFilter);
		
		for (int i; i < lootSystem.maxLootItemsPerContainer && !spawned; i++)
		{
			IEntity entity;
			bool success = SpawnItem(entity);
			
			// container filled or tried to spawn something too big
			if (!success)
				break;
			
			// if loot failed to spawn but min not reached, refund attempt and continue to hopefully get something valid next time
			if (!entity && i <= minLootItems && attempts < attemptLimit)
			{
				attempts++;
				i--;
				continue;
			}
			
			attempts = 0;
			
			if (accumulatedSupplyValue >= maxValue || accumulatedSpawnedVolume >= maxVolume)
				break;
		}
		
		spawned = true;
		// @TODO probably should set a despawn timestamp and call DespawnLoot from system update when that is exceeded
		lootSystem.callQueue.CallLater(DespawnLoot, lootSystem.lootDespawnTime);
	}
	
	// returns spawned entity if successful, null if unable to find slot
	bool SpawnItem(out IEntity entity)
	{
		SCR_EntityCatalogEntry entry;
		
		// if category filters set, get random item from random enabled category loot data
		if (categoryFilter.Count())
		{
			DL_LootCategory category = categoryFilter.GetRandomElement();
			SCR_WeightedArray<SCR_EntityCatalogEntry> categoryData = lootSystem.categoryLootData.Get(category);
			if (!categoryData)
			{
				PrintFormat("DL_LootSystem: Unable to find category %1 in lootSystem.categoryLootData for %2!", SCR_Enum.GetEnumName(DL_LootCategory, category), this, LogLevel.ERROR);
				lootSystem.lootDataWeighted.GetRandomValue(entry);
			}
			else
			{
				categoryData.GetRandomValue(entry);
				//PrintFormat("DL_LootSystem: Selected category %1, got item %2", SCR_Enum.GetEnumName(DL_LootCategory, category), entry.GetEntityName());
			}
		}
		
		// otherwise fallback to global loot data
		else if (lootSystem.lootDataWeighted.Count())
			lootSystem.lootDataWeighted.GetRandomValue(entry);
		
		if (!entry)
			return false;
		
		SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!item)
			return false;
		
		SCR_EArsenalItemType itemType = item.GetItemType();
		if (!itemType)
			return false;
		
		if (lootSystem.itemBlacklist.Contains(itemType))
			return false;
		
		if (accumulatedSupplyValue + item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT) >= maxSupplyValue)
			return false;
		
		ResourceName prefab = entry.GetPrefab();
		EntitySpawnParams params = new EntitySpawnParams();
		params.Parent = this;
		entity = GetGame().SpawnEntityPrefabEx(prefab, true, null, params);
		if (!entity)
			return false;
		
		vector mins;
		vector maxes;
		entity.GetBounds(mins, maxes);
		
		float itemVolume = ((maxes[0] - mins[0]) * (maxes[1] - mins[1]) * (maxes[2] - mins[2])) * 1000;
		if (itemVolume + accumulatedSpawnedVolume >= maxVolume)
		{
			// tried to spawn entity that won't fit, so delete and exit out
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			delete entity;
			return false;
		}
		
		SCR_UniversalInventoryStorageComponent inv = SCR_UniversalInventoryStorageComponent.Cast(FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!inv)
			return false;
		
		InventoryStorageSlot slot = inv.FindSuitableSlotForItem(entity);
		if (slot)
			slot.AttachEntity(entity);
		else
		{
			// tried to spawn entity that won't fit, so delete and exit out
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			delete entity;
			return false;
		}

		accumulatedSupplyValue += Math.Min(item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT), 1);
		accumulatedSpawnedVolume += itemVolume;
		
		return true;
	}
	
	void DespawnLoot()
	{
		
		// check if any players near
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
			if (!player || player.GetCharacterController().GetLifeState() == ECharacterLifeState.DEAD)
				continue;

			// if player is nearby, prevent despawn and queue another despawn attempt
			if (vector.DistanceSq(player.GetOrigin(), GetOrigin()) < preventDespawnDistanceSq)
			{
				lootSystem.callQueue.CallLater(DespawnLoot, lootSystem.lootDespawnTime);
				return;
			}
		}
		
		// queue parent entity component to have trigger re-created
		// so loot container is deleted entirely until a player is nearby again
		lootSystem.spawnComponents.Insert(DL_LootSpawnComponent.Cast(GetParent().FindComponent(DL_LootSpawnComponent)));
		delete this;
		
		/*SCR_UniversalInventoryStorageComponent inv = SCR_UniversalInventoryStorageComponent.Cast(FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!inv)
			return;
		
		array<InventoryItemComponent> items = {};
		inv.GetOwnedItems(items);
		
		foreach (InventoryItemComponent item : items)
			SCR_EntityHelper.DeleteEntityAndChildren(item.GetOwner());
		
		spawned = false;
		accumulatedSupplyValue = 0;
		accumulatedSpawnedVolume = 0;*/
	}
}

class DL_LootSpawnComponentClass : ScriptComponentClass
{
}

// attaches to any GenericEntity that should check if it's a loot container prefab based on ResourceName whitelist/blacklist
// and inserts into system to await container trigger creation if allowed
class DL_LootSpawnComponent : ScriptComponent
{
	[Attribute("", uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(DL_LootCategory), desc: "Category filter for loot to spawn in this container, will be set automatically based on map descriptor proximity if empty", category: "Dynamic Loot")]
	ref array<DL_LootCategory> categoryFilter;
	
	void DL_LootSpawnComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, ent.GetEventMask() | EntityEvent.INIT);
		if (parent)
			parent.SetEventMask(parent.GetEventMask() | EntityEvent.INIT);
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!Replication.IsServer())
			return;
		
		DL_LootSystem sys = DL_LootSystem.GetInstance();
		if (!sys)
			return;
		
		// technically not a ResourceName but does contain it so works for contains matches.
		// getting prefab data seems to get root prefab e.g. building
		ResourceName prefab = owner.ToString();
		bool canSpawn = sys.CanSpawnLoot(prefab);
		if (!canSpawn)
			return;
		
		sys.spawnComponents.Insert(this);
	}
}

class DL_LootSpawnCategoryProviderComponentClass : ScriptComponentClass
{
}

// attaches to any entity with a `MapDescriptorComponent`,
// will have category evaluated and be registered so nearby spawns
// can be categorized
class DL_LootSpawnCategoryProviderComponent : ScriptComponent
{
	ref array<DL_LootCategory> categories = {};
	MapDescriptorComponent descriptor;
	
	[Attribute("-1", desc: "Radius this provider should apply its categories to loot spawns in, -1 will use category fallback values", category: "Dynamic Loot - Categorization")]
	float radius;
	
	void DL_LootSpawnCategoryProviderComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, ent.GetEventMask() | EntityEvent.INIT);
		if (parent)
			parent.SetEventMask(parent.GetEventMask() | EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit();
		
		if (!Replication.IsServer())
			return;
		
		DL_LootSystem sys = DL_LootSystem.GetInstance();
		if (!sys)
			return;
		
		descriptor = MapDescriptorComponent.Cast(owner.FindComponent(MapDescriptorComponent));
		categories.Copy(Categorize(owner));
		
		if (!radius || radius == -1)
			radius = GetCategoryRadius(owner);
		
		if (categories.Count())
			sys.categoryProviders.Insert(this);
		//PrintFormat("DL_LootSpawnCategoryProviderComponent: Initialized %1 with categories %2", this, categories);
	}
	
	// @TODO make these configurable
	array<DL_LootCategory> Categorize(IEntity owner)
	{
		array<DL_LootCategory> detected = {};
		MapItem item = descriptor.Item();
		if (item)
		{
			switch (item.GetDisplayName())
			{
				case "#AR-MapLocation_Military":
				case "#AR-MapLocation_Airport":
					detected.Insert(DL_LootCategory.URBAN);
					detected.Insert(DL_LootCategory.MILITARY);
				break;
				case "#AR-MapLocation_Industrial":
				case "#AR-MapLocation_Factory":
				case "#AR-MapLocation_PowerPlant":
				case "#AR-MapLocation_Harbour":
					detected.Insert(DL_LootCategory.URBAN);
				case "#AR-MapLocation_Sawmill":
				case "#AR-MapLocation_Farm":
				case "#AR-MapLocation_Landfill":
					detected.Insert(DL_LootCategory.INDUSTRIAL);
				break;
				case "#AR-MapLocation_Hospital":
					detected.Insert(DL_LootCategory.MEDICAL);
				break;
			}
		}
		
		switch (descriptor.GetBaseType())
		{
			case EMapDescriptorType.MDT_NAME_CITY:
			case EMapDescriptorType.MDT_NAME_TOWN:
				detected.Insert(DL_LootCategory.URBAN);
			break;
			case EMapDescriptorType.MDT_NAME_SETTLEMENT:
			case EMapDescriptorType.MDT_NAME_VILLAGE:
				detected.Insert(DL_LootCategory.RURAL);
			break;
			
			case EMapDescriptorType.MDT_POLICE:
				detected.Insert(DL_LootCategory.POLICE);
			break;
			
			case EMapDescriptorType.MDT_BASE:
			case EMapDescriptorType.MDT_BUNKER:
			case EMapDescriptorType.MDT_TOWER:
				if (!detected.Contains(DL_LootCategory.MILITARY))
					detected.Insert(DL_LootCategory.MILITARY);
			break;
			
			case EMapDescriptorType.MDT_HOSPITAL:
				if (!detected.Contains(DL_LootCategory.MEDICAL))
					detected.Insert(DL_LootCategory.MEDICAL);
			break;			
			
			case EMapDescriptorType.MDT_FIREDEP:
				if (!detected.Contains(DL_LootCategory.INDUSTRIAL))
					detected.Insert(DL_LootCategory.INDUSTRIAL);
						
			case EMapDescriptorType.MDT_STORE:
			case EMapDescriptorType.MDT_HOTEL:
			case EMapDescriptorType.MDT_PUB:
				if (!detected.Contains(DL_LootCategory.COMMERCIAL))
					detected.Insert(DL_LootCategory.COMMERCIAL);
			break;
		}
		
		return detected;
	}
	
	float GetCategoryRadius(IEntity owner)
	{
		float maxRadius = 0;
		foreach (DL_LootCategory category : categories)
		{
			float categoryRadius = 0;
			
			if ({DL_LootCategory.MILITARY}.Contains(category))
				categoryRadius = 500;	
			
			else if ({DL_LootCategory.INDUSTRIAL, DL_LootCategory.COMMERCIAL}.Contains(category))
				categoryRadius = 75;
			
			else if ({DL_LootCategory.MEDICAL, DL_LootCategory.POLICE}.Contains(category))
				categoryRadius = 25;
			
			else if ({DL_LootCategory.URBAN, DL_LootCategory.RURAL}.Contains(category))
			{
				switch (descriptor.GetBaseType())
				{
					case EMapDescriptorType.MDT_NAME_CITY:
						categoryRadius = 1500;
					break;
					case EMapDescriptorType.MDT_NAME_TOWN:
						categoryRadius = 800;
					break;
					default:
						categoryRadius = 400;
					break;
				}
			}
			
			if (categoryRadius > maxRadius)
				maxRadius = categoryRadius;
		}
		
		return maxRadius;
	}
}
class DL_LootSpawnRarityProviderComponentClass : ScriptComponentClass
{
}

// attaches to any entity with a `MapDescriptorComponent`,
// will have category evaluated and be registered so nearby spawns
// can be categorized
class DL_LootSpawnRarityProviderComponent : ScriptComponent
{
	MapDescriptorComponent descriptor;
	[Attribute("10", desc: "Maximum container rarity multiplier applied 0m from provider", category: "Dynamic Loot - Rarity")]
	float multiplier;
	
	[Attribute("1000", desc: "Radius this provider should apply its rarity multiplier to loot spawns in, scaling down to 1x beyond it", category: "Dynamic Loot - Categorization")]
	float radius;
	
	void DL_LootSpawnRarityProviderComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, ent.GetEventMask() | EntityEvent.INIT);
		if (parent)
			parent.SetEventMask(parent.GetEventMask() | EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit();
		
		if (!Replication.IsServer())
			return;
		
		DL_LootSystem sys = DL_LootSystem.GetInstance();
		if (!sys)
			return;
		
		descriptor = MapDescriptorComponent.Cast(owner.FindComponent(MapDescriptorComponent));
		sys.rarityProviders.Insert(this);
	}
}