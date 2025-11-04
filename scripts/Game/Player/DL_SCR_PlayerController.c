modded class SCR_PlayerController : PlayerController
{
	void AskOpenContainer(DL_LootSpawn spawner)
	{
		PrintFormat("DynamicLoot: AskOpenContainer(%1)", spawner);
		Rpc(Rpc_AskOpenContainer_S, Replication.FindId(spawner.FindComponent(RplComponent)), SCR_PlayerController.GetLocalPlayerId());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
    private void Rpc_AskOpenContainer_S(RplId spawnerId, int playerId)
    {
		int attemptLimit = 25;
		int attempts = 0;
		
		PrintFormat("DynamicLoot: Rpc_AskOpenContainer_S(%1, %2)", spawnerId, playerId);
		
        RplComponent rplC = RplComponent.Cast(Replication.FindItem(spawnerId));
		if (!rplC)
			return;
		
		DL_LootSpawn spawn = DL_LootSpawn.Cast(rplC.GetEntity());
		if (!spawn)
			return;
		
		DL_LootSystem sys = DL_LootSystem.GetInstance();
		sys.OnContainerToggled(spawn, true);
		
		for (int i; i < sys.maxLootItemsPerContainer && !spawn.spawned; i++)
		{
			IEntity entity;
			bool success = spawn.SpawnLoot(entity);
			
			// container filled or tried to spawn something too big
			if (!success)
				break;
			
			// if loot failed to spawn but min not reached, refund attempt and continue to hopefully get something valid next time
			if (!entity && i <= spawn.minLootItems && attempts < attemptLimit)
			{
				attempts++;
				i--;
				continue;
			}
			
			attempts = 0;
			
			// chance to spawn less than max based on accumulated value
			if (Math.RandomInt(1, spawn.maxSupplyValue) <= Math.Min(spawn.accumulatedSupplyValue, spawn.maxSupplyValue) || spawn.accumulatedSpawnedVolume >= spawn.maxVolume)
				break;
		}
		
		spawn.spawned = true;
		
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		pc.TellOpenContainer(spawnerId);
    }
	
	void TellOpenContainer(RplId spawnerId)
	{
		Rpc(Rpc_DoOpenContainer_P, spawnerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void Rpc_DoOpenContainer_P(RplId spawnerId)
	{
		RplComponent rplC = RplComponent.Cast(Replication.FindItem(spawnerId));
		if (!rplC)
			return;
		
		DL_LootSpawn spawn = DL_LootSpawn.Cast(rplC.GetEntity());
		if (!spawn)
			return;
		
		ChimeraCharacter char = ChimeraCharacter.Cast(GetControlledEntity());
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(char.GetCharacterController().GetInventoryStorageManager());
		DL_LootSystem sys = DL_LootSystem.GetInstance();
		sys.OnContainerToggled(spawn, true);
		manager.SetStorageToOpen(spawn);
		manager.OpenInventory();
	}
}
