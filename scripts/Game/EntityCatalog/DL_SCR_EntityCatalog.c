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
	
	// copy provided entity list into local entity list, omitting any resources already processed by loot system
	int MergeEntityListRef(notnull array<ref SCR_EntityCatalogEntry> entityList, EEntityCatalogType catalogType, FactionKey faction)
	{
		DL_LootSystem lootSystem = DL_LootSystem.GetInstance();
		foreach (ref SCR_EntityCatalogEntry entityEntry : entityList)
		{
			if (!entityEntry || !entityEntry.GetPrefab() || lootSystem.processedResources.Contains(entityEntry.GetPrefab()))
				continue;
			
			SCR_ArsenalItem item = SCR_ArsenalItem.Cast(entityEntry.GetEntityDataOfType(SCR_ArsenalItem));
			if ((!item || !item.IsEnabled()) && catalogType != EEntityCatalogType.VEHICLE)
				continue;
			
			lootSystem.processedResources.Insert(entityEntry.GetPrefab(), faction);
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