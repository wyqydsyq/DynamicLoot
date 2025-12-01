[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomEntityCatalogCatalog(EEntityCatalogType, "m_eEntityCatalogType", "m_aEntityEntryList", "m_aMultiLists")]
modded class SCR_EntityCatalog
{
	// copy local entity list into provided entity list
	/*int MergeEntityList(notnull out array<SCR_EntityCatalogEntry> entityList)
	{
		foreach (SCR_EntityCatalogEntry entityEntry : m_aEntityEntryList)
		{
			entityList.Insert(entityEntry);
			m_mPrefabIndexes.Insert(entityEntry.GetPrefab(), m_aEntityEntryList.Find(entityEntry));
		}
			
		return entityList.Count();
	}*/
	
	// copy provided entity list into local entity list
	int MergeEntityListRef(notnull array<ref SCR_EntityCatalogEntry> entityList)
	{
		foreach (ref SCR_EntityCatalogEntry entityEntry : entityList)
		{
			if (!entityEntry)
				continue;
			
			m_aEntityEntryList.Insert(entityEntry);
			m_mPrefabIndexes.Insert(entityEntry.GetPrefab(), m_aEntityEntryList.Find(entityEntry));
		}
			
		return m_aEntityEntryList.Count();
	}
	
	ref array<ref SCR_EntityCatalogEntry> GetEntityListRef()
	{
		if (!m_aEntityEntryList)
			m_aEntityEntryList = new array<ref SCR_EntityCatalogEntry>();
		
		return m_aEntityEntryList;
	}
		
	void SetEntityList(array<ref SCR_EntityCatalogEntry> entityList)
	{
		m_aEntityEntryList = entityList;
	}
	
	void SetCatalogType(EEntityCatalogType type)
	{
		m_eEntityCatalogType = type;
	}
};