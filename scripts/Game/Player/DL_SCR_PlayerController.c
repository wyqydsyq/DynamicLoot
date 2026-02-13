modded class SCR_PlayerController : PlayerController
{
	void AskOpenContainer(DL_LootSpawn spawner)
	{
		Rpc(Rpc_AskOpenContainer_S, Replication.FindId(spawner.FindComponent(RplComponent)), SCR_PlayerController.GetLocalPlayerId());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
    private void Rpc_AskOpenContainer_S(RplId spawnerId, int playerId)
    {
        RplComponent rplC = RplComponent.Cast(Replication.FindItem(spawnerId));
		if (!rplC)
			return;
		
		DL_LootSpawn spawn = DL_LootSpawn.Cast(rplC.GetEntity());
		if (!spawn)
			return;
		
		spawn.SpawnLoot();
		
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
		
		CharacterVicinityComponent vicinity = CharacterVicinityComponent.Cast(char.FindComponent(CharacterVicinityComponent));
		if (!vicinity)
			return;
		
		vicinity.SetItemOfInterest(spawn);
		
		manager.SetStorageToOpen(spawn);
		manager.SetLootStorage(spawn);
		manager.OpenInventory();
	}
}
