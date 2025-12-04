class DL_LootSpawnClass : GenericEntityClass
{
}

class DL_LootSpawn : GenericEntity
{
	[Attribute("", UIWidgets.Flags, desc: "Category filter for loot to spawn in this container, any category can spawn if empty", category: "Dynamic Loot")]
	SCR_EArsenalItemType categoryFilter;
	
	DL_LootSystem lootSystem;
	bool spawned = false;
	
	int minLootItems = 1;
	float maxSupplyValue = 10000;
	float accumulatedSupplyValue = 0;
	
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
	}
	
	void SpawnLoot()
	{
		int attemptLimit = 25;
		int attempts = 0;
		
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
			
			// chance to spawn less than max based on accumulated value
			if (Math.RandomInt(1, maxSupplyValue) <= Math.Min(accumulatedSupplyValue, maxSupplyValue) || accumulatedSpawnedVolume >= maxVolume)
				break;
		}
		
		spawned = true;
		lootSystem.callQueue.CallLater(DespawnLoot, lootSystem.lootDespawnTime);
	}
	
	// returns spawned entity if successful, null if unable to find slot
	bool SpawnItem(out IEntity entity)
	{
		SCR_EntityCatalogEntry entry;
		
		if (lootSystem.lootDataWeighted.Count())
			lootSystem.lootDataWeighted.GetRandomValue(entry);
		
		if (!entry)
			return false;
		
		SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!item)
			return false;
		
		SCR_EArsenalItemType itemType = item.GetItemType();
		if (!itemType)
			return false;
		
		/*if (categoryFilter && itemType != categoryFilter)
			return false;*/
		
		if (lootSystem.itemBlacklist.Contains(itemType))
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

		accumulatedSupplyValue += Math.Min(item.GetSupplyCost(SCR_EArsenalSupplyCostType.GADGET_ARSENAL, true), 1);
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