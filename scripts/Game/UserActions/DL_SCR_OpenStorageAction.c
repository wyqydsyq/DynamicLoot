modded class SCR_OpenStorageAction : SCR_InventoryAction
{
	int attemptLimit = 25;
	int attempts = 0;
	
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CharacterVicinityComponent vicinity = CharacterVicinityComponent.Cast(pUserEntity.FindComponent(CharacterVicinityComponent));
		if (!vicinity)
			return;
		
		DL_LootSpawn spawn = DL_LootSpawn.Cast(pOwnerEntity);
		if (!spawn)
		{
			super.PerformActionInternal(manager, pOwnerEntity, pUserEntity);
			return;
		}
		
		//pUserEntity.FindComponent(inventorycomp);
		vicinity.SetItemOfInterest(spawn);

		// RPC to server
		//GetGame().GetCallqueue().Call(AskOpenContainer, spawn)
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));
		pc.AskOpenContainer(spawn);
	}
};