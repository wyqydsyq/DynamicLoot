modded class SCR_UniversalInventoryStorageComponent : UniversalInventoryStorageComponent
{
	override bool ShouldHideInVicinity()
	{
		if (DL_LootSpawn.Cast(GetOwner()))
			return true;
		
		return super.ShouldHideInVicinity();
	}
}
