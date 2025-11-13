[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomEntityCatalogCatalog(EEntityCatalogType, "m_eEntityCatalogType", "m_aEntityEntryList", "m_aMultiLists")]
modded class SCR_EntityCatalog
{
	int MergeEntityList(notnull out array<SCR_EntityCatalogEntry> entityList)
	{
		foreach (SCR_EntityCatalogEntry entityEntry : m_aEntityEntryList)
			entityList.Insert(entityEntry);
			
		return entityList.Count();
	}
	
	// merge local entry list into provided entry list
	int MergeEntityListRef(notnull array<ref SCR_EntityCatalogEntry> entityList)
	{
		foreach (ref SCR_EntityCatalogEntry entityEntry : entityList)
			m_aEntityEntryList.Insert(entityEntry);
			
		return m_aEntityEntryList.Count();
	}
	
	ref array<ref SCR_EntityCatalogEntry> GetEntityListRef()
	{
		return m_aEntityEntryList;
	}
	
	void SetEntityList(array<ref SCR_EntityCatalogEntry> entityList)
	{
		m_aEntityEntryList = entityList;
	}
};