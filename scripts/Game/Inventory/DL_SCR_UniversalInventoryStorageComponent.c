modded class SCR_UniversalInventoryStorageComponent : UniversalInventoryStorageComponent
{
	// broken due to https://feedback.bistudio.com/T196429
	/*override bool ShouldHideInVicinity()
	{
		IEntity owner = GetOwner();
		bool isLootSpawn = DL_LootSpawn.Cast(owner);
		PrintFormat("DL: ShouldHideInVicinity(): %1 = %2", owner, isLootSpawn);
		if (isLootSpawn)
			return true;
		
		IEntity char = SCR_PlayerController.GetLocalMainEntity();
		CharacterVicinityComponent vicinity = CharacterVicinityComponent.Cast(char.FindComponent(CharacterVicinityComponent));
		if (!vicinity)
			return super.ShouldHideInVicinity();
		
		return super.ShouldHideInVicinity();
	}*/
}
