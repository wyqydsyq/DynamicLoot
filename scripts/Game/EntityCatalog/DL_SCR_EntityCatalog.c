[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomEntityCatalogCatalog(EEntityCatalogType, "m_eEntityCatalogType", "m_aEntityEntryList", "m_aMultiLists")]
modded class SCR_EntityCatalog
{
	int MergeEntityList(notnull out array<SCR_EntityCatalogEntry> entityList)
	{
		foreach (SCR_EntityCatalogEntry entityEntry: m_aEntityEntryList)
			entityList.Insert(entityEntry);
			
		return entityList.Count();
	}
};